#!/usr/bin/env python

############################################################################
#
# SAGE UI Users Server - A server that handles users, fsManagers and chat for SAGE
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


from threading import *
import string, socket, os, os.path, sys, time, SimpleXMLRPCServer, xmlrpclib


# some global constants

CHUNK_SIZE = 2048          # we use fixed size messages... in bytes
SOCKET_TIMEOUT = 1         # in seconds
MSGLEN = CHUNK_SIZE
SEPARATOR = '\0'         # null character for separation
NEW_STYLE_VER = "2.82"   # below this UIs don't support system IP/port for machines

USER_SERVER_PORT = 15558  # port for the server to listen for User connections
SAGE_SERVER_PORT = 15557   # port for the server to listen for SAGE connections
MAX_USER_CONNECTIONS = 100  # maximum number of simultaneous User connections
MAX_SAGE_CONNECTIONS = 50  # maximum number of simultaneous SAGE connections

PRINT_TO_SCREEN = False  # for debugging
                        #(prints our messages onto the screen as well as the log file)

socket.setdefaulttimeout(SOCKET_TIMEOUT)


messageNames = {}
messageNames[ 2000 ] = "Register"
messageNames[ 2001 ] = "Incoming Chat Message"
messageNames[ 2002 ] = "Check Username"
messageNames[ 2003 ] = "Unregister from room"
messageNames[ 30000 ] = "Machine Status"
messageNames[ 30001 ] = "User Status"
messageNames[ 30002 ] = "Outgoing Chat Message"
messageNames[ 30003 ] = "Username OK"
messageNames[ 31000 ] = "Info Message"




#  MESSAGES:
#
#
#  Notes:
#  --------------------
#  All messages are sent in this format (as strings):
#  code
#  data
#
#  For example:
#  "2002"
#  "Ratko"
#
#
#  All machines are always keyed by machineId that the users should connect to to control SAGE
#
#
#  <<< UI  --->  SERVER >>>
#  CODE    FORMAT                 MESSAGE
#  ----------------------------------------------------------------
#  2000    username                register this user with the Users server
#          info
#          machine_id              the machines the user is connected to
#
#  2001    from={username}         send a chat message to one person or to all
#          to={"all" | id}         id = specific to users connected to a sepcific SAGE machine
#          message
#
#  2002    username                check username for duplicates
#
#  2003    username                unregister this username from the machine specified
#          machine_id
#
#          
#  <<< SERVER  --->  UI >>>
#  CODE    FORMAT                 MESSAGE
#  ----------------------------------------------------------------
# 
#  30000   machine_name            a status list of all the MACHINES registered with the server
#          ip
#          port
#          machineId
#          alive={"1" | "0"}       if the machine is alive, send 1, otherwise send 0
#          displayInfo             in this format: "Xtiles Ytiles tileWidth tileHeight"
#          systemIP systemPort     older UIs don't support this so not sent to them
#          "\x00"               < --- a break between different blocks of data
#          machine_name
#          ip
#          port
#          machineId
#          alive={"1" | "0"}       if the machine is alive, send 1, otherwise send 0
#          displayInfo             in this format: "Xtiles Ytiles tileWidth tileHeight"
#          systemIP systemPort     older UIs don't support this so not sent to them
#          "\x00"
#          ....
#
#
#  30001   username                receive a list of USERS that are connected and their info
#          info
#          machine_id              the machines the user is connected to
#          machine_id
#          "\x00"               < --- a break between different users' info
#          username
#          info
#          machine_id
#          "\x00"
#          ....
#
#  30002   from={username}         receive a chat message from one person,
#          to={"all" | id}        id = specific to users connected to a specific SAGE machine
#          message
#
#  30003   "1" | "0"              1=username OK, 0=username already in use
#
#  31000   message                an informative message... just any string
#
#
#
#  <<< SAGE  --->  SERVER >>>
#  CODE    FORMAT                 MESSAGE
#  ----------------------------------------------------------------
#  
#  100     machine_name version   "i am alive" message from SAGE
#          pubIP pubPort          < --- machine ip and port that SAGE UIs should connect to
#          sysIP sysPort          < --- machine ip and port that the apps should use for streaming
#          displayInfo



# log the actions in a file
os.chdir(sys.path[0])  #change to the folder where the script is running
logFile = open("log.txt", "a")
logFileLock = RLock()  #since multiple threads are writing to the same file

def WriteToFile(text):
    logFileLock.acquire()
    logFile.write( text )
    if PRINT_TO_SCREEN:
        print text
    logFile.flush()  #flush the buffer to a file (especially needed if running as a daemon)
    logFileLock.release()


# record some stats to a file
statsFile = open("stats.txt", "a")
statsFileLock = RLock()

def WriteStats(text):
    statsFileLock.acquire()
    statsFile.write( text )
    if PRINT_TO_SCREEN:
        print text
    statsFile.flush()  #flush the buffer to a file (especially needed if running as a daemon)
    statsFileLock.release()




############################################################################
#
#  CLASS: SingleMachine
#  
#  DESCRIPTION: This is basically a thread that deals with the connection
#               to ONLY ONE SAGE. It's created by the Server class upon
#               connection from sage and it receives a clientsocket that
#               was internally created by the Server. It then uses this
#               socket for all the communication. One of these exists for
#               every sage that is connected to this Server and the Server class
#               keeps these SingleMachine objects in a list.
#
#  DATE:        May, 2005
#
############################################################################

