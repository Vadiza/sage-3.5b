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




########################################################################
#
# Creates some global widgets for the whole display
#
# Author: Ratko Jagodic
#
########################################################################

from globals import *
import overlays.menu as menu
import overlays.button as button
import overlays.icon as icon
import overlays.thumbnail as thumbnail
import overlays.label as label
import overlays.sizer as sizer
import overlays.panel as panel
import overlays.app as appOverlayObj
import overlays.multiSelect as multiSelect
import time, copy, os, os.path
from threading import Thread
import traceback as tb
from math import ceil, sqrt, floor
from sageGate import addGate, getGate

import pprint


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return OrganizationUI()


# must have this to load the plugin automatically
TYPE = UI_PLUGIN


# tile modes
FREE_MODE = 0
TILE_MODE = 1
ONE_PER_DISPLAY_MODE = 2

# To know the each button's state
# when sharing through 'ALL' button
# see _onShare function
ShareWithButtons = []

# Sungwon multi-site sharing
class ShareButton(button.Button):
    def __init__(self, shareWithHost, app=None):
        button.Button.__init__(self, app)

        # Put a special icon
        self.setUpImage( "images/share_button_up.png" )
        self.setDownImage( "images/default_button_up.png" )

        # The button has a member variable for the IP address
        self.__shareWithHost = shareWithHost
        self.__callback = None

        # toggle to enable/disable sharing
        # when a media is dropped on to the 'ALL' button
        # Off to start with
        self.setToggle(True)
        self.setState(BUTTON_DOWN)

        # click on sets the host as slave for its master
        self.onUp(self.onMasterUp)
        self.onDown(self.onMasterDown)

    def onMasterUp(self, btn):
        setMasterSite(self.__shareWithHost)
        print("onMaster", getMasterSite(), getMasterSiteMode() )

    def onMasterDown(self, btn):
        setMasterSite(None)
        print("onMaster", getMasterSite(), getMasterSiteMode() )


    def onDrop(self, callback):
        self.__callback = callback
        button.Button.onDrop(self, self._internalOnDropCallback)

    def _internalOnDropCallback(self, event):
        # write the data here
        # DropEvent in events.py has the data field
        event.data = self.__shareWithHost
        #if self.getState() == BUTTON_UP:
        self.__callback(event)


