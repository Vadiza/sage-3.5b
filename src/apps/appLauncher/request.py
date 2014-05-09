############################################################################
#
# AppLauncher - Application Launcher for SAGE
# Copyright (C) 2006 Electronic Visualization Laboratory,
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
# Direct questions, comments etc about AppLauncher to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#
############################################################################


import subprocess as sp
import traceback as tb
import os, sys, time, os.path
from myprint import *   # handles the printing or logging
from threading import RLock, Thread

opj = os.path.join




#############################################################################
#
#                           THREAD SAFE :-)
#
#############################################################################

class CurrentRequests:
    def __init__(self, nodeHash):
        self._requests = {}  #key = id, value = Request()
        self._nodeHash = nodeHash  #key=IP, value=[0...n]   --> how many apps are running on that node

        self.__submitThread = Thread(target=self.submitRequests)
        self.__requestsToSubmit = []
        self.__submitLock = RLock()
        self.__doRunSubmitThread = True
        self.__submitThread.start()
        self.__lastAppId = 0


    def __getFirstAvailableId(self):
        #""" loops through the running requests until it finds an available id """
        self.cleanup()
        
        self.__submitLock.acquire()
        self.__lastAppId += 1
        newId = self.__lastAppId
        self.__submitLock.release()

        return newId

        """
        def inQueue(appId):
            for r in self.__requestsToSubmit:
                if r.config.getAppId() == appId:
                    return True
            return False
        
        
        self.cleanup()  #first clean up all of the dead requests and their appIds

        self.__submitLock.acquire()
        for i in range(0, 9999):
            if not self._requests.has_key(i) and not inQueue(i):
                self.__submitLock.release()
                return i
        self.__submitLock.release()
        """
        

    def __getNextAvailableNode(self):
        ''' it loops through all the nodes of this cluster and returns the
            one that is running the fewest apps
        '''
        currentMin = 99999
        currentIP = None
        for ip, numApps in self._nodeHash.iteritems():
            #if currentIP == None:  currentIP=ip  #initial case
            if numApps == 0:
                self._nodeHash[ip] = numApps+1  #increase the number of apps running on this node
                return ip
            elif numApps <= currentMin:   #any machine with less than or equal to apps is a candidate
                currentMin = numApps
                currentIP = ip

        self._nodeHash[currentIP] = currentMin+1  #increase the number of apps running on this node
        return currentIP


    def getRequest(self, appId):
        """ get the request based on its appID (port number in fact) """
        self.__submitLock.acquire()
        if self._requests.has_key(appId):
            r = self._requests[appId]
            self.__submitLock.release()
            return r
        else:
            self.__submitLock.release()
            return False


    def getStatus(self):
        """ returns the current app status as a hash of appNames keyed by appId """
        
        self.cleanup()
        status = {}
        self.__submitLock.acquire()
        for appId, request in self._requests.iteritems():
            status[str(appId)] = (request.config.getAppName(), request.command, request.targetMachine)

        self.__submitLock.release()
        return status
        

    def addRequest(self, config):
        """ modifies the appId parameter of the config and creates a new Request object """

        # modify the appId parameter before writing the config to a file
        appId = self.__getFirstAvailableId()
        config.setAppId(appId)
        
        # if the app can be run on the nodes, get the machine with the lowest load (fewest apps running)
        if config.getRunOnNodes():
            nodeIP = self.__getNextAvailableNode()
            config.setTargetMachine(nodeIP)
            WriteLog( "\nNODE IP = "+ nodeIP + "-" + str(self._nodeHash[nodeIP])+ "\n\n")
        else:
            if self._nodeHash.has_key( config.getTargetMachine() ):
                self._nodeHash[config.getTargetMachine()] += 1
            
        # make the request
        request = SSHRequest(config)

        # submit it... in a separate thread
        self.__submitLock.acquire()
        self.__requestsToSubmit.append(request)
        self.__submitLock.release()
        
        return appId


    def submitRequests(self):
        while self.__doRunSubmitThread:
            
            if len(self.__requestsToSubmit) > 0:
                self.__submitLock.acquire()
                request = self.__requestsToSubmit.pop(0)
                self.__submitLock.release()

                res = request.submit()

                if res != -1:
                    self.__submitLock.acquire()
                    self._requests[ request.config.getAppId() ] = request
                    self.__submitLock.release()

            time.sleep(0.5)


    def stopSubmitThread(self):
        self.__doRunSubmitThread = False
        

    def stopRequest(self, appId):
        """ stops the request forcefully """

        self.__submitLock.acquire()
        
        if self._requests.has_key(appId):
            ret = self._requests[appId].kill()
            self.__submitLock.release()
            time.sleep(1)
            self.cleanup()
            
            return ret
        else:
            self.__submitLock.release()
            return False


    def cleanup(self):
        """ this runs every so often and checks whether the requests that we started are still alive
            if they are not alive, they are removed from the list of requests and their port (appId) is recycled
        """
        self.__submitLock.acquire()
        
        for appId, request in self._requests.items():
            if not request.isAlive():
                if request.config.getCommand().lower().startswith("vncviewer"):
                    WriteLog( ">>>> CLEANING up after: VNCViewer, appId = " + str(appId) )
                else:
                    WriteLog( ">>>> CLEANING up after: " + request.config.getCommand() + "  appId = " + str(appId) )
                request.deletePIDFile()
                #if request.config.getRunOnNodes():
                if self._nodeHash.has_key( request.config.getTargetMachine() ):
                    self._nodeHash[ request.config.getTargetMachine() ] -= 1  #decrease the num of apps running on this node
                del self._requests[appId]

        self.__submitLock.release()



