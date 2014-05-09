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


import string, os, copy, os.path, sys, tempfile
import traceback as tb
from myprint import *   # handles the printing or logging

opj = os.path.join
sys.path.append( opj(os.environ["SAGE_DIRECTORY"], "bin" ) )
from sagePath import getUserPath, SAGE_DIR

## # read the master IP from the fsManager.conf
## f = open(os.environ["SAGE_DIRECTORY"]+"/bin/fsManager.conf", "r")

## FS_IP = f.readline().split()[2].strip()    #fsManager IP
## FS_PORT = f.readline().split()[1].strip()  #system port
## f.close()

     

class OneConfig:
    ''' describes one configuration for an app '''
    
    def __init__(self, name, appName, dynamic=False):
        self._configName = name
        self._dynamic = dynamic     # dynamic config???
        self._appName = appName
        
        self._configFilename = getUserPath("applications", appName+".conf")
        self._launcherId = ""

        self._binDir = "$SAGE_DIRECTORY/bin/"  # where the binary resides - this is where the config is copied to
        self._nodeNum = 1
        self._position = (100, 100)        # initial position of the window on SAGE
        self._size = (-1, -1)          # initial size of the window on SAGE
        self._command = ""                 # the actual command used to start the application
        self._targetMachine = ""           # the render machine where the app will be started
        self._protocol = "TCP"
        self._masterIP = None              # the master machine of a parallel application
        self._fsIP = None                  # which SAGE will the app connect to (if not using sageBridge)
        self._fsPort = None                # which SAGE will the app connect to (if not using sageBridge)
        self._useBridge = False            # if True the app will connect to sageBridge instead of fsManager
        self._bridgeIP = ""                # the machine for sage bridge
        self._bridgePort = ""              # the machine for sage bridge

        self._additionalParams = ""        # any additional parameters you want to specify... used for testing

        # audio stuff
        self._audioFile = ""
        self._framePerBuffer = 512
             
        self._nwID = 1
        self._msgPort = 23010
        self._syncPort = 13010
        self._nodeNum = 1
        self._appId = 0                    # the port number for the app on the render machine
        self._blockSize = (64,64)
        self._blockThreshold = 0
        self._streamType = "SAGE_BLOCK_HARD_SYNC"    # sync mode
        self._staticApp = False             # static applications dont refresh their windows so sage needs to keep the last frame
        self._runOnNodes = False    # if an app has to connect to the outside world or requires
                                    #an SDL/GLUT window for rendering then it can't run on the nodes
        

    def getName(self): return self._configName
    def getAppName(self): return self._appName
    def isDynamic(self): return self._dynamic
    def getConfigFilename(self): return self._configFilename

    # audio stuff
    def setAudioFile(self, f):
        self._audioFile = f
    def getAudioFile(self):
        return self._audioFile
    def setFramePerBuffer(self, fpb):
        self._framePerBuffer = fpb
    def getFramePerBuffer(self):
        return self._framePerBuffer
    

    def setConfigFilename(self):
        appName = self._appName
        if appName == "imageviewer" or appName == "pdfviewer" or appName=="mplayer" or appName=="VNCViewer":
            tf = tempfile.mkstemp(prefix=appName, dir=getUserPath("applications", "conf"))
            self._configFilename = tf[1]

  
    def getLauncherId(self):
        return self._launcherId
    def setLauncherId(self, launcherId):
        self._launcherId = launcherId

    def setBinDir(self, d):
        self._binDir = d
    def getBinDir(self):
        return self._binDir

    def setNodeNum(self, num): self._nodeNum = num
    def getNodeNum(self): return self._nodeNum

    def setPosition(self, pos): self._position = pos
    def getPosition(self): return self._position

    def setSize(self, size): self._size = size
    def getSize(self): return self._size

    def setCommand(self, command): self._command = command
    def getCommand(self): return self._command

    def setTargetMachine(self, target): self._targetMachine = target
    def getTargetMachine(self): return self._targetMachine

    def setProtocol(self, protocol):
        if protocol == "tvTcpModule.so" or protocol=="TCP":
            self._protocol = "TCP"
        else:
            self._protocol = "UDP"
    def getProtocol(self): return self._protocol

    def setMasterIP(self, ip):
        self._masterIP = ip
    def getMasterIP(self):
        return self._masterIP

    def setFSIP(self, ip):
        self._fsIP = ip
    def getFSIP(self):
        return self._fsIP
    
    def setFSPort(self, port):
        self._fsPort = port
    def getFSPort(self):
        return self._fsPort

    def setBridgeIP(self, ip):
        self._bridgeIP = ip
    def getBridgeIP(self):
        return self._bridgeIP

    def setBridgePort(self, port):
        self._bridgePort = port
    def getBridgePort(self):
        return self._bridgePort

    def setUseBridge(self, doUse):
        self._useBridge = doUse
    def getUseBridge(self):
        return self._useBridge

    def setNWId(self, id):
        self._nwID = id
    def getNWId(self):
        return self._nwID

    def setMsgPort(self, port):
        self._msgPort = port
    def getMsgPort(self):
        return self._msgPort

    def setSyncPort(self, port):
        self._syncPort = port
    def getSyncPort(self):
        return self._syncPort

    def setAppId(self, id):
        self._appId = id
    def getAppId(self):
        return self._appId

    def setBlockSize(self, size):
        self._blockSize = size
    def getBlockSize(self):
        return self._blockSize

    def setBlockThreshold(self, threshold):
        self._blockThreshold = threshold
    def getBlockThreshold(self):
        return self._blockThreshold

    def setStreamType(self, mode):
        self._streamType = mode
    def getStreamType(self):
        return self._streamType

    def setStaticApp(self, do):
        self._staticApp = do
    def getStaticApp(self):
        return self._staticApp

    def setRunOnNodes(self, run):
        self._runOnNodes = run
    def getRunOnNodes(self):
        return self._runOnNodes

    def setAdditionalParams(self, param):
        self._additionalParams += param + "\n"
    def getAdditionalParams(self):
        return self._additionalParams

    def writeToFile(self):
        s = ""

        # sage bridge stuff
        if self.getUseBridge():      # using sageBridge
            s += 'bridgeOn true\n'
            s += 'bridgeIP %s\n'% self.getBridgeIP()
            s += 'bridgePort %s\n'% self.getBridgePort()
        else:                        # not using sageBridge
            s += 'bridgeOn false\n'
            
        s += 'fsIP %s\n'% self.getFSIP()
        s += 'fsPort %s\n'% self.getFSPort()
        s += 'masterIP %s\n'% self.getMasterIP()
        s += 'nwID %d\n' % self.getNWId()
        s += 'msgPort %d\n' % self.getMsgPort()
        s += 'syncPort %d\n' % self.getSyncPort()
        s += 'nodeNum %d\n' % self.getNodeNum()
        s += 'appID %d\n' % self.getAppId()
        s += 'launcherID %s\n' % self.getLauncherId()
        s += 'pixelBlockSize %d %d\n' % (self.getBlockSize()[0], self.getBlockSize()[1])
        s += 'blockThreshold %d\n' % self.getBlockThreshold()
        s += 'winX %d\n' % self.getPosition()[0]
        s += 'winY %d\n' % self.getPosition()[1]
        s += 'winWidth %d\n' % self.getSize()[0]
        s += 'winHeight %d\n' % self.getSize()[1]
        s += 'streamType %s\n' % self.getStreamType()
        s += 'nwProtocol %s\n' % self.getProtocol()

        # audio
        if self.getAudioFile():
            s += 'audioOn true\n'
            s += 'audioFile %s\n' % self.getAudioFile()
            s += 'audioType read\ndeviceNum -1\n'
            s += 'framePerBuffer %d\n' % self.getFramePerBuffer()
            
        # static app
        if self.getStaticApp():
            s += 'asyncUpdate true\n'
        else:
            s += 'asyncUpdate false\n'

        # additional params
        s += self.getAdditionalParams()
            
        f = open(self._configFilename, "w")
        f.write(s)
        f.close()


    def getConfigString(self):
        """ returns a tuple of strings: (configName, optionalArgs that the app was started with) """
        return (self.getName(), self.getCommand().split(" ", 1)[1].strip())
    



