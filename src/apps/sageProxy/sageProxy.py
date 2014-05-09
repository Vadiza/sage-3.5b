#!/usr/bin/python

############################################################################
#
# Copyright (C) 2005 Electronic Visualization Laboratory,
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


from SimpleXMLRPCServer import *
from time import sleep
import socket, sys, string, os.path, SocketServer, base64, time, optparse
from sageUIDataInfo import *
from SAGEGate import *
import traceback as tb
from threading import RLock

XMLRPC_PORT = 20001 #9192
REDIRECT = True


# to output all the error messages to a file
outputLock = RLock()
def WriteLog(message):
    outputLock.acquire()
    try:
        if not REDIRECT:
            #tb.print_exc()
            print message
        else:
            f = open("sage_proxy.log", "a")
            #tb.print_exc(file=f)
            f.write(message+"\n")
            f.close()
    except:
        pass
    outputLock.release()





# a mix-in class for adding the threading support to the XMLRPC server
class ThreadedXMLRPCServer(SocketServer.ThreadingMixIn, SimpleXMLRPCServer):
    allow_reuse_address = True


class Proxy:
    def __init__(self, host=socket.gethostname(), port=20001, passwd=""):
        self.sageData = sageUIDataInfo()   # a datastructure that stores the display and app state
        
        self.sageGate = SAGEGate(WriteLog)         # communication channel with sage
        self.sageGate.registerCallbackFunction( 40000, self.sageData.setSageStatus )
        self.sageGate.registerCallbackFunction( 40001, self.sageData.setSageAppInfo )
        self.sageGate.registerCallbackFunction( 40003, self.sageData.sageAppShutDown )
        self.sageGate.registerCallbackFunction( 40004, self.sageData.setSageDisplayInformation )
        self.sageGate.registerCallbackFunction( 40005, self.sageData.setSageZValue )

        # now connect to sage and register with it
        for i in range(20):  # try to connect to SAGE for X seconds
            WriteLog("Trying to connecto to SAGE on: "+host+":"+str(port))
            if self.sageGate.connectToSage(host, port) != 0:
                self.sageGate.registerSage()
                break
            time.sleep(1)
        else:  # we didn't manage to connect to sage in X seconds... so quit
            sys.exit(0)

	self.sessionPassword = passwd


    def getDisplayInfo(self):
        """ Returns: a list (integers): [totalNumTiles, numTilesX, numTilesY, totalWidth,
                                         totalHeight, tileWidth, tileHeight]
            Returns: -1 if failed for whatever reason
        """
        try:
            return self.sageData.getSageDisplayInformation()
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def getAppList(self):
        """ Returns: a list (strings) of app names available for running
            Returns: -1 if failed for whatever reason
        """
        try:
            return self.sageData.getAvailableAppNames()
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def getZValues(self):
        """ Returns: a list (integers) of z values [numChanges, appId, zValue, appId, zValue ....]
            Returns: -1 if failed for whatever reason
        """
        try:
            return self.sageData.getZvalues()
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def getAppStatus(self, appId=-1):
        """ If called without parameters it will return the status for all currently running applications.
            If called with appId parameter it will return the status just for that application.
            Returns: a hash where the key is appId (string) and the value is a list of app status:
                    [string appName, int appId, int left, int right, int bottom,
                     int top, int sailID, int zValue, int configNum, string title]
            Returns: -1 if failed for whatever reason
             
        """
        try:
            if appId == -1:
                return self.sageData.getAllAppInfo()
            else:
                return self.sageData.getAppInfo(appId)
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def getAllAppID(self):
	""" Returns all the app ID currently running on SAGE
        """
	try:
	    return self.sageData.getAllAppIDs()
	except:
	    WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
	    return -1


    def getNewAppID(self):
	try:
	   return self.sageData.getNewAppID()
	except:
	   WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
           return -1


    def bringToFront(self, appId):
        """ Brings the application window to front (on top of the other ones).
            Returns: 1 if everythin went OK and the message was sent
            Returns: -1 if the message was not sent for whatever reason (including
                     if the app was already on top)
        """
        try:
                if self.sageData.appExists(appId) and \
                   (not self.sageData.isTopWindow(appId)) and \
                   self.sageGate.bringToFront(appId):
                    return 1
                else:
                    return -1
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1
        

    def moveWindow(self, appId, x, y):
        """ Moves a window to the specified location (absolute position) in tiled display coordinates.
            The x,y is interpreted as the lower left corner of the application.
            All parameters are integers.
            Returns: 1 if successful.
            Returns: -1 if failed for whatever reason.
        """
        try:
            app = self.sageData.getSAGEApp(appId)
            left = x
            right = x+app.getWidth()
            bottom = y
            top = y+app.getHeight()
            if self.sageGate.resizeWindow(appId, left, right, bottom, top) == -1:
                return -1
            return 1
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def resizeWindow(self, appId, left, right, bottom, top):
        """ Resizes a window to the specified size in tiled display coordinates.
            The lower left corner of the display is considered 0,0.
            All parameters are integers.
            Returns: 1 if successful.
            Returns: -1 if failed for whatever reason.
        """
        try:
            
            # prevent 0 or negative size
            if (right-left < 1) or (top-bottom < 1):
                app = self.sageData.getSAGEApp(appId)
                ar = app.getWidth() / float(app.getHeight())
                if ar > 1:
                    newH = 300 # min app size
                    newW = newH*ar
                else:
                    newW = 300 # min app size
                    newH = newW/ar
                
                right = left+newW
                top = bottom+newH

            if self.sageGate.resizeWindow(appId, left, right, bottom, top) == -1:
                return -1
            return 1
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def executeApp(self, appName, configName="default", pos=False, size=False, shareable=False, optionalArgs=""):
        """ Starts a new application with appName (string) and configNum (int). You can also optionally specify
            initial position and size of the application window that you pass in as a tuple of integers.
            If the application itself requires some command line arguments during startup, those can be
            specified as well with the optionalArgs parameter (string).
            Shareable parameter is used when you want to run the application through sageBridge which
            means that it can be shared among other displays. If False it will run the app locally.
            Returns: the new status of all the apps in the same format as getAppStatus
                    (sleeps 2 seconds before returning it to ensure that the datastructure was updated
                     by the messages from SAGE about the new app)
            Returns: -1 if failed for whatever reason
        """
        try:
            if self.sageGate.executeApp(appName, configName, pos, size, shareable, optionalArgs) == -1:
                return -1
            sleep(2)        #wait for the message to arrive from sage and then return the status
            return self.sageData.getAllAppInfo()
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1


    def closeApp(self, appId):
        """ Closes the app corresponding to the specified appId.
            Returns: the new status of all the apps in the same format as getAppStatus
                    (sleeps 2 seconds before returning it to ensure that the datastructure was updated
                     by the messages from SAGE about the closed app)
            Returns: -1 if failed for whatever reason
        """
        try:
            if not self.sageData.appExists(appId):
                return -1
            
            if self.sageGate.shutdownApp(appId) == -1:
                return -1
            sleep(2)        #wait for the message to arrive from sage and then return the status
            return self.sageData.getAllAppInfo()
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1
        

    def shareDesktop(self, sz, displayNum, ip, passwd, shareable=False):
        #oa = ip +" "+ str(displayNum) +" "+ str(sz[0]) +" "+ str(sz[1]) +" "+ passwd
        oa = ip +" "+ str(displayNum) +" "+ passwd
        return self.executeApp("VNCViewer", "default", False, sz, shareable, oa)


    def shareApp(self, appId, fsIP, fsPort):
        """ Sends a request to fsManager to share the specified application to the specified display
            Parameters: - appId of the application you want to share
                        - ip address and port number of the machine to stream to
            Returns: 1 if succeeded
            Returns: -1 if failed for whatever reason
        """
        try:
            if self.sageGate.streamApp(appId, fsIP, fsPort) == -1:
                return -1
            else:
                return 1
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return -1

        
    def authenticate(self, passwd):
	if (self.sessionPassword == passwd):
	   return 1
	else:
	   return -1	




