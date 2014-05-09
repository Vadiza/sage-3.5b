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



#### joysticks

from managerConn import ManagerConnection
import wx, time, sys


#DEVICE_TYPE = "joystick"   # a plugin for this type must exist in "devices" called puck.py

# types of messages sent to the manager
MOVE = "1"
BUTTON = "2"
ARROW = "3"
WHEEL = "4"

class Joystick:
    def __init__(self, joyId, manager):
        self.joy = wx.Joystick(joyId)

        self.dontUse = False  # dont report this device
        if self.joy.GetProductName().startswith('Nintendo'):
            self.dontUse = True
            
        self.isFake = self.joy.GetProductName().startswith('PPJoy')
        self.isWiimote = False
        
        if self.isFake:
            # determine if it's a wii (PPJoy >= 13 is a wii)
            self.isWiimote = int(self.joy.GetProductName().lstrip("PPJoy Virtual joystick ")) > 12
            if self.isWiimote:
                self.joyId = "joystick"+str(joyId)+"_wiimote"
            else:
                self.joyId = "joystick"+str(joyId)+"_fake"
        else:
            self.joyId = "joystick"+str(joyId)+"_real"

        self.manager = manager
        self.xAxis, self.yAxis, self.prevX, self.prevY = 0,0,0,0
        self.buttons = [False, False, False, False, False]
        self.pov = -1

        # for dealing with drift
        xmin = self.joy.GetXMin()
        xmax = self.joy.GetXMax()
        self.__range = abs(xmax - xmin)
        self.__center = abs((xmin + xmax)/2.0) #so that 0 <-> max and -max/2 <-> max/2 both work
        self.drift = 0.1 # maximum of 10% drift accounted for
        if self.isFake: self.drift = 0.001
        self.usefulRange = 1 - self.drift


    def checkAxis(self):
        (self.xAxis, self.yAxis) = self.joy.GetPosition()
        self.__normalize()
        self.__accountForDrift()
            
        # report if there has been change
        if (not self.isWiimote and (self.xAxis != 0 or self.yAxis != 0)) or \
               (self.isWiimote and (self.xAxis != self.prevX or self.yAxis != self.prevY)):
            msg = MOVE +" "+str(self.xAxis)+" "+str(self.yAxis)
            self.manager.sendMessage(self.joyId, "joystick", msg)

        self.prevX, self.prevY = self.xAxis, self.yAxis   # remember for next time


    def checkButtons(self):
        bits = self.joy.GetButtonState()
        buttons = [bits & 1 == 1, bits & 2 == 2, bits & 4 == 4, bits&8==8, bits&16==16]
        for i in range(0,5):
            if self.buttons[i] != buttons[i]:
                self.buttons[i] = buttons[i]

                msg = None
                if i < 3:
                    msg = BUTTON+" "+str(i+1)+" "+str(int(buttons[i]))
                elif i == 3 and buttons[i]:
                    msg = WHEEL+" -3"
                elif i == 4 and buttons[i]:
                    msg = WHEEL+" 3"
                
                if msg:
                    self.manager.sendMessage(self.joyId, "joystick", msg)
                    

    def checkArrow(self):
        pov = self.joy.GetPOVPosition()
        if self.pov != pov:
            self.pov = pov
            if pov < 65000:  # ie NOT center
                msg = ARROW+" "+str(pov)
                self.manager.sendMessage(self.joyId, "joystick", msg)


    def __normalize(self):
        """ map any type of coords to normalized coords in range -1 to 1 """
        self.xAxis = ((self.xAxis - self.__center) / (self.__range/2.0))
        self.yAxis = ((self.yAxis - self.__center) / (self.__range/2.0))
                        
        
    def __accountForDrift(self):
        x = self.xAxis
        y = self.yAxis
        
        # apply the movement threshold and get the normalized crds
        if abs(x) <= self.drift:
            x = 0
        else:
            if x > 0:  x = float(x-self.drift)/self.usefulRange
            else: x = float(x+self.drift)/self.usefulRange

        if abs(y) <= self.drift:
            y = 0
        else:
            if y > 0:  y = float(y-self.drift)/self.usefulRange
            else: y = float(y+self.drift)/self.usefulRange

        self.xAxis = x
        self.yAxis = y

            

class CaptureJoysticks:
    
    def __init__(self, manager):
        self.manager = manager
        self.app = wx.App(redirect=False)
        self.numJoy = wx.Joystick().GetNumberJoysticks()
        if self.numJoy > 0:
            self.createJoysticks()
            self.capture()
        else:
            print "No joysticks connected..."
            self.manager.quit()

    def createJoysticks(self):
        self.joysticks = []
        for j in range(0,self.numJoy):
            joy = Joystick(j, self.manager)
            if not joy.dontUse:
                self.joysticks.append( joy )
            else:
                del joy
        
    def capture(self):
        while True:
            for j in self.joysticks:
                j.checkButtons()
                j.checkAxis()
                j.checkArrow()
            time.sleep(1.0/40)




# you can optionally pass in a port number of the manager on command line
port = 20005
if len(sys.argv) < 2:
    print "Usage: python "+sys.argv[0]+" sageIP"
    sys.exit(0)
elif len(sys.argv) > 2:
    port = int(sys.argv[2])

# start everything off
CaptureJoysticks( ManagerConnection(sys.argv[1], port) )
    
