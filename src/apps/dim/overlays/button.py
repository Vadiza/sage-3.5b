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


def makeNew(app=None):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Button(app)


TYPE = WIDGET_PLUGIN


# messages
SET_STATE = 0

# graphical states for a button (UP & DOWN coincide with
# button events: BUTTON_DOWN & BUTTON_UP as defined in globals.py)
DOWN_STATE = 0
UP_STATE   = 1
MOUSE_OVER = 2
MOUSE_NOT_OVER = 3


class Button(Overlay, EventHandler):
    
    def __init__(self, app=None):
        Overlay.__init__(self, OVERLAY_BUTTON)
        EventHandler.__init__(self, app)
        
        # register for the events that are fired by devices
        self.registerForEvent(EVT_CLICK, self._onClick)
        self.registerForEvent(EVT_MOVE, self._onMove)
        self.registerForEvent(EVT_LEFT_WINDOW, self._onLeftWindow)
        self.registerForEvent(EVT_ENTERED_WINDOW, self._onEnteredWindow)

        # paths to images drawn for different button states
        self.__upImage = "images/default_button_up.png"
        self.__downImage = ""
        self.__overImage = ""

        self.__doPlaySound = True

        # button can be a toggle
        self._isToggle = False
        self.__radioFunc = True
        self.__clicked = False
        self._manual = False
        self._radio = False

        # current state of the button
        self.__state = UP_STATE
        
        # callbacks you can register
        self.__onDown = None
        self.__onUp = None
        self._lastDevice = None
        
        self.__minTimeBetweenClicks = 0.5  # by default, allow rapid successive clicks
        self.__lastUp = 0.0
        self.__lastDown = 0.0


    def initialize(self, overlayId):
        Overlay.initialize(self, overlayId)

        # force a different initial state
        if self.__state != UP_STATE:
            newState = UP_STATE
            newState, self.__state = self.__state, newState
            self.setState(newState)
    

    # -------    WIDGET PARAMETERS   -----------------    

    def onDown(self, callback):
        """ the callback must accept one parameter which 
        will be this button object itself """
        self.__onDown = callback


    def onUp(self, callback):
        """ the callback must accept one parameter which 
        will be this button object itself """
        self.__onUp = callback


    def setUpImage(self, filename):
        """ relative to $SAGE_DIRECTORY/ """
        self.__upImage = filename

            
    def setDownImage(self, filename):
        """ relative to $SAGE_DIRECTORY/ """
        self.__downImage = filename


    def setOverImage(self, filename):
        """ relative to $SAGE_DIRECTORY/ """
        self.__overImage = filename


    def preventAccidentalClicks(self, doPrevent=True, minTime=5.0):
        if doPrevent:
            self.__minTimeBetweenClicks = minTime  # seconds
        else:
            self.__minTimeBetweenClicks = 0.5


    def setToggle(self, isToggle, doManual=False):
        """ is this a toggle button with 2 distinct states?
        - doManual=True allows the user to manually control the
        state of the button using setState method
        """
        self._isToggle = isToggle
        self._manual = doManual


    def setRadio(self, doRadio=True):
        self._radio = doRadio


    def setState(self, state):
        """ allows you to toggle the button programatically
            - state is either BUTTON_UP or BUTTON_DOWN
        """
        if state != self.__state:
            self.__state = state
            self.sendOverlayMessage(SET_STATE, state)
                                
            # send the event to the app if appWidget
            if self._isAppWidget() and state in self.widgetEvents:
                self._sendAppWidgetEvent(state)
        
                
    def getState(self):
        """ query the current button state... useful for toggle buttons """
        if self.__state == UP_STATE:
            return BUTTON_UP
        else:
            return BUTTON_DOWN


    def doPlaySound(self, doPlay):
        """ if True, it will play click sounds (default is True) """
        self.__doPlaySound = doPlay


    def __doCallback(self, st=BUTTON_UP):
        now = time.time()
        if st == BUTTON_UP and now-self.__lastUp > self.__minTimeBetweenClicks:
            self.__lastUp = now
            self.__onUp(self)
        elif st == BUTTON_DOWN and now-self.__lastDown > self.__minTimeBetweenClicks:
            self.__lastDown = now
            self.__onDown(self)
        
        
    # -------     XML STUFF     -----------------    

    def _graphicsToXml(self, parent):
        self._addGraphicsElement(parent, "up", self.__upImage)
        if self.__downImage:
		self._addGraphicsElement(parent, "down", self.__downImage)
        if self.__overImage == "":
		self._addGraphicsElement(parent, "over", self.__overImage)

    def _specificToXml(self, parent):
        if self._isToggle:
            parent.appendChild(self.doc.createElement("toggle"))


    def _onSpecificInfo(self):
        """ specific info was parsed... use it here """
        if "toggle" in self.xmlElements:
            self._isToggle = True
     
    # -------    GENERIC  EVENTS   -----------------    

    def _onLeftWindow(self, event):
        self._captured = False
        self.sendOverlayMessage(SET_STATE, MOUSE_NOT_OVER, 0)


    def _onEnteredWindow(self, event):
        self.sendOverlayMessage(SET_STATE, MOUSE_OVER)



    # -------    DEVICE EVENTS   -----------------
    
    def _onMove(self, event):
        """ need this to receive LEFT_WINDOW and ENTERED_WINDOW events """
        self._lastDevice = event.device
        pass

 
    def _onClick(self, event):
        """ handles the drawing and sending events to app if appWidget 
            May be overridden but make sure you call this method first.
            It also sets the __state variable which you can query with getState().
        """
        self._lastDevice = event.device
        
        if self._radio:
            if event.isDown:
                if self.__state != DOWN_STATE:
                    self.__state = DOWN_STATE
                    self.sendOverlayMessage(SET_STATE, DOWN_STATE)
                if self.__onDown:
                    self.__doCallback(BUTTON_DOWN)
                

        else:

            if event.isDown:
                if self._isToggle:
                    return

                if self.__doPlaySound:
                    playSound(CLICK)

                self.sendOverlayMessage(SET_STATE, DOWN_STATE)
                self.__state = DOWN_STATE

                # send the event to the app if appWidget
                if self._isAppWidget() and BUTTON_DOWN in self.widgetEvents:
                    self._sendAppWidgetEvent(BUTTON_DOWN)
                elif self.__onDown:   # execute local callback
                    self.__doCallback(BUTTON_DOWN)
                    

            else:
                if self._isToggle:

                    if self.__doPlaySound:
                        playSound(CLICK)

                    if self.__state == DOWN_STATE:

                        if not self._manual:
                            self.sendOverlayMessage(SET_STATE, UP_STATE)
                            self.__state = UP_STATE

                        # send the event to the app if appWidget
                        if self._isAppWidget() and BUTTON_UP in self.widgetEvents:
                            self._sendAppWidgetEvent(BUTTON_UP)
                        elif self.__onUp:  # execute local callback
                            self.__doCallback(BUTTON_UP)

                    else:
                        if not self._manual:
                            self.sendOverlayMessage(SET_STATE, DOWN_STATE)
                            self.__state = DOWN_STATE

                        # send the event to the app if appWidget
                        if self._isAppWidget() and BUTTON_DOWN in self.widgetEvents:
                            self._sendAppWidgetEvent(BUTTON_DOWN)
                        elif self.__onDown:   # execute local callback
                            self.__doCallback(BUTTON_DOWN)

                else:  # not toggle
                    self.sendOverlayMessage(SET_STATE, UP_STATE)
                    self.__state = UP_STATE

                    # send the event to the app if appWidget
                    if self._isAppWidget() and BUTTON_UP in self.widgetEvents:
                        self._sendAppWidgetEvent(BUTTON_UP)                    
                    elif self.__onUp:  # execute local callback
                        self.__doCallback(BUTTON_UP)
                        
