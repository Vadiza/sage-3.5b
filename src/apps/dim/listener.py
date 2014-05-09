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




import traceback as tb
from SocketServer import ThreadingTCPServer, StreamRequestHandler
import socket
from globals import *



class Listener(ThreadingTCPServer):
    """ the main server listening for connections from HWCapture """
    
    allow_reuse_address = True
    request_queue_size = 50
    
    def __init__(self, port, onMsgCallback):
        self.onMsgCallback = onMsgCallback
        ThreadingTCPServer.__init__(self, ('', int(port)), SingleHWCapture)
        self.socket.settimeout(0.5)

        
    def verify_request(self, request, client_address):
        """ check whether this client already exists """
        return True


    def serve_forever(self):
        while doRun():
            try:
                self.handle_request()
            except socket.timeout:
                pass
            except KeyboardInterrupt:
                exitApp()
                break
        print "Listener closed"
        


class SingleHWCapture(StreamRequestHandler):      
    """ one of these per HWCapture client """

    
    def handle(self):
        """ this is where we parse the incoming messages
            and pass the data to the correct device objects.
        """
        self.connection.settimeout(0.5)  # so that we can quit properly
        self.deviceId = None

        #
        # SagePointer (mouse events) connect here
        #
        #print "ASKEES: NEW CONNECTION: LISTENER: 20005"

        while doRun():
            try:
                # read one line from the socket (blocking mode)
                self.rfile.flush()
                msg = self.rfile.readline().strip()
                if len(msg) == 0:
                    break

                # separate into pieces
                (deviceId, deviceType, data) = msg.split(' ', 2)
                self.deviceId = deviceId
                
                # call the function that's going to handle this further
                self.server.onMsgCallback(deviceId, deviceType, data)

            except socket.timeout:
                continue
            except:
                tb.print_exc()  
                break
                
        print "HWCapture receiver closed"
        


    def finish(self):
        """ do some cleanup here... ie remove all the
            devices connected to that machine """

        # remove the device
        getDevMgr().removeDevice(self.deviceId)
        StreamRequestHandler.finish(self)
