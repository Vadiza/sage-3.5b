############################################################################
#
# SAGE UI - A Graphical User Interface for SAGE
# Copyright (C) 2005 Electronic Visualization Laboratory,
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



# placement
MIDDLE = -1
LEFT = 0
RIGHT = 1
BOTTOM = 2
TOP = 3


class SageDisplay:
    def __init__(self, tn,x,y,w,h,tw,th,dId):
        self.tileNumber = tn
        self.dimX = x
        self.dimY = y
        self.cols = x   # just an alias
        self.rows = y
        self.desktopWidth = w
        self.desktopHeight = h
        self.tileWidth = tw
        self.tileHeight = th
        self.sageW = w
        self.sageH = h
        self.displayId = dId
        self.placement = MIDDLE  # where this display is with respect to display 0 (-1 means this is display 0 = middle)
        #self.orientation = 0  # where down is in degrees. 0 is down, 90 is right, 180 is up, 270 is left
        
    def getValues(self):
        return [self.tileNumber, self.dimX, self.dimY,
                self.desktopWidth, self.desktopHeight,
                self.tileWidth, self.tileHeight, self.displayId]

    def getId(self):
        return self.displayId
    


class SageDisplayInfo:

    def __init__(self):
        self.displays = {}


    def getAllDisplays(self):
        """ returns a list of SageDisplay objects """
        return self.displays.values()


    def getNumDisplays(self):
        return len(self.displays)
    

    def addDisplay(self, tileNum, dimX, dimY, deskWidth, deskHeight, tileWidth, tileHeight, displayId=0):
        d = SageDisplay(tileNum, dimX, dimY, deskWidth, deskHeight, tileWidth, tileHeight, displayId)
        self.displays[displayId] = d


    def getTotalWidth(self):
        totalW = 0
        for disp in self.displays.values():
            pos = disp.placement
            if pos == LEFT or pos == MIDDLE or pos == RIGHT:
                totalW += disp.sageW
        return totalW


    def getTotalHeight(self):
        totalH = 0
        for disp in self.displays.values():
            pos = disp.placement
            if pos == TOP or pos == MIDDLE or pos == BOTTOM:
                totalH += disp.sageH
        return totalH
    

    def getDisplay(self, displayId):
        return self.displays[displayId]

            
    def getDisplayInfo(self, displayId=0):
        return self.displays[displayId].getValues()
                
