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
    return Puck(deviceId)


# what each puck does
PAN_PUCK = [1, 3, 10, 11]  # 10&11 are fake mice
ROTATE_PUCK = [2]
ZOOM_PUCK = [4]
SPECIAL_PUCK = [5]


# puck types
DOWN = 2
UP = 1


class Puck(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "puck", deviceId, needPointer=True)
        
        # current state of the device
        self.puckType = 1
        self.x = 0   # position in SAGE coords
        self.y = 0   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened
        self.angle = 0   # current angle
        self.prevDx = 0  # used for zoom
        self.prevDy = 0
        puckName = deviceId.split(":")[1]
        self.puckId = int( puckName.split("_")[1])   # the actual puckId from the tracker
        
        if self.puckId in SPECIAL_PUCK:
            self.specialDevice = True     # e.g. a magic carpet lens... generates different events


    def onPointerAdded(self, pointerOverlay):
        """ overriden so that we can set the correct pointer shape
            for the event that this puck produces """
        self.pointer = pointerOverlay

        if self.puckId in PAN_PUCK:
            self.pointer.setAnalogEvent(EVT_PAN)
        elif self.puckId in ZOOM_PUCK:
            self.pointer.setAnalogEvent(EVT_ZOOM)
        elif self.puckId in ROTATE_PUCK:
            self.pointer.setAnalogEvent(EVT_ROTATE)
            

    def onMessage(self, data, firstMsg=False):
        tokens = data.split()
        puckId =   int(tokens[1].strip())
        x =      float(tokens[2].strip())
        y =      float(tokens[3].strip())
        angle =  float(tokens[4].strip())+90
        puckType = int(tokens[5].strip())

        # always convert to SAGE coords first
        x = self.bounds.getX() + int(round(float(x) * self.bounds.getWidth()))
        y = self.bounds.getY() + int(round(float(y) * self.bounds.getHeight()))

        x,y = self.smooth(x,y)
        
        # set the correct state for the first msg
        ## if firstMsg:
##             self.x = x
##             self.y = y
##             self.angle = angle
##             self.puckType = puckType
##             if self.pointer:
##                 self.pointer.movePointer(self.x, self.y)
##                 self.pointer.pointerAngle(int(self.angle))
##             return


        # click
        if (self.puckType != puckType) and (self.puckId not in SPECIAL_PUCK):
            self.clickX, self.clickY = x, y
            self.prevDy = self.prevDx = 0
            self.puckType = puckType
            forEvt = 0
            if self.puckId in PAN_PUCK or self.dragMode:  
                forEvt = EVT_PAN
            elif self.puckId in ROTATE_PUCK:
                forEvt = EVT_ROTATE
            elif self.puckId in ZOOM_PUCK:
                forEvt = EVT_ZOOM

            if (puckId == 11):  # hack for rightClick
                self.postEvtClick(x, y, 2, puckType==DOWN, forEvt)  # if puckType=2, isDown=True
            else:
                self.postEvtClick(x, y, 1, puckType==DOWN, forEvt)  # if puckType=2, isDown=True
        
        
        # move
        dx = dy = 0
        if self.x != x or self.y != y:
            dx = x - self.x ;  self.x = x
            dy = y - self.y ;  self.y = y

            if puckType == UP:    # no buttons pressed
                self.postEvtMove(self.x, self.y, dx, dy)

            elif puckType == DOWN:  # button pressed means different things depending on the puck
                if self.puckId in PAN_PUCK or self.dragMode:  
                    self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
                elif self.puckId in ROTATE_PUCK:
                    self.postEvtAnalog2(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
                elif self.puckId in ZOOM_PUCK:
                    dx = self.x - self.clickX  # accumulated dx,dy from the click pos
                    dy = self.y - self.clickY
                    dx, dy = self.__rotatePoints(dx, dy)
                    self.postEvtAnalog3(self.clickX, self.clickY, self.x, self.y, dx-self.prevDx, dy-self.prevDy, 0)
                    self.prevDx = dx
                    self.prevDy = dy
                    
                    
            # move the pointer
            if self.pointer: self.pointer.movePointer(self.x, self.y)        


        # rotation
        if self.angle != angle:
            da = angle - self.angle
            self.angle = angle
            if da < -300:    # we went over the zero degree line counterclockwise, a hack
                da = 360 + da
            elif da > 300:   # we went over the zero degree line clockwise, a hack
                da = (360 - da)*(-1)

            if self.pointer: self.pointer.pointerAngle(int(self.angle))

            # convert to normalized angle
            da = (da/360.0)


    def __rotatePoints(self, x, y):
        """ rotate a point around origin... used for making
            rotation events orientation independent """
        a = -radians(self.angle)
        newX = x*cos(a) - y*sin(a)
        newY = x*sin(a) + y*cos(a)
        return newX, newY
    