class SingleMachine(Thread):
    
    def __init__(self, socket, address, server):
        Thread.__init__(self)
        self.socket = socket
        self.socket.settimeout(SOCKET_TIMEOUT)
        self.server = server
        self.threadKilled = False
        self.name = ""
        self.ip = address[0]  # these 2 are use for control communication
        self.port = ""
        self.systemIP = ""   # these 2 are used for streaming the data
        self.systemPort = ""
        self.sageVersion = None   #version of SAGE
        self.oldStyleSAGE = False
        self.machineId = self.ip+":"+str(self.port)
        self.displayInfo = ""
        self.receivedRegisterMessage = False   # in case SAGE connects but never registers the thread will be running forever
        self.lastReportTime = None  #this will be the time of the last report from fsManager
        self.maxReportInterval = 6.0   #allow up to 6 seconds between the sage reports and then consider the fsManager disconnected

        t = Timer(10, self.WaitForRegisterMessage)
        t.start()


    def WaitForRegisterMessage(self):
        if not self.receivedRegisterMessage:
            self.Stop(False)


        # stops the thread, breaks the connection and unregisters the machine from this server
    def Stop(self, unregister=True):
        # record the stats
        stats = "\nSAGE disconnected: "+self.name+" "+str(self.machineId)+" "+str(self.displayInfo)+" "+str(time.asctime())
        WriteStats(stats)
        
        WriteToFile( "\n*** Connection closed with SAGE: \"" + self.name + "\"  <" + time.asctime() + ">")
        self.threadKilled = True
        if unregister:
            self.server.UnregisterMachine(self.GetId())
        self.socket.close()
    

    #-----------------------------------------------
    # data access methods
    #-----------------------------------------------

    def GetIP(self):
        return self.ip

    def GetName(self):
        return self.name

    def GetSystemIP(self):
        return self.systemIP

    def GetSystemPort(self):
        return self.systemPort

    def GetPort(self):
        return self.port
    
    def IsAlive(self):
        return not self.threadKilled

    def GetDisplayInfo(self):
        return self.displayInfo

    def GetId(self):
        return self.machineId


#-------------------------------------------------------
#  RECEIVING
#-------------------------------------------------------

        # runs in a thread... started in the Server class
    def run(self):        
        while not self.threadKilled:
            try:              
                # this first receive will receive the length of the data
                cleanMsg = self.GetMessage()
                if not cleanMsg:
                    break  #connection was closed
                msgHeader = string.split( cleanMsg, "\n", 1 )
                if len( msgHeader ) > 0:
                    code = int(msgHeader[0])
                    data = msgHeader[1]
                else:
                    WriteToFile( "\n ERROR: message could not be split correctly into (code, data)")
                    break

                
                # call the appropriate function to handle the incoming data
                if code == 100:
                    self.OnRegisterMachineMessage(data)
                	    
            
            except socket.timeout:
                # if the fsManager hasn't reported in a while, assume it's dead and quit
                if self.lastReportTime:
                    if time.time() - self.lastReportTime > self.maxReportInterval:
                        WriteToFile( "\nERROR: Time expired with SAGE connection " + self.name)
                        self.Stop()
                continue
            except socket.error:
                WriteToFile( "\nERROR: UsersServer: socket error in connection with SAGE: " + self.name)
                self.Stop()
                break
       
        # outside of while loop
        self.threadKilled = True
        WriteToFile("\nThread from "+self.name +" died")
		
			    
    def GetMessage(self, MSG_SIZE=CHUNK_SIZE):
        # this first receive will receive the length of the data
        msg = self.socket.recv(MSG_SIZE)
        if len( msg ) < 2:
            WriteToFile("\nERROR: message length < 2. Closing the connection with SAGE "+self.name)
            self.Stop()   #connection was closed
            return False
        

        # since recv is not guaranteed to receive exactly CHUNK_SIZE bytes
        # so keep receiving until it gets the whole chunk
        while len( msg ) < MSG_SIZE:
            msg = msg + self.socket.recv(MSG_SIZE - len( msg))


        # strip all the empty spaces from the message
        cleanMsg = self.CleanBuffer( msg )
        cleanMsg = string.strip( cleanMsg )
        return cleanMsg
        
    
    
    # converts all non-printable characters from the buffer to white spaces
    # (so that they can be removed using string.strip() function)
    def CleanBuffer( self, stBuffer ):
        stNewBuffer = ""
            
        for ch in stBuffer:
            if ( ch in string.printable ):
                stNewBuffer = stNewBuffer + ch
            else:
                stNewBuffer = stNewBuffer + " "
			    
        return stNewBuffer


