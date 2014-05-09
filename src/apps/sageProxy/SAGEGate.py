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
	Z Value Change                  1008
	Bring To Front                  1010
	App Properties Change           1011
	App Frame Rate Change           1012
	SAGE Shutdown                   1100
	

	SAGE ----> SAGE UI
	--------------------------
	Status				40000 
	App Info			40001 APPID L R T B SAIL-ID
	Performance			40002
	App Shutdown                    40003
	Display Info                    40004
	Z Order Change                  40005
	

	STORE INFORMATION
"""



from threading import Thread
import socket, sys, string, os.path, xmlrpclib


TIMEOUT_INTERVAL = 2
BLANK = ' '
HEADER_ITEM_LEN = 8
APP_LAUNCHER_PORT = 19010


class SAGEGate:

	def __init__(self, logFunc ):
                self.hashCallbackFunction = {}    # (AKS 2004-10-16)
		self.threadkilled = False
		self.connected = False
		self.WriteLog = logFunc  #redirect output
	
		# used for printing out informative messages (on sending and receiving)
		self.hashOutgoingMessages = {}  #RJ 2005-01-24
		self.hashOutgoingMessages[1000] = "Register UI"
		self.hashOutgoingMessages[1001] = "App Start"
		self.hashOutgoingMessages[1002] = "App Shutdown"
		self.hashOutgoingMessages[1003] = "Move"
		self.hashOutgoingMessages[1004] = "Resize"
		self.hashOutgoingMessages[1005] = "Request Performance"
		self.hashOutgoingMessages[1006] = "Stop Performance"
		self.hashOutgoingMessages[1007] = "Background Color"
		self.hashOutgoingMessages[1008] = "Z Value"
		self.hashOutgoingMessages[1010] = "Bring To Front"
		self.hashOutgoingMessages[1011] = "App Properties Change"
		self.hashOutgoingMessages[1012] = "App Frame Rate Change"
		self.hashOutgoingMessages[1015] = "Stream Request"
		self.hashOutgoingMessages[1100] = "Shutdown"

		self.hashIncomingMessages = {} #RJ 2005-01-24
		self.hashIncomingMessages[40000] = "SAGE Status Message"
		self.hashIncomingMessages[40001] = "App Info Return"
		self.hashIncomingMessages[40002] = "Performance Info"
		self.hashIncomingMessages[40003] = "App Shutdown"
		self.hashIncomingMessages[40004] = "Display Info"
		self.hashIncomingMessages[40005] = "Z Change"
		

					
	def makemsg(self,dst,code,appcode,size,data):
		msg = '%8s\0%8s\0%8s\0%s\0' %(dst,code,appcode,data)
		size = len(msg) + 9
		msg = '%8s\0%s' %(size,msg)
		return msg	


	def connectToAppLauncher(self, host=socket.gethostname(), port=APP_LAUNCHER_PORT):
		self.appLauncher = xmlrpclib.ServerProxy("http://" + host + ":" + str(port))
		try:
			self.getAppList()
		except socket.error:
			return False
		except:
			pass
		return True
		
	
	##################################################################	
	#  Connect To Sage
	##################################################################	
	def connectToSage(self,host=socket.gethostname(),port=20001):
		if self.connected == True: return 0

		## connect to the app launcher first
		if not self.connectToAppLauncher(host, APP_LAUNCHER_PORT):
			self.WriteLog("\n===> Can't connect to the appLauncher")
			return -1
		
		## create socket
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		if self.sock is None: return 0

		if host: self.sageHost = host
		if port > 0: self.sagePort = port
		## connect
		try:
			self.sock.connect((host,port))
		except socket.error:
			self.WriteLog('can\'t connect to SAGE on '+ self.sageHost + ":" + str(self.sagePort))
			return 0

                #### RJ 2005-01-21
                # without this, this thread will never exit because sock.recv is blocking and it will wait until
                # something comes in. What happens is, "while" checks if threadkilled is false, it is so it goes
                # in here and blocks until something is received. In the meantime, we want to quit so we set
                # threadkilled to True but since we are blocked by sock.recv, we wont be able to get to the "while" again
                self.sock.settimeout(TIMEOUT_INTERVAL)  
		
		##### FOR Thread
		self.t = Thread(target=self.recvWorker,args=())
		self.t.setDaemon(True)
		self.t.start()

		self.threadkilled = False
		self.connected = True

		#print 'connected to SAGE',self.sageHost,self.sagePort
		return 1 
	
	def disconnectFromSage(self):
		if self.connected == False: return 0
		
		self.threadkilled = True
		self.connected = False
		
		self.t.join()
		self.sock.close()
		del self.sock
		#print 'disconnected from SAGE',self.sageHost,self.sagePort

		return 1
	#################################################################

	def sendmsg(self,msg):
		totalcount = 0
		try:
			totalcount = self.sock.send(msg)
		except socket.error:
			#print 'socket error'
			totalcount = -1
		except Exception:
			totalcount = -1
		return totalcount


	# get the applist from the applauncher first and call the appropriate function
	def getAppList(self):
		try:
			appList = self.appLauncher.getAppList()
		except socket.error:
			return -1  #the server is not running
		else:
			self.hashCallbackFunction[ 40000 ]( appList )
			


	##################################################################	
	# Register
	##################################################################	
	# 1000 none
	# #############40000 SAGE-STATUS Format (?)
	# 40004 display info format
	# 40001 app-inst-ID left right top bottom sail-ID
	##################################################################	
	def registerSage(self):
		if self.connected == False:
			return 0
		msg = self.makemsg('',1000,'',0,'')
		status = self.sendmsg(msg)
		return status

	##################################################################	
	# Execute
	##################################################################	
	# 1001 app-name 100 100 (app-name(?)) 
	# 40001 app-inst-ID left right top bottom sail-ID
	##################################################################	
	def executeApp(self, appName, configName="default", pos=False, size=False, shareable=False, optionalArgs=""):
		if self.connected == False: return 0
		if not appName: return 0

		return self.appLauncher.startDefaultApp(appName, self.sageHost, self.sagePort+1, shareable, configName, pos, size, optionalArgs)

		


	##################################################################	
	# Shutdown
	##################################################################	
	# 1002 app-instance
	# 40003 app-Inst-ID (?)
	##################################################################	
	def shutdownApp(self,appId):
		if self.connected == False: return 0
		#if not appId: return 0

		#msg = self.makemsg('',1002,'',len(appId),appId)
		data = "" + str(appId)
		msg = self.makemsg('',1002,'',len(data),data)
		status = self.sendmsg(msg)
		return status
		
	##################################################################	
	# Move
	##################################################################	
	# 1003 app-instance dist-X,dist-Y
	##################################################################	
	def moveWindow(self,appId,distX,distY):
		if self.connected == False: return 0

		#make sure all the coordinates are ints
		distX = int(distX)
		distY = int(distY)
		
		data = str(appId) + BLANK + str(distX) + BLANK + str(distY)
		msg = self.makemsg('',1003,'',len(data),data)
		status = self.sendmsg(msg)
		return status
	
   	##############################################################
	# Resize
	# 1004 app-instance lef,right,top,bottom
	##################################################################
	def resizeWindow(self,appId,left=0,right=0,bottom=0,top=0):
		if self.connected == False: return 0
		#if not appId: return 0

		#make sure all the coordinates are ints
		left = int(left)
		right = int(right)
		bottom = int(bottom)
		top = int(top)

		data = str(appId) + BLANK + str(left) + BLANK + str(right) + BLANK + str(bottom) + BLANK + str(top)
		msg = self.makemsg('',1004,'',len(data),data)
		status = self.sendmsg(msg)
		return status

	###########################################################
	# Performance Information
	###########################################################
	# 1005 app-instance sending-rate
	# 1006 app-instance 	
	###########################################################	
	def startPerformance(self,appId,sendingrate=2):
		if self.connected == False: return 0
		data = "%d %d" % (appId, sendingrate )
		msg = self.makemsg('',1005,'',len(data),data)
		status = self.sendmsg(msg)	
		return status
	
	def stopPerformance(self,appId):
		if self.connected == False: return 0
		appId = str(appId) # convert the data to string format
		
		msg = self.makemsg('',1006,'',len(appId),appId)
		status = self.sendmsg(msg)
		return status


	####################################	
	# Background Color
	# 1007 red,green blue	
	##################################################################
	def setBgColor(self,(red,green,blue)=(1,1,1)):
		if self.connected == False: return 0

		data = str(red) + BLANK + str(green) + BLANK + str(blue)
		msg = self.makemsg('',1007,'',len(data),data)
		status = self.sendmsg(msg)
		return status


	####################################	
	# bring the application window to the top (front)
	# 1010 app-inst-ID 
	##################################################################
	def bringToFront(self, appId):
		if self.connected == False: return 0

		data = str(appId)
		msg = self.makemsg('',1010,'',len(data),data)
		status = self.sendmsg(msg)
		return status


	####################################	
	# Change App Properties
	# 1011 appId, fsmIP, fsmPort, appConfigNum	
	##################################################################
	def changeAppProperties(self,appId, newTitle, newTitleColor=(-1,-1,-1), newBorderColor=(-1,-1,-1)):
		if self.connected == False: return 0

		data = str(appId) + BLANK + str(newTitle)
		data = data + BLANK + str(newTitleColor[0]) + BLANK + str(newTitleColor[1]) + BLANK + str(newTitleColor[2])
		data = data + BLANK + str(newBorderColor[0]) + BLANK + str(newBorderColor[1]) + BLANK + str(newBorderColor[2])
		#data = data + BLANK + str(newTitle)
		msg = self.makemsg('',1011,'',len(data),data)
		status = self.sendmsg(msg)
		return status

	####################################	
	# Change App Frame Rate
	# 1012 appId, fsmIP, fsmPort, appConfigNum	
	##################################################################
	def changeAppFrameRate(self,appId, newFrameRate):
		if self.connected == False: return 0

		data = str(appId) + BLANK + str(newFrameRate)
		msg = self.makemsg('',1012,'',len(data),data)
		status = self.sendmsg(msg)
		return status


	####################################	
	# Stream Request
	# 1015 appId, fsmIP, fsmPort
	##################################################################
	def streamApp(self,appId, fsmIP, fsmPort):
		if self.connected == False: return 0

		data = str(appId) + BLANK + str(fsmIP) + BLANK + str(fsmPort)
		msg = self.makemsg('',1015,'',len(data),data)
		status = self.sendmsg(msg)
		return status
	

	####################################	
	# SAGE shutdown
	# 1100 	<none>
	##################################################################
	def shutdownSAGE(self):
		if self.connected == False: return 0
		msg = self.makemsg('',1100,'',0,'')
		status = self.sendmsg(msg)
		return status


	### informs the user when the connection breaks
	def showConnectionClosedDialog(self):
		pass

	
	##############
	## Recv Worker Thread
	##################################################################
	def recvWorker(self):
		
		while self.threadkilled == False:   #doesn't work as expected without the sock.settimeout (look below)

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
						break
					
				if self.threadkilled == True:
					#wx.CallAfter(self.showConnectionClosedDialog)
					break

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
					code = incomingMsg[ 10:18 ].strip()
					appCode = incomingMsg[ 19:27 ].strip()
					data = incomingMsg[ 28: ].strip()

					# print the message out
					if int(code) in self.hashIncomingMessages and int(code) != 40002:
						self.WriteLog("\n\tRECEIVED:  " + self.hashIncomingMessages[int(code)])
						lines = data.split('\n')
						if len(lines) < 2:
							self.WriteLog("\t\t   [" + lines[0] + "]\n\n")
						else:
							for i in range(0, len(lines)):
								if i == 0:
									self.WriteLog("\t\t   [" + lines[i])
								elif i == len(lines)-1:
									self.WriteLog("\t\t    " + lines[i] + "]\n\n")
								else:
									self.WriteLog("\t\t    " + lines[i])

				    
			except socket.timeout:
				continue
			except socket.error:
				#print 'socket error on receive'
				continue
			except:
				self.WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
				continue
		

			## SAGE Status
                        if self.threadkilled:
                            break

		        # now call the appropriate function to update the datastructure
		        if self.hashCallbackFunction.has_key( int(code) ):
			    self.hashCallbackFunction[ int(code) ]( data )

		


	# converts all non-printable characters from the buffer to white spaces
	# (so that they can be removed using string.strip() function)
        def cleanBuffer( self, stBuffer ):
            stNewBuffer = ""
            
            for ch in stBuffer:
                if ( ch in string.printable ):
                    stNewBuffer = stNewBuffer + ch
                else:
                    stNewBuffer = stNewBuffer + " "
            
            return stNewBuffer




        def registerCallbackFunction( self, msgID, function ):
            self.hashCallbackFunction[ msgID ] = function
                

