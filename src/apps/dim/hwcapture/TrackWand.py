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
Created on Apr 24, 2013

@author: Andrew Ring
@author: Luc Renambot (edits: July 2013)
'''
from numpy.ma.core import max

from multiprocessing import Array, Process, Value
from CoordinateCalculator import CoordinateCalculator
import socket, sys, struct
import math, time
from euclid import Quaternion, Vector3
from SageMouseSetter import SageMouseSetter

# command line arguments parsing
import argparse

WORLD_CAMERA_MAX_ANGLE_X = 40.5 * math.pi / 180
WORLD_CAMERA_MAX_ANGLE_Y = 18.5 * math.pi / 180

MAX_ANGLE = 0.85

def average(mylist):
    sumNum = 0
    for i in range(len(mylist)):
        sumNum += mylist[i]
    if len(mylist) < 1:
        return 0
    return sumNum/len(mylist)
    
def contactCave(positionArray, sage, sage_port, tracker, tracker_port):

    #create object for yossi's code
    #this will be take values from quaternion
    #appears to need set_position() and set_orientation()
    #will return a tuple that is two doubles
    #each double represents how far along that axis the pointer should be placed
    #(0,0) is the upper left corner
    #(1,1) is the lower right corner
    screenPosition = CoordinateCalculator()

    # in meters
    CAVE_RADIUS = 127.50 * 2.54 / 100.0

    DATA_PORT    = 55000
    BUFFER_SIZE = 512

    xAxis = Vector3(1,0,0)
    yAxis = Vector3(0,-1,0)
    
    udpsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udpsock.bind(("", DATA_PORT))
    
    omicron = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    omicron.connect((tracker, tracker_port))

    MESSAGE = "data_on,%d" % udpsock.getsockname()[1]
    print("Sending to oMicron [%s:%d]: [%s]" % (tracker, tracker_port, MESSAGE))
    omicron.send(MESSAGE)
    
    #start sage mouse connection
    sageMouse = SageMouseSetter('wandTracker:wandTracker', sage, sage_port)

    cposition=[0.0,0.0,0.0]
    cposition[0] = 0.0
    cposition[1] = 0.0
    cposition[2] = 0.0

    caxis=[0.0,0.0,0.0]
    caxis[0] = 0.0
    caxis[1] = 1.0
    caxis[2] = 0.0

    org=[0.0,0.0,0.0]
    dir=[0.0,0.0,0.0]
    normal = [0.0,0.0,0.0]
    newpos = [0.0,0.0,0.0]
    lam=0.0
    cradius=CAVE_RADIUS
    normal[0] = 0.0
    normal[1] = 0.0
    normal[2] = 0.0
    newpos[0] = 0.0
    newpos[1] = 0.0
    newpos[2] = 0.0

    mouseButton = None
    triggerButton = False
    numMovesToAverage = 3 #chosen by trial and error
    currentMove = 0
    xPosList = []
    yPosList = []
    for __ in range(numMovesToAverage):
        xPosList.append(0.0)
        yPosList.append(0.0)
    while True:
        msg, addr = udpsock.recvfrom(BUFFER_SIZE)
        offset = 0;
        msglen = len(msg)
        if offset < msglen:
            timestamp = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t timestamp %d" % timestamp)
        if offset < msglen:
            sourceId = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t sourceId %d" % sourceId)
        if offset < msglen:
            serviceId = struct.unpack_from('<i', msg, offset)
            offset += 4
            # print("\t serviceId %d" % serviceId)
        if offset < msglen:
            serviceType = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t serviceType %d" % serviceType)
        if offset < msglen:
            etype = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t type %d" % etype)
        if offset < msglen:
            flags = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t flags %d" % flags)
        if offset < msglen:
            posx = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t posx %f" % posx)
        if offset < msglen:
            posy = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t posy %f" % posy)
        if offset < msglen:
            posz = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t posz %f" % posz)
        if offset < msglen:
            orw = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t orw %f" % orw)
        if offset < msglen:
            orx = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t orx %f" % orx)
        if offset < msglen:
            ory = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t ory %f" % ory)
        if offset < msglen:
            orz = struct.unpack_from('<f', msg, offset)
            offset += 4
            # print("\t orz %f" % orz)
        if offset < msglen:
            extraDataType = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t extraDataType %d" % extraDataType)
        if offset < msglen:
            extraDataItems = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t extraDataItems %d" % extraDataItems)
        if offset < msglen:
            #extraDataMask = struct.unpack_from('<I', msg, offset)
            offset += 4
            # print("\t extraDataMask %d" % extraDataMask)
        r_roll = math.asin(2.0*orx[0]*ory[0] + 2.0*orz[0]*orw[0])
        r_yaw = math.atan2(2.0*ory[0]*orw[0]-2.0*orx[0]*orz[0] , 1.0 - 2.0*ory[0]*ory[0] - 2.0*orz[0]*orz[0])
        r_pitch = math.atan2(2.0*orx[0]*orw[0]-2.0*ory[0]*orz[0] , 1.0 - 2.0*orx[0]*orx[0] - 2.0*orz[0]*orz[0])
        #print("\t orientation %f %f %f" % (r_roll,r_yaw,r_pitch))
       
	# Wand
        if etype[0] == 3 and serviceType[0] == 1 and sourceId[0] == 1:
            q = Quaternion(orw[0], orx[0],ory[0],orz[0])
            refVec = Vector3(0.0,0.0,-1.0)
            v = q * refVec
            dir[0] = v.x
            dir[1] = v.y
            dir[2] = v.z
            
            #This code should take the x,y,z,roll,pitch,yaw and set the positionArray to the correct values
            #Should have these values at this point in time
            #depending on how the socket code works, this may need to be moved down to if etype[0]=3
            #set x,y,z in calculator
            screenPosition.set_position(posx[0],posy[0],posz[0])
            #set roll,pitch,yaw in calculator
            screenPosition.set_orientation(v.x,v.y,v.z)
            #actually tell it to calculate the doubles
            screenPosition.calculate()
            #lock array
            positionArray.acquire()
            #set x in array
            #x is a double between 1 and 0
            positionArray[0]=screenPosition.get_x()
            #set y in array
            #y is a double between 1 and 0
            positionArray[1]=screenPosition.get_y()
            
            if positionArray[0] >= 0 and positionArray[1] >= 0 and positionArray[0] <=1 and positionArray[1] <= 1:
                xPosList[currentMove] = positionArray[0]
                yPosList[currentMove] = positionArray[1]
                currentMove = (currentMove+1)%numMovesToAverage
                #if currentMove is 0:
                if triggerButton:
                    sageMouse.mouseMove(average(xPosList), 1-average(yPosList))
            
            #release array so that sage code can access it
            positionArray.release()

        if etype[0] == 5:
            #print "DOWNNNN"
            # at this point need to send flag to another function
            #2 = right mouse button
            #4 = left mouse button
            #print "Button down ID: ", flags[0]
            mouseMovement = "down"

            if(flags[0] == 2):
                mouseButton = "right"
            elif(flags[0] == 4):
                mouseButton = "left"
            elif(flags[0] == 256): # down joystick
                pass
            elif(flags[0] == 128): # above trigger
                mouseButton = "double"  # simulate double click
            elif(flags[0] == 512): # trigger
                triggerButton = not triggerButton
            elif(flags[0] == 1024):
                mouseButton = "upArrow"
            elif(flags[0] == 2048):
                mouseButton = "downArrow"
            elif(flags[0] == 4096):
                mouseButton = "leftArrow"
            elif(flags[0] == 8192):
                mouseButton = "rightArrow"
            else:
                mouseButton = None
            if mouseButton is not None:
                sageMouse.mouseButtonEvent(mouseButton,mouseMovement)
            
        if etype[0] == 6:
            #print "UPPPPP"
            #at this point need to send flag to another function
            #2 = right mouse button
            #4 = left mouse button
            #print "Button up ID: ", flags[0]
            mouseMovement = "up"
            

            if mouseButton is not None:
                sageMouse.mouseButtonEvent(mouseButton,mouseMovement)
                mouseButton = None
    

    
def main(sage, sage_port, tracker, tracker_port):
    testing = Array('d',[0.0,0.0])
    contactCave(testing, sage, sage_port, tracker, tracker_port)
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--tracker', default='127.0.0.1',
	help="IP address of the oMicron tracking server (default: 127.0.0.1)")
    parser.add_argument('--tracker_port', type=int, default=28000,
	help="Port of the oMicron server (default: 28000)")
    parser.add_argument('-s', '--sage', default='127.0.0.1',
	help="IP address of the SAGE head node (default: 127.0.0.1)")
    parser.add_argument('--sage_port', type=int, default=20005,
	help="Port the SAGE head node (default:20005)")
    args = parser.parse_args()
    main(args.sage, args.sage_port, args.tracker, args.tracker_port)

