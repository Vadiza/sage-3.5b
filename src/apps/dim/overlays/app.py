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




from overlay import Overlay
from thumbnail import Thumbnail
import multiSelect
from eventHandler import EventHandler
from globals import *
from math import cos, sin, radians
import time
from threading import Thread, RLock
from sageGate import addGate, getGate


def makeNew(app):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return App(app)


TYPE = WIDGET_PLUGIN


# messages to the overlay on the display side
RESET  = 0
DRAG   = 1
RESIZE = 2
ON_CORNER = 4
ZOOM   = 5
MINIMIZE = 6
MOUSE_OVER = 7
DIM    = 8
CLOSING_FADE = 9
RESET_CLOSING_FADE = 10


# corner orientations
APP          = 0
APP_BOTTOM   = -2
APP_TOP      = -1
BOTTOM_LEFT  = 1
TOP_LEFT     = 2
TOP_RIGHT    = 3
BOTTOM_RIGHT = 4
TOP_RIGHT_X  = 5

TIME_TO_CLOSE = 0.8  # how long we have to hold EVT_MULTI_TOUCH event to close the app



class App(Overlay, EventHandler):
    
    def __init__(self, app):
        Overlay.__init__(self, OVERLAY_APP)
        EventHandler.__init__(self, app)
        
        # must set size and pos first
        self._canScale = False
        self.setSize(1.0, 1.0)   # same size as the app
        self.setPos(0.0, 0.0)    # right over the app
        self._zOffset = APP_OVERLAY_Z
        self._allowSelection = True

        # some initial params
        self.minAppSize = 250   # min app size in either direction
        self.capturedCorner = APP
        self.lastClickPos = (0,0)
        self.lastClickDeviceId = None
        self.overCorner = APP
        self.bounds = self.getBounds()  # from EventHandler base class
        self._clickDownTime = 0.0
        self.__startedDrag = False  # for accidental touches...

        # if less than X seconds has passed between two double clicks,
        # maximize to the whole display, otherwise, just within the section
        self._lastDoubleClick = 0.0
        self.__maximizeThreshold = 1.3  # seconds between two double clicks...

        # for closing apps
        self.__multiTouchStartTime = -1

        self.org = getLayoutOrganization()

        # get the filename from appLauncher
        if "imageviewer" in app.getName().lower() or \
            "mplayer" in app.getName().lower() or \
            "stereo3d" in app.getName().lower() or \
            "pdf" in app.getName().lower():
            # get the config info from the right appLauncher
            if app.getLauncherId() != "none":
                appLauncher = xmlrpclib.ServerProxy("http://" + app.getLauncherId())
                try:
                    res = appLauncher.getAppConfigInfo( app.getAppId() )
                except:
                    print "\nUnable to connect to appLauncher on", app.getLauncherId(), "so not saving this app: ", app.getName() 
                else:
                    print "appLauncher.getAppConfigInfo : ", res
                    if res != -1:
                        configName, optionalArgs = res
                        if "imageviewer" in app.getName().lower():
                            # filename at the beginning of the command line for images
                            self.fullpathname = optionalArgs.split()[0].strip('"')
                            self.filename     = os.path.basename( self.fullpathname )
                        else:
                            # filename at the end of the command line for movies
                            self.fullpathname = optionalArgs.split()[-1].strip('"')
                            self.filename = os.path.basename( self.fullpathname )
                        if getShowWindowTitleBar():
                            self.setTooltip(self.filename, LARGER)


    def initialize(self, overlayId):
        self.overlayId = overlayId

        # register for the events that are fired by devices
        self.registerForEvent(EVT_MOVE, self.__onOver)
        self.registerForEvent(EVT_CLICK, self.__onClick)
        self.registerForEvent(EVT_DOUBLE_CLICK, self.__onDoubleClick)
        self.registerForEvent(EVT_BIG_CLICK, self.__onBigClick)
        self.registerForEvent(EVT_DRAG, self.__onDrag)
        self.registerForEvent(EVT_FAST_DRAG, self.__onFastDrag)
        self.registerForEvent(EVT_ROTATE, self.__onRotate)
        self.registerForEvent(EVT_ZOOM, self.__onZoom)
        self.registerForEvent(EVT_ARROW, self.__onArrow)
        self.registerForEvent(EVT_LEFT_WINDOW, self.__onLeftWindow)
        self.registerForEvent(EVT_ENTERED_WINDOW, self.__onEnteredWindow)
        self.registerForEvent(EVT_Z_CHANGE, self.__onZChange)
        self.registerForEvent(EVT_MULTI_TOUCH_HOLD, self.__onMultiTouchHold)
        self.registerForEvent(EVT_MULTI_TOUCH_SWIPE, self.__onMultiTouchSwipe)

        # register for the events from the special devices
        self.registerForEvent(EVT_MOVE_SPECIAL, self.__onSpecialMove)
        self.registerForEvent(EVT_ENTERED_WINDOW_SPECIAL, self.__onEnteredWindowSpecial)
        self.registerForEvent(EVT_LEFT_WINDOW_SPECIAL, self.__onLeftWindowSpecial)

        # keyboard events
        self.registerForEvent(EVT_KEY, self.__onKeyboard)

        # register for the drop event for dropping thumbnails from media browser
        self.onDrop(self.__onDrop)

        if self.app.isMinimized():
            self.sendOverlayMessage(MINIMIZE, self.app.getId(), int(True))
        else:
            self._resetOverlay()


    def _destroy(self):
        #self.getUserData().slideHide()
        EventHandler._destroy(self)


    # -------    GENERIC  EVENTS   -----------------    

        # when thumbnails are dropped from the media browser, 
        # just forward the event to the wall object which will show the files
    def __onDrop(self, dropEvt):
        if isinstance(dropEvt.object, Thumbnail) or \
           isinstance(dropEvt.object, multiSelect.MultiSelect):
            getEvtMgr().getWallObj()._onDrop(dropEvt)


    def __onZChange(self, event):
        # Deal with sync'ed layout
        #print("__onZChange: something changed: ", self.app.isShared, event)
        if self.app.isShared and self.app.section and self.app.section.isShared and getMasterSiteMode():
            # Get handlers on the remote site
            rgate, rdata = getGate( getMasterSite() )
            # Look for the right app
            rapps = rdata.getRunningApps()
            for k in rapps:
                v = rapps[k]
                if v.appName == self.app.appName:
                    if self.app.appId == v.appId:
                        print("   Sending bringToFront to the remote site:", v.windowId)
                        zvalue = getSageData().getZvalue(self.app.windowId)
                        if zvalue == 0:
                            rgate.bringToFront(v.windowId)
                        if zvalue == 1:
                            rgate.pushToBack(v.windowId)
        
    def _onAppChanged(self, event):
        if self.app == event.app:
            EventHandler._onAppChanged(self, event)  # updates the bounds in the base class' method
            self._updateAppOverlays()
            self._resetOverlay()

            # Deal with sync'ed layout
            al,ar,at,ab = self.app.getBounds()
            ratio = self.app.getAspectRatio()
            ax,ay = self.app.getCenterX(),self.app.getCenterY()
            aw,ah = self.app.getWidth(),self.app.getHeight() 

            print("_onAppChanged: something changed: ", self.app.isShared, al,ar,at,ab)
            if self.app.isShared and self.app.section and self.app.section.isShared and getMasterSiteMode():
                localw = float ( self.app.section.getWidth() )
                localh = float ( self.app.section.getHeight() )
                localx = float ( self.app.section.getX() )
                localy = float ( self.app.section.getY() )

                ll = al / localw
                lr = ar / localw
                lt = at / localh
                lb = ab / localh

                lx = (ax-localx) / localw
                ly = (ay-localy) / localh
                lw = aw / localh
                lh = ah / localh

                # Get handlers on the remote site
                rgate, rdata = getGate( getMasterSite() )

                # Get the remote display dimensions
                dremote = rdata.getDisplayInfo()
                remotew = float ( dremote.sageW )
                remoteh = float ( dremote.sageH * 0.97)
                offsetx = 0.0
                offsety = 0.0
                # adjust remote dimensions to match local aspect ratio
                if (remotew/remoteh) >= (localw/localh):
                    remotew = remoteh * localw / localh
                    offsetx = ( float ( dremote.sageW ) - remotew ) / 2.0
                else:
                    remoteh = remotew * localh / localw
                    offsety = ( float ( dremote.sageH * 0.97 ) - remoteh ) / 2.0

                # Calculate the remote size
                rl = ll * remotew
                rr = lr * remotew
                rt = lt * remoteh
                rb = lb * remoteh

                rx = lx * remotew
                ry = ly * remoteh
                rw = lw * remoteh
                rh = lh * remoteh

                rl = rx - rw/2
                rr = rx + rw/2
                rt = ry + rh/2
                rb = ry - rh/2

                # center the remote windows
                if offsetx > 0.0:
                    rl += offsetx
                    rr += offsetx
                if offsety > 0.0:
                    rb += offsety
                    rt += offsety
                rb += float ( dremote.sageH * 0.03 )
                rt += float ( dremote.sageH * 0.03 )
                print("Remote position should be:",  int(rl), int(rr), int(rb), int(rt))

                # Get local information
                local_apps = getSageGate().getAppStatus()
                local_params = local_apps[str(event.app.appId)][1].split()
                directory, local_filename = os.path.split( local_params[-1] )
                remote_apps  = rgate.getAppStatus()
                running_apps = rdata.getRunningApps()
                winner = None
                for k in remote_apps:
                    ra = remote_apps[k]
                    rparams = ra[1].split()
                    rdir, rfile = os.path.split( rparams[-1] )
                    if rfile == local_filename:
                        winner = int(k)
                for v in running_apps.values():
                    remote_filename = v.title
                    # print("One remote app", v.sailID, v.appName, v.title, v.windowId, v.appId, v.launcherId, v.zValue, v.configNum)
                    if v.appName == event.app.appName:
                        if v.appId == winner:
                            print ("    Remote app to control>", v.sailID)
                            #rgate.sendAppEvent(APP_QUIT, v.sailID)
                            rgate.resizeWindow(v.windowId, int(rl), int(rr), int(rb), int(rt))


                # # Look for the right app
                # rapps = rdata.getRunningApps()
                # for k in rapps:
                #     v = rapps[k]
                #     if v.appName == self.app.appName:
                #         if self.app.appId == v.appId:
                #             print("   Sending the new position to the remote site:", v.windowId)
                #             rgate.resizeWindow(v.windowId, int(rl), int(rr), int(rb), int(rt))


    def __onLeftWindow(self, event):
        # change the app overlay to default
        if self.overCorner != APP:
            self.sendOverlayMessage(ON_CORNER, self.overCorner, 0)
            self.overCorner = APP

        # change the pointer to default
        pointer = event.device.pointer
        event.device.dragMode = False
        if pointer:
            pointer.showOutApp()
            pointer.resetPointer()

        self.sendOverlayMessage(MOUSE_OVER, 0, self.app.getId())


    def __onEnteredWindow(self, event):
        self.sendOverlayMessage(MOUSE_OVER, 1, self.app.getId())
        
        

    # -------    SPECIAL DEVICE EVENTS   -----------------

    def __onSpecialMove(self, event):
        x,y = self.__toAppCoords(event.x, event.y)
        dx,dy = self.__toAppDist(event.dX, event.dY)
        self.sageGate.sendAppEvent(EVT_MOVE, self.app.getSailID(), event.device.getSpecialId(), x, y, dx, dy)


    def __onEnteredWindowSpecial(self, event):
        pointer = event.device.pointer
        if pointer: pointer.hide()


    def __onLeftWindowSpecial(self, event):
        pointer = event.device.pointer
        if pointer: pointer.show()



    # -------    DEVICE EVENTS   -----------------
    
    def __onMultiTouchHold(self, event):
        
        now = time.time()
        
        if event.lifePoint == GESTURE_LIFE_BEGIN:
            self.__multiTouchStartTime = now  # starting time

        elif event.lifePoint == GESTURE_LIFE_MIDDLE:
            timeDiff = now - self.__multiTouchStartTime
            if timeDiff < TIME_TO_CLOSE:
                fadeTo = 1 - timeDiff / TIME_TO_CLOSE
                self.__showClosingFade(max(0.0, fadeTo))

            else:                  # we held it long enough so close the app
                self.__resetClosingFade()
                event.device._resetMultiTouch()
                self.sageGate.shutdownApp(self.app.getId())
        
        elif event.lifePoint == GESTURE_LIFE_END:
            self.__resetClosingFade()

                
    def __onMultiTouchSwipe(self, event):
        self.app.section._onMultiTouchSwipe(event)          # forward the swipe event to the section we are in


    def __onOver(self, event):
        corner = self.app.hitTest(event.x, event.y, self.cornerSize)
        # touch screen shouldnt worry about corners at all
        if event.device.deviceType == "pqlabs":
            corner = APP

        device = event.device
        pointer = device.pointer

        # TO APP
        if not event.device.globalMode:   
            x,y = self.__toAppCoords(event.x, event.y)
            dx,dy = self.__toAppDist(event.dX, event.dY)
            self.sageGate.sendAppEvent(EVT_MOVE, self.app.getSailID(), -1, x, y, dx, dy)

        # TO APP OVERLAY
        else:
            if pointer:
                if corner > APP and (self.app.getWidth() > self.cornerSize*2 and \
                                     self.app.getHeight() > self.cornerSize*2) and \
                                     not self.isSelected():    # corners
                    pointer.showResizePointer(corner)
                    pointer.showOutApp()
                    device.dragMode = True
                    if self.overCorner != corner:
                        self.sendOverlayMessage(ON_CORNER, corner, 1)
                        self.sendOverlayMessage(ON_CORNER, self.overCorner, 0)
                        self.overCorner = corner


                else:              # everything but the corners
                    pointer.resetPointer()
                    device.dragMode = True
                    if self.overCorner != corner:
                        self.sendOverlayMessage(ON_CORNER, self.overCorner, 0)
                        self.overCorner = corner


    ##             else:      # interior of the app
    ##                 #pointer.resetPointer()
    ##                 pointer.showInApp()
    ##                 device.dragMode = False
    ##                 if self.overCorner != corner:
    ##                     self.sendOverlayMessage(ON_CORNER, self.overCorner, 0)
    ##                     self.overCorner = corner


    def __onKeyboard(self, event):
        # TO APP
        x, y = event.x, event.y
        if not event.device.globalMode:
            self.sageGate.sendAppEvent(EVT_KEY, self.app.getSailID(), -1, x, y, event.key)

                
    def __onClick(self, event):
        touch = (event.device.deviceType == "pqlabs")
        if touch:
            corner = APP   # touch screen shouldnt worry about corners at all
        else:
            corner = self.app.hitTest(event.x, event.y, self.cornerSize)

        pointer = event.device.pointer
        x, y = event.x, event.y

        # TO APP
        if not event.device.globalMode:  
            x,y = self.__toAppCoords(x,y)
            self.sageGate.sendAppEvent(EVT_CLICK, self.app.getSailID(), -1, x, y, event.btnId, int(event.isDown), event.forEvt+31000)

            # change to global mode
            if event.btnId == 3 and not event.isDown:
                event.device.toGlobalMode()


        # TO APP OVERLAY
        else:
            # luc
            # change to global mode
            #print("BUTTON: ", event.btnId)
            if event.btnId == 3 and not event.isDown:
                event.device.toAppMode(self.bounds)


            # BUTTON DOWN
            if event.isDown:

                if self.app.isMinimized(): 
                    self.app.restoreFromMinimze()
                    return
                
                event.device.toEvtHandler = self   # tell the device to forward all events here (until the button goes up)

                # Send APP_QUIT if the X was clicked
                if corner == TOP_RIGHT_X:
                    self.sageGate.sendAppEvent(APP_QUIT, self.app.getSailID())

                if event.forEvt == EVT_DRAG:
                    self.capturedCorner = corner
                    self.lastClickPos = (x, y)
                    if pointer: pointer.pushState()    # preserve the state of the pointer

                    self.app.bringToFront()

                    # touches shouldnt be able to resize by dragging corners
                    if self.capturedCorner> APP and (self.app.getWidth() > self.cornerSize*2 and \
                                                     self.app.getHeight() > self.cornerSize*2) and \
                                                     not self.isSelected():  # corner
                        self._captured = True
                        self.lastClickDeviceId = event.device.deviceId
                        if pointer: pointer.showResizePointer(self.capturedCorner)
                        self.sendOverlayMessage(RESIZE, 0,0,self.capturedCorner)

                    else:      # everything but the corners
                        if not touch:
                            self._captured = True
                            self.allowDrag(True)
                            self.lastClickDeviceId = event.device.deviceId
                            if pointer: pointer.showDragPointer()
                            self.sendOverlayMessage(DRAG, 0,0, self.capturedCorner)
                        else:
                            self.__startedDrag = False
                            self._clickDownTime = time.time()  # so we can disregard accidental touches
                        
                        #self.getUserData().slideShow()
                        
                    ##else:         # to app
                    ##    x,y = self.__toAppCoords(x,y)
                    ##    self.sageGate.sendAppEvent(EVT_CLICK, self.app.getSailID(), -1, x, y, event.btnId, 1, event.forEvt+31000)


            # BUTTON UP
            elif not event.isDown:
                self._captured = False
                dX = x - self.lastClickPos[0]
                dY = y - self.lastClickPos[1]
                
                if pointer: pointer.popState()   # return to the previous pointer state

                if event.forEvt == EVT_DRAG:
                    if not touch and self.capturedCorner > APP and (self.app.getWidth() > self.cornerSize*2 and \
                                                      self.app.getHeight() > self.cornerSize*2) and \
                                                      not self.isSelected():  # corners
                        if event.device.deviceId == self.lastClickDeviceId:
                            self.lastClickDeviceId = None
                            self.__resizeBounds(dX, self.capturedCorner)
                            self.app.resizeWindow(self.bounds.left,
                                                       self.bounds.right, self.bounds.bottom,
                                                       self.bounds.top)
                    else:              # everything but the corners  
                        if event.device.deviceId == self.lastClickDeviceId:
                            self.allowDrag(False)
                            self.lastClickDeviceId = None

                            #self.getUserData().slideHide()

                            # was app dropped on a button or a new section?
                            # if not, move the window normally
                            if not event.droppedOnTarget:
                                newSec = getLayoutSections().isAppInNewSection(self.app, x, y)

                                if newSec and newSec._isMinimizePanel:
                                    event.deselectAll = True

                                if newSec and not self.app.maximized:   # the app is in another section now
                                    newSec.fitAppInSection(self.app, x, y)

                                else:        # just move the window
                                    #print "app moved: ", self.app.getId()
                                    self.app.moveWindow(dX, dY)


                elif event.forEvt == EVT_ZOOM or event.forEvt == EVT_FAST_DRAG:
                    self.app.resizeWindow(self.bounds.left,
                                               self.bounds.right, self.bounds.bottom,
                                               self.bounds.top)
        

    def __onDoubleClick(self, event):
        if self.app.isMinimized(): return
        
        pointer = event.device.pointer
        x, y = event.x, event.y

        # TO APP
        if not event.device.globalMode:  
            x,y = self.__toAppCoords(x,y)
            self.sageGate.sendAppEvent(EVT_DOUBLE_CLICK, self.app.getSailID(), -1, x, y, event.btnId, 0, event.forEvt+31000)

        else:
		now = time.time()
		twoDoubleClicks = False  # by default, we use the second double click to restore...
		if now - self._lastDoubleClick < self.__maximizeThreshold:
		    twoDoubleClicks = True
		    self._lastDoubleClick = 0.0
		else:
		    self._lastDoubleClick = time.time()

		# do something depending on 1 or 2 double clicks in a row
		self.app.bringToFront()
		self.org._onAppDoubleClick(event, self, twoDoubleClicks)

		 # reset the time if we just went from full maximize to restore
		if not self.app.maximized and not self.app.sectionMaximized:
		    self._lastDoubleClick = 0.0  
		    
		# if more than one app is selected, change their times as well
		if self.isSelected():
		    for a in event.device.getMultiSelect().getSelectedOverlays():
			a._lastDoubleClick = self._lastDoubleClick



    def __onBigClick(self, event):
        if self.app.isMinimized(): return
        if event.lifePoint != GESTURE_LIFE_BEGIN: return  # use the gesture only once

        pointer = event.device.pointer
        x, y = event.x, event.y
        self.sageGate.pushToBack(self.app.getId())


    def __onDrag(self, event):
        touch = (event.device.deviceType == "pqlabs")
        startX, startY = event.startX, event.startY
        x, y = event.x, event.y
        dx, dy, dz = event.dX, event.dY, event.dZ
        pointer = event.device.pointer

        # TO APP
        if not event.device.globalMode:
            x,y = self.__toAppCoords(x, y)
            dx,dy = self.__toAppDist(dx, dy)
            self.sageGate.sendAppEvent(EVT_DRAG, self.app.getSailID(), -1, x, y, dx, dy, dz)

        # TO APP OVERLAY
        else:
            if self.app.isMinimized(): return

            # if the drag event originated from elsewhere, don't use it
            if event.device.toEvtHandler != self and event.original:
                return
            
            elif self.capturedCorner > APP and (self.app.getWidth() > self.cornerSize*2 and \
                                                self.app.getHeight() > self.cornerSize*2) and \
                                                not self.isSelected():       # corner
                self.sendOverlayMessage(RESIZE, dx, dy, self.capturedCorner)
                
            else:        # everything but the corners
                if touch:
                    if time.time() - self._clickDownTime > ACCIDENTAL_TOUCH_THRESHOLD:
                        if not self.__startedDrag:
                            self.allowDrag(True)
                            self.lastClickDeviceId = event.device.deviceId
                            self.__startedDrag = True
                            self.lastClickPos = (event.device.realX, event.device.realY)
                        self.sendOverlayMessage(DRAG, dx, dy, APP)
                    else:
                        self.__startedDrag = False
                else:
                    self.sendOverlayMessage(DRAG, dx, dy, APP)
                

    
    def __onFastDrag(self, event):
        startX, startY = event.startX, event.startY
        x, y = event.x, event.y
        dx, dy, dz = event.dX, event.dY, event.dZ
        pointer = event.device.pointer

        # TO APP OVERLAY
        # if the drag event originated from elsewhere, don't use it
        if event.device.toEvtHandler != self and event.original:
            return
        else:     
            dX = dx*5
            dY = dy*5

            # just change the bounds
            self.bounds.left += dX
            self.bounds.right += dX
            self.bounds.top += dY
            self.bounds.bottom += dY

            self.sendOverlayMessage(DRAG, dX, dY, APP)

    
    def __onRotate(self, event):
        x, y = event.x, event.y
        dx, dy, dz = event.dX, event.dY, event.dZ

        # TO APP
        if not event.device.globalMode:
            x,y = self.__toAppCoords(x, y)
            dx,dy = self.__toAppDist(dx, dy)
            self.sageGate.sendAppEvent(EVT_ROTATE, self.app.getSailID(), -1, x, y, dx, dy, dz)



    def __onZoom(self, event):
        startX, startY = event.startX, event.startY
        x, y = event.x, event.y
        dx, dy, dz = event.dX, event.dY, event.dZ

        # TO APP
        if not event.device.globalMode:
            x,y = self.__toAppCoords(event.x, event.y)
            dx,dy = self.__toAppDist(dx, dy)
            self.sageGate.sendAppEvent(EVT_ZOOM, self.app.getSailID(), -1, x, y, dx, dy, dz)
            
        # TO APP OVERLAY
        else:
            if self.app.isMinimized(): return

            dZoom = dx

            # aspect ratio
            appAR = self.app.getAspectRatio()

            # adjust the dZoom if we went too small
            if appAR < 0:
                if self.bounds.getWidth() == self.minAppSize and dZoom < 0:
                    return
                elif (self.bounds.getWidth() + 2*dZoom) < self.minAppSize:
                    dx = -1 * (self.bounds.getWidth() - self.minAppSize)/2
                else:
                    dx = dZoom
                dy = int(round(dx/appAR))

            else:
                if self.bounds.getHeight() == self.minAppSize and dZoom < 0:
                    return
                elif (self.bounds.getHeight() + 2*dZoom) < self.minAppSize:
                    dy = -1 * (self.bounds.getHeight() - self.minAppSize)/2
                else:
                    dy = dZoom
                dx = int(round(dy*appAR))

            # just change the bounds
            self.bounds.left -= dx
            self.bounds.right += dx
            self.bounds.top += dy
            self.bounds.bottom -= dy
            
 
            self.sendOverlayMessage(ZOOM, self.bounds.left,
                                    self.bounds.right, self.bounds.top,
                                    self.bounds.bottom)

            
    def __onArrow(self, event):
        self.sageGate.sendAppEvent(EVT_ARROW, self.app.getSailID(), -1, event.arrow)




    # -------    HELPER METHODS   -----------------


