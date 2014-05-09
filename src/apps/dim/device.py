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




import events
from globals import *
import overlays.pointer as pointer
import overlays.multiSelect as multiSelect
import overlays.app as app
import time
import overlays.wall as wall
from threading import Timer

# for smoothing of input data
SMOOTHING_BUFFER_SIZE = 10   # how many previous values do we take into account?
SMOOTHING_MODIFIER    = 0.2  # the amount of smoothing, and delay (0=none, 1=tons)


DOUBLE_CLICK_TIME = 0.7  # how many seconds between clicks still counts as double click



class Device:
    """ events is a list of eventIds that this device can generate """

    def __init__(self, deviceType, deviceId, needPointer, displayId=0):
        self.deviceType = deviceType
        self.deviceId = deviceId
        self.displayId = displayId
        self.destroyMe = False   # used for touch interaction
        self.__smoothingBuffer = []  # history of previous values (tuples of (x,y))
        
        # set the bounds
        di = getSageData().getDisplayInfo(self.displayId)
        self.bounds = Bounds(0, di.sageW, di.sageH, 0)
        self.displayBounds = self.bounds   # first bounds are always equal to the display size
        self.globalMode = True  # False if in appMode... ie the events are forwarded to the app
        self.toEvtHandler = None  # all our the events should go to this handler (ie device captured by the evtHandler)
        self.lastHandler = None  # this is used for generating EVT_LEFT_WINDOW and EVT_ENTERED_WINDOW
        self.dragMode = False     # an overlay can request this device to produce drag event for a bit
        self._prevEvent = None   # what was the previous posted event???
        self._captureMode = False  # for capturing the screenshots
        self.__forEvt = None  # to keep track of which button is in the middle of a click now

        # this is for app specific stuff such as magic carpet lenses
        # these devices send special events
        self.specialDevice = False   
        self.specialId = 0        
        
        self.evtMgr = getEvtMgr()  # from globals
        self.overlayMgr = getOverlayMgr()  # from globals
        self.devMgr = getDevMgr()   # from globals

        self.evtMgr.register(EVT_SCREENSHOT_SAVED, self.__onScreenshotSaved)

        # this is the object that allows multi overlay manipulation and selection
        self.__multiSelect = multiSelect.makeNew()
        self.__dragSelected = []  # temp array of apps that are selected via drag select

        # double click time tracking
        self.__firstClickTime = self.__lastClickTime = 0.0
        self._hasDoubleClick = False  # has separate double click event?
        
        # we create a pointer if needed
        self.pointer = None   
        if needPointer:
            self.makePointer()

    
    def makePointer(self, x=0, y=0):
        deviceName = self.deviceId.split(":")[1]
        self.pointer = self.devMgr.getPointerForDevice(deviceName)
        if not self.pointer:
            ptrToAdd = pointer.makeNew(x,y)
            if not deviceName.startswith("joystick"):
                ptrToAdd.setTooltip(" ".join(deviceName.split("%")))
                ptrToAdd.displayId = self.displayId
            if self.deviceType == "pqlabs":
                ptrToAdd.setImage("images/touch.png")
            self.overlayMgr.addOverlay(ptrToAdd, self.onPointerAdded)
        #else:
            #self.pointer.show()  # already exists so just show it


    def destroy(self):
        if self.pointer:
            self.deselectAllOverlays()
            self.overlayMgr.removeOverlay(self.pointer.overlayId)
            """
            if self.deviceType == "pqlabs":
                #self.pointer.hide()  # just hide it...
                pass
            else:
                self.overlayMgr.removeOverlay(self.pointer.overlayId)
            """

    def onPointerAdded(self, pointerOverlay):
        self.pointer = pointerOverlay


    def selectOverlay(self, overlay):
        self.__multiSelect.selectOverlay(overlay)


    def deselectOverlay(self, overlay):
        self.__multiSelect.deselectOverlay(overlay)
        if overlay in self.__dragSelected:
            self.__dragSelected.remove(overlay)


    def eventToMultiSelect(self, event):
        self.__multiSelect.distributeEvent(event)

        
    def deselectAllOverlays(self):
        """ called by the Wall object if it detects a click """
        self.__multiSelect.deselectAllOverlays()
        self.__dragSelected = []
        
        
    def getMultiSelect(self):
        return self.__multiSelect
        

    def toGlobalMode(self):
        self.bounds = self.displayBounds
        self.globalMode = True


    def toAppMode(self, appBounds):
        self.bounds = appBounds
        self.globalMode = False

        # now convert the position to those new bounds
        #self.x = float(self.x - newBounds.left) / newBounds.getWidth()
        #self.y = float(self.y - newBounds.bottom) / newBounds.getHeight()
        
        
    def onMessage(self, data, firstMsg=False):
        """ This method gets called when a message arrives from the HWCapture
            for this device. In here you should provide the conversion from
            HW events to the SAGE App Events. FirstMsg just signifies
            that this is the very first message right after the device creation.
            
            Must be implemented by the subclass.
        """
        
        raise NotImplementedError

    def isAlive(self):
        return True

    def setSpecialId(self, newId):
        self.specialId = newId


    def getSpecialId(self):
        """ When using special devices you need to specify which deviceId will be reported """
        return self.specialId


    def __onDragSelect(self, event):
        if self.pointer:
            l,r,t,b = event.startX, event.x, event.y, event.startY

            # swap values if necessary
            if l > r: l,r = r,l
            if b > t: t,b = b,t

            self.pointer.showSelection(l,r,t,b)

            overlayType = self.__multiSelect.getSelectedOverlayType()

            # in case we started the selection on the wall, nothing will be selected
            if not overlayType:
                overlayType = OVERLAY_APP

            if overlayType == OVERLAY_APP or overlayType == OVERLAY_THUMBNAIL:
                for overlay in self.overlayMgr.getOverlaysOfType([overlayType]):
                    if overlay._allowSelection and overlay.isVisible() and overlay._isParentVisible():
                        if overlayType == OVERLAY_APP and overlay.app.isMinimized():
                            continue
                        
                        overlaps = overlay.doesOverlap(l,r,t,b)

                        # first time selecting this app
                        if overlaps and not overlay in self.__dragSelected:
                            self.selectOverlay(overlay)
                            self.__dragSelected.append(overlay)

                        # this app was selected but now it's not so deselect it
                        elif not overlaps and overlay in self.__dragSelected:
                            self.deselectOverlay(overlay)
                    elif overlay.isVisible() and not overlay._isParentVisible():
                        pass #print "Im visible but parent isnt!!! ", overlayType

            
    def __startSelection(self, event):
        evtHandler = self.evtMgr.getEvtHandlerAtPos(event.x, event.y, 0, event)

        if isinstance(evtHandler, wall.Wall):
            self.__dragSelected = []
            event.device.deselectAllOverlays()

        # if we are dealing with handlers that can be selected
        elif evtHandler and evtHandler._allowSelection:

            if not evtHandler.isSelected():
                self.selectOverlay(evtHandler)
                self.__dragSelected.append(evtHandler)
            else:
                self.deselectOverlay(evtHandler)

        self.toEvtHandler = evtHandler
        

    def __stopSelection(self, event):
        self.toEvtHandler = None
        if self.pointer:
            self.pointer.resetPointer()



    
    def setCaptureMode(self, doCapture):
        """ when the capture mode is enabled for a device """
        
        if doCapture:
            self._captureMode = True
            if self.pointer: self.pointer.showCross()
        else:
            self._captureMode = False
            if self.pointer: self.pointer.resetPointer()
            

    def __startScreenCapture(self, event):
        """ when we start selecting a capture region with a device """
        self.startX, self.startY = event.x, event.y


    def __onDragCapture(self, event):
        """ as we are selecting a capture region with a device """
        if self.pointer:
            l,r,t,b = event.startX, event.x, event.y, event.startY

            # swap values if necessary
            if l > r: l,r = r,l
            if b > t: t,b = b,t

            if distance(self.startX, event.x, self.startY, event.y) > 150*getGlobalScale():
                self.pointer.showCapture(True,l,r,t,b)
            else:
                self.pointer.showCapture(False,l,r,t,b)


    def __stopScreenCapture(self, event):
        """ when we stop selecting a capture region with a device """
        self.setCaptureMode(False)

        if distance(self.startX, event.x, self.startY, event.y) > 150*getGlobalScale():
            saveScreenshot(Bounds(min(event.x, self.startX), max(event.x, self.startX),
                                  max(event.y, self.startY), min(event.y, self.startY)),
                           saveState=False,
                           caller=self)
            

    def __onScreenshotSaved(self, event):
        if event.caller == self:
            bounds = event.bounds
            showFile(event.screenshot, bounds.getCenterX(), bounds.getCenterY())
            



    #---------------------------------   DEVICE EVENTS   -----------------------------
    #
    #  these are the methods to call when an event occurs
    #  this will pass the event on to the system to be handled
    #
    #---------------------------------------------------------------------------------
            
    def postEvent(self, evt):
        if not self._captureMode:
            self._prevEvent = evt
            self.evtMgr.postEvent( evt )

    
    def postEvtMove(self, newX, newY, dX, dY):
        evt = events.MoveEvent(self, newX, newY, dX, dY, self.toEvtHandler)
        self.postEvent( evt )
            

    def postEvtAnalog1(self, startX, startY, x, y, dX, dY, dZ):
        evt = events.Analog1Event(self, startX, startY, x, y, dX, dY, dZ, self.toEvtHandler)

        # prevent other click events from happening if we are in the middle of a click already
        if self.__forEvt != EVT_ANALOG1 and self.deviceType != "pqlabs":
            return

        
        if self.globalMode and self._captureMode:
            if distance(startX, x, startY, y) > 60*getGlobalScale():
                self.__onDragCapture(evt)
            #else:
            #    self._captureMode = False
            #    if self.pointer: self.pointer.resetPointer()

        self.postEvent( evt )

            
    def postEvtAnalog2(self, startX, startY, x, y, dX, dY, dZ):
        evt = events.Analog2Event(self, startX, startY, x, y, dX, dY, dZ, self.toEvtHandler)

        # prevent other click events from happening if we are in the middle of a click already
        if self.__forEvt != EVT_ANALOG2 and self.deviceType != "pqlabs":
            return


        if self.globalMode:
            if distance(startX, x, startY, y) > 60*getGlobalScale():
                self.__onDragSelect(evt)
                
        self.postEvent( evt )


    def postEvtAnalog3(self, startX, startY, x, y, dX, dY, dZ):
        evt = events.Analog3Event(self, startX, startY, x, y, dX, dY, dZ, self.toEvtHandler)

        # prevent other click events from happening if we are in the middle of a click already
        if self.__forEvt != EVT_ANALOG3 and self.deviceType != "pqlabs":
            return

        self.postEvent( evt )

    def postEvtFastDrag(self, startX, startY, x, y, dX, dY, dZ):
        evt = events.FastDragEvent(self, startX, startY, x, y, dX, dY, dZ, self.toEvtHandler)
        self.postEvent( evt )

    def postEvtDoubleClick(self, x, y, btnId, forEvt):
        #if self.globalMode:
	evt = events.DoubleClickEvent(self, x, y, btnId, forEvt, self.toEvtHandler)
	self.postEvent( evt )

    def postEvtMultiTouchSwipe(self, x, y, numPoints, dX, dY, startX, startY, lifePoint):
        # this is multiple fingers AT THE SAME TIME... not successive clicks...
        evt = events.MultiTouchSwipeEvent(self, x, y, numPoints, dX, dY, startX, startY, lifePoint, self.toEvtHandler)
        self.postEvent( evt )

    def postEvtMultiTouchHold(self, x, y, numPoints, lifePoint):
        # this is multiple fingers AT THE SAME TIME... not successive clicks...
        evt = events.MultiTouchHoldEvent(self, x, y, numPoints, lifePoint, self.toEvtHandler)
        self.postEvent( evt )

    def postEvtBigClick(self, x, y, lifePoint):
        evt = events.BigClickEvent(self, x, y, lifePoint, self.toEvtHandler)
        self.postEvent( evt )
    
    def postEvtClick(self, x, y, btnId, isDown, forEvt):
        evt = events.ClickEvent(self, x, y, btnId, isDown, forEvt, self.toEvtHandler)

        # prevent other click events from happening if we are in the middle of a click already

        if self.toEvtHandler and self.toEvtHandler._captured and self.__forEvt != forEvt:
            return

        # prevent other buttons from doing anything while we are in the middle of a click already
        if isDown:
            self.__forEvt = forEvt
        else:
            self.__forEvt = None
        
        # intercept the selection...
        if self.globalMode and forEvt == EVT_ROTATE:
            if isDown:
                self.__startSelection(evt)
            else:
                self.__stopSelection(evt)
            return

        # intercept the screen capture...
        elif self.globalMode and forEvt == EVT_DRAG and self._captureMode:
            if isDown:
                self.__startScreenCapture(evt)
            else:
                self.__stopScreenCapture(evt)
            return

        # keep track of time for double clicks
        if isDown and forEvt == EVT_DRAG:
            self.__firstClickTime = self.__lastClickTime
            self.__lastClickTime = time.time()

        # if a touch is dragging an app, make it droppable at far distances... 
        # otherwise it gets dropped where the actual touch is
        #if isinstance(self.toEvtHandler, app.App) and self.deviceType == "pqlabs":
        #    x = self.toEvtHandler.lastClickPos[0] + self.toEvtHandler.pqDX
        #    y = self.toEvtHandler.lastClickPos[1] + self.toEvtHandler.pqDY

        # generate a drop event if needed
        dropEvt = None
        if not isDown:
            if self.toEvtHandler and \
                self.toEvtHandler._doesAllowDrag() and forEvt == EVT_DRAG:
                dropEvt = events.DropEvent(self, x, y, self.toEvtHandler, None)
                self.postEvent(dropEvt)

        if dropEvt:
            evt.droppedOnTarget = dropEvt.droppedOnTarget

        self.postEvent( evt )  # the click event!!!

        # if we dropped all the selected objects, deselect them if needed
        if dropEvt and dropEvt.deselectAll:
            dropEvt.device.deselectAllOverlays()
        
        if evt.deselectAll:
            evt.device.deselectAllOverlays()

        # not captured anymore
        if not isDown:
            self.toEvtHandler = None

        # post EVT_DOUBLE_CLICK if it happened...
        #if not isDown: print "Time: ", time.time() - self.__firstClickTime
        if not self._hasDoubleClick and not isDown and forEvt == EVT_PAN and \
               (time.time() - self.__firstClickTime < DOUBLE_CLICK_TIME):
            evt = events.DoubleClickEvent(self, x, y, btnId, forEvt, self.toEvtHandler)
            self.postEvent( evt )
            self.__lastClickTime = 0.0
        

    def postEvtArrow(self, arrow, x, y):
        evt = events.ArrowEvent(self, arrow, x, y, self.toEvtHandler)
        self.postEvent( evt )


    def postEvtKey(self, key, x, y):
        evt = events.KeyEvent(self, key, x, y, self.toEvtHandler)
        self.postEvent( evt )


    def postEvtCustom(self, data):
        evt = events.CustomEvent(self, data, self.toEvtHandler)
        self.postEvent( evt )
        



    ### should be called by the subclass if smoothing is desired for every frame (new input)
    ### takes in two new values for x and y and returns smoothed values for x and y
    def smooth(self, newX, newY, bufSize=SMOOTHING_BUFFER_SIZE, smoothAmount=SMOOTHING_MODIFIER):
        self.__smoothingBuffer.insert(0, (newX, newY))   # insert the newest

        if len(self.__smoothingBuffer) > bufSize:
            del self.__smoothingBuffer[ len(self.__smoothingBuffer)-1 ]  # delete the oldest

        # do the non-linear averaging
        totX, totY, totMod = 0.0, 0.0, 0.0
        mod = 1.0 
        for i in range(0, len(self.__smoothingBuffer)):
            totX += self.__smoothingBuffer[i][0]*mod
            totY += self.__smoothingBuffer[i][1]*mod
            totMod += mod
            mod = mod*smoothAmount

        smoothX = int(round(totX / totMod))
        smoothY = int(round(totY / totMod))

        # record the new value
        self.__smoothingBuffer[0] = (smoothX, smoothY)
                
        return smoothX, smoothY
        
