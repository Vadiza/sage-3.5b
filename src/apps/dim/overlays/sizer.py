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


def makeNew(app=None, direction=HORIZONTAL):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Sizer(app, direction)


TYPE = WIDGET_PLUGIN


class Sizer(Overlay, EventHandler):
    
    def __init__(self, app=None, direction=HORIZONTAL):
        """ If you pass in None for app, the widget will reside on the top layer
            above all apps but below pointers.
        """
        Overlay.__init__(self, OVERLAY_SIZER)
        EventHandler.__init__(self, app)
        self.__direction = direction
        
        # let the events pass through since we are transparent
        self._eventTransparent = True  

##         self.__spacers = {}  # key=parentOrder, value=size


##     def addSpacer(self, size):
##         self.__spacers[self._getNewParentOrder()] = size


##     def _getNewParentOrder(self):
##         """ overridden to handle spacers properly """
##         childrenPO = EventHandler._getNewParentOrder()

##         spacerPO = 0
##         if (self.__spacers) > 0:
##             spacerPO = max(self.__spacers)+1

##         return max(childrenPO, spacerPO)
        
        
    # to xml
    def _specificToXml(self, parent):
        s = self.doc.createElement("sizer")
        s.setAttribute("direction", str(self.__direction))
        parent.appendChild(s)


    # from xml
    def _onSpecificInfo(self):
        """ specific info was parsed... use it here """
        if "sizer" in self.xmlElements:
            self.__direction = int(self.xmlElements["sizer"][0].attrs["direction"])
            

    def _fitAroundChildren(self):
        w = h = 0;

        if self.__direction == HORIZONTAL:
            for child in self._getChildren().values():
                if child.isVisible() and not child._isSlideHidden():
                    w += child.getWidth() + child.pos.xMargin # add up the widths
                    if child.getHeight() + child.pos.yMargin > h:  # get max height
                        h = child.getHeight() + child.pos.yMargin 
                
        else:
            for child in self._getChildren().values():
                if child.isVisible() and not child._isSlideHidden():
                    h += child.getHeight() + child.pos.yMargin # add up the heights
                    if child.getWidth() + child.pos.xMargin > w:  # get max width
                        w = child.getWidth() + child.pos.xMargin 

        return w,h

        
    def _positionChildren(self):
        """ updates the position of all children """
        currX = self.getX()
        currY = self.getY()+self.getHeight()
        
        # sort the children based on their order in the sizer
        children = self._getChildren()
        sortedChildren = children.keys()
        sortedChildren.sort()
       
        # update the position of all children
        for childId in sortedChildren:
            child = children[childId]
            if not child.isVisible(): 
                continue

            if child._isSlideHidden():
                if self.__direction == HORIZONTAL:
                    child._updateY(self.getBounds())
                else:    # vertical
                    child._updateX(self.getBounds())
            else:

                if self.__direction == HORIZONTAL:
                    child._setX(currX + child.pos.xMargin)
                    child._updateY(self.getBounds())
                    currX += child.getWidth() + child.pos.xMargin

                else:    # vertical
                    child._setY(currY - child.getHeight() - child.pos.yMargin)
                    child._updateX(self.getBounds())
                    currY = currY - child.getHeight() - child.pos.yMargin
                
            child._positionChildren()
