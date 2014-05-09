############################################################################
#
# SAGE UI - A Graphical User Interface for SAGE
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




"""
	SAGE UI --> SAGE
	---------------------------	
	Register 			1000
	Execute				1001
	Shutdown			1002
	Move				1003
	Resize				1004
	Start Performance		1005
	Stop Performance		1006
	BackColor			1007
	Bring to Front                  1010
	App Properties Change           1011
	App Frame Rate Change           1012
	Stream Request                  1014
        Rotate Window                   1018
	SAGE Shutdown                   1100
	Add Object                      1200
        Move Object                     1201
        Remove Object                   1202
        Object Message                  1203
        Show Object                     1204
        Hide Object                     1205
        

	SAGE ----> SAGE UI
	--------------------------
	Status				40000 
	App Info			40001 APPID L R T B SAIL-ID
	Performance			40002
	App Shutdown                    40003
	Display Info                    40004
	Z Order Change                  40005
	App Exec Config                 40006
        Display Connections             40007
        Overlay Object Info             40018
	

	STORE INFORMATION
"""


from threading import Thread, RLock
import socket, sys, string, os.path, xmlrpclib, time
import traceback as tb
from globals import *


### GLOBALS ###

TIMEOUT_INTERVAL = 0.5
BLANK = ' '
HEADER_ITEM_LEN = 8
APP_LAUNCHER_PORT = 19010  #the default port, usually retrieved from the sage server though
SAGE_SERVER_PORT = 8009  #the xmlrpc port of the sage server



class AppLauncher:

    def __init__(self, launcherId, name, ip, port, appList):
        self.port = port
        self.appList = appList
        self.ip = ip
        self.launcherId = launcherId
        self.name = name
	self.connected = False


    def connect(self):
	if not self.connected:
	    socket.setdefaulttimeout(3)  #set the timeout to 3 seconds so that we dont wait forever
	    self.server = xmlrpclib.ServerProxy("http://" + self.ip + ":" + str(self.port))
	    try:
		self.server.test() #just use this as a way of testing whether the server is running or not
		self.connected = True
	    except socket.error:
		return False
	    except:
		tb.print_exc()
		return False
	return True


    def getId(self):
        return self.launcherId

    def getIP(self):
        return self.ip

    def getAppList(self):
        return self.appList

    def setAppList(self, appList):
        self.appList = appList

    def getPort(self):
        return self.port

    def getName(self):
        return self.name
    
    def getServerHandle(self):
        self.connect()
        return self.server






####################################################################
#
# DESCRIPTION: This is the base class for communication with SAGE.
#              You need to inherit from this class and override
#              the "onMessage" method. You can use the registerCallbackFunction
#              and the hashCallbackFunction to store your callbacks
#              and then retrieve them from your overridden onMessage.
#
# DATE: Aug, 2007
#
####################################################################

