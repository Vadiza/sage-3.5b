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



import sys, socket, wx
sys.path.append("../dim/hwcapture")
from managerConn import ManagerConnection
from threading import Timer
from threading import Thread
import time

TRACKING_IP = "" #"67.58.41.21"
TRACKING_PORT = 7105         # what port is the remote tracker running on (for initial connection)
MSG_SIZE = 99               # message padding


SHOW_WINDOW = True




class Xtion:

    def __init__(self, xtionId, x, y):

        self.xtionId = xtionId	# gesture ID
        self.x = x   # normalized coords
        self.y = y   # normalized coords

    def setAll(self, x, y):
        """ returns True if there was a change in any data
            False otherwise
        """
        change = False
        
        if self.x != x: change=True
        self.x = x

        if self.y != y: change=True
        self.y = y

        return change



class CaptureXtion:

    def __init__(self, manager):
        self.manager = manager
	self.xtion = {}  # keyed by ID, value=Xtion object
          


        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('', TRACKING_PORT)) # '' = local. We are the host
     	self.socket.listen(1)
	self.conn, addr = self.socket.accept()
	print 'Connected by', addr
	#while 1:
	#    test_data = self.conn.recv(MSG_SIZE)
	 #   print repr(test_data)

  # self.socket.settimeout(0.1)

        # some vars
        self.threadKilled = False

        # start listening for connections in a thread
        self.t = Thread(target=self.receiver, args=())
        self.t.start()
		

    def stop(self):
        self.threadKilled = True


    def receiver(self):
	print "receiver"
        while not self.threadKilled:
            try:
		msgString = ""
		msgString = self.conn.recv(MSG_SIZE)
                msg = msgString.strip()   # remove the NULLs
                self.processXtionData(msg)  
 #               while len(msgString) < MSG_SIZE:
#		    #print "while loop"
 #                   msgString += self.conn.recv(MSG_SIZE)      # receive the fixed message size
		    #print msgString + "\n"		
	
                if len(msgString) < 2:
                    self.stop()
                    break

                # record only the first reported position OR
                # record all of them based on the callback variable
#                elif len(msgString) >= MSG_SIZE:
#		    print msgString + "\n"
#                    msg = msgString.strip()   # remove the NULLs
#		    print msg + "\n"
#                    self.processXtionData(msg)                    
                    
            except socket.timeout:
                pass    # do nothing since it's just a timeout...
            except socket.error:
                continue  # may wanna do something here...
                
        self.socket.close()



    def processXtionData(self, msg):
	#print "Processing data"	
	# print "msg= ", msg
#	print str(msg)
        tokens = msg.split()
	#print "xtionId =",repr(tokens[0])
	#print "x =",repr(tokens[1])
	#print "y =",repr(tokens[2])
        xtionId =   int(tokens[0].strip())
        x =      float(tokens[1].strip())
        y =      float(tokens[2].strip())


        # send a message to the manager only if it's a new gesture or the state of
        # an existing one changed
        if not xtionId in self.xtion:     
            z = Xtion(xtionId, x, y)
            self.xtion[xtionId] = z
#            self.manager.sendMessage("xtion_"+str(xtionId), "xtionTest", msg)
	    self.manager.sendMessage("xtion_1", "xtionTest", msg)
        else:
            z = self.xtion[xtionId]
            if z.setAll(x, y):
#                self.manager.sendMessage("xtion_"+str(xtionId), "xtionTest", msg)
		self.manager.sendMessage("xtion_1", "xtionTest", msg)


##### you can optionally pass in a port number of the manager on command line
port = 20005

if len(sys.argv) > 3:
    port = int(sys.argv[3])

##### start everything off
CaptureXtion( ManagerConnection(sys.argv[1], port) )