### sets up the parser for the command line options
def get_commandline_options():
    parser = optparse.OptionParser()

    h = "if set, prints output to console, otherwise to sage_proxy.log"
    parser.add_option("-v", "--verbose", action="store_true", help=h, dest="verbose", default=False)

    h = "which machine is sage running on (default is localhost)"
    parser.add_option("-s", "--server", dest="server", help=h, default="localhost")

    h = "the port number where sage is accepting ui connections (default is 20001)"
    parser.add_option("-p", "--port", help=h, type="int", dest="port", default=20001)

    h = "specify the password for SAGE Web UI access (default is empty)"
    parser.add_option("-x", "--pass", dest="passwd", help=h, default="")

    return parser.parse_args()



def main(argv):
    global REDIRECT
    
    (options, args) = get_commandline_options()
    REDIRECT = not options.verbose
    port = options.port
    host = options.server
    passwd = options.passwd
    
    p = Proxy(host, port, passwd)

    # always use the fsManager ui port + 3 for the xmlrpc port
    global XMLRPC_PORT
    XMLRPC_PORT = port+3
    
    server = ThreadedXMLRPCServer(("", XMLRPC_PORT), SimpleXMLRPCRequestHandler, False)
    server.allow_reuse_address = True
    server.register_instance(p)
    server.register_introspection_functions()
    server.serve_forever()
	


if __name__ == '__main__':
    import sys, os
    main(['', os.path.basename(sys.argv[0])] + sys.argv[1:])
