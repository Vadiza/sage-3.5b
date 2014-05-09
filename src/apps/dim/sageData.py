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



# python stuff
import string, sys, os.path, os, pickle, xmlrpclib, time, shutil
import traceback as tb

# my imports
from sageApp import SageApp, SageAppInitial
from sageDisplayInfo import SageDisplayInfo
from globals import *
import events


opj = os.path.join


## Main class to store all the messages returned by SAGE
class SageData:


    #### constructor initializes member values       
    def __init__(self, autosave, sg) :
        self.hashApps = {}           # all the apps available for running
        self.hashAppStatusInfo = {}  # apps currently running
        self.displayInfo = SageDisplayInfo()

        # minimize bounds... loaded from a state and to be applied to the sageApp when created
        # key = appName+str(pos+size), value = minimizeBounds
        self.__minBoundsByApp = {}  
        
        self.__firstAutosave = True
        self.autosave = autosave
        
        self.sageGate = sg
        self.sageGate.registerCallbackFunction(40000, self.setSageStatus)
        self.sageGate.registerCallbackFunction(40001, self.setSageAppInfo)
        #self.sageGate.registerCallbackFunction(40002, self.setSagePerfInfo)
        self.sageGate.registerCallbackFunction(40003, self.sageAppShutDown)
        self.sageGate.registerCallbackFunction(40004, self.setDisplayInfo)
        self.sageGate.registerCallbackFunction(40005, self.setSageZValue)
        self.sageGate.registerCallbackFunction(40006, self.setSageAppExecConfig)
        self.sageGate.registerCallbackFunction(40007, self.setDisplayConnections)
        self.sageGate.registerCallbackFunction(40018, self.addOverlay)
        self.sageGate.registerCallbackFunction(40019, self.removeOverlay)
        self.sageGate.registerCallbackFunction(40020, self.onChangedOverlay)


    ### clears the current state (to be done when reconnecting)
    def clear(self):
        self.hashApps = {}           # all the apps available for running??
        self.hashAppStatusInfo = {}
        
        
    #### Set the sage status
    def setSageStatus(self, appHash) :  
        for appName, configs in appHash.iteritems():
            self.hashApps[appName] = SageAppInitial(appName, configs)
            

    #----------------------------------------------------------------------


    ### Set the possible execution configurations for each app
    def setSageAppExecConfig(self, data):
        tokens = string.split( data, '\n', 1 )
        appName = tokens[0]
        data = tokens[1] #the rest
        configList = string.split( data, "config ")

        del configList[0]  #remove the first item in the list

        for config in configList:
            if appName in self.hashApps.keys():
                (name, stringConfig) = string.split(config, ":", 1)
                self.hashApps[appName].AddConfig(name, stringConfig)


    #----------------------------------------------------------------------
 
    
    #### Set the SAGE display information
    #### 
    def setDisplayInfo(self, data):
        listTokens = string.split(data, '\n')

        for i in range(0, len(listTokens), 3):
            tileNumTokens = string.split(listTokens[i], ' ')
            desktopTokens = string.split(listTokens[i+1], ' ')
            tileConfTokens = string.split(listTokens[i+2], ' ')            
            
            # so that we can support the old sage as well
            displayId = 0
            if len(tileConfTokens) > 2:
                displayId = int(tileConfTokens[2])

            self.displayInfo.addDisplay(int(tileNumTokens[0]), int(tileNumTokens[1]), int(tileNumTokens[2]),
                                   int(desktopTokens[0]), int(desktopTokens[1]),
                                   int(tileConfTokens[0]), int(tileConfTokens[1]), displayId)

        # for one display and old SAGE there are no connection messages so just
        # pretend that one came in for drawing everything correctly
        if self.displayInfo.getNumDisplays() == 1:
            self.setDisplayConnections("")
        

    #----------------------------------------------------------------------
    
    # returns the SageDisplayInfo object
    def getDisplayInfo(self, displayId=0):
        return self.displayInfo.getDisplay(displayId)


    #----------------------------------------------------------------------


    def setDisplayConnections(self, data):
        if data:
            for connection in data.split('\n'):
                tokens = connection.split()
                displayId = int(tokens[2])
                self.displayInfo.getDisplay(displayId).placement = int(tokens[1])

        evt = events.DisplayInfoEvent(self.displayInfo)
        getEvtMgr().postEvent(evt)


    #----------------------------------------------------------------------
    

    #### Get the new list of z values from SAGE
    #### and update local hashes, then call the function to update the UI visually
    def setSageZValue(self, message):
        writeLog(LOG_WINDOW_Z_CHANGE, message)
        tokens = string.split(message)
        numZChanges = int(tokens[0])  #the first item that comes in is the number of z changes
        zHash = {}   # key=windowId, value=new z value
        
        # loop through all the tokens and update the z values of the apps
        for i in range(numZChanges):
            self.setZvalue( int(tokens[i*2+1]), int(tokens[i*2+2]) )
            zHash[int(tokens[i*2+1])] = int(tokens[i*2+2])
            
        evt = events.ZChangeEvent(zHash)
        getEvtMgr().postEvent(evt)


    #----------------------------------------------------------------------

    
    #### Set the SAGE app status
    #### prints Invalid app ID if does not exists
    def setSageAppInfo(self, data):
        listTokens = string.split(data)

        # to support old sage as well
        displayId = 0
        orientation = 0
        appId = 0
        launcherId = "none"
        resX = -1
        resY = -1
        myMsg = False

        appName = listTokens[0]
        windowId = int( listTokens[ 1 ] )
        left = int( listTokens[ 2 ] )
        right = int( listTokens[ 3 ] )
        bottom = int( listTokens[ 4 ] )
        top = int( listTokens[ 5 ] )
        sailId = int( listTokens[ 6 ] )
        zValue = int( listTokens[ 7 ] )

        if len(listTokens) > 8:
            orientation = int(listTokens[8])
            displayId = int(listTokens[9])
            if len(listTokens) > 11:
                appId = int(listTokens[10])
                launcherId = listTokens[11]
                if len(listTokens) > 13:
                    resX = int(listTokens[12])
                    resY = int(listTokens[13])
                    if len(listTokens) > 14:
                        myMsg = bool(int(listTokens[14]))

        newApp = None
     
            # ------  EXISTING APP  --------#
        if windowId in self.hashAppStatusInfo:
            writeLog(LOG_WINDOW_CHANGE, data)
            
            # update the app in the hash 
            self.hashAppStatusInfo[ windowId ].setAll( appName, windowId, left, 
                                                       right, bottom, top, sailId, 
                                                       zValue, orientation, displayId, 
                                                       appId, launcherId)
            # make the event
            evt = events.AppInfoEvent(self.hashAppStatusInfo[windowId], myMsg)

            # ------ NEW APP -------#
        else:
            extra = ""
            if appName.lower().startswith("vnc"):
                if launcherId != "none":
                    appLauncher = xmlrpclib.ServerProxy("http://" + launcherId)
                    try:
                        res = appLauncher.getAppConfigInfo( appId )
                        if res != -1:
                            extra = "ARGS "+res[1]
                    except:
                        extra = ""

            writeLog(LOG_WINDOW_NEW, data, extra)
            # when new app is started it is assigned a z=0 but since previous app on top had z=0,
            # we set this one even higher temporarily (z=-1) so that it gets drawn on top
            # the new z order message comes right after the app is started so this -1 is temporary

            for app in self.hashAppStatusInfo.itervalues():
                if app.getZvalue() == zValue:
                    zValue = -1
                    
            newApp = SageApp( appName, windowId, left, right, bottom, top, sailId, 
                              zValue, orientation, displayId, appId, launcherId, resX, resY)

            self.hashAppStatusInfo[ windowId ] = newApp

            # make the event
            evt = events.NewAppEvent(newApp)

        # post the event about the state change
        getEvtMgr().postEvent(evt)

        if newApp:
            # set its minimized bounds... if available and applicable...
            p = newApp.getPos()
            s = newApp.getSize()
            appname = newApp.getName()
            minBoundsKey = appName+str(p+s)
            if minBoundsKey in self.__minBoundsByApp:
                newApp._setMinimizedBounds(self.__minBoundsByApp[minBoundsKey])
                del self.__minBoundsByApp[minBoundsKey]
                

        if self.autosave:
            if self.__firstAutosave:
                self.__deleteOldAutosaves()
                self.__firstAutosave = False
            self.saveState("_autosave_LATEST", "")


    #----------------------------------------------------------------------
    
    
    ##### ShutDown the sage application
    ##### prints invalid app ID if doesn't exist
    def sageAppShutDown(self, data):
        writeLog(LOG_WINDOW_REMOVE, data)
        listTokens = string.split(data)
        windowId = int(listTokens[0])

        if windowId in self.hashAppStatusInfo :
            # do this first and then remove the app from the hash!!
            # make and post the event
            evt = events.AppKilledEvent(self.hashAppStatusInfo[windowId])
            getEvtMgr().postEvent(evt)

            # delete the app
            del self.hashAppStatusInfo[windowId]

        if self.autosave:
            if self.__firstAutosave:
                self.__deleteOldAutosaves()
                self.__firstAutosave = False
            self.saveState("_autosave_LATEST", "")


    #----------------------------------------------------------------------


    def addOverlay(self, data):
        evt = events.ObjectInfoEvent(data)
        getEvtMgr().postEvent(evt)


    #----------------------------------------------------------------------


    def removeOverlay(self, data):
        evt = events.ObjectRemovedEvent(data)
        getEvtMgr().postEvent(evt)


    #----------------------------------------------------------------------


    def onChangedOverlay(self, data):
        evt = events.ObjectChangedEvent(data)
        getEvtMgr().postEvent(evt)


    #----------------------------------------------------------------------
    
    
    #   Get the z value of the app
    #   @arg appInstID App instance ID of the app
    #   @return Returns Z value if appID exists else returns -1
    def getZvalue(self, windowId):
        if windowId in self.hashAppStatusInfo:
            return self.hashAppStatusInfo[windowId].getZvalue()
        else:
            print("getZvalue: Invalid app instance ID")
            return

            
    #----------------------------------------------------------------------


    #   Set the Z value of the app
    #   @arg appInstId App instance ID of the app
    #   @arg value New Z value to be set
    def setZvalue(self, windowId, value):
        if (windowId in self.hashAppStatusInfo):
            self.hashAppStatusInfo[windowId].setZvalue(value)
        else:
            print ('setZvalue: Invalid app instance ID')
        return


    #----------------------------------------------------------------------


    # returns the SageApp associated with the given appInstId
    def getApp(self, appInstId):
        if appInstId in self.hashAppStatusInfo:
            return self.hashAppStatusInfo[appInstId]
        else:
            return None
    

    #----------------------------------------------------------------------

    
    ### (RJ 2005-01-15)
    ### Returns a list of all the available apps
    def getAvailableApps( self ):
        return self.hashApps

    ### (AKS 2004-10-23)
    ### Get all app IDs
    ### currently running on sage
    ### @return list of app ids
    def getAllAppIDs(self) :
        return self.hashAppStatusInfo.keys()


    #----------------------------------------------------------------------

    
    ### Returns a hash of all running apps
    def getRunningApps( self ):
        return self.hashAppStatusInfo


    #----------------------------------------------------------------------


    # checks all the apps and reports whether any one of them was
    # hit with a click and which region was hit (corners or shape in general)
    # if more than one shape was hit it returns the one on the top
    def checkHits(self, x, y):
        zHash = {}  #key=z value, value=SageApp

        try:
            for windowId, sageApp in self.hashAppStatusInfo.items():
                zHash[sageApp.getZvalue()] = sageApp

            zKeys = zHash.keys()
            zKeys.sort()

            for z in zKeys:
                hitResult = zHash[z].hitTest(x,y)
                if hitResult >= 0:
                    return (zHash[z], hitResult)
        except:
            pass
        return (None, -1)


    #----------------------------------------------------------------------


    # returns (windowId, zValue) or (-1,sys.maxint) if no apps
    def getTopApp(self):
        minZ = (-1, sys.maxint)
        try:
            for windowId, app in self.hashAppStatusInfo.items():
                if app.getZvalue() < minZ[1]:
                    minZ = (windowId, app.getZvalue())
        except:
            pass
        return minZ
        


    #----------------------------------------------------------------------


    def saveState(self, stateName, description=""):
        
        #appLaunchers = self.sageGate.getLaunchers()
        appList = []

        # sort the apps by z first...
        appsByZ = {}  # key=z, value=SageApp obj
        for app in self.hashAppStatusInfo.values():
            appsByZ[app.getZvalue()] = app
        keys = appsByZ.keys()
        keys.sort(reverse=True)

        # gather all the data that needs to be saved for each app
        for zValue in keys: 
            app = appsByZ[zValue]

            # get the config info from the right appLauncher
            if app.getLauncherId() != "none":
                appLauncher = xmlrpclib.ServerProxy("http://" + app.getLauncherId())
                try:
                    res = appLauncher.getAppConfigInfo( app.getAppId() )
                except:
                    print "\nUnable to connect to appLauncher on", app.getLauncherId(), \
                          "so not saving this app: ", app.getName() 
                    continue
                
                if res == -1:
                    print "	failed to save", app.getLauncherId()
                    continue   # skip this app... something went wrong
                configName, optionalArgs = res
                
                # get the other app parameters from sageApp object
                pos = app.getPos()
                size = app.getSize()
                minBounds = app._getMinimizedBounds()

                # append the tuple of app's data to the list that will be saved
                appList.append( (app.getLauncherId(), app.getName(), configName,
                                 pos, size, optionalArgs, minBounds) )


        # now save the sections too
        sectionState = getLayoutSections().getState()
        
        # open the file and write to it
        try:   #in case the file and directory permissions are not right
            f = open(opj(SAVED_STATES_DIR, stateName+".state"), "w")
            pickle.Pickler(f, 0).dump( (description, appList, sectionState) )
            f.close()
        except IOError:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            return False

        return True


    def __deleteOldAutosaves(self):
        # remove old autosave files...
        try:
            saves = os.listdir(SAVED_STATES_DIR)
            if "_autosave_PREV.state" in saves:
                os.remove(opj(SAVED_STATES_DIR, "_autosave_PREV.state"))
            if "_autosave_LATEST.state" in saves:
                shutil.move(opj(SAVED_STATES_DIR, "_autosave_LATEST.state"), opj(SAVED_STATES_DIR, "_autosave_PREV.state"))
        except:
            print "ERROR while deleting old states:"
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))


    def loadState(self, stateName):
        """ tries to reload the apps from the saved state """
        writeLog(LOG_STATE_LOAD)
        
        self.sageGate.updateLauncherList()
        appList = []
        description = ""

        # load the state from a file
        try:
            f = open(opj(SAVED_STATES_DIR, stateName+".state"), "r")
            savedStuff = pickle.Unpickler(f).load()
            if len(savedStuff) == 2:
                (description, appList) = savedStuff
                sectionState = None
            else:
                (description, appList, sectionState) = savedStuff
                print "Loading state: description ", description
                print "Loading state: appList ", appList
                print "Loading state: sectionState ", sectionState
            f.close()
        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            print "\nUnable to read saved state file: "+stateName+".state"
            return False


        # first, load all the sections
        getLayoutSections().loadState(sectionState)

        # try re-running all the apps
        count = 0
        for appInfo in appList: 
            if len(appInfo) == 6:
                launcherId, appName, configName, pos, size, optionalArgs = appInfo
            elif len(appInfo) == 7:
                launcherId, appName, configName, pos, size, optionalArgs, minBounds = appInfo
                self.__minBoundsByApp[appName+str(pos+size)] = minBounds  # a hack to save the mininmize bounds and apply them to the appInfo when it comes back from sage

            self.sageGate.executeRemoteApp(launcherId, appName, configName, pos, size, optionalArgs)
            time.sleep(2)
            count+=1
            
        return True



    def deleteState(self, stateName):
        """ tries to delete an existing state """
        try:
            filePath = opj(SAVED_STATES_DIR, stateName+".state")
            os.remove(filePath)
        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            print "\nUnable to delete the saved state: ", filePath
            return False

        return True

            

    def getStateList(self):
        """ returns a hash of key=stateName, value=description """
        stateHash = {}
        appList = []
        description = ""
        
        
        # load all the states and read descriptions from them
        for fileName in os.listdir(SAVED_STATES_DIR):
            filePath = opj(SAVED_STATES_DIR, fileName)
            if os.path.isfile(filePath) and os.path.splitext(filePath)[1] == ".state":
                try:
                    stateName = os.path.splitext( os.path.split(filePath)[1] )[0]
                    f = open(filePath, "rb")

                    # allow loading of older style files that didn't have sections
                    savedStuff = pickle.Unpickler(f).load()
                    if len(savedStuff) == 2:
                        (description, appList) = savedStuff
                        sectionState = None
                    else:
                        (description, appList, sectionState) = savedStuff
                    f.close()

                    stateHash[stateName] = description
                except:
                    print "\nUnable to read saved state file: "+filePath
                    continue

        return stateHash