#-------------------------------------------------------
#  MESSAGE CALLBACKS
#-------------------------------------------------------

    def OnRegisterMachineMessage(self, data):
        """ there are two different fsManagers so handle them differently """
        if self.receivedRegisterMessage:
            # record the time when it was reported
            self.lastReportTime = time.time()
            return
        
        self.receivedRegisterMessage = True
        tokens = string.split(data, "\n", 3)
        if len(tokens[0].split()) == 1: # based on this we will know which fsManger is this
            self.name = tokens[0]
            self.ip = tokens[1] 
            self.port = int(tokens[2])
            self.systemIP = self.ip   #make them the same as public if not given
            self.systemPort = self.port
            self.oldStyleSAGE = True
        else:  # new style SAGE includes the system port/ip as well as the sage version
            (self.name, self.sageVersion) = tokens[0].split()
            sys = tokens[1].split()
            self.systemIP = sys[0].strip()
            self.systemPort = int(sys[1].strip())

            pub = tokens[2].split()
            self.ip = pub[0].strip()
            self.port = int(pub[1].strip())
            
        self.machineId = self.ip+":"+str(self.port)
        self.displayInfo = tokens[3]
        self.server.RegisterMachine(self)

        # record the stats
        stats = "\nSAGE connected: "+self.name+" "+str(self.machineId)+" "+str(self.displayInfo)+" "+str(time.asctime())
        WriteStats(stats)
        






############################################################################
#
#  CLASS: SingleUser
#  
#  DESCRIPTION: This is basically a thread that deals with the connection
#               to ONLY ONE SAGE UI. It's created by the Server class upon
#               connection from a user and it receives a clientsocket that
#               was internally created by the Server. It then uses this
#               socket for all the communication. One of these exists for
#               every user that is connected to this Server and the Server class
#               keeps these SingleUser objects in a list.
#
#  DATE:        May, 2005
#
############################################################################

class SingleUser(Thread):
    
    def __init__(self, socket, address, server):
        Thread.__init__(self)
        self.socket = socket
        self.socket.settimeout(SOCKET_TIMEOUT)
        self.server = server
        self.threadKilled = False
        self.username = ""
        self.info = ""
        self.machineList = []   # the SAGE machines that this user is connected to ( a list of machineIds )
        self.ip = address[0]
        self.registered = False
        self.ui_version = "2.0"  # default
        self.newStyle = False
        
        self.messageCallback = {}
        self.messageCallback[ 2000 ] = self.OnRegister
        self.messageCallback[ 2001 ] = self.OnChatMessage
        self.messageCallback[ 2002 ] = self.OnCheckUsername
        self.messageCallback[ 2003 ] = self.OnUnregisterUser

        # send the first reply message with machine status
        self.server.OnConnectUser(self)
        self.stopped = False


    def Stop(self, unregister=True):
        self.stopped = True
        # record the stats
        stats = "\nUser disconnected: "+self.GetName()+" "+str(self.ip)+" "+str(time.asctime())
        WriteStats(stats)
        self.threadKilled = True
        WriteToFile( "\n*** Connection closed with user: \"" + self.username + "\"  <" + time.asctime() + ">")
        if unregister and self.registered:
            self.server.UnregisterUser(self, self.username)
        self.server.OnDisconnectUser(self)
        #self.threadKilled = True
        self.socket.close()
    

    def GetInfo(self):
        return self.info

    def GetName(self):
        return self.username

    def GetMachines(self):
        return self.machineList

    def IsNewStyle(self):
        return self.newStyle

    
#-------------------------------------------------------
#  RECEIVING
#-------------------------------------------------------

        # runs in a thread... started in the Server class
    def run(self):        
        while not self.threadKilled:
            try:
                # this first receive will receive the length of the data
                cleanMsg = self.GetMessage()
                if not cleanMsg:
                    break  #connection was closed
                msgHeader = string.split( cleanMsg, "\n", 1 )
                if len( msgHeader ) > 0:
                    code = int(msgHeader[0])
                    data = msgHeader[1]
                else:
                    WriteToFile( "\n ERROR: message could not be split correctly into (code, data)")
                    break

                # print what we received
                if messageNames.has_key(code):
                    WriteToFile( "RECEIVED: \"" + messageNames[code] + "\" from " + self.username + "   (" + str(self.ip) + ")")
                
                # call the appropriate function to handle the incoming data
                if self.messageCallback.has_key( code ):
                    self.messageCallback[ code ](data)
                else:
                    WriteToFile("\nERROR: Message code " + str(code) + " unrecognized")
                	    
            
            except socket.timeout:
                continue
            except socket.error:
                WriteToFile( "\nERROR: UsersServer: socket error in connection with: " + self.username)
                self.Stop()
                break
 
        if self.stopped == False:
            WriteToFile("Stopping the thread")
            self.Stop()
        WriteToFile("Thread from " + self.username + " has died")
			    
    def GetMessage(self, clean=True, MSG_SIZE=CHUNK_SIZE):
        # this first receive will receive the length of the data
        msg = self.socket.recv(MSG_SIZE)
        if len( msg ) < 2:
            self.Stop()   #connection was closed
            return False
		    

        # since recv is not guaranteed to receive exactly CHUNK_SIZE bytes
        # so keep receiving until it gets the whole chunk
        while len( msg ) < MSG_SIZE:
            msg = msg + self.socket.recv(MSG_SIZE - len( msg))

        if clean:
            # strip all the empty spaces from the message
            cleanMsg = self.CleanBuffer( msg )
            cleanMsg = string.strip( cleanMsg )
            return cleanMsg
        else:
            return msg

    
    
    # converts all non-printable characters from the buffer to white spaces
    # (so that they can be removed using string.strip() function)
    def CleanBuffer( self, stBuffer ):
            stNewBuffer = ""

            for ch in stBuffer:
                if ch == '\0':   #this is our message separator so handle it separately
                    stNewBuffer += ch
                elif ch in string.printable:
                    stNewBuffer += ch
                else:
                    stNewBuffer += " "
            return stNewBuffer
    


