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
    return Joystick(deviceId)

UP_ARROW = 1
DOWN_ARROW = 2
LEFT_ARROW = 3
RIGHT_ARROW = 4

LEFT=1
RIGHT=2
MIDDLE=3

MOVE=1
CLICK=2
ARROW=3
WHEEL=4

WHEEL_CLICK_TIME = 0.75  # seconds before there is a click event after wheel event

class Joystick(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "joystick", deviceId, True, displayId=0)
        isFake = deviceId.split("_")[1].startswith('fake')
        
        if isFake:
            self.joyToMouseRatio = 2000
        else:
            self.joyToMouseRatio = 100
        self.displayFactor = 1
        
        # current state of the device
        self.buttons = [False, False, False]   # left, right, middle
        self.x = 100   # position in SAGE coords
        self.y = 100   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened

        self.lastWheelTime = time.time()
        self.wheelClickThread = Thread(target=self.postWheelUpClick)
        self.wheelClickThread.start()
        self.wheeling = False
        

    def postWheelUpClick(self):
        while doRun():
            if (time.time() - self.lastWheelTime) > WHEEL_CLICK_TIME and self.wheeling:
                forEvt = EVT_ZOOM
                self.postEvtClick(self.x, self.y, 4, False, forEvt)
                self.wheeling = False

            time.sleep(0.1)
        

    def onMessage(self, data, firstMsg=False):
        tokens = data.strip().split()
        msgCode = int(tokens[0])
           
        # move
        if msgCode == MOVE:
            
            # the axis information
            xAxis = float(tokens[1])
            yAxis = float(tokens[2])

            # determine x, y, dX, dY
            (dX, dY) = self.__toSAGEPos(xAxis, yAxis)
                    
            # fire the appropriate event
            if self.buttons[0]:     # analog1, pan
                self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dX, dY, 0)

            elif self.buttons[1]:   # analog2, rotate
                self.postEvtAnalog3(self.clickX, self.clickY, self.x, self.y, dX, dY, 0)
                
            elif self.buttons[2]:   # analog3, zoom
                self.postEvtAnalog2(self.clickX, self.clickY, self.x, self.y, dX, dY, 0)

            else:                   # move
                self.postEvtMove(self.x, self.y, dX, dY)
                

            # move the pointer
            if self.pointer: self.pointer.movePointer(self.x, self.y)
            

        # button press
        elif msgCode == CLICK:  # button press
            btnId = int(tokens[1])
            isDown = bool(int(tokens[2]))
            
            # apply the state change if it's a valid buttonId
            if btnId <= len(self.buttons):
                forEvt = 0
                if btnId == LEFT:  forEvt = EVT_PAN
                elif btnId == RIGHT:  forEvt = EVT_ROTATE
                elif btnId == MIDDLE:  forEvt = EVT_ZOOM
                self.buttons[btnId-1] = isDown
                self.postEvtClick(self.x, self.y, btnId, isDown, forEvt)

                self.clickX, self.clickY = self.x, self.y
                

        elif msgCode == ARROW:  # arrow
            pov = int(tokens[1])

            arrow = -1
            if pov == 0:
                arrow = UP_ARROW
            elif pov == 9000:
                arrow = RIGHT_ARROW
            elif pov == 18000:
                arrow = DOWN_ARROW
            elif pov == 27000:
                arrow = LEFT_ARROW
                
            if arrow > -1:
                self.postEvtArrow(arrow, self.x, self.y)
                

        elif msgCode == WHEEL:
            forEvt = EVT_ZOOM
            numSteps = int(tokens[1])
            
            if (time.time() - self.lastWheelTime) > WHEEL_CLICK_TIME:
                self.wheeling = True
                self.postEvtClick(self.x, self.y, 4, True, forEvt)
               
            self.postEvtAnalog3(self.clickX, self.clickY, self.x, self.y, numSteps*15, 0, 0)
            self.lastWheelTime = time.time()



    def __toSAGEPos(self, xAxis, yAxis):
        
        dX = int(xAxis * self.joyToMouseRatio * self.displayFactor)
        dY = int(yAxis * self.joyToMouseRatio * self.displayFactor) * (-1)  # -1 for reverse

        dX, dY = self.smooth(dX, dY)

        # make sure the pointer is within display bounds
        # check left and right (x)
        if (self.x + dX) > self.bounds.right:
            dX = self.bounds.right - self.x
        elif (self.x + dX) < self.bounds.left:
            dX = -(self.x-self.bounds.left)

        # check top and bottom (y)
        if (self.y + dY) > self.bounds.top:
            dY = self.bounds.top - self.y
        elif (self.y + dY) < self.bounds.bottom:
            dY = -(self.y-self.bounds.bottom)

        # update the position
        self.x += dX
        self.y += dY

        return dX, dY
        
        
