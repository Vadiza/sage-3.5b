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



class Event:
    """ the base class for all the events """
    
    def __init__(self, eventId, eventType, toEvtHandler=None):
        self.eventType = eventType
        self.eventId = eventId
        self.toEvtHandler = toEvtHandler
        self.original = True  # this is False only if it comes from multiApp object

        

#-----------------------------------------------------
#       DEVICE EVENTS (position dependent)
#-----------------------------------------------------


class MoveEvent(Event):
    
    def __init__(self, device, newX, newY, dX, dY, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_MOVE_SPECIAL
        else:
            evtId = EVT_MOVE
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = newX
        self.y = newY
        self.dX = dX
        self.dY = dY


class ClickEvent(Event):
    
    def __init__(self, device, x, y, btnId, isDown, forEvt, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_CLICK_SPECIAL
        else:
            evtId = EVT_CLICK
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.btnId = btnId
        self.isDown = isDown
        self.forEvt = forEvt
        
        # if an object was actually dropped on another one as a result of a click
        self.droppedOnTarget = None  

        # should we deselect all objects after the event?
        self.deselectAll = False


class DoubleClickEvent(Event):
    
    def __init__(self, device, x, y, btnId, forEvt, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_DOUBLE_CLICK_SPECIAL
        else:
            evtId = EVT_DOUBLE_CLICK
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.btnId = btnId
        self.forEvt = forEvt


class MultiTouchHoldEvent(Event):
    # this is multiple fingers AT THE SAME TIME... not successive clicks...
    def __init__(self, device, x, y, numPoints, lifePoint, toEvtHandler=None):
        Event.__init__(self, EVT_MULTI_TOUCH_HOLD, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.numPoints = numPoints
        self.lifePoint = lifePoint


class MultiTouchSwipeEvent(Event):
    # this is multiple fingers AT THE SAME TIME... not successive clicks...
    def __init__(self, device, x, y, numPoints, dX, dY, startX, startY, lifePoint, toEvtHandler=None):
        Event.__init__(self, EVT_MULTI_TOUCH_SWIPE, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.numPoints = numPoints
        self.dX = dX
        self.dY = dY
        self.startX = startX
        self.startY = startY
        self.lifePoint = lifePoint


class BigClickEvent(Event):
    
    def __init__(self, device, x, y, lifePoint, toEvtHandler=None):
        Event.__init__(self, EVT_BIG_CLICK, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.lifePoint = lifePoint


class Analog1Event(Event):

    def __init__(self, device, startX, startY, x, y, dX, dY, dZ, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_ANALOG1_SPECIAL
        else:
            evtId = EVT_ANALOG1
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.startX = startX
        self.startY = startY
        self.dX = dX
        self.dY = dY
        self.dZ = dZ


class Analog2Event(Event):

    def __init__(self, device, startX, startY, x, y, dX, dY, dZ, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_ANALOG2_SPECIAL
        else:
            evtId = EVT_ANALOG2
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.startX = startX
        self.startY = startY
        self.dX = dX
        self.dY = dY
        self.dZ = dZ


class Analog3Event(Event):

    def __init__(self, device, startX, startY, x, y, dX, dY, dZ, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_ANALOG3_SPECIAL
        else:
            evtId = EVT_ANALOG3
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.startX = startX
        self.startY = startY
        self.dX = dX
        self.dY = dY
        self.dZ = dZ


class FastDragEvent(Event):

    def __init__(self, device, startX, startY, x, y, dX, dY, dZ, toEvtHandler=None):
        evtId = EVT_FAST_DRAG
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.startX = startX
        self.startY = startY
        self.dX = dX
        self.dY = dY
        self.dZ = dZ



class ArrowEvent(Event):

    def __init__(self, device, arrow, x, y, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_ARROW_SPECIAL
        else:
            evtId = EVT_ARROW
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.arrow = arrow
        self.x = x
        self.y = y


class KeyEvent(Event):

    def __init__(self, device, key, x, y, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_KEY_SPECIAL
        else:
            evtId = EVT_KEY
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.key = key
        self.x = x
        self.y = y


class CustomEvent(Event):

    def __init__(self, device, data, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_CUSTOM_SPECIAL
        else:
            evtId = EVT_CUSTOM
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.data = data

class WindowEnteredEvent(Event):

    def __init__(self, device, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_ENTERED_WINDOW_SPECIAL
        else:
            evtId = EVT_ENTERED_WINDOW
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device


class WindowLeftEvent(Event):

    def __init__(self, device, toEvtHandler=None):
        if device.specialDevice:
            evtId = EVT_LEFT_WINDOW_SPECIAL
        else:
            evtId = EVT_LEFT_WINDOW
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device


class DropEvent(Event):
    
    def __init__(self, device, x, y, obj, toEvtHandler=None):
        evtId = EVT_DROP
        Event.__init__(self, evtId, DEVICE_EVENT, toEvtHandler)
        self.device = device
        self.x = x
        self.y = y
        self.object = obj  # the object that is being dropped
        
        # if an object was actually dropped on another one as a result of a click
        self.droppedOnTarget = None

        # should we deselect all objects after they have been dropped?
        self.deselectAll = False

        # Sungwon multi-site sharing
        self.data = None



#----------------------------------------------------------
#       GENERIC EVENTS (all who register for it receive it)
#----------------------------------------------------------


class AppInfoEvent(Event):

    def __init__(self, sageApp, myMsg=False):
        Event.__init__(self, EVT_APP_INFO, GENERIC_EVENT)
        self.app = sageApp
        self.myMsg = myMsg  # was DIM the one that caused the change in the first place?
        

class NewAppEvent(Event):

    def __init__(self, sageApp):
        Event.__init__(self, EVT_NEW_APP, GENERIC_EVENT)
        self.app = sageApp


class NewRemoteAppEvent(Event):

    def __init__(self, sageApp):
        Event.__init__(self, EVT_NEW_REMOTEAPP, GENERIC_EVENT)
        self.app = sageApp


class PerfInfoEvent(Event):

    def __init__(self, data):
        Event.__init__(self, EVT_PERF_INFO, GENERIC_EVENT)
        tokens = string.split(data)
        # FIX - ie complete


class DisplayInfoEvent(Event):

    def __init__(self, dispInfo):
        Event.__init__(self, EVT_DISPLAY_INFO, GENERIC_EVENT)
        self.displayInfo = dispInfo


class AppKilledEvent(Event):

    def __init__(self, sageApp):
        Event.__init__(self, EVT_APP_KILLED, GENERIC_EVENT)
        self.app = sageApp


class ZChangeEvent(Event):

    def __init__(self, zHash):
        Event.__init__(self, EVT_Z_CHANGE, GENERIC_EVENT)
        self.zHash = zHash  # key=appId, value=new z value

        
class ObjectInfoEvent(Event):

    def __init__(self, data):
        Event.__init__(self, EVT_OBJECT_INFO, GENERIC_EVENT)
	# problem is that old version of XML lib do not add a \n at the end
	# Luc
        lines = data.split("\n", 1)
        objTok = lines[0].split()

        # object data (not in XML)
        self.overlayId = int(objTok[0])
        self.overlayType = objTok[1]
        self.widgetId = int(objTok[2])
        self.winId = int(objTok[3])
        self.app = getSageData().getApp(self.winId)  # could be None
        
        # the xml string
        self.xml = lines[1]


class ObjectRemovedEvent(Event):
    
    def __init__(self, data):
        Event.__init__(self, EVT_OBJECT_REMOVED, GENERIC_EVENT)
        self.overlayId = int(data)


class ObjectChangedEvent(Event):

    def __init__(self, data):
        Event.__init__(self, EVT_OBJECT_CHANGED, GENERIC_EVENT)
        tokens = data.split(" ", 2)
        self.overlayId = int(tokens[0])
        self.msgCode = int(tokens[1])
        self.data = tokens[2]


class WallClickedEvent(Event):

    def __init__(self, data):
        Event.__init__(self, EVT_WALL_CLICKED, GENERIC_EVENT)
        tokens = data.split(" ", 1)
        self.x = int(tokens[0])
        self.y = int(tokens[1])


class ScreenshotSavedEvent(Event):

    def __init__(self, screenshot, stateName, stateSaved=True, bounds=None, caller=None):
        Event.__init__(self, EVT_SCREENSHOT_SAVED, GENERIC_EVENT)
        self.bounds = bounds
        self.screenshot = screenshot
        self.stateName = stateName
        self.stateSaved = stateSaved
        self.caller = caller  # button that initiated this screenshot save
