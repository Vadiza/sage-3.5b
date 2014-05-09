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



# python stuff
import sys, string
import os.path
import os


# my imports
from SAGEApp import SAGEApp, SAGEAppInitial
from sageDisplayInfo import sageDisplayInfo



## Main class to store all the messages returned by SAGE
class sageUIDataInfo:

    #### constructor initializes member values       
    def __init__(self):
        self.hashApps = {}           # all the apps available for running??
        self.hashAppStatusInfo = {}  # apps currently running
        self.displayInfo = sageDisplayInfo()
        self.newAppID = -1
        
    #### Set the sage status
    def setSageStatus(self, appHash) :  

        # loop through the available apps and create SAGEAppInitial objects for all of them
        for appName, configs in appHash.iteritems():
            hashSingleAppInfo = {}
            self.hashApps[appName] = SAGEAppInitial(appName, configs)
            

    #----------------------------------------------------------------------


##     ### Set the possible execution configurations for each app
##     def setSageAppExecConfig(self, data):
##         tokens = string.split( data, '\n', 1 )
##         appName = tokens[0]
##         data = tokens[1] #the rest
##         configList = string.split( data, "config ")

##         del configList[0]  #remove the first item in the list

##         for config in configList:
##             if appName in self.hashApps.keys():
##                 (name, stringConfig) = string.split(config, ":", 1)
##                 self.hashApps[appName].AddConfig(name, stringConfig)


    #----------------------------------------------------------------------
    
    ### sets the display information for the display we are currently connected to
    ### this data is received from the fsManager
    def setSageDisplayInformation(self, data):
        listTokens = string.split(data, '\n')
        tileNumTokens = string.split(listTokens[0], ' ')
        desktopTokens = string.split(listTokens[1], ' ')
        tileConfTokens = string.split(listTokens[2], ' ')

        self.displayInfo.setValues(int(tileNumTokens[0]), int(tileNumTokens[1]), int(tileNumTokens[2]),
                                   int(desktopTokens[0]), int(desktopTokens[1]),
                                   int(tileConfTokens[0]), int(tileConfTokens[1]))

        return


    #----------------------------------------------------------------------
    

    #### Get the new list of z values from SAGE and update local hashes
    def setSageZValue(self, message):
        tokens = string.split(message)
        numZChanges = int(tokens[0])  #the first item that comes in is the number of z changes

        # loop through all the tokens and update the z values of the apps
        for i in range(numZChanges):
            self.setZvalue( int(tokens[i*2+1]), int(tokens[i*2+2]) )

            
    #----------------------------------------------------------------------


    ### returns True if the app is already on top, False otherwise
    def isTopWindow(self, appId):
        if self.getZvalue(appId) == 0:
            return True   # meaning the app was already on top
        else:
            return False
        
        
    #----------------------------------------------------------------------

    
    #### Set the SAGE app status
    #### prints Invalid app ID if does not exists
    def setSageAppInfo(self, stData):
        listTokens = string.split(stData)

        iAppID = int( listTokens[ 1 ] )
        if iAppID in self.hashAppStatusInfo:
            self.hashAppStatusInfo[ iAppID ].setAll( listTokens[0], int(listTokens[1]),
                   int(listTokens[2]), int(listTokens[3]), int(listTokens[4]), int(listTokens[5]),
                   int(listTokens[6]), int(listTokens[7])) 
        else:
            self.hashAppStatusInfo[ iAppID ] = SAGEApp( listTokens[0], int(listTokens[1]),
                   int(listTokens[2]), int(listTokens[3]), int(listTokens[4]), int(listTokens[5]),
                   int(listTokens[6]), int(listTokens[7]))


            

##     ### increase the z value of all the apps (because the new one will be on top)
##     def updateZsAfterAdd(self, appId):
##         for app in self.hashAppStatusInfo.itervalues():
##             app.setZvalue(app.getZvalue() + 1)


    #----------------------------------------------------------------------
    
    
    ##### ShutDown the sage application
    ##### prints invalid app ID if doesn't exist
    def sageAppShutDown(self, stData):
        listTokens = string.split(stData)
        appId = int(listTokens[0])

#        self.updateZsAfterRemove(appId) 

        if appId in self.hashAppStatusInfo :
            del self.hashAppStatusInfo[appId]


##     ### decrease the z value of all the apps that were below the deleted one
##     def updateZsAfterRemove(self, appId):
##         deletedApp = self.hashAppStatusInfo[appId]
##         for app in self.hashAppStatusInfo.itervalues():
##             if app.getZvalue() > deletedApp.getZvalue() and app.getId() != deletedApp.getId():
##                 app.setZvalue(app.getZvalue() - 1)

    #----------------------------------------------------------------------

    
    #   Get the z value of the app
    def getZvalue(self, appInstId):
        if appInstId in self.hashAppStatusInfo:
            appInfo = self.hashAppStatusInfo.get(appInstId)
            return appInfo.getZvalue()
        else:
            #print("getZvalue: Invalid app instance ID")
            return -1

            
    #----------------------------------------------------------------------


    #   Set the Z value of the app
    def setZvalue(self, appId, value):
        if (appId in self.hashAppStatusInfo):
            self.hashAppStatusInfo[appId].setZvalue(value)
        else:
            print ('setZvalue: Invalid app instance ID')
        return -1


    #----------------------------------------------------------------------


    def getSageDisplayInformation(self) :
        return self.displayInfo.getValues()


    #----------------------------------------------------------------------
    
    
    ### Get the sage app Info
    def getAppInfo(self, appId):
        if appId in self.hashAppStatusInfo:
            return {str(appId): self.hashAppStatusInfo[appId].getAll()}
        else:
            return -1


    #----------------------------------------------------------------------


    def getAllAppInfo(self):
        appStatus = {}  #key = appId, value = list of app params
        for appId, sageApp in self.hashAppStatusInfo.iteritems():
            appStatus[str(appId)] = sageApp.getAll()
        return appStatus
    

    #----------------------------------------------------------------------


    # returns the SAGEApp associated with the given appInstId
    def getSAGEApp(self, appId):
        return self.hashAppStatusInfo[appId]

    
    #----------------------------------------------------------------------


    # returns true if the app with a specified appId is currently running
    def appExists(self, appId):
        return appId in self.hashAppStatusInfo
    

    #----------------------------------------------------------------------
 

    ### Get all app IDs currently running on sage
    def getAllAppIDs(self) :
        return self.hashAppStatusInfo.keys()

   
    #----------------------------------------------------------------------

    ### Get the new APP ID
    def getNewAppID(self) :
	return self.newAppID


    #----------------------------------------------------------------------
 
     ### Returns a list of all currently running apps
    def getAllApps( self ):
        return self.hashAppStatusInfo
    

    #----------------------------------------------------------------------
    

    ### Get list of application names that are available for running
    def getAvailableAppNames( self ):
        return self.hashApps.keys()


    #----------------------------------------------------------------------

    
    ### Returns a list of all the available apps
    def getAvailableApps( self ):
        return self.hashApps


    #----------------------------------------------------------------------

    
    ### Returns a list of z values in the following format:
    ### [numChanges, appId, zValue, appId, zValue...]
    def getZvalues( self ):
        apps = self.getAllApps()
        zValues = []  #this will hold the return list of z values
        zValues.append(len(apps))
        for app in apps.itervalues():
            zValues.append(app.getId())
            zValues.append(app.getZvalue())
             
        return zValues