class SageGateBase:
	
    def __init__(self, sageServerHost="sage.sl.startap.net", useAppLauncher=False, forceAppLauncher=None, onDisconnect=None, verbose=False):
	self.hashCallbackFunction = {}
	self.threadkilled = False
	self.connected = False
	self.sageHost = None
	self.sagePort = 20001
        self.forceAppLauncher = forceAppLauncher         # use this appLauncher if specified
        self.useAppLauncher = useAppLauncher   # should we use the appLauncher at all?
        self.onDisconnect = onDisconnect       # call this function if disconnected
        self.verbose = verbose                 # print the output?
        self.sageServerHost = sageServerHost           # where the sage server is running
        self.launchers={}                      # a hash of appLaunchers currently running

	# used for printing out informative messages (on sending and receiving)
	self.hashOutgoingMessages = {}  
	self.hashOutgoingMessages[1000] = "Register UI"
	self.hashOutgoingMessages[1001] = "App Start"
	self.hashOutgoingMessages[1002] = "App Shutdown"
	self.hashOutgoingMessages[1003] = "Move"
	self.hashOutgoingMessages[1004] = "Resize"
	self.hashOutgoingMessages[1005] = "Request Performance"
	self.hashOutgoingMessages[1006] = "Stop Performance"
	self.hashOutgoingMessages[1007] = "Background Color"
	self.hashOutgoingMessages[1010] = "Bring To Front"
	self.hashOutgoingMessages[1011] = "App Properties Change"
	self.hashOutgoingMessages[1012] = "App Frame Rate Change"
	self.hashOutgoingMessages[1014] = "Stream Request"
        self.hashOutgoingMessages[1018] = "Rotate Window"
        self.hashOutgoingMessages[1200] = "Add Object"
        self.hashOutgoingMessages[1201] = "Move Object"
        self.hashOutgoingMessages[1202] = "Remove Object"
        self.hashOutgoingMessages[1203] = "Object Message"
	self.hashOutgoingMessages[1100] = "Shutdown"

	self.hashIncomingMessages = {} 
	self.hashIncomingMessages[40000] = "SAGE Status Message"
	self.hashIncomingMessages[40001] = "App Info Return"
	self.hashIncomingMessages[40002] = "Performance Info"
	self.hashIncomingMessages[40003] = "App Shutdown"
	self.hashIncomingMessages[40004] = "Display Info"
	self.hashIncomingMessages[40005] = "Z Change"
	self.hashIncomingMessages[40006] = "App Exec Config"
        self.hashIncomingMessages[40007] = "Display Connections"
        self.hashIncomingMessages[40018] = "Overlay Object Info"



    def makemsg(self,dst,code,appcode,size,data):
        # assemble the message into a string
	msg = '%8s\0%8s\0%8s\0%s\0' %(dst,code,appcode,data)
	size = len(msg) + 9
	msg = '%8s\0%s' %(size,msg)

        # print the output if requested
        if self.verbose and int(code) < 1200:  # dont print the draw object messages cause there are many of them
            print "\n\tSEND:  " + self.hashOutgoingMessages[int(code)]
            print "\t       [" + data + "]\n\n"
            
	return msg	


    def connectToAppLauncher(self, host=socket.gethostname()):
	socket.setdefaulttimeout(3)  #set the timeout to 3 seconds so that we dont wait forever
        if self.forceAppLauncher:    # overriding with the one from the command line
            self.appLauncher = xmlrpclib.ServerProxy("http://"+self.forceAppLauncher)
        else:                         # try to find the appropriate app launcher
            self.appLauncher = self.__getMyAppLauncher(host)
            if type(self.appLauncher) is type(None):  # in case we couldn't find one, just assume it's running
                self.appLauncher = xmlrpclib.ServerProxy("http://" + host + ":" + str(APP_LAUNCHER_PORT))

        # now test the connection
	try:
	    self.appLauncher.listMethods() #just use this as a way of testing whether the server is running or not
	except socket.error:
	    return False
	except:
	    pass
	return True


    def getPort(self):
	return self.sagePort

    def getHost(self):
	return self.sageHost


    ##################################################################	
    #  Connect To Sage
    ##################################################################	
    def connectToSage(self,host=socket.gethostname(),port=20001):
	''' returns 1 if succeeded, 0 if no connection to SAGE and -1 if no connection to appLauncher'''
	if self.connected == True: return 0

	## create socket
	self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	if self.sock is None: return 0


        #### RJ 2005-01-21
	# without this, this thread will never exit because sock.recv is blocking and it will wait until
	# something comes in. What happens is, "while" checks if threadkilled is false, it is so it goes
	# in here and blocks until something is received. In the meantime, we want to quit so we set
	# threadkilled to True but since we are blocked by sock.recv, we wont be able to get to the "while" again
	self.sock.settimeout(TIMEOUT_INTERVAL)
        

	if host: self.sageHost = host
	if port > 0: self.sagePort = port
	## connect
	try:
	    self.sock.connect((host,port))
	except socket.error:
	    print 'can\'t connect to SAGE', self.sageHost,self.sagePort	
	    return 0

	##### FOR Thread
	self.threadkilled = False
        self.senderLock = RLock()
	self.t = Thread(target=self.receiverThread,args=())
	self.t.start()
	self.connected = True

        # start the overlay sender thread
        self.overlayMsgLock = RLock()
        self.overlayMsgQueue = []   # combined overlay messages (list of string messages)
        self.overlayMsgFreq = 50    # send messages roughly 50 times a sec
        self.overlaySender = Thread(target=self.overlaySenderThread)
        self.overlaySenderKilled = False
        self.overlaySender.start()

	# now connecting to the appLauncher running on the
        # same machine as SAGE we are connecting to
	if self.useAppLauncher and not self.connectToAppLauncher(host):
	    print "\n===> Can't connect to the appLauncher"
	    return -1
	else:
	    print 'connected to SAGE',self.sageHost,self.sagePort
	    return 1



        ### attempts to find the appLauncher that's running on the same machine as myHost
        ### (by comparing IP addresses of the sage machine and reported appLaunchers)
    def __getMyAppLauncher(self, myHost):
        self.updateLauncherList()  #get a new fresh list of appLaunchers
        
	def sameByIP(first, second):
	    try:
		first = socket.gethostbyname(first)[0]
		second = socket.gethostbyname(second)[0]
                if first == second:
                    return True
                else:
                    return False
            except:
                return False

        # loop through all the known launchers and try to find the one that matches myHost
	for launcher in self.launchers.itervalues():
	    try:
		if myHost == launcher.getIP():
		    return launcher.getServerHandle()  #returns the server object that we can call functions on
                elif sameByIP(myHost, launcher.getIP):
                    return launcher.getServerHandle()
                else:
                    return None
            except:
                return None
                
    
    
    ### connects to the sage server and retrieves the list of all app launchers running
    def updateLauncherList(self):
	sageServer = xmlrpclib.ServerProxy("http://"+self.sageServerHost+":"+str(SAGE_SERVER_PORT))
	try:
	    # a hash comes back (key=launcherId, value=appList - that's another hash of appNames and configs)
	    launcherHash = sageServer.GetRegisteredLaunchers()
	except socket.error:
	    print "no connection to the sage server... can't get the list of appLaunchers"
	except:
	    tb.print_exc()
	else:
            self.launchers={}
	    for launcherString, appList in launcherHash.iteritems():
		(name, launcherId) = launcherString.split(":",1)
		(ip, port) = launcherId.split(":",1)
		self.launchers[launcherId] = AppLauncher(launcherId, name, ip, port, appList)
		
	return self.launchers


    ### returns a hash of all the appLaunchers currently running
    def getLaunchers(self):
	return self.updateLauncherList()
    

    # get the applist from the applauncher first and call the appropriate function
    def getAppList(self):
        appList = {}
	try:
	    appList = self.appLauncher.getAppList()
            self.hashCallbackFunction[ 40000 ]( appList )
	except socket.error:
	    self.hashCallbackFunction[ 40000 ]( appList ) #return appList  #the server is not running
	except:
	    tb.print_exc()
            return False


    def isConnected(self):
        return self.connected
	

    def disconnectFromSage(self, isSocketError=False):
        """ isSocketError should be True when we didn't close
            the connection intentionally but rather the connection
            broke for some reason. In that case the onDisconnect
            callback is called.
        """
	if self.connected == False: return 0

	self.threadkilled = True
        self.overlaySenderKilled = True
	self.connected = False		
	#self.t.join()
	self.sock.close()
	del self.sock
	print 'disconnected from SAGE',self.sageHost,self.sagePort

        if isSocketError and self.onDisconnect:
            self.onDisconnect()
        
	return 1


    def sendmsg(self, data, code, sailId=''):
        if not self.connected:
            return 0

        self.senderLock.acquire()

        msg = self.makemsg(sailId,code,'',len(data),data)
	totalcount = 0
	try:
	    totalcount = self.sock.send(msg)
	except socket.error:
	    print 'SageGateBase: socket error on send'
	    totalcount = 0
            self.disconnectFromSage( isSocketError=True )
	except Exception:
            tb.print_exc()
	    totalcount = 0

        self.senderLock.release()
	return totalcount


    ##################################################################	
    # Register
    ##################################################################	
    # 1000 none
    # 40004 display info format
    ##################################################################	
    def registerSage(self):
 	if not self.connected:
	    return 0
 	return self.sendmsg('', 1000)


    ##################################################################	
    # Execute
    ##################################################################	
    # 1001 app-name 100 100 (app-name(?)) 
    # 40001 app-inst-ID left right top bottom sail-ID
    ##################################################################	
    def executeApp(self, appName, configName="default", pos=False, size=False, optionalArgs="", useBridge=False, sageIP=None, sagePort=None):
	if self.connected == False: return 0
	if not appName: return 0

        if not sageIP:  sageIP = self.sageHost
        if not sagePort: sagePort = self.sagePort+1
        
        # try running the app (return -1 if failed for whatever reason)
        try:
            res = self.appLauncher.startDefaultApp(appName, sageIP, sagePort, useBridge, configName, pos, size, optionalArgs)
        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            return -1
        else:
            return res


    def executeRemoteApp(self, launcherId, appName, configName="default", pos=False, size=False, optionalArgs="", useBridge=False, sageIP=None, sagePort=None):
	if self.connected == False: return 0
	if not appName: return 0

        if not sageIP:  sageIP = self.sageHost
        if not sagePort: sagePort = self.sagePort+1

        # try running the app (return -1 if failed for whatever reason)
	if launcherId in self.launchers:
            server = self.launchers[launcherId].getServerHandle()
            try:
                res = server.startDefaultApp(appName, sageIP, sagePort, useBridge, configName, pos, size, optionalArgs)
            except:
                print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
                return -1
            else:
                return res
	else:
	    print "Launcher not found: ", launcherId
	    return -1


	
    ##################################################################	
    # Shutdown
    ##################################################################	
    # 1002 app-instance
    # 40003 app-Inst-ID (?)
    ##################################################################	
    def shutdownApp(self,windowId):
	if self.connected == False: return 0

	data = str(windowId)
	return self.sendmsg(data, 1002)


    ##################################################################	
    # Forceful Shutdown
    ##################################################################	
    def forceShutdownApp(self, portNum):
	if self.connected == False: return 0

	# the portNum is basically the windowId in the appLauncher context
	return self.appLauncher.stopApp(portNum)   


    ##################################################################	
    # Move
    ##################################################################	
    # 1003 app-instance dist-X,dist-Y
    ##################################################################	
    def moveWindow(self,windowId,distX,distY):
	if self.connected == False: return 0

	#make sure all the coordinates are ints
	distX = int(distX)
	distY = int(distY)

	data = str(windowId) + BLANK + str(distX) + BLANK + str(distY)
	return self.sendmsg(data, 1003)
	

    ##############################################################
    # Resize
    # 1004 app-instance lef,right,top,bottom
    ##################################################################
    def resizeWindow(self,windowId,left=0,right=0,bottom=0,top=0):
	if self.connected == False: return 0
	#if not windowId: return 0

	#make sure all the coordinates are ints
	left = int(left)
	right = int(right)
	bottom = int(bottom)
	top = int(top)

	data = str(windowId) + BLANK + str(left) + BLANK + str(right) + BLANK + str(bottom) + BLANK + str(top)
	return self.sendmsg(data, 1004)
	

    ###########################################################
    # Performance Information
    ###########################################################
    # 1005 app-instance sending-rate
    # 1006 app-instance 	
    ###########################################################	
    def startPerformance(self,windowId,sendingrate=2):
	if self.connected == False: return 0

	data = "%d %d" % (windowId, sendingrate )
	return self.sendmsg(data, 1005)	
	

    def stopPerformance(self,windowId):
	if self.connected == False: return 0

	data = str(windowId) # convert the data to string format
	return self.sendmsg(data, 1006)
	


    ####################################	
    # Background Color
    # 1007 red,green blue	
    ##################################################################
    def setBgColor(self,(red,green,blue)=(1,1,1)):
	if self.connected == False: return 0

	data = str(red) + BLANK + str(green) + BLANK + str(blue)
	return self.sendmsg(data, 1007)
	

    ####################################	
    # bring the application window to the top (front)
    # 1010 app-inst-ID 
    ##################################################################
    def bringToFront(self, windowId):
	if self.connected == False: return 0

	data = str(windowId)
	return self.sendmsg(data, 1010)
	


    ####################################	
    # Change App Properties
    # 1011 windowId, fsmIP, fsmPort, appConfigNum	
    ##################################################################
    def changeAppProperties(self,windowId, newTitle, newTitleColor=(-1,-1,-1), newBorderColor=(-1,-1,-1)):
	if self.connected == False: return 0

	data = str(windowId) + BLANK + str(newTitle)
	data = data + BLANK + str(newTitleColor[0]) + BLANK + str(newTitleColor[1]) + BLANK + str(newTitleColor[2])
	data = data + BLANK + str(newBorderColor[0]) + BLANK + str(newBorderColor[1]) + BLANK + str(newBorderColor[2])
	return self.sendmsg(data, 1011)
	

    ####################################	
    # Change App Frame Rate
    # 1010 windowId, fsmIP, fsmPort, appConfigNum	
    ##################################################################
    def changeAppFrameRate(self,windowId, newFrameRate):
	if self.connected == False: return 0

	data = str(windowId) + BLANK + str(newFrameRate)
	return self.sendmsg(data, 1012)
	


    ####################################	
    # Stream Request
    # 1014 windowId, fsmIP, fsmPort
    ##################################################################
    def streamApp(self,windowId, fsmIP, fsmPort):
	if self.connected == False: return 0

	data = str(windowId) + BLANK + str(fsmIP) + BLANK + str(fsmPort)
	return self.sendmsg(data, 1014)
	


    ####################################	
    # Rotate Window
    # 1018 windowId, degree
    ##################################################################
    def rotateWindow(self, windowId, degree):
	if self.connected == False: return 0

	data = str(windowId) + BLANK + str(degree)
	return self.sendmsg(data, 1018)



    ####################################	
    # Overlay Messages
    # 1200 - 1205
    ##################################################################
    def addOverlay(self, overlayType, x, y, w, h, isGlobal, drawOrder):
        data = '%s %s %s %s %s %s %s' % (overlayType, x, y, w, h, int(isGlobal), drawOrder)
        return self.sendmsg(data, 1200)


    def moveOverlay(self, id, dx, dy):
        """ relative movement """
        data = '%s %s %s' % (id, dx, dy)
        return self.sendmsg(data, 1201)
    

    def showOverlay(self, id, doShow):
        data = '%s %s' % (id, str(int(doShow)))
        return self.sendmsg(data, 1204)


    def sendOverlayMessage(self, id, *data):
        """ this actually puts the messages in a queue
            which are then sent at fixed intervals """

        # first assemble the data into a string
        msg = '%s' % (id)
        for d in data:   
            msg += " "+str(d)

        self.overlayMsgLock.acquire()
        self.overlayMsgQueue.append(msg)
        self.overlayMsgLock.release()


    def __sendMultipleOverlayMessages(self, msg):  # a bunch of messages combined into one
        return self.sendmsg(msg, 1203)


    def removeOverlay(self, id):
        data = '%s' % (id)
        return self.sendmsg(data, 1202)
        

    ####################################	
    # SAGE App events
    # 31000 - 31007
    ##################################################################

    def sendAppEvent(self, eventId, sailId, *data):
        # first assemble the data into a string
        msg = ''
        for d in data:   
            msg += " "+str(d)
        self.sendmsg(msg, 31000+eventId, sailId)


    ####################################	
    # SAGE shutdown
    # 1100 	<none>
    ##################################################################
    def shutdownSAGE(self):
	if self.connected == False: return 0
	return self.sendmsg('', 1100)



    ##############
    #  Overlay Sender Thread
    #    - Sends combined overlay messages at fixed intervals
    ##################################################################
    def overlaySenderThread(self):
        while not self.overlaySenderKilled and doRun():
            self.overlayMsgLock.acquire()

            # iterate through the msg queue and assemble the messages into a string
            msg = ""
            for m in self.overlayMsgQueue:
                msg += m + "\n"            # separate messages with \n
            self.overlayMsgQueue = []      # clear the queue
            self.overlayMsgLock.release()

            # send the message if there is something to send
            msg = msg.strip()
            if msg != "":
                self.__sendMultipleOverlayMessages(msg)
                
            # sleep for a certain time
            time.sleep(1.0/self.overlayMsgFreq)

        print "Overlay message sender thread closed"
	

    ##############
    #  Receiver Thread
    #    - receives messages from SAGE in a thread
    ##################################################################
    def receiverThread(self):

	while self.threadkilled == False and doRun():   #doesn't work as expected without the sock.settimeout (look below)

	    #############################
	    try:

		code = ""
		incomingMsg = ""
		msgSize = ""

		# first make sure you read the whole 8 bytes for the size
		while len(msgSize) < HEADER_ITEM_LEN:
		    msgSize = self.sock.recv(HEADER_ITEM_LEN)
		    if len( msgSize ) == 0:
			self.threadkilled = True
                        self.overlaySenderKilled = True
                        self.disconnectFromSage( isSocketError=True )
			break

                if self.threadkilled: break
                
		# this is the number of bytes that the total message contains
		msgSize = msgSize.replace('\x00', '')
		sizeLeft = int(msgSize) - HEADER_ITEM_LEN    # we already read the size so read the rest of the bytes

		# read the rest of the message
		while len( incomingMsg ) < sizeLeft:
		    incomingMsg = incomingMsg + self.sock.recv( sizeLeft - len(incomingMsg) )

		# extract the tokens from the message
		if len( incomingMsg ) > 0:
		    incomingMsg = incomingMsg.replace('\x00', ' ')
		    dst = incomingMsg[ 1:9 ].strip()
		    code = int(incomingMsg[ 10:18 ].strip())
		    appCode = incomingMsg[ 19:27 ].strip()
		    data = incomingMsg[ 28: ].strip()

		    # print the message out (except performance info since there are many of them)
                    if self.verbose and code in self.hashIncomingMessages and code != 40002:
                        print "\n\tRECEIVED:  " + self.hashIncomingMessages[code]
                        lines = data.split('\n')
                        if len(lines) < 2:
                            print "\t\t   [" + lines[0] + "]\n\n"
                        else:
                            for i in range(0, len(lines)):
                                if i == 0:
                                    print "\t\t   [" + lines[i]
                                elif i == len(lines)-1:
                                    print "\t\t    " + lines[i] + "]\n\n"
                                else:
                                    print "\t\t    " + lines[i]


	    except socket.timeout:
		continue
	    except socket.error:
		#print 'SageGateBase: socket error on receive'
                #self.disconnectFromSage( isSocketError=True )
		continue
	    #except:
	    #	print 'exception: ', sys.exc_info()[0], sys.exc_info()[1]
	    #	break
	    ############################

	    if self.threadkilled:
		break

	    # finally, do something with this message (ie call the subclass' message handler)
            self.onMessage( code, data )

        print "SageGate receiver thread closed"
        

    def cleanBuffer( self, stBuffer ):
        """ converts all non-printable characters from the buffer to white spaces
            (so that they can be removed using string.strip() function)
        """
	stNewBuffer = ""

	for ch in stBuffer:
	    if ( ch in string.printable ):
		stNewBuffer = stNewBuffer + ch
	    else:
		stNewBuffer = stNewBuffer + " "

	return stNewBuffer



    def onMessage(self, code, data):
        """ this is the function that gets called after a message arrives successfully
            it must be overridden by the subclass
        """
        raise NotImplementedError


    def registerCallbackFunction( self, msgID, function ):
	self.hashCallbackFunction[ msgID ] = function





        


