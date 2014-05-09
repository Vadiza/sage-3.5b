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
from globals import *


def makeNew(app=None, direction=VERTICAL):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Splitter(app, direction)


TYPE = WIDGET_PLUGIN


# messages
SET_STATE = 0

# button graphical states (different from __state variable)
MOUSE_OVER = 0
MOUSE_NOT_OVER = 1



class Splitter(Overlay, EventHandler):
    
    def __init__(self, app=None, direction=VERTICAL):
        Overlay.__init__(self, OVERLAY_SPLITTER)
        EventHandler.__init__(self, app)

        self.direction = direction
        self.__enlarged = False
        self.__scaleMultiplier = 4.0
        self._canScale = False
        self.mouseOverScale = 1.0
        self.setTransparency(255)
        self.z = BOTTOM_Z - 10
        self.relPos = 0.5  # relative to the section it splits
        self._preCollapsedPos = self.relPos
        self.__buffer = int(20 * getGlobalScale())
        self._collapsed = False

        self.registerForEvent(EVT_DRAG, self.__onDrag)
        self.registerForEvent(EVT_CLICK, self.__onClick)
        self.registerForEvent(EVT_MOVE, self.__onMove)
        self.registerForEvent(EVT_LEFT_WINDOW, self._onLeftWindow)
        self.registerForEvent(EVT_ENTERED_WINDOW, self._onEnteredWindow)

        if direction == VERTICAL:
            self.__image = opj("images", "vertical_divider.png")
        else:
            self.__image = opj("images", "horizontal_divider.png")


     # for proper pickling...
    def __getstate__(self):
        d = {}
        d['direction'] = self.direction

        # the small shifts in pos and size are to offset the adjustments in setPos and setSize
        if self.direction == HORIZONTAL:
            d['size'] = (self.getWidth(), self.getHeight())
            d['pos'] = (self.getX(), self.getY())
        else:
            d['size'] = (self.getWidth(), self.getHeight())
            d['pos'] = (self.getX(), self.getY())
        d['relPos'] = self.relPos
        d['_preCollapsedPos'] = self._preCollapsedPos
        return d
    
    

    # -------    WIDGET PARAMETERS   -----------------    

    
    def _refresh(self):
        EventHandler._refresh(self)
        self.__calculateRelPos()


    def setPos(self, x, y):
        EventHandler.setPos(self, x, y)
        
        
    def setRelPos(self, rp):
        self.relPos = rp
        sec = self.getUserData()

        # now calculate the absolute x,y in SAGE coords based on the section-relative position
        if self.direction == HORIZONTAL:
            x = sec.getX()
            y = int(sec.bounds.bottom + sec.bounds.getHeight()*rp)
        else:
            y = sec.getY()
            x = int(sec.bounds.left + sec.bounds.getWidth()*rp) 

        newSpace = abs(x-self.getX()), abs(y-self.getY())
        EventHandler.setPos(self, x, y)
        
        return newSpace


    def setImage(self, filename):
        """ relative to $SAGE_DIRECTORY/ """
        self.__image = filename


    def isIn(self, x, y):
        """ overridden to support resizing on mouse_over"""
        bW = self.getBounds().getWidth()
        bH = self.getBounds().getHeight()
        bX = self.getBounds().getX()
        bY = self.getBounds().getY()

        if self.direction == VERTICAL:
            newW = int(bW * self.__scaleMultiplier*2.0) #self.mouseOverScale)
            newH = bH
        else:
            newH = int(bH * self.__scaleMultiplier*2.0) #self.mouseOverScale)
            newW = bW
        newX = bX - int((newW-bW)/2.0)
        newY = bY - int((newH-bH)/2.0)

        if newX <= x and (newX+newW) >= x and newY <= y and (newY+newH) >= y:
            return True
        else:
            return False


    def enlarge(self, doEnlarge):

        if doEnlarge and not self.__enlarged:
            self._tempZOffset = -0.005
            self.z += -0.005
            self.mouseOverScale = self.__scaleMultiplier
            self.sendOverlayMessage(SET_STATE, MOUSE_OVER, self.__buffer)
            self.__enlarged = doEnlarge
            
        elif not doEnlarge and self.__enlarged:
            self._tempZOffset = 0.0
            self.z -= -0.005
            self.mouseOverScale = 1.0
            self.sendOverlayMessage(SET_STATE, MOUSE_NOT_OVER, self.__buffer)
            self.__enlarged = doEnlarge


    def __onMove(self, event):
        """ need this to receive LEFT_WINDOW and ENTERED_WINDOW events """
        pass


    def _onLeftWindow(self, event):
        self.enlarge(False)


    def _onEnteredWindow(self, event):
        self.enlarge(True)


    def __onClick(self, event):
        if event.isDown:
            self._captured = True
            event.device.toEvtHandler = self
            self.sendOverlayMessage(SET_STATE, MOUSE_OVER, self.__buffer)
        else:
            self._captured = False
            self.sendOverlayMessage(SET_STATE, MOUSE_OVER, self.__buffer)
            self._refresh()

            # notify other splitters that we moved something
            self.getUserData()._onSplitterMoved()

        # since window entered and window left events dont really work with touch
        pqlabs = (event.device.deviceType == "pqlabs")
        if pqlabs and not event.isDown:
            self.enlarge(False)


    def __calculateRelPos(self):
        sec = self.getUserData()
        if self.direction == HORIZONTAL:
            self.relPos = float(self.getBounds().getY() - sec.bounds.bottom) / sec.bounds.getHeight()
        else:
            self.relPos = float(self.getBounds().getX() - sec.bounds.left) / sec.bounds.getWidth() 


    def __onDrag(self, event):
        x = event.x
        y = event.y
        sec = self.getUserData()
        
        if self.direction == VERTICAL:
            y = self.pos.y   # keep y the same
            
            # limit the movement
            dL = x - sec.bounds.left
            dR = sec.bounds.right - x
            minD = sec.bounds.getWidth() * 0.05
            if dL < minD or dR < minD:
                x = self.pos.x

        else:
            x = self.pos.x   # keep x the same

            # limit the movement
            dB = y - sec.bounds.bottom
            dT = sec.bounds.top - y
            minD = sec.bounds.getHeight() * 0.05
            if dB < minD or dT < minD:
                y = self.pos.y

        self.setPos(x, y)
        
 
          # to xml
    def _specificToXml(self, parent):
        s = self.doc.createElement("splitter")
        s.setAttribute("scaleMultiplier", str(self.__scaleMultiplier))
        s.setAttribute("direction", str(self.direction))
        s.setAttribute("buffer", str(self.__buffer))
        parent.appendChild(s)


    def _graphicsToXml(self, parent):
        self._addGraphicsElement(parent, "image", self.__image)
