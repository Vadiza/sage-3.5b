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
from thumbnail import Thumbnail
from globals import *


def makeNew(app=None):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return EnduranceThumbnail(app)


TYPE = WIDGET_PLUGIN


# messages
SET_RATING = 2


class EnduranceThumbnail(Thumbnail):
    
    def __init__(self, app=None):
        Thumbnail.__init__(self, app)
        self.overlayType = OVERLAY_ENDURANCE_THUMBNAIL
        self.__rating = 0
        self.registerForEvent(EVT_ZOOM, self._onZoom)
        self.__maxRating = 5
        self.allowDrag(True)
        

    def setRating(self, newRating):
        self.__rating = newRating


    def getRating(self):
        return self.__rating
    

    # to xml
    def _specificToXml(self, parent):
        s = self.doc.createElement("enduranceThumbnail")
        s.setAttribute("rating", str(self.__rating))
        parent.appendChild(s)
        Thumbnail._specificToXml(self, parent)


    # from xml
    def _onSpecificInfo(self):
        """ specific info was parsed... use it here """
        if "enduranceThumbnail" in self.xmlElements:
            self.__rating = int(self.xmlElements["enduranceThumbnail"][0].attrs["rating"])
        Thumbnail._onSpecificInfo(self)
            

    # -------    GENERIC  EVENTS   -----------------    

    def _onZoom(self, event):
        dZoom = event.dX

        # change the rating
        if dZoom > 0:
            self.__rating -= 1
        else:
            self.__rating += 1

        # keep it within bounds
        if self.__rating < 0: self.__rating = 0
        elif self.__rating > self.__maxRating: self.__rating = self.__maxRating

        # update the drawing
        self.sendOverlayMessage(SET_RATING, self.__rating)

        # send the event to the app if appWidget
        if self._isAppWidget() and ENDURANCE_THUMBNAIL_RATING in self.widgetEvents:
            self._sendAppWidgetEvent(ENDURANCE_THUMBNAIL_RATING, self.__rating)
            
        
        