class OrganizationUI:
    def __init__(self):
        self.addWidget = getOverlayMgr().addWidget
        self.sageGate = getSageGate()
        self.sageData = getSageData()
        self.evtMgr = getEvtMgr()
        self.overlayMgr = getOverlayMgr()

        self.fileServer = getFileServer()
        
        # display stuff
        self.disp = self.sageData.getDisplayInfo()
        self.sageW, self.sageH = self.disp.sageW, self.disp.sageH

        # event stuff
        self.evtMgr.register(EVT_NEW_APP, self.__onNewApp)
        self.evtMgr.register(EVT_APP_KILLED, self.__onAppKilled)
        #self.evtMgr.register(EVT_APP_INFO, self.__onAppInfo)

        # adjust the usable height to NOT include the bottom minimize bar
        self.disp.usableH -= int(self.sageH * MINIMIZE_BAR_SIZE)
        self.disp.usableZeroY = int(self.sageH * MINIMIZE_BAR_SIZE)

        # tile stuff
        self.__prevButton = None
        self.__tileMode = FREE_MODE
        self.__tiledWindows = []
        
        if SHOW_USER_INTERFACE:
            # the UI is created when the display info comes in... _onDisplayInfo
            self.__makeUI()

        # so that others can use this object to initiate organization
        setLayoutOrganization(self)


    def __makeUI(self):
        self.__fs = NORMAL
        
        p = panel.makeNew()
        p.align(CENTER, TOP)
        p.fitInWidth(False)
        p.fitInHeight(False)
        p.setSize(1.0, 40)
        p.setBackgroundColor(0,0,0)
        p.setTransparency(150)
        p.z = TOP_Z

        # adjust the usable height to NOT include the top UI bar
        hh = p.getFinalHeight()
        self.disp.usableH -= hh
        
        topSizer = sizer.makeNew()
        topSizer.align(CENTER, TOP, yMargin=-1)
        #topSizer.align(CENTER, TOP, yMargin=4)
        topSizer.addChild(self.__makeActionMenu())
        #topSizer.addChild(self.__makeLayoutButtons())

        p.addChild(topSizer)
        self.addWidget(p)

        
    def makeLayoutButtons(self, bs=30, alpha=255):
        btnWidth = int(bs*1.5) 

        gb = thumbnail.makeNew()
        gb.align(CENTER, CENTER, xMargin=20, yMargin=20)
        gb.setToggle(True)
        gb.setLabel("Tile", fontSize=SMALLER)
        gb.alignLabel(CENTER)
        gb.setSize(btnWidth, bs)
        gb.onDown(self.__onLayoutBtn)
        gb.onUp(self.__onLayoutBtn)
        gb.setTransparency(alpha)
        return gb

        """
        s = sizer.makeNew()
        s.align(CENTER, TOP)

        l = label.makeNew(label="Layout Mode:")
        l.align(CENTER, CENTER, xMargin=100)
        l.drawBackground(False)
        l.setTransparency(150)
        s.addChild(l)
        
        gb = button.makeNew()
        gb.setRadio()
        #gb.setToggle(True, True)
        gb.align(CENTER, CENTER, xMargin=5)
        gb.setLabel("Grid", fontSize=self.__fs)
        gb.alignLabel(CENTER)
        gb.setSize(btnWidth,30)
        gb.onDown(self.__onLayoutBtn)
        gb.setUserData(TILE_MODE)
        s.addChild(gb)

        ob = button.makeNew()
        ob.setRadio()
        #ob.setToggle(True, True)
        ob.setLabel("1 per Tile", fontSize=self.__fs)
        ob.alignLabel(CENTER)
        ob.setSize(btnWidth,30)
        ob.align(CENTER, CENTER, xMargin=5)
        ob.onDown(self.__onLayoutBtn)
        ob.setUserData(ONE_PER_DISPLAY_MODE)
        s.addChild(ob)

        self.fb = button.makeNew()
        #self.fb.setToggle(True, True)
        self.fb.setRadio()
        self.fb.setLabel("Free", fontSize=self.__fs)
        self.fb.alignLabel(CENTER)
        self.fb.setSize(btnWidth,30)
        self.fb.align(CENTER, CENTER, xMargin=5)
        self.fb.onDown(self.__onLayoutBtn)
        self.fb.setUserData(FREE_MODE)
        self.fb.setState(BUTTON_DOWN)
        s.addChild(self.fb)
    
        self.__prevButton = self.fb

        return s
        """
        

    def __makeActionMenu(self):
        s = sizer.Sizer()
        s.align(CENTER, TOP)

        l = label.makeNew(label="Drop windows to:")
        l.drawBackground(False)
        l.setTransparency(150)
        l.align(CENTER, CENTER, xMargin=50*getGlobalScale())
        s.addChild(l)
        
        if getSharedHost():
            """
            b = button.Button()
            b.setLabel("Share with: "+str(getSharedHostName()), fontSize=self.__fs)
            b.alignLabel(CENTER)
            b.setSize(300,40)
            b.align(CENTER, TOP)
            b.onDrop(self.__onShare)
            s.addChild(b)
            """
            # Sungwon multi-site sharing
            names = getSharedHostName()
            hosts = getSharedHost()
            if len(hosts) > 1:
                # attach 'all' button
                b = button.Button()
                # Put a special icon
                b.setUpImage( "images/shareall_button_up.png" )
                b.setLabel("All sites", fontSize=self.__fs)
                b.alignLabel(CENTER)
                b.setSize(200,40)
                b.align(CENTER, TOP, xMargin=30*getGlobalScale())
                b.onDrop(self.__onShare)
                b.onUp(self.__onSync)
                s.addChild(b)

            for i in range(len(hosts)): 
                # each shareWith button has ip address
                b = ShareButton(str(hosts[i]))
                b.setLabel(str(names[i]), fontSize=self.__fs)
                b.alignLabel(CENTER)
                b.setSize(200,40)
                b.align(CENTER, TOP, xMargin=30*getGlobalScale())
                b.onDrop(self.__onShare)
                s.addChild(b)
                ShareWithButtons.insert(i, b)

        b = button.Button()
        b.setLabel("Push to Back", fontSize=self.__fs)
        b.alignLabel(CENTER)
        b.setSize(350,40)
        if getSharedHost():
            b.align(CENTER, TOP, xMargin=70*getGlobalScale())
        else:
            b.align(CENTER, TOP, xMargin=40*getGlobalScale())
        b.onDrop(self.__onPushToBack)
        s.addChild(b)

        b = button.Button()
        b.setLabel("Remove", fontSize=self.__fs)
        b.alignLabel(CENTER)
        b.setSize(500,40)
        b.align(CENTER, TOP,xMargin=70*getGlobalScale())
        b.onDrop(self.__onRemoveBtn)
        s.addChild(b)

        return s
    

    #######-----------   EVENT HANDLING  -----------#######


    def __onRemoveBtn(self, event):
        def __removeThumb(thumb):
            self.overlayMgr.removeWidget(thumb)

            filePath, mediaType = thumb.getUserData()
            if mediaType == "saved":    # deleting states
                try:
                    statePath = os.path.splitext(filePath)[0]+".state"
                    os.remove(statePath) # the state
                    os.remove(filePath)  # the image (screenshot thumbnail)
                except:
                    tb.print_exc()
                   
            elif os.path.dirname(filePath).endswith("Trash"):  # deleting images
                self.fileServer.DeleteFilePermanently(filePath)
                
            else:
                self.fileServer.DeleteFile(filePath)
                    

            # app window dropped?
        if isinstance(event.object, appOverlayObj.App):
            self.sageGate.shutdownApp(event.object.app.getId())
            event.droppedOnTarget = True

            # thumbnail from mediaBrowser dropped?
        elif isinstance(event.object, thumbnail.Thumbnail):
            __removeThumb(event.object)
            event.droppedOnTarget = True

            # many objects dropped?
        elif isinstance(event.object, multiSelect.MultiSelect):
            selectedOverlayType = event.object.getSelectedOverlayType()
            selectedOverlays = event.object.getSelectedOverlays()

            # close apps
            if selectedOverlayType == OVERLAY_APP:
                for appObj in event.object.getSelectedApps():
                    self.sageGate.shutdownApp(appObj.getId())

            # delete thumbnails
            elif selectedOverlayType == OVERLAY_THUMBNAIL:
                for obj in selectedOverlays:
                    __removeThumb(obj)

            event.droppedOnTarget = True
            event.object.deselectAllOverlays()            


    def __onMaximizeWindow(self, event):
        selectedOverlays = []
        
        if isinstance(event.object, appOverlayObj.App):
            doMax = not event.object.app.maximized

            event.object.app.toggleMaximized()
            event.droppedOnTarget = True
            selectedOverlays.append(event.object)

        elif isinstance(event.object, multiSelect.MultiSelect):
            selectedApps = event.object.getSelectedApps()
            selectedOverlays = event.object.getSelectedOverlays()
            
            doMax = not selectedApps[0].maximized
            if doMax:
                self.tileWindows(selectedApps)
            else:
                self.restoreFromMaximize(selectedApps)

            event.droppedOnTarget = True

        # dim or un-dim the other apps
        #for app in self.overlayMgr.getOverlaysOfType(OVERLAY_APP):
        #    if app not in selectedOverlays and not app.app.isMinimized():
        #        app.dim(doMax)

                
    def _onAppDoubleClick(self, event, appOverlay, twoDoubleClicks):
        apps = []  # SageApp objects
        app = appOverlay.app

        # on which apps are we operating?
        if appOverlay.isSelected():
            apps = event.device.getMultiSelect().getSelectedApps()
        else:
            apps.append(app)

        # figure out what to do
        if twoDoubleClicks and app.sectionMaximized and app.section != getLayoutSections().root:    # just a single double click
            #print "FULL maximize"
            
            self.tileWindows(apps, saveAllBounds=False)
            for a in apps:
                writeLog(LOG_WINDOW_MAXIMIZE, a.getId(), a.getBounds())
                a.sectionMaximized = False
                a.maximized = True
            
        else:
            if not app.maximized and not app.sectionMaximized:   # section maximize
                #print "section maximize"
                self.tileWindows(apps, sectionToTile=app.section)
                for a in apps:
                    writeLog(LOG_WINDOW_MAXIMIZE, a.getId(), a.getBounds())
                    a.sectionMaximized = True
                    a.maximized = False
            else:                      # restore
                #print "restoring SINGLE"
                self.restoreFromMaximize(apps)
                
            
        # dim or un-dim the other apps
        #for app in self.overlayMgr.getOverlaysOfType(OVERLAY_APP):
        #    if app not in selectedOverlays and not app.app.isMinimized():
        #        app.dim(doMax)



    def __onPushToBack(self, event):
        if isinstance(event.object, appOverlayObj.App):
            self.sageGate.pushToBack(event.object.app.getId())
            event.object._updateAppOverlays()
            event.object._resetOverlay() 
            event.droppedOnTarget = True

        elif isinstance(event.object, multiSelect.MultiSelect):
            selectedOverlays = event.object.getSelectedOverlays()
            for appOverlay in selectedOverlays:
                self.sageGate.pushToBack(appOverlay.app.getId())
                appOverlay._updateAppOverlays()
                appOverlay._resetOverlay()
            event.droppedOnTarget = True
            event.deselectAll = True


    def __onSync(self, event):
        local_apps = self.sageData.getRunningApps()
        for lapp in local_apps:
            if local_apps[lapp].isShared:
                # sync the remote copies
                self.overlayMgr.getAppOverlay(local_apps[lapp])._sendAllRemoteSync()
                # sync itself
                self.sageGate.sendAppEvent(APP_SYNC, local_apps[lapp].getSailID())

    def __onShare(self, event):
        if isinstance(event.object, appOverlayObj.App):
            # event.data is defiend in DropEvent in events.py
            # event.data is the ip address
            if event.data == None:
                # to ALL
                hosts = getSharedHost()
                buttons = ShareWithButtons
                for i in range(len(hosts)):
                    if buttons[i].getState() == BUTTON_UP:
                        addGate(hosts[i])
                        self.sageGate.showContentRemotely(event.object.app, hosts[i])
                        event.object.app.sharedHosts.add(hosts[i])
            else:
                addGate(event.data)
                self.sageGate.showContentRemotely(event.object.app, event.data)
                event.object.app.sharedHosts.add(event.data)
            # Enables the sync behavior across sites
            event.object.app.isShared = True
            event.object._updateAppOverlays()
            event.object._resetOverlay() 
            event.droppedOnTarget = True
            # send a SYNC message to the app after sharing
            self.sageGate.sendAppEvent(APP_SYNC, event.object.app.getSailID())

        
        elif isinstance(event.object, multiSelect.MultiSelect):
            selectedOverlays = event.object.getSelectedOverlays()
            for appOverlay in selectedOverlays:
                # send a SYNC message to the app befor sharing
                self.sageGate.sendAppEvent(APP_SYNC, appOverlay.app.getSailID())
                if event.data == None:
                    # to ALL
                    hosts = getSharedHost()
                    buttons = ShareWithButtons
                    for i in range(len(hosts)):
                        if buttons[i].getState() == BUTTON_UP:
                            addGate(hosts[i])
                            self.sageGate.showContentRemotely(appOverlay.app, hosts[i])
                            appOverlay.app.sharedHosts.add(hosts[i])
                else:
                    addGate(event.data)
                    self.sageGate.showContentRemotely(appOverlay.app, event.data)
                    appOverlay.app.sharedHosts.add(event.data)
                appOverlay.app.isShared = True
                appOverlay._updateAppOverlays()
                appOverlay._resetOverlay()
            event.droppedOnTarget = True
            event.deselectAll = True



    def __onLayoutBtn(self, btn):
        section = btn.getUserData()
	print("Section ", section)
        
        if btn.getState() == BUTTON_DOWN:
            writeLog(LOG_SECTION_TILE, section)
            section.tiled = True
            self.tileWindows(sectionToTile=section)
        else:
            section.tiled = False

        
        """
        toMode = btn.getUserData()

        # no mode switch
        #if toMode == self.__tileMode: return

        # to free mode
        if toMode == FREE_MODE:
            self.unTileWindows()

        # to some tile mode
        else:
            # just switching between modes shouldn't save the bounds
            saveB = False
            if self.__tileMode == FREE_MODE:
                saveB = True
            
            # apply the tiling algorithms depending on the mode
            if toMode == TILE_MODE:
                self.tileWindows(saveAllBounds=saveB)
            elif toMode == ONE_PER_DISPLAY_MODE:
                self.tileOnePerDisplay(saveAllBounds=saveB)

        # change the visual state of buttons
        if self.__prevButton != btn:
            self.__prevButton.setState(BUTTON_UP)
            self.__prevButton = btn
        #btn.setState(BUTTON_DOWN)

        self.__tileMode = toMode
        """


            
    def __onNewApp(self, event):
        if self.__tileMode == TILE_MODE:
            event.app.saveBoundsBeforeTile()
            self.tileWindows(saveAllBounds=False)
        elif self.__tileMode == ONE_PER_DISPLAY_MODE:
            event.app.saveBoundsBeforeTile()
            self.tileOnePerDisplay(saveAllBounds=False)


    def __onAppKilled(self, event):
        print("onAppKilled", event.app.appId)
        # If app is shared and in master mode
        if event.app.isShared and getMasterSiteMode():
            local_apps = getSageGate().getAppStatus()
            local_params = local_apps[str(event.app.appId)][1].split()
            directory, local_filename = os.path.split( local_params[-1] )
            print ("Trying to kill>", local_filename, event.app.appId, event.app.sailID, event.app.appName, event.app.title, event.app.windowId, event.app.launcherId, event.app.zValue, event.app.configNum)
            for ip in event.app.sharedHosts:
                rgate, rdata = getGate(ip)
                remote_apps  = rgate.getAppStatus()
                running_apps = rdata.getRunningApps()
                print("Remote apps:", remote_apps)
                winner = None
                for k in remote_apps:
                    ra = remote_apps[k]
                    rparams = ra[1].split()
                    rdir, rfile = os.path.split( rparams[-1] )
                    if rfile == local_filename:
                        winner = int(k)
                print("Running apps:", running_apps)
                for v in running_apps.values():
                    remote_filename = v.title
                    print("One remote app", v.sailID, v.appName, v.title, v.windowId, v.appId, v.launcherId, v.zValue, v.configNum)
                    if v.appName == event.app.appName:
                        if v.appId == winner:
                            print ("    Remote app to kill>", v.sailID)
                            rgate.sendAppEvent(APP_QUIT, v.sailID)
        if event.app in self.__tiledWindows:
            self.__tiledWindows.remove(event.app)



                    

        #if self.__tileMode:
        #    self.tileWindows(saveAllBounds=False)


