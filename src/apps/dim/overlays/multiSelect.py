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


# this is the proxy for allowing the selection and manipulation of multiple apps at once


from overlay import Overlay
from eventHandler import EventHandler
import app as appOverlayObj
from globals import *
from math import cos, sin, radians
import copy, time


def makeNew():
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return MultiSelect()


# doesn't need to be autoloaded
#TYPE = WIDGET_PLUGIN



class MultiSelect(EventHandler):
    
    def __init__(self):
        EventHandler.__init__(self)
        
        # must set size and pos first
        self.setSize(1,1)   
        self.setPos(-1, -1)

        self.__selectedOverlays = []


    def selectOverlay(self, overlay):
        # deselect any other objects of a different type
        self.__removeOtherTypes(overlay)
        
        if overlay not in self.__selectedOverlays:
            self.__selectedOverlays.append(overlay)
            overlay.select(True)


    def deselectOverlay(self, overlay):
        if overlay in self.__selectedOverlays:
            self.__selectedOverlays.remove(overlay)
            overlay.select(False)


    def deselectAllOverlays(self):
        for overlay in self.__selectedOverlays:
            overlay.select(False)
        self.__selectedOverlays = []


        # allow only one type of object to be selected at any time
    def __removeOtherTypes(self, overlay):
        doRemove = False
        for o in self.__selectedOverlays:
            if o.getType() != overlay.getType():
                doRemove = True
                break
            
        if doRemove:
            self.deselectAllOverlays()
        

    def getSelectedOverlayType(self):
        if len(self.__selectedOverlays) > 0:
            return self.__selectedOverlays[0].getType()
        else:
            return None
        

    def distributeEvent(self, event):
        event.original = False
        removeAll = False
        
        for overlay in copy.copy(self.__selectedOverlays):
            copyOfEvent = copy.copy(event)
            callback = overlay._getCallback(copyOfEvent.eventId)
            if callback:
                callback(copyOfEvent)
                
                # if the event is a click event, copy the deselectAll flag to the original event
                if event.eventId == EVT_CLICK:
                    event.deselectAll = copyOfEvent.deselectAll



    def getSelectedOverlays(self):
        """ returns overlays """
        return self.__selectedOverlays


    def getSelectedApps(self):
        """ returns SageApp objects """
        apps = []
        for overlay in self.__selectedOverlays:
            if isinstance(overlay, appOverlayObj.App): 
                apps.append(overlay.app)
        return apps

    