#-------------------------------------------------------
#  SENDING
#-------------------------------------------------------

        # make the message with the right code and send it
    def MakeMsg(self, code, data):
        msg = '%8s\n%s' %(code,data)
        WriteToFile( "SEND: \"" + messageNames[code] + "\" to " + self.username)
        msg = msg + ' ' * (MSGLEN-len(msg))
        self.Send(msg)

        # just performs the send operation
    def Send(self, msg):
        try:
            self.socket.send(msg)
        except socket.error:
            WriteToFile( "\nERROR: UsersServer: Could not send message: socket error with: "+self.username )
            self.socket.close()
            #self.Stop()

            # record the stats
            stats = "\nUser disconnected: "+self.GetName()+" "+str(self.ip)+" "+str(time.asctime())
            WriteStats(stats)
            #self.Stop()
            # the run() method will catch this as well and it will handle the cleanup

        # these functions are called by the main Server class in order to send
        # messages to all the users via their own sockets used in this SingleUser class
    def SendChatMessage(self, message):
        self.MakeMsg(30002, message)


        # form a message and send the users status
    def SendUsersStatusMessage(self, statusList):
        message = ""

        # make the message out of the machine status list
        for userInfo in statusList:
            if message != "":  message += SEPARATOR   #add the separator before adding each new machine info    
            for infoItem in userInfo:
                message += str(infoItem) + "\n"

        self.MakeMsg(30001, message)


        # form a message and send the machine status
    def SendMachinesStatusMessage(self, statusList):
        message = ""

        # make the message out of the machine status list
        for machineInfo in statusList:
            if message != "":  message += SEPARATOR   #add the separator before adding each new machine info    
            for infoItem in machineInfo:
                message += str(infoItem) + "\n"
            
        self.MakeMsg(30000, message)
        

    def SendUsernameOKMessage(self, usernameOK):
        self.MakeMsg( 30003, str(int(usernameOK)) )


        
#-------------------------------------------------------
#  MESSAGE CALLBACKS
#-------------------------------------------------------

    def OnRegister(self, data):
        tokens = string.split(data, "\n")
        self.username = tokens[0]
        self.info = tokens[1]
        machineId = tokens[2]
        if not machineId in self.machineList:
            self.machineList.append(machineId)

        # record the stats
        stats = "\nUser registered: "+self.GetName()+" "+str(self.ip)+" "+str(machineId)+" "+str(time.asctime())
        WriteStats(stats)

        self.server.RegisterUser(self, self.username)

        # if the version of the connected UI handles system ip/port
        # for each machine, send it after the registration message
        if self.IsNewStyle():
            status = self.server.MakeNewMachinesStatusList()
            self.SendMachinesStatusMessage(status)
        

    def OnChatMessage(self, data):
        tokens = string.split(data, "\n", 2)
        if tokens[1] == "all":
            toRoom = "all"
        else:
            toRoom = tokens[1]
        self.server.ForwardChatMessage(self, toRoom, data)
        

    def OnCheckUsername(self, data):
        tokens = string.split(data, "\n")
        if len(tokens) > 1:  # sageui v2.82+ sends a version number
            self.ui_version == tokens[1].strip()
            self.newStyle = True
        self.SendUsernameOKMessage(self.server.IsUsernameOK(self, tokens[0]) )


    def OnUnregisterUser(self, data):
        tokens = string.split(data, "\n")
        username = tokens[0]
        machineId = tokens[1]
        if machineId in self.GetMachines():
            self.machineList.remove(machineId)
            # record the stats
            stats = "User unregistered: "+self.GetName()+" "+str(self.ip)+" "+str(machineId)+" "+str(time.asctime())
            WriteStats(stats)
            self.server.UpdateUsers()
        
    


############################################################################
#
#  CLASS: SingleLauncher
#  
#  DESCRIPTION: This class describes one appLauncher connection for use by
#               SAGE UIs. It mainly contains a list of applications and their configs
#
#
############################################################################

class SingleLauncher:
    def __init__(self, launcherId, name, ip, port, appList):
        self.port = port
        self.appList = appList
        self.ip = ip
        self.launcherId = launcherId
        self.name = name
        self.oldT = time.time()
        self.maxReportTime = 10  #allow maximum 8 seconds between reports

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
    
    def isAlive(self):
        if (time.time() - self.oldT) < self.maxReportTime:
            return True
        else:
            return False

    def report(self):
        self.oldT = time.time()



############################################################################
#
#  CLASS: Server
#  
#  DESCRIPTION: This server should run as a deamon and constantly listen for
#               connections on a certain port. Once it accepts a connection
#               it spawns a thread (SingleUser) that takes care of listening for the
#               incoming messages and sending messages on that socket.
#               It also relays all the incoming messages to their corresponding
#               recepients. To do that it keeps track of all the users connected
#               in a list registeredUsers[]. It also listens for connections
#               from fsManagers and keeps track of them in connectedMachines[] (
#               a hash of singleMachines).
#
#  DATE:        May, 2005
#
############################################################################

