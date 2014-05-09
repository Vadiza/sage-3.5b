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




from threading import RLock
import time
from globals import *
import traceback as tb
import sys
from events import *
from eventHandler import EventHandler
from overlay import Overlay
from overlays.wall import Wall
from overlays.app import App
from overlays.thumbnail import Thumbnail
from overlays.button import Button



class EventManager:
    """ receives all the events and then forwards them to
        all the event handlers that want them
    """

    def __init__(self):
        self.__evtLock = RLock()
        self.__evtHandlers = {}   # keyed by eventId, value=list of callbacks
        self.__allHandlers = []   # a list of evtHandlers that have some events registered
        self.__prevHandlers = {}   # key=deviceId, value=previous event handler
        
        self.wallObj = None
        
        self.register(EVT_DISPLAY_INFO, self.__onDisplayInfo)

    
    def getWallObj(self):
        return self.wallObj


    def addHandlers(self):
        pass


    def __onDisplayInfo(self, event):
        di = event.displayInfo

        # add a wall object that captures events not destined for any handlers
        for display in di.getAllDisplays():
            self.wallObj = self.__importHandlerPlugin("wall").makeNew(display)  # one for each display

        # figure out the default scale factor based on the display size
        display = di.getDisplay(0)
        if display.sageW / float(display.sageH) > 16/9.0:   
            scale = display.sageH / 1080.0  # height determines the scale
        else:
            scale = display.sageW / 1920.0  # width determines the scale
            
        scale = scale / (92.0/PPI)

        # cap the scale so that it doesn't get ridiculous
        if scale < MIN_SCALE:
            scale = MIN_SCALE
        elif scale > MAX_SCALE:
            scale = MAX_SCALE
        
        setGlobalScale(scale)

        # set the ENLARGE_WIDGETS_DIST (dynamic scaling of
        # widgets based on the distance from a pointer)
        setEnlargeWidgetsThreshold(scale)
        

    def __importHandlerPlugin(self, name):
        plugin = __import__("overlays."+name, globals(), locals(), [name])
        return plugin


    def _removeDevice(self, deviceId):
        if deviceId in self.__prevHandlers:
            del self.__prevHandlers[deviceId]
            

    def register(self, eventId, callback):
        """ register the evtHandler to receive events of eventId """
        self.__evtLock.acquire()
        
        if eventId not in self.__evtHandlers:
            self.__evtHandlers[eventId] = []
        self.__evtHandlers[eventId].append( callback )
        if not callback in self.__allHandlers and isinstance(callback.im_self, EventHandler):
            self.__allHandlers.append(callback.im_self)

        self.__evtLock.release()
        

    def unregister(self, callbackHash, handler=None):
        """ unregisters the evtHandler from the specified events """
        self.__evtLock.acquire()

        # remove from the list of previous handlers... if any
        if handler:
            for deviceId, prevHandler in self.__prevHandlers.items():
                if prevHandler == handler:
                    self.__prevHandlers[deviceId] = None

        # remove from the list of all event handlers and callbacks
        for eventId, callback in callbackHash.iteritems():
            try:
                self.__evtHandlers[eventId].remove(callback)

                if callback.im_self in self.__allHandlers:
                    self.__allHandlers.remove(callback.im_self)
            except ValueError:
                continue
                
        self.__evtLock.release()
            

    def postEvent(self, event):
        """ thread safe """
        self.__evtLock.acquire()
        
        # handle differently depending on whether
        # it's a device event or a generic event
        if event.eventId in self.__evtHandlers:
            if event.eventType == DEVICE_EVENT:
                self.__sendToSingleEvtHandler(event)
            else:
                if event.eventType == EVT_DISPLAY_INFO:
                    self.__onDisplayInfo(event)   # create objects that are display dependent

                self.__sendToMultipleEvtHandler(event)
            
        self.__evtLock.release()


    def __findChildAtPos(self, parent, x, y):
        foundChild = parent
        currentZ = parent.z
        
        for child in parent._getChildren().values():
            if child.isIn(x,y) and \
                   ((isinstance(child, Overlay) and child.isVisible()) or not isinstance(child, Overlay)):
                   #not child._eventTransparent:
                foundChild = self.__findChildAtPos(child, x, y)

        return foundChild


    def getEvtHandlerAtPos(self, x, y, displayId=0, event=None):
        """ Find an event handler for an event at (x,y) on displayId """
        foundParent = None
        foundChild = None
        lastZ = BOTTOM_Z+1
        if event and event.eventType == DEVICE_EVENT:
            deviceId = event.device.deviceId
        else:
            deviceId = None

        if deviceId not in self.__prevHandlers:
            self.__prevHandlers[deviceId] = None
            

        # first find the top level parent that should get this event
        for parent in getOverlayMgr().getTopLevelParents():
            if parent.displayId == displayId and \
                   parent.z <= lastZ and \
                   ((isinstance(parent, Overlay) and parent.isVisible()) or not isinstance(parent, Overlay)) and \
                   parent.isIn(x,y):
                   #not parent._eventTransparent:
                foundParent = parent
                lastZ = parent.z

        # we found the parent, now search through its children, recursively
        if foundParent:

            # search through children recursively
            foundChild = self.__findChildAtPos(foundParent, x, y)
            isPrevIn = (self.__prevHandlers[deviceId] and self.__prevHandlers[deviceId].isIn(x,y))

            # however, even if a new child is found, maybe we are still in the
            # old one (e.g. expanded thumbnail with temporary z)...
            if ((not foundChild and isPrevIn) or \
                (foundChild != self.__prevHandlers[deviceId] and isPrevIn and \
                 self.__prevHandlers[deviceId].z < foundChild.z)):
                foundChild = self.__prevHandlers[deviceId]

        # finally, don't use event-transparent handlers (such as sizers)
        if foundChild and foundChild._eventTransparent:
            foundChild = None
            
        self.__prevHandlers[deviceId] = foundChild
        return foundChild


    def __sendToSingleEvtHandler(self, event):
        """ forward the event to a handler that is at the position
            of the event and wants the current event
        """
        x, y, eventId, displayId, device = event.x, event.y, event.eventId, event.device.displayId, event.device
        callback = None

        # if the event goes to a specific evtHandler, no need to search for one
        if event.toEvtHandler: 
            callback = event.toEvtHandler._getCallback(eventId)

            # generate EVT_ENTERED_WINDOW and EVT_LEFT_WINDOW events
            if event.toEvtHandler._doesAllowDrag() and eventId == EVT_DRAG:
                self.__enlargeWidgets(event)
                handler = self.getEvtHandlerAtPos(x,y, displayId, event)
                if handler != device.lastHandler:
                    if device.lastHandler and device.lastHandler != event.toEvtHandler:
                        self.__sendEvent(WindowLeftEvent(device),
                                         device.lastHandler._getCallback(EVT_LEFT_WINDOW))
                        
                    if handler and handler._doesAllowDrop():
                        self.__sendEvent(WindowEnteredEvent(device),
                                         handler._getCallback(EVT_ENTERED_WINDOW))
                    
                    device.lastHandler = handler

                    
        else:
            # find the object under this current position
            handler = self.getEvtHandlerAtPos(x,y, displayId, event)

            # enlarges widgets as the mouse approaches them
            if (getEnlargeWidgets() or getEnlargeAppWidgets()) and \
                   event.eventId == EVT_MOVE or event.eventId == EVT_DRAG:
                self.__enlargeWidgets(event)

            if handler and not handler._captured:
                callback = handler._getCallback(eventId)

            # generate EVT_ENTERED_WINDOW and EVT_LEFT_WINDOW events
            if handler != device.lastHandler:   # handler changed

                # only allow move events to cross handler borders
                # e.g. if drag originated in one handler, don't let it carry over to another one
                if (eventId >= EVT_ANALOG1 and eventId <= EVT_ANALOG3) or \
                   (eventId >= EVT_ANALOG1_SPECIAL and eventId <= EVT_ANALOG3_SPECIAL):
                    return   

                if device.lastHandler:
                    evtId = EVT_LEFT_WINDOW
                    if device.specialDevice: evtId = EVT_LEFT_WINDOW_SPECIAL
                    self.__sendEvent(WindowLeftEvent(device),
                                     device.lastHandler._getCallback(evtId))

                if handler and callback:      # if there is no callback, don't do anything
                    evtId = EVT_ENTERED_WINDOW
                    if device.specialDevice: evtId = EVT_ENTERED_WINDOW_SPECIAL
                    self.__sendEvent(WindowEnteredEvent(device),
                                     handler._getCallback(evtId))
                    
                device.lastHandler = handler


        self.__sendEvent(event, callback)
        

    def __sendToMultipleEvtHandler(self, event):
        """ forward to all the evtHandlers registered for this eventId """
        for callback in self.__evtHandlers[event.eventId]:
            self.__sendEvent(event, callback)


    def __sendEvent(self, event, callback):
        """ executes callback with event as a parameter """
       
        if callback:      
            #print "%s -for- %s" % (event.__class__.__name__, callback.im_self)
            try:
                
                evtHandler = callback.im_self

                # if the overlay is selected, forward events to all other selected overlays
                if isinstance(evtHandler, EventHandler) and evtHandler.isSelected() and \
                   (event.eventId == EVT_CLICK or event.eventId == EVT_DRAG or \
                    event.eventId == EVT_ZOOM or event.eventId == EVT_DROP):
                    event.device.eventToMultiSelect(event)

                    
                    # to handle the EVT_DROP for multiple selected apps
                elif event.eventId == EVT_DROP and event.object.isSelected():
                    # replace the object that is being dropped with the multiSelect object
                    event.object = event.device.getMultiSelect()
                    callback(event)

                    
                else:   # just pass it directly to the event handler
                    ## obj = callback.im_self
