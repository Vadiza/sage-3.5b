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




# works with Tactile multitouch table from EVL
# run where fsManager is running as: python tactile.py tracker_IP
#


from managerConn import ManagerConnection
import sys, socket, os, time
from threading import Thread
from math import sqrt


RECEIVING_UDP_PORT = 8041  # where the tracker will send UDP data to
TRACKER_PORT = 7340        # what port is the remote tracker running on (for initial connection)
MSG_SIZE = 99
STROKE_THRESHOLD = 0.15   # x% of the whole touchscreen display
STROKE_LIFE = 1/10.0        # max time between dots in a stroke... otherwise the stroke is finished

#DEVICE_TYPE = "tactile"   # a plugin for this type must exist in "devices" called puck.py

os.chdir(sys.path[0])



class CaptureTouch:
    
    def __init__(self, manager):
        self.manager = manager
        self.lastTimestamp = 0
        
        # keyed by strokeId, value=(x,y) tuple (strokeId is a starting dotId)
        self.currentStrokes = {}  
                                  
        # now connect to the tracker on a TCP port
        self.connectToTracker()


    def connectToTracker(self):
        self.tcpSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcpSocket.settimeout(0.1)
        self.connected = False

        print "Tactile plugin: Trying to connect to tracker on ", sys.argv[1]
        while not self.connected:
            try:
                self.tcpSocket.connect( (sys.argv[1], TRACKER_PORT) )
                self.connected = True
            except socket.error:
                time.sleep(2)
                        
        # send the init msg
        try:
            msg = "data_on,"+str(RECEIVING_UDP_PORT)
            msg = msg + int(MSG_SIZE - len(msg))*" "  # pad the message up to 99 chars
            self.tcpSocket.sendall(msg)
            
        except socket.error:
            print "\nTactile plugin: unable to send init TCP message to tracker on port %d...exiting\n" % TRACKER_PORT
            self.udpSocket.close()
            sys.exit()

        else:
            print "Tactile plugin: connected to tracker"
            # finally start the receiver for the tracker data sent via UDP
            self.startReceiver()
        
        

    def startReceiver(self):
        # create the tracker server for receiving puck data
        self.udpSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udpSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.udpSocket.bind(('', RECEIVING_UDP_PORT))
        self.udpSocket.settimeout(0.01)

        # some vars
        self.threadKilled = False

        # start listening for connections in a thread
        self.t = Thread(target=self.receiver, args=())
        self.t.start()


    def stop(self):
        self.threadKilled = True


    def receiver(self):
        while not self.threadKilled:
            try:
                msgString = ""
                while len(msgString) < MSG_SIZE:
                    msgString += self.udpSocket.recv(MSG_SIZE)      # receive the fixed message size

                if len(msgString) < 2:
                    self.stop()
                    break

                # record only the first reported position OR
                # record all of them based on the callback variable
                elif len(msgString) == MSG_SIZE:
                    msg = msgString.strip()   # remove the NULLs
                    self.processTouchData(msg)                    
                    
            except socket.timeout:
                self.cleanup()    # do nothing since it's just a timeout...
            except socket.error:
                continue  # may wanna do something here...
                
        self.udpSocket.close()



    def processTouchData(self, msg):
        timestamp, flag, rest = msg.split(":", 2)
        dotId, x, y = rest.split(",", 3)
        timestamp, dotId, x, y = int(timestamp), float(dotId), float(x), float(y)

        # for now process one finger only and don't process out-of-order data
        if self.lastTimestamp > timestamp: # or flag!="d":
            return    
        self.lastTimestamp = timestamp

        # first check whether this new dot is a part of an older stroke
        # (strokeId is just a dotId that the stroke began with)
        strokeId = self.getStrokeId(int(dotId), flag, x, y)

        # then save it as a part of it
        msg = "%s %d %f %f" % (flag, strokeId, x, y)
        self.manager.sendMessage("touch_"+str(strokeId), "tactile", msg)
            

    def cleanup(self):
        """ remove old strokes """
        for dotId, (x, y, flag, lastTime) in self.currentStrokes.items():

            # delete old strokes after sending the last message for that stroke (click_up event)
            if time.time()-lastTime > STROKE_LIFE:
                msg = "%s %d %f %f" % (flag+"last", dotId, x, y)
                self.manager.sendMessage("touch_"+str(dotId), "tactile", msg)
                del self.currentStrokes[dotId]
                continue



    def getStrokeId(self, newDotId, newFlag, newX, newY):
        def dist(x1,y1, x2,y2):
            return sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) )
        
        strokeId = newDotId
        minD = 1.0

        currentTime = time.time()
        
        # loop through all the saved strokes and check
        # whether the current dot belongs to any existing stroke
        for dotId, (x, y, flag, lastTime) in self.currentStrokes.items():
            d = dist(x,y, newX,newY)
            if d < STROKE_THRESHOLD and d < minD and flag == newFlag and \
                   currentTime-lastTime < STROKE_LIFE:
                minD = d
                strokeId = dotId

        self.currentStrokes[strokeId] = (newX,newY,newFlag,time.time())
        return strokeId
    

# you can optionally pass in a port number of the manager on command line
port = 20005
if len(sys.argv) < 1:
    print "\nTactile plugin: Must supply tracker_ip on the command line\n"
elif len(sys.argv) > 2:
    port = int(sys.argv[2])

# start everything off
CaptureTouch( ManagerConnection("localhost", port) )
    