class AppConfig:
    ''' a collection of all the configurations for an app '''
    
    def __init__(self, appName):
        self._configs = {}   #key=configName, value=OneConfig object
        self._appName = appName

    def getAppName(self):
        return self._appName

    def addConfig(self, oneConfig):
        self._configs[oneConfig.getName()] = oneConfig

    def getConfig(self, configName):
        return self._configs[configName]
    
    def getDefaultConfig(self):
        return self._configs.values()[0]  #return an arbitrary config file

    def getAllConfigs(self):
        return self._configs
    
    def getAllConfigNames(self):
        return self._configs.keys()
    
    def makeConfigFile(self, configName):
        config = self.getConfig(configName)
        config.writeToFile()



    

class Configurations:
    ''' a collection of all applications and their configurations '''
    
    def __init__(self, configFile):
        self._configFile = configFile
        self._lastModTime = None  #last modification time to the config file
        # sageBridge stuff
        self._bridgeIP = None
        self._bridgePort = None

        self._appConfigs = {}   #key=appName, value=AppConfig object
        self._readConfig()
        #self._printConfig()


        # so that we can change the config file without restarting the appLauncher
        # checks the last modification time so that we don't reload unnecessarily
    def reloadConfigFile(self):
        try:
            lastModTime = os.path.getmtime(self._configFile)
            if lastModTime != self._lastModTime:
                self._appConfigs = {}  #empty out the hash
                self._readConfig()
                self._lastModTime = lastModTime
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            
        
    def getConfig(self, appName, configName):   #returns a copy so that it can be safely modified without the destroying what's in the config file
        return copy.deepcopy( self._appConfigs[appName].getConfig(configName) )

    def getDefaultConfig(self, appName):   #returns a copy so that it can be safely modified without destroying what's in the config file
        return copy.deepcopy( self._appConfigs[appName].getDefaultConfig() )   

    def getApp(self, appName):
        return self._appConfigs[appName]

    def getAppList(self):   # returns just the names of the apps
        return self._appConfigs.keys()

    def getBridgeIP(self):
        return self._bridgeIP

    def getBridgePort(self):
        return self._bridgePort

    def _printConfig(self):
        for name, app in self._appConfigs.iteritems():
            print "\n----------------------------------------"
            print "Config For: ", name
            for name, config in app.getAllConfigs().iteritems():
                print "Config: ", name
                print "nodeNum = ", config.getNodeNum()
                print "pos = ", config.getPosition()
                print "size = ", config.getSize()
                print "command = ", config.getCommand()
                print "target = ", config.getTargetMachine()
                print "protocol = ", config.getProtocol()
                print "runOnNodes = ", config.getRunOnNodes()

        print "\n----------------------------------------"
        print "bridgePort = ", self._bridgePort
        print "bridgeIP = ", self._bridgeIP


    def getConfigHash(self):
        """ returns a hash of all the configurations without the objects... just tuples of strings and ints """
        strHash = {}   #keyed by appName, value = a list of configNames
        for appName, app in self._appConfigs.iteritems():
            strHash[appName] = app.getAllConfigNames()
        return strHash
    
                
    def _readConfig(self):
        f = open(self._configFile, "r")
        lines = f.readlines()
        f.close()

        self.appconfig = None
        self.oneconfig = None
        self.lineCounter = 0
        
        for line in lines:
            self.lineCounter += 1

            # allow comments with #
            if '#' in line:
                line = line.split('#')[0].strip()
                
                
            if '{' in line:
                appName = line.replace('{', ' ').strip()
                self.appconfig = AppConfig(appName)
                
            elif 'configName' in line:
                if self.oneconfig:
                    self.appconfig.addConfig(self.oneconfig)
                self.oneconfig = OneConfig(line.lstrip('configName').strip(), self.appconfig.getAppName())

            elif 'nodeNum' in line:
                self.oneconfig.setNodeNum(int(line.lstrip('nodeNum').strip()))

            elif 'Init' in line:
                lineTokens = line.split()
                pos = (int(lineTokens[1]), int(lineTokens[2]))
                size = (int(lineTokens[3]), int(lineTokens[4]))
                self.oneconfig.setPosition(pos)
                self.oneconfig.setSize(size)

            elif 'exec' in line:
                bla, target, command = line.split(' ', 2)
                self.oneconfig.setTargetMachine(target.strip())
                if not self.oneconfig.getMasterIP():   #if it has been set, dont overwrite it
                    self.oneconfig.setMasterIP(target.strip())
                self.oneconfig.setCommand(command.strip())

            elif 'nwProtocol' in line:
                self.oneconfig.setProtocol(line.lstrip('nwProtocol').strip())

            elif 'bridgeIP' in line:
                self.oneconfig.setBridgeIP(line.split()[1].strip())

            elif 'bridgePort' in line:
                self.oneconfig.setBridgePort(line.split()[1].strip())

            elif 'runOnNodes' in line:
                self.oneconfig.setRunOnNodes(True)

            elif 'staticApp' in line:
                self.oneconfig.setStaticApp(True)

            elif 'pixelBlockSize' in line:
                s = line.split()
                self.oneconfig.setBlockSize( (int(s[1].strip()), int(s[2].strip()))  )

            elif 'binDir' in line:
                p = line.split()[1].strip()
                if not p.endswith("/"):
                    p += "/"
                self.oneconfig.setBinDir(p)

            elif 'masterIP' in line:
                self.oneconfig.setMasterIP(line.split()[1].strip())

            elif 'audioFile' in line:
                self.oneconfig.setAudioFile(line.split()[1].strip())

            elif 'framePerBuffer' in line:
                self.oneconfig.setFramePerBuffer(int(line.split()[1].strip()))

            elif 'sync' in line:
                mode = line.split()[1].strip()
                if not mode.startswith("SAGE_BLOCK_"):
                    mode = "SAGE_BLOCK_" + mode
                    
                if mode == "SAGE_BLOCK_NO_SYNC" or \
                   mode == "SAGE_BLOCK_SOFT_SYNC" or \
                   mode == "SAGE_BLOCK_HARD_SYNC":
                    self.oneconfig.setStreamType(mode)
                else:
                    WriteLog("\n*** Invalid streamType mode on line: "+str(self.lineCounter)+". Defaulting to NO_SYNC")

            elif '}' in line:
                self.appconfig.addConfig(self.oneconfig)   #save the last config
                self._appConfigs[self.appconfig.getAppName()] = self.appconfig   #save the appConfig
                self.appconfig = None   #reinitialize everything
                self.oneconfig = None


            elif 'defaultBridgeIP' in line:
                self._bridgeIP = line.split()[1].strip()

            elif 'defaultBridgePort' in line:
                self._bridgePort = line.split()[1].strip()


            elif line in string.whitespace:
                pass
            
            else:    # if line is not recognized
                self.oneconfig.setAdditionalParams(line.strip())
             




        
                                 
    