##                     if isinstance(obj, Thumbnail) or isinstance(obj, Button):
##                         l = obj.getLabel() 
##                         if l != "":
##                             print "event to:  ", l
##                         else:
##                             print "event to:  ", obj.getUserData() 
                    callback(event)

                
            except AttributeError:   # if the object doesn't exist anymore, remove its callback
                tb.print_exc()
                if event.eventId in self.__evtHandlers and callback in self.__evtHandlers[event.eventId]:
                    self.__evtHandlers[event.eventId].remove(callback)
                    
                if callback.im_self in self.__allHandlers:
                    self.__allHandlers.remove(callback.im_self)
            except:
                tb.print_exc()



    def __enlargeWidgets(self, event):
        def dist(w):
            x,y = event.x, event.y
            l,r,t,b = w.getBounds().getAll()
        
            # corner cases
            if (x>r and y>t):
                return distance(r,x,t,y)
            elif (x>r and y<b):
                return distance(r,x,b,y)
            elif (x<l and y<b):
                return distance(l,x,b,y)
            elif (x<l and y>t):
                return distance(l,x,t,y)
        
            # side cases
            elif (x<l):
                return l-x
            elif (x>r):
                return x-r
            elif (y<b):
                return b-y
            elif (y>t):
                return y-t
            else:
                return 0.0   # we are inside the widget


        # get all the widgets that can be scaled
        widgets = getOverlayMgr().getOverlaysOfType([OVERLAY_BUTTON, OVERLAY_MENU])
        
        for w in widgets:            
            if w._isAppWidget() and not getEnlargeAppWidgets():
                continue
            elif not w._isAppWidget() and not getEnlargeWidgets():
                continue

            mult = 1.0
            d = dist(w)

            if d < getEnlargeWidgetsThreshold():
                mult = ENLARGE_WIDGETS_MAX - (ENLARGE_WIDGETS_MAX-1) * (float(d) / getEnlargeWidgetsThreshold())
            
            if mult != w._getScaleMultiplier():
                w._setScaleMultiplier(mult)
                
            

