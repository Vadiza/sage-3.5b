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


def makeNew(deviceId):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Touch(deviceId)



class Touch(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "tactile", deviceId, needPointer=True)
        
        # current state of the device
        self.x = 0   # position in SAGE coords
        self.y = 0   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened
        self.prevDx = 0  # used for zoom
        self.prevDy = 0


    def onMessage(self, data, firstMsg=False):
        flag, strokeId, x, y = data.split()
        strokeId, x, y = int(strokeId), float(x), float(y)
        
        # always convert to SAGE coords first
        x = int(round(float(x) * self.displayBounds.getWidth()))
        y = int(round(float(y) * self.displayBounds.getHeight()))

        if flag[0] == "d":
            forEvt = EVT_PAN
        elif flag[0] == "z":
            forEvt = EVT_ZOOM
 
        # click
        if firstMsg:
            self.clickX, self.clickY = x, y
            self.x, self.y = x, y
            self.prevDy = self.prevDx = 0
            self.postEvtClick(x, y, 1, True, forEvt)
            return

        elif flag.endswith("last"):
            self.postEvtClick(x, y, 1, False, forEvt)
            self.destroyMe = True
            return
        
        # move
        dx = dy = 0
        if self.x != x or self.y != y:
            dx = x - self.x ;  self.x = x
            dy = y - self.y ;  self.y = y

            if flag[0] == "d" or self.dragMode:
                self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
            elif flag[0] == "z":
                dx = self.x - self.clickX  # accumulated dx,dy from the click pos
                dy = self.y - self.clickY
                dx, dy = self.__rotatePoints(dx, dy)
                self.postEvtAnalog3(self.clickX, self.clickY, self.x, self.y, dx-self.prevDx, dy-self.prevDy, 0)
                self.prevDx = dx
                self.prevDy = dy
                    
            # move the pointer
            if self.pointer: self.pointer.movePointer(self.x, self.y)        