#    def dim(self, doDim):
#        self.sendOverlayMessage(DIM, int(doDim))


    def minimize(self, doMin):
        if self.app.isMinimized() != doMin:
            self.app.minimize(doMin)
            self.sendOverlayMessage(MINIMIZE, self.app.getId(), int(doMin))

        self.sageGate.sendAppMessage(APP_MINIMIZED, self.app.getSailID(), int(doMin))
        self.app.bringToFront()
       
            
    def _resetOverlay(self):
        self.__setCornerSize()
        self.sendOverlayMessage(RESET, self.cornerSize, self.displayId)
        self.overCorner = APP

    
    def __showClosingFade(self, fadeTo):
        self.sendOverlayMessage(CLOSING_FADE, fadeTo)

    def __resetClosingFade(self):
        self.__multiTouchStartTime = -1   # reset the start time
        self.sendOverlayMessage(RESET_CLOSING_FADE)

    
    def __toAppCoords(self, x, y):
        """ converts to the normalized coords with respect to the app window
            accounts for orientation of the sage window
        """
        orientation = self.app.getOrientation()
        
        if orientation == 0:
            x -= self.bounds.left
            y -= self.bounds.bottom
            normX = float(x)/self.bounds.getWidth()
            normY = float(y)/self.bounds.getHeight()
        elif orientation == 180:
            x = self.bounds.right - x
            y = self.bounds.top - y
            normX = float(x)/self.bounds.getWidth()
            normY = float(y)/self.bounds.getHeight()
        elif orientation == 90:
            tmp = y
            y = self.bounds.right - x
            x = tmp - self.bounds.bottom
            normX = float(x)/self.bounds.getHeight()
            normY = float(y)/self.bounds.getWidth()
            
        elif orientation == 270:
            tmp = y
            y = x - self.bounds.left
            x = self.bounds.top - tmp
            normX = float(x)/self.bounds.getHeight()
            normY = float(y)/self.bounds.getWidth()
            
        return normX, normY

            
    def __toAppDist(self, dx, dy):
        """ converts to the normalized coords with respect to the app window
            accounts for orientation of the sage window
        """
        orientation = self.app.getOrientation()

        if orientation == 0:
            normX = float(dx)/self.bounds.getWidth()
            normY = float(dy)/self.bounds.getHeight()
        elif orientation == 180:
            dx = -dx
            dy = -dy
            normX = float(dx)/self.bounds.getWidth()
            normY = float(dy)/self.bounds.getHeight()
        elif orientation == 90:
            tmp = dy
            dy = -dx
            dx = tmp
            normX = float(dx)/self.bounds.getHeight()
            normY = float(dy)/self.bounds.getWidth()
        elif orientation == 270:
            tmp = dy
            dy = dx
            dx = -tmp
            normX = float(dx)/self.bounds.getHeight()
            normY = float(dy)/self.bounds.getWidth()
            
        #normX = float(dx)/self.bounds.getWidth()
        #normY = float(dy)/self.bounds.getHeight()

        return normX, normY

        #return float(dx)/self.bounds.getWidth(), float(dy)/self.bounds.getHeight()
            

    def __setCornerSize(self):
        maxSize = 400
        minSize = 30
        self.cornerSize = min(maxSize, max(minSize, int(min(self.bounds.getHeight(),
                                                            self.bounds.getWidth()) / 8)))
        self.cornerSize = int(self.cornerSize*getCornerScale())


    def _updateAppOverlays(self):
        self.sendOverlayMessage(SET_APP_BOUNDS, self.app.getId())

        
    def __resizeBounds(self, dx, corner):
        # change the bounds based on the amount moved since the click
        # also, don't resize smaller than the minAppSize
        (l,r,t,b) = self.bounds.getAll()
        
        if corner == BOTTOM_LEFT:
            l+=dx; b=b+dx/self.bounds.aspectRatio
            if r-l < self.minAppSize and (r-l) <= (t-b):
                l = r - self.minAppSize
                b = t - int(self.minAppSize / self.bounds.aspectRatio)
            elif t-b < self.minAppSize and (t-b) < (r-l):
                b = t - self.minAppSize
                l = r - int(self.minAppSize * self.bounds.aspectRatio)
        elif corner == TOP_LEFT:
            l+=dx; t=t-dx/self.bounds.aspectRatio 
            if r-l < self.minAppSize and (r-l) <= (t-b):
                l = r - self.minAppSize
                t = b + int(self.minAppSize / self.bounds.aspectRatio)
            elif t-b < self.minAppSize and (t-b) < (r-l):
                t = b + self.minAppSize
                l = r - int(self.minAppSize * self.bounds.aspectRatio)
        elif corner == TOP_RIGHT:
            r+=dx; t=t+dx/self.bounds.aspectRatio
            if r-l < self.minAppSize and (r-l) <= (t-b):
                r = l + self.minAppSize
                t = b + int(self.minAppSize / self.bounds.aspectRatio)
            elif t-b < self.minAppSize and (t-b) < (r-l):
                t = b + self.minAppSize
                r = l + int(self.minAppSize * self.bounds.aspectRatio)
        elif corner == BOTTOM_RIGHT:
            r+=dx; b=b-dx/self.bounds.aspectRatio
            if r-l < self.minAppSize and (r-l) <= (t-b):
                r = l + self.minAppSize
                b = t - int(self.minAppSize / self.bounds.aspectRatio)
            elif t-b < self.minAppSize and (t-b) < (r-l):
                b = t - self.minAppSize
                r = l + int(self.minAppSize * self.bounds.aspectRatio)

        self.bounds.setAll(l,r,t,b)

