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




from globals import *
from eventHandler import EventHandler


class Overlay:
    """ this describes the basic overlay - ie an object
        that's drawn on the screen
    """

    def __init__(self, overlayType):
        self.overlayType = overlayType
        self.overlayId = -1
        self.sageGate = getSageGate()
        self.visible = True
  
        
    def initialize(self, overlayId):
        """ override this to start using the overlay (i.e. don't
            do anything with it until you call this because you
            don't have the overlayId yet.
            remember to set the overlayId in your overridden method
        """
        self.overlayId = overlayId


    def getId(self):
        return self.overlayId
    

    def getType(self):
        return self.overlayType


    def isVisible(self):
        return self.visible


    def toggleVisible(self):
        if self.visible:
            self.hide()
        else:
            self.show()
            

    def hide(self):
        if self.visible:
            self.visible = False
            if self.overlayId > -1:
                self.sageGate.showOverlay(self.overlayId, False)
                if isinstance(self, EventHandler):
                    #self._hideChildren()
                    self._refresh()


    def show(self):
        if not self.visible:
            self.visible = True
            if self.overlayId > -1:
                self.sageGate.showOverlay(self.overlayId, True)
                if isinstance(self, EventHandler):
                    #self._showChildren()
                    self._refresh()


    def sendOverlayMessage(self, *params):
        if self.overlayId > -1:
            self.sageGate.sendOverlayMessage(self.overlayId, *params)


    def update(self, now):
        """ do something to update your drawing or whatever
            the *now* parameter is the current time in seconds... float
        """
        pass
    
