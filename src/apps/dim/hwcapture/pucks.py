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



####  lambdatable pucks

from managerConn import ManagerConnection
import sys, socket, os
from threading import Thread


TRACKING_PORT = 7000  # where the tracker will send UDP data to
#DEVICE_TYPE = "puck"   # a plugin for this type must exist in "devices" called puck.py

os.chdir(sys.path[0])

class Puck:
    """ describes one puck """
    
    def __init__(self, puckId, x, y, angle, puckType):
        self.puckId = puckId
        self.puckType = puckType  # not covered
        self.x = x   # normalized coords
        self.y = y   # normalized coords
        self.angle = angle  # in degrees
        self.name = "puck_"+str(puckId)

        
    def setAll(self, x, y, angle, puckType):
        """ returns True if there was a change in any data
            False otherwise
        """
        change = False
        
        if self.puckType != puckType: change=True
        self.puckType = puckType

        if self.x != x: change=True
        self.x = x

        if self.y != y: change=True
        self.y = y

        if self.angle != angle: change=True
        self.angle = angle

        return change
        
    


class CapturePucks:
    
    def __init__(self, manager):
        self.manager = manager
        self.pucks = {}  # keyed by ID, value=Puck object

        # create the tracker server for receiving puck data
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('', TRACKING_PORT))
        self.socket.settimeout(0.1)

        # some vars
        self.threadKilled = False

        # start listening for connections in a thread
        self.t = Thread(target=self.receiver, args=())
        self.t.start()


    def stop(self):
        self.threadKilled = True
        self.t.join()


    def receiver(self):
        MSG_SIZE = 60
        while not self.threadKilled:
            try:
                
                msgString = ""
                while len(msgString) < MSG_SIZE:
                    msgString += self.socket.recv(MSG_SIZE)      # receive the fixed message size

                if len(msgString) < 2:
                    self.stop()
                    break

                # record only the first reported position OR
                # record all of them based on the callback variable
                elif len(msgString) == MSG_SIZE:
                    msg = msgString.replace('\x00', '').strip()   # remove the NULLs
                    self.processPuckData(msg)                    
                    
            except socket.timeout:
                pass    # do nothing since it's just a timeout...
            except socket.error:
                #print "socket error in connection with the tracker..."
                #self.stop()
                continue
                
        self.socket.close()


    def processPuckData(self, msg):
        #print "msg = ", msg
        tokens = msg.split()
        ## puckId =   int(msg[ 11:21 ].strip())
##         x =      float(msg[ 21:31 ].strip())
##         y =      float(msg[ 31:41 ].strip())
##         angle =  float(msg[ 41:51 ].strip())
##         puckType = int(msg[ 51:61 ].strip())

        puckId =   int(tokens[1].strip())
        x =      float(tokens[2].strip())
        y =      float(tokens[3].strip())
        angle =  float(tokens[4].strip())
        puckType = int(tokens[5].strip())

        # send a message to the manager only if it's a new puck or the state of
        # an existing one changed
        if not puckId in self.pucks:
            p = Puck(puckId, x, y, angle, puckType)
            self.pucks[puckId] = p
            self.manager.sendMessage(p.name, "puck", msg)
        else:
            p = self.pucks[puckId]
            if p.setAll(x, y, angle, puckType):
                self.manager.sendMessage(p.name, "puck", msg)

        

# you can optionally pass in a port number of the manager on command line
port = 20005
if len(sys.argv) > 2:
    port = int(sys.argv[2])

# start everything off
CapturePucks( ManagerConnection(sys.argv[1], port) )
    