class Server:
    def __init__(self):
        self.serverRunning = True

        # start the XMLRPC server in a thread
        xmlrpcServer = Thread(target=self.StartXMLRPCServer)
        xmlrpcServer.start()

        # start the two servers listening on separate ports
        machinesServer = Thread(target=self.StartMachinesServer)
        machinesServer.start()

        try:
            self.StartUsersServer()  #start this one in the main thread so that we can capture
                                 #keystrokes such as Ctrl-C
        except KeyboardInterrupt:
            WriteToFile ("\n******  Shutting down the server  *******")
            self.serverRunning = False
            self.CloseAllUserConnections()
            self.CloseAllSAGEConnections()
            self.xmlrpc.server_close()
            #logFile.close()



#------------------------------------------------------------------------------
#  XML-RPC STUFF  -  FOR REMOTE ADMINISTRATION, APPLAUNCHER AND SAGE UI PROXY
#------------------------------------------------------------------------------

    def StartXMLRPCServer(self):
        self.registeredLaunchers = {}  #key=launcherID, value=SingleLauncher object
        
        # start the XML-RPC server
        self.xmlrpc = XMLRPCServer(("", 8009))

        # users
        self.xmlrpc.register_function(self.GetRegisteredUsers)
        self.xmlrpc.register_function(self.GetUserInfo)
        self.xmlrpc.register_function(self.DisconnectUser)

        # machines
        self.xmlrpc.register_function(self.GetMachineInfo)
        self.xmlrpc.register_function(self.GetRegisteredMachines)
        self.xmlrpc.register_function(self.DisconnectMachine)

        # appLauncher
        self.xmlrpc.register_function(self.ReportLauncher)
        self.xmlrpc.register_function(self.GetRegisteredLaunchers)
        self.xmlrpc.register_function(self.UnregisterLauncher)

        WriteToFile ("Starting the XML-RPC Server...\n")
        while self.serverRunning:
            try:
                self.xmlrpc.handle_request()  #accept and process xmlrpc requests
                self.__checkLaunchers()   #check to see whether every launcher is still alive
            except socket.timeout:
                continue
            except:
                WriteToFile( "\n=====> XMLRPC Server ERROR:" )
                WriteToFile( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
                continue



        ### loops through all the app launchers and checks whether they are still alive
        ### the minimum frequency of checks is defined by the timeout set int XMLRPCServer constructor
    def __checkLaunchers(self):
        for l in self.registeredLaunchers.values():
            if not l.isAlive():
                WriteToFile("Launcher "+l.getName()+"("+l.getId()+") unregistered")
                self.UnregisterLauncher(l.getId())
                
            
        ### called by each appLauncher in order to register with the server
    def ReportLauncher(self, launcherName, launcherIP, launcherPort, appList):
        launcherId = launcherIP+":"+str(launcherPort)
        if launcherId in self.registeredLaunchers:
            self.registeredLaunchers[launcherId].report()  # launcher already exists so just update its last report time
            self.registeredLaunchers[launcherId].setAppList(appList)
        else:
            l = SingleLauncher(launcherId, launcherName, launcherIP, launcherPort, appList)
            WriteToFile("Launcher "+l.getName()+"("+l.getId()+") registered")
            self.registeredLaunchers[launcherId] = l
        return launcherId


        ### removes the appLauncher from a list of registered ones
    def UnregisterLauncher(self, launcherId):
        if launcherId in self.registeredLaunchers:
            del self.registeredLaunchers[ launcherId ] 
        return 1
    

        ### return a hash of all the app launchers running
        ### key= "name:launcherId" , value=appList  (that's another hash of appNames and their configs)
    def GetRegisteredLaunchers(self):
        tempHash = {}
        for l in self.registeredLaunchers.itervalues():
            tempHash[ l.getName()+":"+l.getId() ] = l.getAppList()
        return tempHash
    


        ### return a list of currently registered users and machines
    def GetRegisteredUsers(self):
        self.registeredUsersLock.acquire()
        users = self.registeredUsers.keys()
        self.registeredUsersLock.release()
        return users

    def GetRegisteredMachines(self):
        self.connectedMachinesLock.acquire()
        machineList = []
        for machineId, singleMachine in self.connectedMachines.iteritems():
            machineList.append(singleMachine.GetName() + " - " + str(machineId))
        self.connectedMachinesLock.release()
        return machineList

##     def GetConnectedUsers(self):
##         self.connectedUsersLock.acquire()
##         users = []
##         for user in self.connectedUsers:
##             users.append(user.GetName())
##         self.connectedUsersLock.release()
##         return users

        ### return user and machine info
    def GetUserInfo(self, username):
        self.registeredUsersLock.acquire()
        if self.registeredUsers.has_key(username):
            singleUser = self.registeredUsers[username]
            machineList = []
            self.connectedMachinesLock.acquire()
            for machineId in singleUser.GetMachines():
                if self.connectedMachines.has_key(machineId):
                    machineList.append(self.connectedMachines[machineId].GetName())
            self.connectedMachinesLock.release()
            self.registeredUsersLock.release()
            return machineList  #singleUser.GetMachines()
        else:
            self.registeredUsersLock.release()
            return -1

    def GetMachineInfo(self, machineId):
        self.connectedMachinesLock.acquire()
        if self.connectedMachines.has_key(machineId):
            m = self.connectedMachines[machineId]
            self.connectedMachinesLock.release()

            #now make a list of all the users that are connected to this machine
            self.registeredUsersLock.acquire()
            userList = []
            for name, singleUser in self.registeredUsers.iteritems():
                if machineId in singleUser.GetMachines():
                    userList.append(name)
            self.registeredUsersLock.release()
            return (m.GetName(), m.GetIP(), m.GetPort(), m.GetId(), m.IsAlive(), m.GetDisplayInfo(), userList)
        else:
            self.connectedMachinesLock.release()
            return (-1,-1,-1,-1,-1,-1,-1)


        ### allow the user to close individual connections with SAGE and users
    def DisconnectUser(self, username):
        self.registeredUsersLock.acquire()
        if self.registeredUsers.has_key(username):
            singleUser = self.registeredUsers[username]
            singleUser.Stop()
        self.registeredUsersLock.release()
        return True


    def DisconnectMachine(self, machineId):
        self.connectedMachinesLock.acquire()
        if self.connectedMachines.has_key(machineId):
            singleMachine = self.connectedMachines[machineId]
            singleMachine.Stop()
        self.connectedMachinesLock.release()
        return True



#----------------------------------------------------------------------------------------
#   THE SERVERS RUNNING IN THREADS RECEIVING CONNECTIONS FROM USERS AND SAGES
#----------------------------------------------------------------------------------------


        # runs in the main thread
    def StartUsersServer(self):
        self.registeredUsers = {}   # a hash of SingleUsers for every registered user  (keyed by username)
        self.registeredUsersLock = RLock()  #used to lock the access to self.registeredUsers hash

        self.connectedUsers = []    # this includes all the users that are connected to the server
        self.connectedUsersLock = RLock()  # but not necessarily registered with it. (so registeredUsers is a subset of this)

        self.pendingUsernames = []   # usernames that have been checked with the server but not yet registered
        self.pendingUsernamesLock = RLock()
        
        # create the server socket and accept a connection
        serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if serversocket is None:
                WriteToFile( "\n\nERROR: Server socket could not be created... exiting" )
                return False
        serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        serversocket.bind(("", USER_SERVER_PORT))
        serversocket.listen(MAX_USER_CONNECTIONS)
        serversocket.settimeout(SOCKET_TIMEOUT)

        WriteToFile( "Users Server waiting for connections on port " +  str(USER_SERVER_PORT) +  "...\n" )
        while self.serverRunning:
            try:
                (clientsocket, address) = serversocket.accept()
            except socket.timeout:
                continue
            except:
                WriteToFile( "\n\nUsers Server Not accepting any more connections... exiting  <" + time.asctime() + ">\n" )
                self.CloseAllUserConnections()
                self.serverRunning = False
                break

            WriteToFile( "\n*** Connection accepted from " + str(address[0]) + " <" + time.asctime() + ">" )
            # create a SingleUser instance and start the receiver in a thread
            t = SingleUser(clientsocket, address, self)
            self.connectedUsersLock.acquire()
            self.connectedUsers.append(t)   #add the user to the list of all connected users
            self.connectedUsersLock.release()
            t.start()
        WriteToFile("\nUsers Server exited")
        

        # runs in a thread
    def StartMachinesServer(self):
        self.connectedMachines = {}   # a hash of SingleMachines for every connected SAGE (keyed by id)            
        self.connectedMachinesLock = RLock()

                # create the server socket and accept a connection
        serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if serversocket is None:
                WriteToFile( "\n\nERROR: SAGE Server socket could not be created... exiting" )
                return False
        serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        serversocket.bind(("", SAGE_SERVER_PORT))
        serversocket.listen(MAX_SAGE_CONNECTIONS)  
        serversocket.settimeout(SOCKET_TIMEOUT)
        
        WriteToFile( "SAGE Server waiting for connections on port " +  str(SAGE_SERVER_PORT) +  "...\n" )
        while self.serverRunning:
            try:
                (clientsocket, address) = serversocket.accept()
            except socket.timeout:
                continue
            except:
                WriteToFile( "\n\nSAGE Server Not accepting any more connections... exiting  <" + time.asctime() + ">\n" )
                self.CloseAllSAGEConnections()
                self.serverRunning = False
                break

            WriteToFile( "\n*** SAGE Connection accepted from " + str(address[0]) + " <" + time.asctime() + ">" )
            # create a SingleMachine instance and start the receiver in a thread
            t = SingleMachine(clientsocket, address, self)
            t.start()
        WriteToFile("\nSAGE Server exited")
        self.xmlrpc.server_close()


    def CloseAllUserConnections(self):
        for singleUser in self.registeredUsers.itervalues():
            singleUser.Stop(unregister=False) #we dont want to unregister because the server is closing anyway

    def CloseAllSAGEConnections(self):
        for singleMachine in self.connectedMachines.itervalues():
            singleMachine.Stop(unregister=False) #we dont want to unregister because the server is closing anyway
            


#-------------------------------------------------------
#  MESSAGE CALLBACKS - USERS
#-------------------------------------------------------

        # when the user connects, we need to send him the list of all the SAGE
        # machines registered with this server
    def OnConnectUser(self, singleUser):
        self.connectedMachinesLock.acquire()
        singleUser.SendMachinesStatusMessage(self.MakeMachinesStatusList())
        self.connectedMachinesLock.release()


        # if the user connected but never registered, we still have to remove him from this list
    def OnDisconnectUser(self, singleUser):

        # remove from the pending usernames if connected
        self.pendingUsernamesLock.acquire()
        if singleUser.GetName() in self.pendingUsernames:
            self.pendingUsernames.remove(singleUser.GetName())
        self.pendingUsernamesLock.release()

        # remove from connected users list
        self.connectedUsersLock.acquire()
        if singleUser in self.connectedUsers:
            self.connectedUsers.remove(singleUser)
        self.connectedUsersLock.release()
        

        # adds the new user keyed by its name and makes the new status list
        # returns: true if successful, false otherwise (if username already exists)
    def RegisterUser(self, singleUser, username):
        self.registeredUsersLock.acquire()
        if not self.registeredUsers.has_key(username) or self.registeredUsers[username].ip == singleUser.ip:
            self.registeredUsers[ username ] = singleUser  # add the user to the list
            singleUser.registered = True
            
        # remove from the list of pending usernames
        self.pendingUsernamesLock.acquire()
        if username in self.pendingUsernames:
            self.pendingUsernames.remove(username)
        self.pendingUsernamesLock.release()

        # update the status of other users
        self.UpdateUsers()
        self.registeredUsersLock.release()



    def UnregisterUser(self, singleUser, username):
        
        # remove the user from the list of all the connected users
        self.connectedUsersLock.acquire()
        if singleUser in self.connectedUsers:
            self.connectedUsers.remove(singleUser)
            WriteToFile("removed "+username+" from connectedUsers")
        self.connectedUsersLock.release()

        # now remove him from the list of registered users
        self.registeredUsersLock.acquire()
        if self.registeredUsers.has_key( username ):
            del self.registeredUsers[ username ]
            self.UpdateUsers()
            WriteToFile("removed "+username+" from registeredUsers")
        self.registeredUsersLock.release()

        # now, check all the rooms that the user was connected to and see if any
        # of them are empty now that the user has left... if there are empty rooms,
        # close them
        emptyRooms = False
        self.connectedMachinesLock.acquire()
        for room in self.connectedMachines.keys()[:]:  #loop through all the machines just in case there are some daemons
            if self.connectedMachines.has_key(room) and (not self.connectedMachines[room].IsAlive()) and self.IsRoomEmpty(room):
                emptyRooms = True
                del self.connectedMachines[room]
                WriteToFile("closed the room "+room)
        if emptyRooms:
            self.UpdateMachines()
        self.connectedMachinesLock.release()


        # updates all the users with the new status (based on self.registeredUsers)
    def UpdateUsers(self):
        self.registeredUsersLock.acquire()
        statusList = self.MakeUsersStatusList()
        for username, singleUser in self.registeredUsers.iteritems():
            if len(singleUser.GetMachines()) > 0:
                singleUser.SendUsersStatusMessage(statusList)
        self.registeredUsersLock.release()

        
        # forwads the chat message either to all the chat rooms or a specific one
    def ForwardChatMessage(self, sender, toRoom, message):
        self.registeredUsersLock.acquire()
        for name, singleUser in self.registeredUsers.iteritems():
            singleUser.SendChatMessage(message)
        self.registeredUsersLock.release()


        # checks for duplicates in usernames
    def IsUsernameOK(self, singleUser, username):
        self.registeredUsersLock.acquire()
        self.pendingUsernamesLock.acquire()
        if username in self.registeredUsers:   # username already exists
            if self.registeredUsers[username].ip == singleUser.ip:  # its the same user reconnecting so it's OK
                usernameTaken = False
            else:
                usernameTaken = True
        elif username in self.pendingUsernames:
            usernameTaken = True
        else:
            usernameTaken = False
            
        if not usernameTaken:
            self.pendingUsernames.append(username)
            t = Timer(2, self.ExpireUsername, [username])
            t.start()
        self.pendingUsernamesLock.release()
        self.registeredUsersLock.release()
        return not usernameTaken


    def ExpireUsername(self, username):
        # remove from the list of pending usernames
        self.pendingUsernamesLock.acquire()
        if username in self.pendingUsernames:
            self.pendingUsernames.remove(username)
        self.pendingUsernamesLock.release()
        
    
        #make the status list consisting of name,info,machine,name,info,machine...
    def MakeUsersStatusList(self):
        statusList = []   
        keys = self.registeredUsers.keys()
        keys.sort()
        for username in keys:
            user = self.registeredUsers[username]
            tempList = []
            tempList.append(username)
            tempList.append(user.GetInfo())
            for machine in user.GetMachines():
                tempList.append(machine)
            statusList.append(tempList)
        return statusList



#-------------------------------------------------------
#  MESSAGE CALLBACKS - MACHINES
#-------------------------------------------------------

        #registers SAGE with the server so that it's visible to the users
    def RegisterMachine(self, singleMachine):
        machineId = singleMachine.GetId()
        self.connectedMachinesLock.acquire()
        if not self.connectedMachines.has_key( machineId ):
            self.connectedMachines[ machineId ] = singleMachine
            self.UpdateMachines()  #update all the users with the new machine status
        else:  #the old singleMachine was still preserved since there were some users in it still
            WriteToFile("\n* The machine "+str(machineId)+" already exists so trying to close the connection with the previous one")
            self.connectedMachines[ machineId ].Stop(False)  # this is a preventative measure just in case it was a zombie
            del self.connectedMachines[ machineId ]  #delete the old one and save the new one
            self.connectedMachines[ machineId ] = singleMachine
            self.UpdateMachines()
        self.connectedMachinesLock.release()


        # updates all the users with the new machine status (based on self.connectedMachines)
    def UpdateMachines(self):
        self.connectedUsersLock.acquire()
        statusList = self.MakeMachinesStatusList()
        newStatusList = self.MakeNewMachinesStatusList()
        for singleUser in self.connectedUsers:
            if singleUser.IsNewStyle():  # ui 2.82 and above gets the systemip/port info as well
                singleUser.SendMachinesStatusMessage(newStatusList)
            else:
                singleUser.SendMachinesStatusMessage(statusList)
        self.connectedUsersLock.release()


        # removes the machine keyed by its machineId
    def UnregisterMachine(self, machineId):
        self.connectedMachinesLock.acquire()
        if self.connectedMachines.has_key( machineId ):
            if self.IsRoomEmpty( machineId ):  #if the room was determined to be empty, close it
                del self.connectedMachines[machineId]    
            self.UpdateMachines()
        self.connectedMachinesLock.release()


        # check if there are any users still left in this room,
        # if there are, return FALSE, otherwise return TRUE
    def IsRoomEmpty(self, machineId):
        roomEmpty = True
        self.registeredUsersLock.acquire()
        registeredUsers = self.registeredUsers.copy()
        self.registeredUsersLock.release()
        for singleUser in registeredUsers.values():
            if machineId in singleUser.GetMachines():
                roomEmpty = False
                break   # there is at least one user still left in the room, so leave it open
        return roomEmpty
                

        # it makes the list of currently connected machines
    def MakeMachinesStatusList(self):
        statusList = []   #make the status list consisting of [name,ip,port,id,alive], [name,ip,port,id,alive], ....
        keys = self.connectedMachines.keys()
        keys.sort()
        for machineId in keys:
            tempList = []
            singleMachine = self.connectedMachines[machineId]
            tempList.append( singleMachine.GetName() )
            tempList.append( singleMachine.GetIP() )
            tempList.append( singleMachine.GetPort() )
            tempList.append( machineId )
            tempList.append( int(singleMachine.IsAlive()) )
            tempList.append( singleMachine.GetDisplayInfo() )
            statusList.append(tempList)
        return statusList

    
        # this is for new style UIs that accept system ip/port
        # as well
    def MakeNewMachinesStatusList(self):
        statusList = [] 
        keys = self.connectedMachines.keys()
        keys.sort()
        for machineId in keys:
            tempList = []
            singleMachine = self.connectedMachines[machineId]
            tempList.append( singleMachine.GetName() )
            tempList.append( singleMachine.GetIP() )
            tempList.append( singleMachine.GetPort() )
            tempList.append( machineId )
            tempList.append( int(singleMachine.IsAlive()) )
            tempList.append( singleMachine.GetDisplayInfo() )
            tempList.append( singleMachine.GetSystemIP()+" "+str(singleMachine.GetSystemPort()))
            statusList.append(tempList)
        return statusList


        


#-----------------------------------------------------------------------------------------------


class XMLRPCServer(SimpleXMLRPCServer.SimpleXMLRPCServer):
    allow_reuse_address = True
    def __init__(self, addr):
        SimpleXMLRPCServer.SimpleXMLRPCServer.__init__(self, addr, logRequests=False)
        self.socket.settimeout(2)  # so that handle_request times out and we can check all appLaunchers

        

def main( argv ):
    global USER_SERVER_PORT
    global SAGE_SERVER_PORT
    global PRINT_TO_SCREEN
    
    WriteToFile("\n\n\n#####################################################################\n")
    WriteToFile("   SAGE Server HAS BEEN RESTARTED\t<" + time.asctime() + ">")
    WriteToFile("\n#####################################################################\n\n\n")

    WriteStats("\n\nSERVER RESTARTED\t"+str(time.asctime())+"\n\n")
    
    # get the arguments (port)
    if len(argv) == 3:
        if "-v" in argv:
            PRINT_TO_SCREEN = True
        else:
            print "Usage: python UsersServer.py [USER_SERVER_PORT] [SAGE_SERVER_PORT]\n"
            sys.exit(0)
    elif len(argv) > 3:
        USER_SERVER_PORT = int(argv[2])
        SAGE_SERVER_PORT = int(argv[3])
        if "-v" in argv:
            PRINT_TO_SCREEN = True
        

    # start the server accepting connections at the specified port
    server = Server()

    
	
if __name__ == '__main__':
	main( ['', os.path.basename( sys.argv[0] )] + sys.argv[1:] )
