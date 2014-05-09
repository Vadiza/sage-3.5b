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



import socket, time
from threading import Thread
import traceback as tb


# some globals

thisMachine = socket.gethostbyname(socket.gethostname())
TIMEOUT_INTERVAL = 3    # seconds




class ManagerConnection:
    """ This should be imported by the hwcapture code and
        used to send the data to the device manager
    """
    
    def __init__(self, host, port=20005):
        #print "ASKEES: NEW CONNECTION: HWCAPTURE: 20005"
        #self.connections = {}   # key=host, value=OneConnection object
        self.conn = OneConnection(host, port, doReconnect=True)
        

##     def newConnection(self, host, port):
##         # make the initial connection to the manager we specify
        
##         conn = OneConnection(host, port)
##         if host in self.connections:
##             self.connections[host].disconnect()
##             del self.connections[host]
##         self.connections[host] = conn
##         conn.start()
        

    def sendMessage(self, deviceName, deviceType, data):
        """ - deviceName can be anything but it needs to be unique to this machine
            (for example: mouse1)
        """
        deviceId = thisMachine+str(deviceName)
        msg = deviceId + " " + deviceType + " " + data + "\n"
        self.conn.sendMessage(msg)
        






class OneConnection:
    """ This is just one connection to the device manager """
    
    def __init__(self, host, port, doReconnect):
        self.doReconnect = doReconnect
        self.host = host
        self.port = port
        self.connected = False

        # connect thread that's constantly running
        self.connThread = Thread(target=self.connectThread)
        self.connThread.start()
    

    def connectThread(self):
        while True:
            
            if not self.connected:
                # set up the socket
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(TIMEOUT_INTERVAL)

                # try connecting now
                try:
                    self.sock.connect(self.host, self.port)

                    # connection successful so start the receiving thread
                    self.connected = True
                    ## self.receiverThread = Thread(target=self.receiver)
##                     self.receiverThread.start()
                    
                except socket.error:
                    print "failed to connect to: ", (self.host, self.port)
                    if not self.doReconnect:
                        break   # one time connections should only connect once
                    time.sleep(4)  # try reconnecting every 4 seconds
                                    
            elif self.doReconnect:    # we are currently connected and we want to reconnect if connection breaks
                time.sleep(2)  # don't loop like crazy if we are connected already

            else:   # we are connected but we don't want to reconnect if connection breaks
                break
            

    def disconnect(self):
        self.connected = False
        self.sock.close()


    def sendMessage(self, msg):
        """ try to send the message. Return False if not sent, True otherwise """
        if not self.connected:  return False

        try:
            self.sock.sendall(msg)
        except socket.error:
            print "unable to send message... socket error"
            self.disconnect()
            return False
        except:
            tb.print_exc()
            return False
            
        return True


    def __sendPing(self):
        self.sendMessage(" \n")
            

   ##  def receiver(self):
##         """ in separate thread """
        
##         while self.connected:

##             try:
                
##             except socket.timeout:
##                 self.__sendPing()
##                 continue
