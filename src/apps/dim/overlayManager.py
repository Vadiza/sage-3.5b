############################################################################
#
# DIM - A Direct Interaction Manager for SAGE
# Copyright (C) 2007 Electronic Visualization Laboratory,
# University of Illinois at Chicago
#
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following disclaimer
#    in the documentation and/or other materials provided with the distribution.
#  * Neither the name of the University of Illinois at Chicago nor
#    the names of its contributors may be used to endorse or promote
#    products derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Direct questions, comments etc about SAGE UI to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#        
############################################################################




from globals import *
from threading import RLock
import os, os.path
import overlays, sys
import traceback as tb


class OverlayManager:
    """ creates and deletes all overlays """

    def __init__(self):
        self.__overlays = {}            # key=overlayId, value=overlay objects
        self.__appWidgetQueue = {}    # a list of events corresponding to app widgets to be added
        self.__appChildrenQueue = []
        self.__topLevelParents = []   # value=overlay objects
        self.__lastWidgetId = -1
        self.evtMgr = getEvtMgr()
        self.sageGate = getSageGate()
        self.__addOverlayLock = RLock()  # for adding one overlay at a time... overlays requested by DIM
        self.__addOverlayQueue = []      # for adding one overlay at a time... overlays requested by DIM

        # other UIs (collections of widgets)
        self.__uiPlugins = []
        self.__lastUIPlugins = []  # plugins to be loaded last because they depend on others
        
        # load all the plugins now
        self.__overlayPlugins = {}  # key=overlayType, value=overlayPlugin
        self.__loadAllPlugins()
        
        # register for events
        self.evtMgr.register(EVT_NEW_APP, self.__onNewApp)
        self.evtMgr.register(EVT_APP_KILLED, self.__onAppKilled)
        self.evtMgr.register(EVT_OBJECT_INFO, self.__onObjectInfo)
        self.evtMgr.register(EVT_OBJECT_REMOVED, self.__onObjectRemoved)
        self.evtMgr.register(EVT_OBJECT_CHANGED, self.__onAppObjectChange)


    def _addGlobalOverlays(self):
        """ load all the UI_PLUGINs here 
        """
        # add the wall object as one of the top level parents
        self.__topLevelParents.append(getEvtMgr().getWallObj())
        
        for uiPlugin in self.__uiPlugins:
            uiPlugin.makeNew()

        for uiPlugin in self.__lastUIPlugins:
            uiPlugin.makeNew()

        # add the default touch pointers
        #getDevMgr().createTouchPointers()

   
    def __onNewApp(self, event):
        """ add the overlays that need to be tied to the app """
        app = event.app
        
        # add all app-requested widgets that came before the app info did...
        

        # now add all the DIM-created app widgets from the queue
        if app.getId() in self.__appWidgetQueue:
            for e in self.__appWidgetQueue[app.getId()]:
                self.__addAppWidget(e, app)
            del self.__appWidgetQueue[app.getId()]


    def __addAppWidget(self, event, app):
        """ adds the widget or if it has a parent and the parent's info 
            hasn't arrived yet, just queue it for later adding
        """
        appWidgetObj = self.__overlayPlugins[event.overlayType].makeNew(app)
        appWidgetObj._xmlToData(event)
        appWidgetObj.initialize(event.overlayId)
                
        # is this widget a child of a parent?
        # if the widget's parent info hasn't arrived yet, queue it
        if appWidgetObj._hasParent():
            parentObj = self.getParentObj(appWidgetObj)
            if parentObj:
                parentObj.addChild(appWidgetObj)
            else:
                self.__appChildrenQueue.append(appWidgetObj)
        else:
            self.__topLevelParents.append(appWidgetObj)

        #print "ADDING APP WIDGET widgetID %d, winID %d" %(event.widgetId, event.winId)
        self.__overlays[ event.overlayId ] = appWidgetObj
        self.__checkQueue()
            

    def __checkQueue(self):
        notAdded = []
        numAdded = 1
        while numAdded > 0:
            notAdded = []
            numAdded = 0

            for obj in self.__appChildrenQueue:
                parentObj = self.getParentObj(obj)
                if parentObj:
                    parentObj.addChild(obj)
                    numAdded += 1
                else:
                    notAdded.append(obj)

            self.__appChildrenQueue = notAdded # effectively deletes the added items


    def __onAppObjectChange(self, event):
        if event.msgCode == SET_LABEL:
            tokens = event.data.split(" ", 1)
            ftSize = int(tokens[0])
            newLabel = tokens[1]

            if event.overlayId in self.__overlays:
                self.__overlays[event.overlayId].setLabel(newLabel)
            


    def __onAppKilled(self, event):
        """ remove the overlays tied to the app """
        windowId = event.app.getId()
        for overlay in self.__overlays.values():
            if overlay.getWindowId() == windowId:
                self.removeOverlay(overlay.getId())


    def __onObjectInfo(self, event):
        """ the overlay was successfully added by SAGE and we got the overlayId """

        # app_widgets aren't added by dim so process differently 
        # (only app widgets have widgetId >-1)
        if event.overlayType in self.__overlayPlugins and event.widgetId > -1:
            if not event.app:          # queue the event for later adding when the app info comes in
                if event.winId not in self.__appWidgetQueue:
                    self.__appWidgetQueue[event.winId] = []
                #print "QUEING APP WIDGET widgetID %d, winID %d" %(event.widgetId, event.winId)
                self.__appWidgetQueue[event.winId].append(event)
            else:
                self.__addAppWidget(event, event.app)
        
                
        # DIM-requested widgets
        elif event.overlayType in self.__overlayPlugins:
            # take care of the queue first
            self.__addOverlayLock.acquire()
            overlayObj, callback = self.__addOverlayQueue.pop(0) # the one whose info just came in           
            if len(self.__addOverlayQueue) > 0:
                objToAdd, cb = self.__addOverlayQueue[0]  # the next one to be added
                addNewOne = True
            else:
                addNewOne = False
            self.__addOverlayLock.release()
            
            # set the object's id that just came in from fsManager
            #overlayObj._xmlToData(event)
            overlayObj.initialize(event.overlayId)

            # add the overlay to the appropriate lists
            self.__overlays[ event.overlayId ] = overlayObj

            # now add a new one (if there are any)
            if addNewOne:
                winId = objToAdd.getWindowId()
                widgetId = objToAdd.getWidgetId()
                self.sageGate.addOverlay(objToAdd.getType(), widgetId, winId, 
                                         objToAdd._dataToXml(objToAdd.getType()))


            # finally report back to the entity that requested the overlay
            if callback:
                callback(overlayObj)


    def __onObjectRemoved(self, event):
        """ removes the overlay locally """
        overlayId = event.overlayId
        if overlayId in self.__overlays:
            obj = self.__overlays[overlayId]
            if obj in self.__topLevelParents:
                self.__topLevelParents.remove(obj)
            obj._destroy()
            del self.__overlays[overlayId]
            

    def removeOverlay(self, overlayId):
        """ removes the overlay from SAGE """
        overlay = self.getOverlay(overlayId)
        if overlay in self.__topLevelParents:
            self.__topLevelParents.remove(overlay)
        
        if overlayId in self.__overlays:
            self.sageGate.removeOverlay(overlayId)
        

    def removeWidget(self, widget):
        """ alias for removeOverlay """
        if widget:
            self.removeOverlay(widget.getId())


    def removeAllOverlays(self):
        for overlayId in self.__overlays.keys():
            self.removeOverlay(overlayId)


    def getNewId(self):
        self.__lastWidgetId -= 1
        return self.__lastWidgetId


    def getOverlaysOfType(self, overlayTypes):
        """ accepts a list of overlayTypes as input and 
            returns a list of overlays that are of that type
        """
        objList = []
        for obj in self.__overlays.values():
            if obj.getType() in overlayTypes:
                objList.append(obj)
        return objList


    def getOverlay(self, overlayId):
        if overlayId in self.__overlays:
            return self.__overlays[overlayId]
        else:
            return None


    def getTopLevelParents(self, parentsList=None):
        """ find topLevelParents that aren't eventTransparent
            If any parent is eventTransparent, return its children
            which arent eventTransparent
        """
        if not parentsList:
            parentsList = self.__topLevelParents
            
        parents = []  # top level parents that we found and arent event transparent
        for w in parentsList:
            if not w._eventTransparent:
                parents.append(w)
            elif w._getNumChildren() > 0:
                parents.extend(self.getTopLevelParents(w._getChildren().values()))
                
        return parents
            

    def getAppWidgets(self, appObj):
        """ it returns a list of all widgets that are tied to the
            passed-in app object AND that don't have a parent (i.e.
            top-level widgets)
        """
        objList = []
        for obj in self.__overlays.values():
            if obj.app == appObj and not obj._hasParent():
                objList.append(obj)
        return objList


    def getAppOverlay(self, appObj):
        """ pass in a sageApp object and get the corresponding appOverlay """
        objList = self.getAppWidgets(appObj)
        for obj in objList:
            if obj.getType() == OVERLAY_APP:
                return obj
            

    def getParentObj(self, obj):
        """  This is necessary because widgetIds can be the same for 
             two different widgets if two different application 
             created them... so we need to find the ones where
             windowIds match as well.
        """
        if not obj._hasParent():
            return None

        for overlay in self.__overlays.values():
            if obj._getParentId() == overlay.getWidgetId() and \
                    obj.getWindowId() == overlay.getWindowId():
                    return overlay
        return None  # otherwise not found


    def addOverlay(self, overlayObj, callback=None):
        """ specify an optional callback to call once the
            overlay is successfully added by SAGE
        """
        
        self.__addOverlayLock.acquire()

        # first check that the widget hasn't been added already
        if hasattr(overlayObj, "_added"):
            if overlayObj._added:
                self.__addOverlayLock.release()
                return
            else:
                overlayObj._added = True

        # if this is the only thing to add, just do it now
        # otherwise it will be added by the __onObjectInfo when the previous one gets added
        self.__addOverlayQueue.append((overlayObj, callback))
        if len(self.__addOverlayQueue) == 1:
            winId = overlayObj.getWindowId()
            widgetId = overlayObj.getWidgetId()
            self.sageGate.addOverlay(overlayObj.getType(), widgetId, winId, 
                                     overlayObj._dataToXml(overlayObj.getType()))

        # add to top level parents... if appropriate
        #if (not overlayObj._hasParent() and not overlayObj._eventTransparent) or \
        #       (overlayObj._hasParent() and overlayObj.parent._eventTransparent):
        if not overlayObj._hasParent():
            if overlayObj not in self.__topLevelParents:
                self.__topLevelParents.append(overlayObj)

        self.__addOverlayLock.release()


    def addWidget(self, widget, callback=None):
        """ alias for addOverlay """
        self.addOverlay(widget, callback)

        # if this widget has any children, add them now
        widget._addChildren()


    def __loadAllPlugins(self):
        """ goes into the 'overlays' directory and loads all the plugins from there """
        loaded = []
        for entry in os.listdir("overlays"):
            name, ext = os.path.splitext(entry)
            if (ext==".py" or ext==".pyc") and not name.startswith("__") and name not in loaded:
                loaded.append(name)
                self.__loadOverlayPlugin( "overlays", name )

        for entry in os.listdir("ui"):
            name, ext = os.path.splitext(entry)
            if (ext==".py" or ext==".pyc") and not name.startswith("__") and name not in loaded:
                loaded.append(name)
                self.__loadOverlayPlugin( "ui", name )
            

    def __loadOverlayPlugin(self, dir, overlayType):
        """ try to load the overlayPlugin for the requested overlayType """
        try:
            overlayPlugin = __import__(dir+"."+overlayType, globals(), locals(), [overlayType])
            if hasattr(overlayPlugin, "TYPE") and \
                    overlayPlugin.TYPE == UI_PLUGIN:
                if hasattr(overlayPlugin, "LOAD_LAST"):
                    self.__lastUIPlugins.append(overlayPlugin)
                else:
                    self.__uiPlugins.append(overlayPlugin)
                print "UI plugin loaded: ", overlayType
            else:
                self.__overlayPlugins[ overlayType ] = overlayPlugin
                print "Overlay plugin loaded: ", overlayType
                
            return True
        except:
            print "\n==========> Error importing module: ", overlayType
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            return False

    