##     def __onAppInfo(self, event):
##         def anyAppChangedAfterTile():
##             for app in self.__tiledWindows:
##                 if not app.isMinimized():
##                     if app.hasMovedAfterTile():
##                         return True
##             return False
        
##         if self.__tileMode != FREE_MODE and anyAppChangedAfterTile():
##             self.__tileMode == FREE_MODE

##             for app in self.__tiledWindows:
##                 if not app.isMinimized():
##                     app.saveBoundsBeforeTile()
##             self.__tiledWindows = []

##             # change the visual state of buttons
##             self.__prevButton.setState(BUTTON_UP)
##             self.__prevButton = self.fb
##             self.fb.setState(BUTTON_DOWN)





    #######-----------   ORGANIZATION METHODS  -------------########

    
    def unTileWindows(self, windows=None):
        if not windows:
            windows = self.__tiledWindows

        # if we are already in the tile mode, restore the previous state of all apps
        for app in windows:
            app.toFreeLayout()
            app.maximized = False
            self.__tiledWindows.remove(app)
        #self.__tiledWindows = []
        #self.__tileMode = FREE_MODE
    


    def restoreFromMaximize(self, windowsToTile):
        # restore back to tile mode from maximized
        for w in windowsToTile:
            writeLog(LOG_WINDOW_RESTORE, w.getId(), w.getBounds())
            w.restore()
        
                            
    def tileWindows(self, windowsToTile=None, saveAllBounds=True, sectionToTile=None):
        # save the bounds of all the windows to be tiled
        appWindows = {}
        if not windowsToTile:
            windowsToTile = self.sageData.getRunningApps().values()
            for w in windowsToTile:
                if sectionToTile and w.section == sectionToTile:
                    appWindows[w.getId()] = w
                    w.maximized = False
                    if saveAllBounds:
                        w.saveBoundsBeforeMax()  #w.saveBoundsBeforeTile()
            #self.__tileMode = TILE_MODE
                    
        else:      # this is only if we are "maximizing" selected apps while in tile mode already
            for w in windowsToTile:
                w.maximized = True
                if saveAllBounds:
                    w.saveBoundsBeforeMax()
                appWindows[w.getId()] = w

        numWindows = len(appWindows)
        if numWindows == 0: return
        

        # the tiling...
        arDiff = self.__displayAr(sectionToTile) / self.__averageWindowAr(appWindows)

        # 3 scenarios... windows are on average the same aspect ratio as the display
        if arDiff >= 0.7 and arDiff <= 1.3:
            numCols = int(ceil(sqrt( numWindows )))
            numRows = int(ceil(numWindows / float(numCols)))

        elif arDiff < 0.7: # windows are much wider than display
            c = int(round(1 / (arDiff/2)))
            if numWindows <= c:
                numRows = numWindows
                numCols = 1
            else:
                numCols = max(2, round(numWindows / float(c)))
                numRows = int(ceil(numWindows / float(numCols)))

        else:              # windows are much taller than display
            c = int(round(arDiff*2))
            if numWindows <= c:
                numCols = numWindows
                numRows = 1
            else:
                numRows = max(2, round(numWindows / float(c)))
                numCols = int(ceil(numWindows / float(numRows)))


        # determine the bounds of the tiling area
        if sectionToTile:
            areaX = sectionToTile.getX()
            areaY = sectionToTile.getY()
            areaW = sectionToTile.getWidth()
            areaH = sectionToTile.getHeight()
        else:
            areaX = self.disp.usableZeroX
            areaY = self.disp.usableZeroY
            areaW = self.disp.usableW
            areaH = self.disp.usableH

        tileW = int(floor(areaW / float(numCols)))
        tileH = int(floor(areaH / float(numRows)))

        # go through them in sorted order
        windowsById = appWindows.keys()
        windowsById.sort()

        r = numRows-1
        c = 0
        for appId in windowsById:
            app = appWindows[appId]
            app.fitInTile(c*tileW+areaX, r*tileH+areaY, tileW, tileH, margin=10, saveBounds=saveAllBounds)
            self.__tiledWindows.append(app)
            c += 1
            if c == numCols:
                c = 0
                r -= 1


    def tileOnePerDisplay(self, windowsToTile=None, saveAllBounds=True):
        # save the bounds of all the windows to be tiled
        appWindows = {}
        if not windowsToTile:
            windowsToTile = self.sageData.getRunningApps().values()
            for w in windowsToTile:
                w.maximized = False
                appWindows[w.getId()] = w

        # if the number of windows is greater than the number of tiles, just tile regularly
        if len(appWindows) > self.disp.getNumTiles():
            self.tileWindows(None, saveAllBounds)
            return

        # save the bounds
        if len(appWindows) == 0:
            return
        
        elif saveAllBounds:
            for w in appWindows.values():
                w.saveBoundsBeforeTile()

        self.__tileMode = ONE_PER_DISPLAY_MODE

        # tile
        m = int(self.disp.getMullionSize() / 2.0)
        tileW = int(floor(self.sageW / float(self.disp.cols)))
        tileH = int(floor(self.sageH / float(self.disp.rows)))

        # go through them in sorted order
        windowsById = appWindows.keys()
        windowsById.sort()


        r = self.disp.rows-1
        c = 0
        for appId in windowsById:
            app = appWindows[appId]
            app.fitInTile(c*tileW, r*tileH, tileW, tileH, margin=int(m/2.0))
            self.__tiledWindows.append(app)
            c += 1
            if c == self.disp.cols:
                c = 0
                r -= 1
        


    #######-----------   HELPER METHODS  -------------########

 

    def __averageWindowAr(self, appWindows):
        num = len(appWindows)
        if num == 0: return 1.0

        totAr = 0.0
        for appId, app in appWindows.items():
            totAr += app.getAspectRatio()

        return totAr / num


    def __displayAr(self, section=None):
        if section:
            return section.getWidth() / float(section.getHeight())
        else:
            return self.disp.usableW / float(self.disp.usableH)
