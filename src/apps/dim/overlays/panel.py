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
from math import floor


def makeNew(app=None):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Panel(app)


TYPE = WIDGET_PLUGIN


# some messages...
MOVE = 0           # by dx, dy
SET_POSITION = 1   # x, y
DRAW_CURTAIN = 2
RESET_CURTAIN = 3



class Panel(Overlay, EventHandler):
    
    def __init__(self, app=None):
        """ If you pass in None for app, the widget will reside on the top layer
            above all apps but below pointers.
        """
        Overlay.__init__(self, OVERLAY_PANEL)
        EventHandler.__init__(self, app)

        self.registerForEvent(EVT_DRAG, self.__onDrag)
        self.registerForEvent(EVT_CLICK, self._onClick)

        self._allowDrag = False  # ONLY USE WITH ABSOLUTE POSITION!!! (wont work when aligned)
        
        self.__fitInWidth = True
        self.__fitInHeight = True

        self.__borderWidth = 0

        self._zOffset -= 0.01  # just below the other widgets

        self.px = self.py = 0


    def drawBorder(self, doDraw, width=1):
        if doDraw:
            self.__borderWidth = width
        else:
            self.__borderWidth = 0
    

    def _drawCurtain(self, direction, normPos):
        if direction == VERTICAL:
            pos = min(1.0, normPos)
        else:
            pos = normPos
            if normPos > 1.0:
                pos = 1.0
            elif normPos < -1.0:
                pos = -1.0
        self.sendOverlayMessage(DRAW_CURTAIN, direction, pos)


    def _resetCurtain(self):
        self.sendOverlayMessage(RESET_CURTAIN)


    def __onDrag(self, event):
        if self._allowDrag:
            self.pos.x += event.dX
            self.pos.x += event.dY
            self.sendOverlayMessage(MOVE, event.dX, event.dY)
            self._refresh()
            

    def _onClick(self, event):
        if self._allowDrag:
            pointer = event.device.pointer
            cx,cy = event.x, event.y
            if event.isDown:
                self.px, self.py = (cx-self.getX()), (cy-self.getY())
                event.device.toEvtHandler = self
                if pointer: pointer.showDragPointer()
            else:
                if pointer: pointer.resetPointer() 
                self.setPos(cx-self.px, cy-self.py)
                self._refresh()

                self.sendOverlayMessage(SET_POSITION, self.getX(), self.getY())


    def fitInWidth(self, doFit):
        self.__fitInWidth = doFit

    
    def fitInHeight(self, doFit):
        self.__fitInHeight = doFit


    # to xml
    def _specificToXml(self, parent):
        f = self.doc.createElement("fit")
        f.setAttribute("width", str(int(self.__fitInWidth)))
        f.setAttribute("height", str(int(self.__fitInHeight)))
        parent.appendChild(f)

        b = self.doc.createElement("border")
        b.setAttribute("width", str(self.__borderWidth))
        parent.appendChild(b)

    # from xml
    def _onSpecificInfo(self):
        """ specific info was parsed... use it here """
        if "fit" in self.xmlElements:
            self.__fitInWidth = int(self.xmlElements["fit"][0].attrs["width"])
            self.__fitInHeight = int(self.xmlElements["fit"][0].attrs["height"])
            


    def _fitAroundChildren(self):
        w = h = 0;
        pb = self._parentBounds

        if self.__fitInWidth:
            for child in self._getChildren().values():
                w += child.getWidth() + child.pos.xMargin # add up the widths
            
        elif self.size.wType == NORMALIZED:
            w = int(floor(pb.getWidth() * self.size.w))
        else:
            w = int(self.size.w * self._getScale())


        if self.__fitInHeight:
            for child in self._getChildren().values():
                h += child.getHeight() + child.pos.yMargin # add up the heights

        elif self.size.hType == NORMALIZED:
            h = int(floor(pb.getHeight() * self.size.h))
        else:
            h = int(self.size.h * self._getScale())   

        return w,h


    def _positionChildren(self):
        """ updates the position of all children """

        # update the position of all children
        for child in self._getChildren().values():
            if not child._isSlideHidden():
                child._updateX(self.getBounds())
                child._updateY(self.getBounds())
                child._positionChildren()
