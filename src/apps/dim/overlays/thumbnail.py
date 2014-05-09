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




from overlay import Overlay
from eventHandler import EventHandler
from button import Button
from globals import *


def makeNew(app=None):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Thumbnail(app)


TYPE = WIDGET_PLUGIN


# messages
SET_STATE = 0
DRAG = 1

# button graphical states (different from __state variable)
DOWN_STATE = 0
UP_STATE   = 1
MOUSE_OVER = 2
MOUSE_NOT_OVER = 3


class Thumbnail(Button):
    
    def __init__(self, app=None):
        Button.__init__(self, app)
        self.overlayType = OVERLAY_THUMBNAIL
        self.mouseOverScale = 1.0
        self.__scaleMultiplier = 1.4
        
        self.registerForEvent(EVT_DRAG, self._onDrag)
        self.__dragging = False
        self.__enlarged = False
        self._allowSelection = True
        self.__drawOutline = False
        

    def isIn(self, x, y):
        """ overridden to support resizing on mouse_over"""
        bW = self.getBounds().getWidth()
        bH = self.getBounds().getHeight()
        bX = self.getBounds().getX()
        bY = self.getBounds().getY()
        
        newW = int(bW * self.mouseOverScale)
        newH = int(bH * self.mouseOverScale)
        newX = bX - int((newW-bW)/2.0)
        newY = bY - int((newH-bH)/2.0)

        if newX <= x and (newX+newW) >= x and newY <= y and (newY+newH) >= y:
            return True
        else:
            return False


    def drawOutline(self, doDraw):
        self.__drawOutline = doDraw


    def setScaleMultiplier(self, newScale):
        self.__scaleMultiplier = newScale


    # to xml
    def _specificToXml(self, parent):
        Button._specificToXml(self, parent)
        
        s = self.doc.createElement("thumbnail")
        s.setAttribute("scaleMultiplier", str(self.__scaleMultiplier))
        s.setAttribute("drawOutline", str(int(self.__drawOutline)))
        parent.appendChild(s)


    # from xml
    def _onSpecificInfo(self):
        """ specific info was parsed... use it here """

        Button._onSpecificInfo(self)
        
        if "thumbnail" in self.xmlElements:
            self.__scaleMultiplier = float(self.xmlElements["thumbnail"][0].attrs["scaleMultiplier"])


    def enlarge(self, doEnlarge):

        if doEnlarge and not self.__enlarged:
            self._tempZOffset = -0.005
            self.z += -0.005
            self.mouseOverScale = self.__scaleMultiplier
            self.sendOverlayMessage(SET_STATE, MOUSE_OVER)
            self.__enlarged = doEnlarge
            
        elif not doEnlarge and self.__enlarged and not self.isSelected():
            self._tempZOffset = 0.0
            self.z -= -0.005
            self._captured = False
            self.mouseOverScale = 1.0
            self.sendOverlayMessage(SET_STATE, MOUSE_NOT_OVER, 0)
            self.__enlarged = doEnlarge


    def select(self, doSelect):
        EventHandler.select(self, doSelect)
        self.enlarge(doSelect)

    
    # -------    GENERIC  EVENTS   -----------------    

    def _onLeftWindow(self, event):
        self.enlarge(False)


    def _onEnteredWindow(self, event):
        self.enlarge(True)


    def _onClick(self, event):
        if event.forEvt == EVT_ZOOM: return
        
        if not event.isDown and self._doesAllowDrag() and self.__dragging:
            self.__dragging = False
            self.sendOverlayMessage(SET_STATE, MOUSE_NOT_OVER, 0)
        elif event.isDown and not self._isToggle:
            event.device.toEvtHandler = self
            Button._onClick(self, event)
            self.sendOverlayMessage(SET_STATE, MOUSE_OVER)
        else:
            if not event.isDown:
                self.sendOverlayMessage(SET_STATE, MOUSE_NOT_OVER, 1)  # 1 for animating scale
            else:
                self.sendOverlayMessage(SET_STATE, MOUSE_OVER)
            Button._onClick(self, event)
        
        # since window entered and window left events dont really work with touch
        #pqlabs = (event.device.deviceType == "pqlabs")
        #if pqlabs and not event.isDown:
        #    self.enlarge(False)


    def _onDrag(self, event):
        # if the drag event originated from elsewhere, don't use it
        #if event.device.toEvtHandler != self or not self._doesAllowDrag():
        #    return
        if not self._doesAllowDrag():
            return 
                
        startX, startY = event.startX, event.startY
        x, y = event.x, event.y
        dx, dy, dz = event.dX, event.dY, event.dZ
        pointer = event.device.pointer

        distX = abs(startX - x)*2
        distY = abs(startY - y)*2

        if distX >= self.getBounds().getWidth() or distY >= self.getBounds().getHeight():
            self.__dragging = True
            self.sendOverlayMessage(DRAG, x, y)
                
            

        
