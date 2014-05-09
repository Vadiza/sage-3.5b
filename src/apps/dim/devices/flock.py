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


def makeNew(deviceId):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return FlockWanda(deviceId)


class FlockWanda(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "flock", deviceId, True, displayId=0)
        self.displayFactor = 1
        
        # current state of the device
        self.buttons = [False, False, False]   # left, right, middle
        self.x = 100   # position in SAGE coords
        self.y = 100   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened

        

    def onMessage(self, data, firstMsg=False):
        tokens = data.strip().split()
        msgCode = tokens[0]


        # MOVE MESSAGE
        if msgCode == "1":
            # always convert to SAGE coords first
            x = int(round(float(tokens[1]) * self.displayBounds.getWidth()))
            y = int(round(float(tokens[2]) * self.displayBounds.getHeight())) 

            # did we even move?
            dx = dy = 0
            #if self.x != x or self.y != y:
            dx = x - self.x ;  self.x = x
            dy = y - self.y ;  self.y = y

            # fire the appropriate event
            if self.buttons[0]:     # analog1, pan
                self.postEvtAnalog1(self.clickX, self.clickY, dx, dy, 0)

            elif self.buttons[1]:   # analog2, rotate
                self.postEvtAnalog3(self.clickX, self.clickY, dx, dy, 0)

            elif self.buttons[2]:   # analog3, zoom
                self.postEvtAnalog2(self.clickX, self.clickY, dx, dy, 0)

            else:                   # move
                self.postEvtMove(self.x, self.y, dx, dy)

            # move the pointer
            if self.pointer:
                self.pointer.movePointer(self.x, self.y)



        # BUTTON MESSAGE
        elif msgCode == "2":
            btnId = int(tokens[1])
            isDown = bool(int(tokens[2]))
            
            # apply the state change if it's a valid buttonId
            if btnId <= len(self.buttons):
                forEvt = 0
                if btnId == 0:  forEvt = EVT_PAN
                elif btnId == 1:  forEvt = EVT_ROTATE
                elif btnId == 2:  forEvt = EVT_ZOOM
                self.buttons[btnId-1] = isDown
                self.postEvtClick(self.x, self.y, btnId, isDown, forEvt)
                self.clickX, self.clickY = self.x, self.y
