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


import sageGateBase as sgb
import wx, socket, xmlrpclib, sys
from globals import *
import traceback as tb



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
                                  forceAppLauncher=getAppLauncher(),
                                  onDisconnect=self.initiateClosedConnectionDialog,
                                  verbose=True)
        

    def showConnectionClosedDialog(self):
        """ informs the user when the connection breaks """
        m = getUsersData().FindMachineByIP(self.sageHost)
        if m: machineName = m.GetName()
        else: machineName = self.sageHost

        msg = "Connection to  <<"+machineName+">>  closed.\nYou will have to reconnect when SAGE becomes available again."
        dlg = wx.MessageDialog(None, msg, "SAGE Connection Closed", style=wx.OK)
	dlg.ShowModal()
	dlg.Destroy()


    def initiateClosedConnectionDialog(self):
        wx.CallAfter(self.showConnectionClosedDialog)


    def executeApp(self, appName, configName="default", pos=False, size=False, optionalArgs="", useBridge=False, sageIP=None, sagePort=None):
	if self.connected == False: return 0
	if not appName: return 0

        # figure out the correct system ports for data streaming (if not overridden by the user)
        if not sageIP:
            machine = getUsersData().FindMachineByIP(self.sageHost)
            if machine:
                sageIP = machine.GetSystemIP()     # the machine told the server what to use for streaming
                if not sagePort: sagePort = machine.GetSystemPort()
            else:
                sageIP = self.sageGate.sageHost    # use the default ip and port
                if not sagePort: sagePort = self.sageGate.sagePort+1

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

        # figure out the correct system ports for data streaming (if not overridden by the user)
        if not sageIP:
            machine = getUsersData().FindMachineByIP(self.sageHost)
            if machine:
                sageIP = machine.GetSystemIP()     # the machine told the server what to use for streaming
                if not sagePort: sagePort = machine.GetSystemPort()
            else:
                sageIP = self.sageGate.sageHost    # use the default ip and port
                if not sagePort: sagePort = self.sageGate.sagePort+1

        # try running the app (return -1 if failed for whatever reason)
	#if launcherId in self.launchers:
            #server = self.launchers[launcherId].getServerHandle()
        try:
            server = xmlrpclib.ServerProxy("http://" + launcherId)
            res = server.startDefaultApp(appName, sageIP, sagePort, useBridge, configName, pos, size, optionalArgs)
        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            print "\nCould not start remote app through appLauncher running on:", launcherId
            return -1
        else:
            return res
        
	

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
            wx.CallAfter(self.hashCallbackFunction[ code ], data)

            
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


    
