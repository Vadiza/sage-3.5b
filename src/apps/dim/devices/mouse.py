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
import time
from threading import Thread


def makeNew(deviceId):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Mouse(deviceId)


MOVE = 1
CLICK = 2
WHEEL = 3
COLOR = 4
DOUBLE_CLICK = 5

# just used by the desktop mouse capture app
HAS_DOUBLE_CLICK = 10


WHEEL_CLICK_TIME = 0.4  # seconds before there is a click event after wheel event


class Mouse(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "mouse", deviceId, needPointer=True)
        
        # current state of the device
        self.x = 0   # position in SAGE coords
        self.y = 0   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened

        self.buttons = {}  #key=BTN_ID, value=True|False
        self.buttons[0] = False
        self.buttons[BTN_LEFT] = False
        self.buttons[BTN_RIGHT] = False
        self.buttons[BTN_MIDDLE] = False
        self.lastBtn = 0

        self.lastWheelTime = time.time()
        self.wheelClickThread = Thread(target=self.postWheelUpClick)
        self.wheelClickThread.start()
        self.wheeling = False
        self.sec = None
                

    def postWheelUpClick(self):
        while doRun():
            if (time.time() - self.lastWheelTime) > WHEEL_CLICK_TIME and self.wheeling:
                self.postEvtClick(self.x, self.y, 4, False, forEvt=EVT_ZOOM)
                self.wheeling = False

            time.sleep(0.1)

    def anyOtherButtonsPressed(self, currentBtnId):
        for btnId, isDown in self.buttons.iteritems():
            if btnId == currentBtnId:
                continue
            elif isDown:
                return btnId
        return False
    

    def onMessage(self, data, firstMsg=False):
        tokens = data.strip().split()

        # keyboard events
        if tokens[0] == 'k':
            keyCode = int(tokens[1])
            self.postEvtKey(keyCode, self.x, self.y)
            return

        msgCode = int(tokens[0])
        # click
        if msgCode == CLICK:
            btnId = int(tokens[1])
            isDown = bool(int(tokens[2]))
            forEvt = 0
            
            if self.buttons[btnId] != isDown and not self.anyOtherButtonsPressed(btnId):
                self.buttons[btnId] = isDown
                self.lastBtn = btnId

                if isDown:
                    self.clickX, self.clickY = self.x, self.y
                
                if btnId == BTN_LEFT:
                    forEvt = EVT_PAN
                elif btnId == BTN_RIGHT:
                    forEvt = EVT_ROTATE
                elif btnId == BTN_MIDDLE:
                    forEvt = EVT_ZOOM

                self.postEvtClick(self.x, self.y, btnId, isDown, forEvt)


        elif msgCode == DOUBLE_CLICK:
            btnId = int(tokens[1])
            forEvt = 0
            if btnId == BTN_LEFT:
                forEvt = EVT_PAN
            elif btnId == BTN_RIGHT:
                forEvt = EVT_ROTATE
            elif btnId == BTN_MIDDLE:
                forEvt = EVT_ZOOM
            self.postEvtDoubleClick(self.x, self.y, btnId, forEvt)

                    
        elif msgCode == WHEEL:
            forEvt = EVT_ZOOM
            numSteps = int(tokens[1])

            if (time.time() - self.lastWheelTime) > WHEEL_CLICK_TIME:
                self.wheeling = True
                self.postEvtClick(self.x, self.y, 4, True, forEvt)

            howMuch = int(numSteps*15*getGlobalScale())
            self.postEvtAnalog3(self.clickX, self.clickY, self.x, self.y, howMuch, 0, 0)
            self.lastWheelTime = time.time()

            
        elif msgCode == MOVE:
            x = float(tokens[1].strip())
            y = float(tokens[2].strip())
            
            # always convert to SAGE coords first
            x = self.bounds.getX() + int(round(float(x) * self.bounds.getWidth()))
            y = self.bounds.getY() + int(round(float(y) * self.bounds.getHeight()))
            
            x,y = self.smooth(x,y)

            
            dx = dy = 0
            if self.x != x or self.y != y:
                dx = x - self.x ;  self.x = x
                dy = y - self.y ;  self.y = y

                if self.buttons[self.lastBtn]:
                    if self.lastBtn == BTN_LEFT:
                        self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
                    elif self.lastBtn == BTN_RIGHT:
                        self.postEvtAnalog2(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
                    elif self.lastBtn == BTN_MIDDLE:
                        self.postEvtAnalog3(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
                else:
                    self.postEvtMove(self.x, self.y, dx, dy)
                    
                # move the pointer
                if self.pointer:
                    self.pointer.movePointer(self.x, self.y)

                    # sharing pointer
                    sec = getLayoutSections().getSectionByXY(self.x, self.y)
                    if sec:
                        deviceName = self.deviceId
                        if sec.isShared:
                            self.sec = sec
                            sec.forwardPointer(deviceName, x,y)
                        else:
                            if self.sec:
                                self.sec.hidePointer(deviceName)
                                self.sec = None



        elif msgCode == COLOR:
            if self.pointer: self.pointer.setColor(tokens[1], tokens[2], tokens[3])

        elif msgCode == HAS_DOUBLE_CLICK:
            self._hasDoubleClick = True
