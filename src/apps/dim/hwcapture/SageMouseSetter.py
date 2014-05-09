############################################################################
#
# DIM - A Direct Interaction Manager for SAGE
# Copyright (C) 2013 Electronic Visualization Laboratory,
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
############################################################################

'''
Created on Apr 28, 2013

@author: Andrew Ring
@author: Luc Renambot (edits: July 2013)
'''

import socket
from threading import Timer

class SageMouseSetter(object):
    '''
    classdocs
    '''

    MOVE = 1
    CLICK = 2
    WHEEL = 3
    COLOR = 4
    DOUBLE_CLICK = 5
    HAS_DOUBLE_CLICK = 10
    
    MOUSE_DOWN = 1
    MOUSE_UP = 0
    
    LEFT = 1
    RIGHT = 2
    
    INITIAL_MESSAGE_FRAME = '{0} mouse 10\n' #deviceName
    MOUSE_MOVE_FRAME = '{0} mouse 1 {1} {2}\n' #deviceName, x, y
    MOUSE_CLICK_FRAME = '{0} mouse 2 {1} {2}\n' #deviceName, button, down/up
    MOUSE_DOUBLE_CLICK_FRAME = '{0} mouse 5 1\n' #deviceName
    SCROLL_FRAME = '{0} mouse 3 {1}\n' #deviceName, val
    
    DOUBLE_CLICK_TIME = 0.35

    def __init__(self,deviceName, host='localhost', port=20005):
        '''
        Constructor
        '''
        self.deviceName = deviceName
        self.sage_host = host
        self.sage_port = port
        self.sage = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sage.connect((self.sage_host, self.sage_port))
        print('CONNECTED TO SAGE: %s:%d' %(self.sage_host, self.sage_port))
        initialMessage = SageMouseSetter.INITIAL_MESSAGE_FRAME.format(self.deviceName)
        self.sage.send(initialMessage)
        self.clicked = False
        self.sendNextUp = True
        self.x = 0.5 # initial position (center)
        self.y = 0.5
        
    def mouseMove(self, x, y):
        '''
        x and y should be 0 to 1 values.
        '''
        
        assert x >= 0
        assert x <= 1
        assert y >= 0
        assert y <= 1
        
	d = (x-self.x)*(x-self.x) + (y-self.y)*(y-self.y)
        if d>3e-6: # small movements are ignored
               moveMessage = SageMouseSetter.MOUSE_MOVE_FRAME.format(self.deviceName, x, y)
               self.sage.send(moveMessage)
               self.x = x
               self.y = y

        
    def mouseButtonEvent(self, button, direction):
        buttonNum = None
        directionNum = None
        
        if button is 'left':
            buttonNum = SageMouseSetter.LEFT
        elif button is 'right':
            buttonNum = SageMouseSetter.RIGHT
        elif button is 'double':
             buttonNum = SageMouseSetter.DOUBLE_CLICK
        elif button is 'downArrow' or button is 'leftArrow':
            buttonNum = SageMouseSetter.WHEEL
            scrollVal = -5
        elif button is 'upArrow' or button is 'rightArrow':
            buttonNum = SageMouseSetter.WHEEL
            scrollVal = 5
            
        if direction is 'up':
            directionNum = SageMouseSetter.MOUSE_UP
        elif direction is 'down':
            directionNum = SageMouseSetter.MOUSE_DOWN
            
        if buttonNum == SageMouseSetter.LEFT: 
            if directionNum == SageMouseSetter.MOUSE_DOWN:
                if self.clicked is True:
                    # double click instead
                    buttonNum = SageMouseSetter.DOUBLE_CLICK
                    self.sendNextUp = False
                else:
                    self.clicked = True
                    self.timerDown = Timer(SageMouseSetter.DOUBLE_CLICK_TIME, self.mouseClickTimeout)
                    self.timerDown.start()
            else:
                if self.sendNextUp is False:
                    self.sendNextUp = True
                    buttonNum = None # Don't do the mouse up for double-click down
        
        if buttonNum is not None and directionNum is not None:
            eventMessage = None
            if buttonNum == SageMouseSetter.LEFT or buttonNum == SageMouseSetter.RIGHT:
                eventMessage = SageMouseSetter.MOUSE_CLICK_FRAME.format(self.deviceName, buttonNum, directionNum)
            elif buttonNum == SageMouseSetter.DOUBLE_CLICK and direction is 'down':
                eventMessage = SageMouseSetter.MOUSE_DOUBLE_CLICK_FRAME.format(self.deviceName)
            elif buttonNum == SageMouseSetter.WHEEL and direction is 'down':
                eventMessage = SageMouseSetter.SCROLL_FRAME.format(self.deviceName, scrollVal)
            if eventMessage is not None:
                self.sage.send(eventMessage)
                
    def mouseClickTimeout(self):
        self.clicked = False

