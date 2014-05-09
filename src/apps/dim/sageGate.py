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


import sageGateBase as sgb
from globals import *


# from sageGate import SageGate
from sageData2 import SageData2
GATES = {}
DATAS = {}

def addGate(h):
    global GATES, DATAS
    if not GATES.has_key( h ):
        # create a new connection
        rgate = SageGate()
        rdata = SageData2(rgate)
        rgate.connectToSage(h) # using default port number
        rgate.registerSage("share")
        GATES[h] = rgate
        DATAS[h] = rdata

def getGate(h):
    global GATES, DATAS
    if not GATES.has_key( h ):
        addGate(h)
    return GATES[h],DATAS[h]

####################################################################
#
# DESCRIPTION: This class provides the main communication interface
#              with SAGE. It subclasses the SageGateBase in order
#              to customize some parameters.
#
# DATE: Aug, 2007
#
####################################################################

class SageGate(sgb.SageGateBase):
    """ this class provides the main communication interface with SAGE
        subclasses the SageGateBase in order to customize some parameters
    """
    
    def __init__(self):
        sgb.SageGateBase.__init__(self,
                                  sageServerHost=getSAGEServer(),
                                  useAppLauncher=True,
                                  forceAppLauncher="localhost:19010",
                                  onDisconnect=self.showConnectionClosedDialog,
                                  verbose=False)
        

    def showConnectionClosedDialog(self):
        """ informs the user when the connection breaks """
        print "Connection to SAGE closed"
        exitApp()


##     def executeApp(self, appName, configName="default", pos=False, size=False, optionalArgs="", useBridge=False, sageIP=None, sagePort=None):
##         """ overridden in order to figure out the correct port and host for the target machine
##             The reason is that SageGateBase doesn't have access to all the machine info (ie usersData)
##         """

##         # figure out the correct system ports for data streaming (if not overridden by the user)
##         sageIP, sagePort = self.__getMachineInfo(sageIP, sagePort)
        
##         return sgb.SageGateBase.executeApp(self, appName, configName, pos, size,
##                                            optionalArgs, useBridge, sageIP, sagePort)


##     def executeRemoteApp(self, launcherId, appName, configName="default", pos=False, size=False, optionalArgs="", useBridge=False, sageIP=None, sagePort=None):
##         """ overridden in order to figure out the correct port and host for the target machine
##             The reason is that SageGateBase doesn't have access to all the machine info (ie usersData)
##         """

##         # figure out the correct system ports for data streaming (if not overridden by the user)
##         sageIP, sagePort = self.__getMachineInfo(sageIP, sagePort)
        
##         return sgb.SageGateBase.executeRemoteApp(self, launcherId, appName, configName, pos, size,
##                                                  optionalArgs, useBridge, sageIP, sagePort)
        

    def onMessage(self, code, data):
        """ handles the incoming messages """

        if code in self.hashCallbackFunction:
            self.hashCallbackFunction[ code ](data)

            
##     def __getMachineInfo(self, sageIP, sagePort):
##         if not sageIP:
##             machine = getUsersData().FindMachineByIP(self.sageHost)
##             if machine:
##                 sageIP = machine.GetSystemIP()     # the machine told the server what to use for streaming
##                 if not sagePort: sagePort = machine.GetSystemPort()
##             else:
##                 sageIP = self.sageHost    # use the default ip and port
##                 if not sagePort: sagePort = self.sagePort+1
##         return (sageIP, sagePort)


    
