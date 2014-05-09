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
# Author: UCSD / Calit2
#        
############################################################################




import device
from globals import *
import time
from threading import Thread



def makeNew(deviceId):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Xtion(deviceId)
	
# 1 = 1 finger
# 2 = fist
# 3 = Victory
# 4 = Lshape
# 5 = Open Palm
# 6 = Pushback

MOVE = 1
DOUBLE_LEFT = 3
LEFT_DRAG = 2
RIGHT_DRAG = 4
REST = 5
CLOSE = 6

class Xtion(device.Device):
    
    def __init__(self, deviceId):
        device.Device.__init__(self, "xtionTest", deviceId, needPointer=True)
        print "<<<<<<<<<<<<<<<<<<<<<< Xtion!!! >>>>>>>>>>>>>>>>>>>>>>>>>>"
        # current state of the device
        self.x = 4000   # position in SAGE coords
        self.y = 1150   # position in SAGE coords
        self.clickX = 0  # where the click happened 
        self.clickY = 0  # where the click happened
	self.lastx = 0
	self.lasty = 0
        self.lastBtn = 0
	self.lastdx = 0
	self.lastdy = 0
	self.leftdown = False
	self.isDown = False
        self.x0 = 0 
        self.x1 = 0 
        self.x2 = 0 
        self.y0 = 0 
        self.y1 = 0 
        self.y2 = 0 
	self.indentx = 0
	self.indenty = 0
	self.tempx = 4000
	self.tempy = 1150

    def onMessage(self, data, firstMsg=False):
	#print repr(data)
        tokens = data.split()
        xtionId = int(tokens[0].strip())
        x = float(tokens[1].strip())
        y = float(tokens[2].strip())
	if self.lastBtn == LEFT_DRAG and xtionId != LEFT_DRAG:
		if self.pointer: self.pointer.setColor(255, 0, 0)
	
	if self.lastBtn == RIGHT_DRAG and xtionId != RIGHT_DRAG:
		if self.pointer: self.pointer.setColor(255, 0, 0)

	if self.leftdown == True:
		# Releasing the pressed button
		if xtionId != LEFT_DRAG and self.lastBtn == LEFT_DRAG:
			# For left drag
			self.postEvtClick(self.x, self.y, 1, False, EVT_PAN)
			self.leftdown = False
		elif xtionId != RIGHT_DRAG and self.lastBtn == RIGHT_DRAG:
			# For right drag
			self.postEvtClick(self.x, self.y, 2, False, EVT_ROTATE)
			self.leftdown = False			
		
	if xtionId == MOVE or xtionId == LEFT_DRAG or xtionId == RIGHT_DRAG:
	# always convert to SAGE coords first. x & y are raw coordinates
		x = x*100
		y = y*100
		#print "X: ",x
		#print "Y: ",y	
		
		# Displacement calculation
		dx = dy = 0
		dx = x - self.lastx ;  self.lastx = x
		dy = y - self.lasty ;  self.lasty = y

		# Accuration calculation
		
		dx2 = dx - self.lastdx; dy2 = dy - self.lastdy
		self.lastdx = dx; self.lastdy = dy

		if xtionId != self.lastBtn: 
		# When gesture changed, ignore the first input data
			dx = dy = 0
		if abs(dx2)>=3000 or abs(dx2)<=15:
			dx = 0
		if abs(dy2)>=3000 or abs(dy2)<=15:
			dy = 0
		
		dx=dx/8  # Smooth vs sensivity. / larger number = smoother
		dy=dy/8
		self.indentx = dx/50
		self.indenty = dy/50
		dx = int(round(self.indentx))
		dy = int(round(self.indenty))
			
		for i in xrange(1, 50):
			# Each position movement will have 10 frames. 
			self.tempx = self.tempx+self.indentx
			self.tempy = self.tempy+self.indenty
			self.x = int(round(self.tempx))
			self.y = int(round(self.tempy))

			# Boundaries of the screen
			if self.x <=0: self.x = 0
			if self.y <=0: self.y = 0
			if self.x >=8196: self.x = 8196
			if self.y >=2304: self.y = 2304
			if self.tempx <=0: self.tempx = 0
			if self.tempy <=0: self.tempy = 0
			if self.tempx >=8196: self.tempx = 8196
			if self.tempy >=2304: self.tempy = 2304

			
			# Only moves pointer if gesture = MOVE or LEFT_DRAG or RIGHT_DRAG
			if xtionId==LEFT_DRAG:
				if self.lastBtn != LEFT_DRAG:
					self.clickX, self.clickY = self.x, self.y
					self.postEvtClick(self.x, self.y, 1, True, EVT_PAN)
					self.leftdown = True
					self.lastBtn = LEFT_DRAG  # setting last botton here each that it would set the click again in the for loop
				if self.pointer: self.pointer.setColor(0, 255, 0)
				self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
				
			if xtionId==RIGHT_DRAG:
				# Experimental right click and drag
				if self.lastBtn != RIGHT_DRAG:
					self.clickX, self.clickY = self.x, self.y
					self.postEvtClick(self.x, self.y, 2, True, EVT_ROTATE)
					self.leftdown = True
					self.lastBtn = RIGHT_DRAG
				self.postEvtAnalog2(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
				if self.pointer: self.pointer.setColor(0, 0, 255)
			elif xtionId==MOVE:
				self.postEvtMove(self.x, self.y, dx, dy)      

				self.lastBtn = MOVE  
		   # move the pointer
			if self.pointer: 				   
				self.pointer.movePointer(self.x, self.y)
		                 # The bigger the smoother, but more delay
				time.sleep(0.00005) # The bigger the smoother, but more delay

 			#End of for loop	
		#print "self.x", self.x
		#print "self.y", self.y

	# End of MOVE/DRAG

	# double click
	if xtionId == DOUBLE_LEFT and self.lastBtn != DOUBLE_LEFT:
		if self.leftdown == False and self.lastBtn != DOUBLE_LEFT:
			self.postEvtDoubleClick(self.x, self.y, 1, 3)	# Functions as a double click, forEvt = 1, EVT_PAN = 3
			self.leftdown = False   
			self.lastBtn = DOUBLE_LEFT
			
	if xtionId == REST:
		if self.pointer: self.pointer.setColor(255, 0, 0)
		self.lastBtn = REST


	if xtionId == CLOSE:
		if self.lastBtn != CLOSE and self.leftdown == False:
			dx= dy = 0
			self.clickX, self.clickY = self.x, self.y
			self.postEvtClick(self.x, self.y, 1, True, EVT_PAN)
			self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
			if self.pointer: self.pointer.movePointer(self.x, self.y)
			self.postEvtAnalog1(6800, 4350, 6800, 4350, dx, dy, 0)
			self.postEvtClick(6800, 4350, 1, False, EVT_PAN)
			self.postEvtAnalog1(self.clickX, self.clickY, self.x, self.y, dx, dy, 0)
			if self.pointer: self.pointer.movePointer(self.x, self.y)
		self.lastBtn = CLOSE

	
