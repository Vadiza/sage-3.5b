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



from globals import *
import time

class SageDrawObject:
    """ describes a base class for a sage draw object """
    def __init__(self, objectId, objectType, sageGate):
        self.objectId = objectId
        self.objectType = objectType
        self.visible = False
        self.sageGate = sageGate
        
    def getId(self):
        return self.objectId

    def getType(self):
        return self.objectType
    
    def isVisible(self):
        return self.visible

    def show(self):
        if not self.visible:
            self.visible = True
            self.sageGate.showObject(self.objectId)

    def hide(self):
        if self.visible:
            self.visible = False
            self.sageGate.hideObject(self.objectId)
        
    



class PointerObject(SageDrawObject):
    """ describes a sage draw object for pointers """
    def __init__(self, objectId, sageGate):
        SageDrawObject.__init__(self, objectId, POINTER_TYPE, sageGate)
        self.clickPos = [-1,-1]  # last click position [x,y]
        self.lastRegion = [-1, None]     # [regionId, app]
        self.region = [-1, None]     # [regionId, app]
        self.state = PTR_NORMAL_STATE  # controls all the drawing of the pointer
        self.ptrState = PTR_NORMAL_STATE  # only used to control the drawing of the cursor itself
        self.leftDown = False
        self.leftDownTime = 0.0


    def updateRegion(self, regionId, app):
        self.lastRegion = self.region
        self.region = [regionId, app]
        

    def showResizePointer(self):
        self.ptrState = PTR_RESIZE_STATE
        self.sageGate.showResizeMouse(self.objectId, self.region[0])


    def reset(self):
        self.ptrState = PTR_NORMAL_STATE
        self.state = PTR_NORMAL_STATE
        self.sageGate.resetMouseState(self.objectId)


    def onMouseDown(self, app, regionId, x, y):
        self.leftDown = True
        self.leftDownTime = time.time()
        self.clickPos = [x,y]
        self.lastRegion = [regionId, app]
        self.sageGate.sendMouseDown(self.objectId)
        

    def onMouseUp(self):
        self.leftDown = False
        self.sageGate.sendMouseUp(self.objectId)