class Request:
    def __init__(self, config):
        self.targetMachine = config.getTargetMachine()
        self.command = config.getCommand()
        self.configFilename = config.getConfigFilename()
        self.processObj = None    # the object corresponding to the process we started
        self.config = config

        

class SSHRequest(Request):

    def submit(self):
        # copy the configuration file over
        try:
            self.config.writeToFile()
            sp.call(["chmod", "g+w", self.configFilename])  #change the permissions of the temp file
            
            retcode = sp.call(["/usr/bin/scp", self.configFilename, self.targetMachine+":"+self.configFilename])
            sp.call(["/usr/bin/ssh", "-x", self.targetMachine, "chmod a+rw "+self.configFilename])
        except:
            WriteLog( "===>  ERROR copying config file... application will use the default configuration:")
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )

        # launch the application via ssh
        try:
            cf = os.path.join("conf", os.path.basename(self.configFilename))
            if self.command.lower().startswith("vncviewer"):
                WriteLog( "\n\n>>>> RUNNING: /usr/bin/ssh -x " + self.targetMachine + " cd "+self.config.getBinDir()+" ;env DISPLAY=:0.0 "+"SAGE_APP_CONFIG=" + cf + " VNCViewer (hidden parameters)")
            else:
                WriteLog( "\n\n>>>> RUNNING: /usr/bin/ssh -x " + self.targetMachine + " cd "+self.config.getBinDir()+" ;env DISPLAY=:0.0 "+"SAGE_APP_CONFIG=" + cf + " " +self.command)
            self.processObj = sp.Popen(args="/usr/bin/ssh -x "+self.targetMachine+" 'cd "+self.config.getBinDir()+" ;env SAGE_APP_CONFIG="+cf+" "+self.command+"'", shell=True)
            #self.processObj = sp.Popen(["/usr/bin/ssh", "-x", self.targetMachine, "cd "+self.config.getBinDir(), ";env SAGE_APP_CONFIG="+os.path.basename(self.configFilename), self.command], shell=True)
        except:
            WriteLog( "===>  ERROR launching application ---------> :")
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return -1
        
        return self.config.getAppId()

        
    def isAlive(self):
        """ returns true if the process is still alive """
        return self.processObj.poll() is None
        

    def kill(self):
        # forcefully kill the application via ssh and delete its pid file if possible
        pidPath = opj(os.path.basename(self.configFilename), "pid")
        killCmd = "/bin/kill -9 `cat "+opj(pidPath, self.config.getAppName()+"_"+str(self.config.getAppId())+".pid")+"`"
        delCmd = "/bin/rm -rf "+opj(pidPath, self.config.getAppName()+"_"+str(self.config.getAppId())+".pid")
        try:
            retcode = sp.Popen(["/usr/bin/ssh", "-x", self.targetMachine, killCmd, ";", delCmd])
            WriteLog( ">>>>  KILLING:  " + killCmd + "\nPID = " + str(self.processObj.pid) + "\n")
        except:
            WriteLog( "===>  ERROR killing application ---------> :")
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return -1
        
        return True


    def deletePIDFile(self):
        # delete the temp file where the app writes its pid
        pidPath = opj(os.path.basename(self.configFilename), "pid")
        delCmd = "/bin/rm -rf "+opj(pidPath, self.config.getAppName()+"_"+str(self.config.getAppId())+".pid")
        try:
            retcode = sp.Popen(["/usr/bin/ssh", "-x", self.targetMachine, delCmd], env={"DISPLAY": ":0.0"})
            WriteLog(">>>>  DELETING:  " + delCmd + "\nPID = " + str(self.processObj.pid) + "\n")
        except:
            WriteLog("===>  ERROR deleting temporary pid file ---------> :")
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
        #    return False
        
        return True
