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



import device
from globals import *
from math import cos, sin, radians
import time


# gestures... and messages
GESTURE_NULL         = 0   # nothing... default... unrecognizable gesture
GESTURE_SINGLE_TOUCH = 1   # 1 finger at a time
GESTURE_DOUBLE_CLICK = 2   # 1 finger, twice in a row
GESTURE_BIG_TOUCH    = 3   # 1 big blob
GESTURE_ZOOM         = 4   # 2 finger pinch gesture
GESTURE_MULTI_TOUCH_HOLD  = 5  # 4 or more fingers at a time in one place
GESTURE_MULTI_TOUCH_SWIPE = 6  # 4 or more fingers at a time moving


ZOOM_FACTOR = 700
SPEEDUP = 3   # multiplier for touch position to move apps faster
#ACCIDENTAL_TOUCH_THRESHOLD = 0 #0.25  # seconds before we actually consider touches


# just for printfs
DEBUG = True


# touch config file specifying where the touch screen is relative to the whole SAGE wall
TOUCH_CONFIG = getPath("touch.conf")


def makeNew(deviceId):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return PQLabs(deviceId)



class PQLabs(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "pqlabs", deviceId, needPointer=False)        

        # current state of the device
        self.x = 0   # position in SAGE coords
        self.y = 0   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened

        # because when we are dragging apps, we modify x,y on the fly
        self.realX = self.realY = 0

        self._hasDoubleClick = True

        # for multi touch tracking
        self.__startMTx, self.__startMTy = 0, 0
        self.__processMultitouch = True

        self.__alive = True

        self.deviceName = self.deviceId.split(":")[1]
        self.touchId = int(self.deviceName.lstrip("pqlabs"))
        print "\n==========   NEW TOUCH <<< %d >>>  ==========" % self.touchId

        self.dispW = self.displayBounds.getWidth()
        self.dispH = self.displayBounds.getHeight()

        # where the touch screen is relative to the SAGE display
        self.readTouchBounds()
        
    
    def readTouchBounds(self):
        # defaults in normalized
        self.touchL = 0.0
        self.touchR = 1.0
        self.touchT = 1.0
        self.touchB = 0.0

        # read custom normalized
        f = open(TOUCH_CONFIG, "r")
        for line in f:
            tokens = line.strip().split()
	    if tokens:
		    if tokens[0].startswith("left"):
			self.touchL = float(tokens[1])
		    elif tokens[0].startswith("right"):
			self.touchR = float(tokens[1])
		    elif tokens[0].startswith("top"):
			self.touchT = float(tokens[1])
		    elif tokens[0].startswith("bottom"):
			self.touchB = float(tokens[1])
        f.close()

        # determine bounds in pixels 
        self.touchL = int(self.touchL*self.dispW)
        self.touchR = int(self.touchR*self.dispW)
        self.touchT = int(self.touchT*self.dispH)
        self.touchB = int(self.touchB*self.dispH)
        
        self.touchW = self.touchR - self.touchL   # width
        self.touchH = self.touchT - self.touchB   # height
        

    def isAlive(self):
        if not self.__alive:
            print "\n*******  DESTROYING << %d >>   *******" % self.touchId
        return self.__alive


    def _resetMultiTouch(self):
        # we used the gesture so kill this touchId
        self.__processMultitouch = False  # to disable any more multitouch gestures
        

    def onMessage(self, data, firstMsg=False):
        tokens = data.split()
        gestureType = int(tokens[0])
        doMove = True  # for disabling move by accidental touches


        # --------   parse the message  ------------

        # common parameters
        x = float(tokens[1])
        y = float(tokens[2])
        self.__lifePoint = int( tokens[len(tokens)-1] )

        # parse the specific parameters
        if gestureType == GESTURE_MULTI_TOUCH_HOLD:
            numTouches = int(tokens[3])

        elif gestureType == GESTURE_MULTI_TOUCH_SWIPE:
            dX = float(tokens[3])
            dY = float(tokens[4])
            numTouches = int(tokens[5])

        elif gestureType == GESTURE_ZOOM:
            zoomAmount = float(tokens[3])


        # always convert to SAGE coords first
        x = int(round( float(x)*self.touchW + self.touchL ))
        y = int(round( float(y)*self.touchH + self.touchB ))
        
        
        # some initial stuff...
        if firstMsg:
            self.makePointer(x,y)    # make the pointer at the right location AFTER we calculated the location

            

        #------   ONE FINGER GESTURE_SINGLE_TOUCH   -------#

        if gestureType == GESTURE_SINGLE_TOUCH:
            
                # BEGINNING of the gesture
            if self.__lifePoint == GESTURE_LIFE_BEGIN:
                self.clickX, self.clickY = x, y
                self.realX, self.realY = x, y
                if DEBUG: print "\nCLICKING DOWN!!!!!!!!!  << %d >>    (%d, %d)" % (self.touchId, x, y)
                self.postEvtClick(x, y, 1, True, EVT_DRAG)

                # END of the gesture
            elif self.__lifePoint == GESTURE_LIFE_END:
                if hasattr(self.toEvtHandler, "overlayType") and self.toEvtHandler.overlayType == OVERLAY_APP:
                    
                    dx = int((x - self.realX) * SPEEDUP * getGlobalScale())
                    dy = int((y - self.realY) * SPEEDUP * getGlobalScale())
                    x = self.x + dx 
                    y = self.y + dy 

                    # keep it within limits
                    if x < 1: x = 1
                    elif x > self.dispW-1: x = self.dispW-1
                    if y < 1: y = 1
                    elif y > self.dispH-1: y = self.dispH-1

                if DEBUG: print "CLICKING UP!!!  << %d >>    (%d, %d)" % (self.touchId, x, y)
                if DEBUG: print 40*"-", "\n\n"
                
                self.postEvtClick(x, y, 1, False, EVT_DRAG)
                self.realX, self.realY = 0, 0


                # SIMPLY MOVING... middle of the gesture
            else:
                #x,y = self.smooth(x,y, 5, 0.4)
                # move
                dx = dy = 0

                # did we even move?
                if self.x != x or self.y != y:

                    # speed up the pointer movement if we are over the app
                    if self.toEvtHandler and hasattr(self.toEvtHandler, "overlayType") and self.toEvtHandler.overlayType == OVERLAY_APP:

                        if time.time() - self.toEvtHandler._clickDownTime > ACCIDENTAL_TOUCH_THRESHOLD:
                            dx = int((x - self.realX) * SPEEDUP * getGlobalScale())
                            dy = int((y - self.realY) * SPEEDUP * getGlobalScale())

                            self.realX = x
                            self.realY = y

                            # keep it within limits
                            if self.x+dx < 1: dx = -self.x + 1
                            elif self.x+dx > self.dispW-1: dx = self.dispW - self.x - 1
                            if self.y+dy < 1: dy = -self.y + 1
                            elif self.y+dy > self.dispH-1: dy = self.dispH - self.y - 1

                            x = self.x + dx 
                            y = self.y + dy 

                        else:  # to prevent accidental touches on apps
                            if self.pointer: self.pointer.hide()
                            self.realX = x
                            self.realY = y
                            self.x = x
                            self.y = y
                            doMove = False

                    else:
                        dx = x - self.x
                        dy = y - self.y

                    if doMove or self.dragMode:
                        self.postEvtAnalog1(self.clickX, self.clickY, x, y, dx, dy, 0)


        elif gestureType == GESTURE_DOUBLE_CLICK:
            self.postEvtDoubleClick(x, y, BTN_LEFT, EVT_DRAG)

        
        elif gestureType == GESTURE_MULTI_TOUCH_HOLD:
            if DEBUG: print "MULTI_TOUCH_HOLD << %d >>" % self.touchId
            if self.__processMultitouch:
                self.postEvtMultiTouchHold(x, y, numTouches, self.__lifePoint)


        elif gestureType == GESTURE_MULTI_TOUCH_SWIPE:
            if DEBUG: print "MULTI_TOUCH_SWIPE << %d >>" % self.touchId
            if self.__processMultitouch:
                self.clickX, self.clickY = x, y

                # mark the beginning
                if self.__lifePoint == GESTURE_LIFE_BEGIN:
                    self.__startMTx, self.__startMTy = x, y
                    
                self.postEvtMultiTouchSwipe(x, y, numTouches, 
                                            x - self.__startMTx, 
                                            y - self.__startMTy,
                                            self.__startMTx, 
                                            self.__startMTy, self.__lifePoint)


        #------   ONE BIG BLOB   -------#

        elif gestureType == GESTURE_BIG_TOUCH:
            if DEBUG: print "BIG TOUCH << %d >>" % self.touchId
            self.clickX, self.clickY = x, y
            self.postEvtBigClick(x, y, self.__lifePoint)


        #------   PINCH ZOOM and TWO FINGER PAN   -------#

        elif gestureType == GESTURE_ZOOM:
            # GESTURE_ZOOM is executed through the traditional mouse mechanism of CLICK_DOWN, DRAG, CLICK_UP

            if self.__lifePoint == GESTURE_LIFE_BEGIN:
                self.clickX, self.clickY = x, y
                if DEBUG: print "BEGIN ZOOM << %d >>" % self.touchId
                self.postEvtClick(self.clickX, self.clickY, 4, True, EVT_ZOOM)

            elif self.__lifePoint == GESTURE_LIFE_END:
                if DEBUG: print "END ZOOM << %d >>" % self.touchId
                self.postEvtClick(x, y, 4, False, EVT_ZOOM)

            else:
                if DEBUG: print "ZOOM GESTURE  << %d >>    (%d, %d)" % (self.touchId, x, y)
                howMuch = int(zoomAmount*ZOOM_FACTOR*getGlobalScale())
                self.postEvtAnalog3(self.clickX, self.clickY, x, y, howMuch, 0, 0)

            
        else:
            print "Other gesture.... type:", gestureType


        # move the pointer
        if doMove and (self.x != x or self.y != y):
            self.x = x
            self.y = y
            if self.pointer: 
                #if DEBUG: print x, y
                self.pointer.show()  # to negate the accidental touch pointer hide
                self.pointer.movePointer(x, y)        


        # kill the gesture if we're done
        if self.__lifePoint == GESTURE_LIFE_END:
            self.__alive = False
