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



# python stuff
import sys, string, pickle, time, os.path, os, xmlrpclib, shutil
import traceback as tb

# my imports
from sageApp import SageApp, SageAppInitial
from sageAppPerfInfo import sageAppPerfInfo
from sageDisplayInfo import SageDisplayInfo
import Graph
from globals import *
from sagePath import getUserPath


## Main class to store all the messages returned by SAGE
class SageData:


    #### constructor initializes member values       
    def __init__(self, sageGate, autosave, displayName):
        self.noOfApps = 0
        self.hashCallback = {}       # functions to call in sageui.py for updating the UI
        self.hashApps = {}           # all the apps available for running??
        self.hashAppStatusInfo = {}  # apps currently running
        self.hashAppPerfInfo = {}
        self.hashFileInfo = {}
        self.displayInfo = SageDisplayInfo()
        self.sageGate = sageGate
        self.autosave = autosave
        self.timeStarted = time.strftime("%Y%m%d-%H%M%S", time.localtime())
        self.displayName = displayName.strip()
        self.__firstAutosave = True
        
        self._sageColor = (0,0,0)
        self.__bPerformanceLogging = True

        # (AKS 2005-02-15) The hash below is used for determining when to
        # calculate the total values for all active applications.  This
        # hash originally belonged in GraphManager of Graph.py.  However,
        # the implementation was not clean and lent itself to infinite
        # recursion.  So, having the total calculation at the source of
        # where the data is kept seemed to make the most sense.
        #self.__hashAppGraphUpdateFlag = {}


        # Constants used for keys in __hashAppPerfTotals (public scope)
        self.I_RENDER_TOTAL_BANDWIDTH = 10
        self.I_RENDER_AVG_FRAME_RATE = 20
        self.I_RENDER_TOTAL_NODES = 30
        self.I_DISPLAY_TOTAL_BANDWIDTH = 40
        self.I_DISPLAY_AVG_FRAME_RATE = 50
        self.I_DISPLAY_TOTAL_NODES = 60

        self.__hashAppPerfTotals = {}
        self.__hashAppPerfTotals[ self.I_RENDER_TOTAL_BANDWIDTH ] = 0.0
        self.__hashAppPerfTotals[ self.I_RENDER_AVG_FRAME_RATE ] = 0.0
        self.__hashAppPerfTotals[ self.I_RENDER_TOTAL_NODES ] = 0
        self.__hashAppPerfTotals[ self.I_DISPLAY_TOTAL_BANDWIDTH ] = 0.0
        self.__hashAppPerfTotals[ self.I_DISPLAY_AVG_FRAME_RATE ] = 0.0
        self.__hashAppPerfTotals[ self.I_DISPLAY_TOTAL_NODES ] = 0
        
        # for knowing when to zero out the totals since
        # sage doesnt send perf data when it's 0
        self.__lastTotalsUpdate = time.time()  

        # (AKS 2005-05-07) Create two PerformanceGraphs (one for total render bandwidth
        # and one for total display bandwidth).  This is a hack...2 is specified in
        # the GraphManager class not for updating purposes but so that the x-axis values
        # are computed correctly.  The created numarray object takes the place of what
        # sageAppPerfInfo does (well, it's adjusted manually later)
        self.__iMaxArraySize = 30
        self.__iUpdateInterval = 2

        self.__pgTotalRenderBandwidth = Graph.PerformanceGraph( "Totals", "Render Bandwidth (Mbps)",
            self.__iMaxArraySize, self.__iUpdateInterval )

        self.__pgTotalDisplayBandwidth = Graph.PerformanceGraph( "Totals", "Display Bandwidth (Mbps)",
            self.__iMaxArraySize, self.__iUpdateInterval )
        self.__sapiPerfTotals = sageAppPerfInfo()

        
    #### Set the sage status
    def setSageStatus(self, appHash) :  
        self.noOfApps = len(appHash) 

        #for line in listTokens:
        for appName, configs in appHash.iteritems():
            hashSingleAppInfo = {}
            self.hashApps[appName] = SageAppInitial(appName, configs)
            
        # when done processing the incoming data, call a function in the UI to update the screen
        if ( 40000 in self.hashCallback ):
            self.hashCallback[ 40000 ]()



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

        if ( 40006 in self.hashCallback ):
            self.hashCallback[ 40006 ](self.hashApps[appName])
       

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

        # (AKS 2004-10-23) Provide the app-id to the callback
        if ( 40004 in self.hashCallback ):
            self.hashCallback[ 40004 ]()


    #----------------------------------------------------------------------


    # returns the SageDisplayInfo object
    def getDisplayInfo(self, displayId=0):
        return self.displayInfo


    #----------------------------------------------------------------------


    def setDisplayConnections(self, data):
        for connection in data.split('\n'):
            tokens = connection.split()
            displayId = int(tokens[2])
            self.displayInfo.getDisplay(displayId).placement = int(tokens[1])

        if ( 40007 in self.hashCallback ):
            self.hashCallback[ 40007 ]()    


    #----------------------------------------------------------------------
    

    #### Get the new list of z values from SAGE
    #### and update local hashes, then call the function to update the UI visually
    def setSageZValue(self, message):
        tokens = string.split(message)
        numZChanges = int(tokens[0])  #the first item that comes in is the number of z changes
        
        # loop through all the tokens and update the z values of the apps
        for i in range(numZChanges):
            self.setZvalue( int(tokens[i*2+1]), int(tokens[i*2+2]) )

        # now call the appropriate function to update the UI visually
        if ( 40005 in self.hashCallback ):
            self.hashCallback[ 40005 ]( )
          
        
    #----------------------------------------------------------------------

    
    #### Set the SAGE app status
    #### prints Invalid app ID if does not exists
    def setSageAppInfo(self, data):
        listTokens = string.split(data)
        listApps = self.hashApps.keys()

        # to support old sage as well
        displayId = 0
        orientation = 0
        appId = 0
        launcherId = "none"
        if len(listTokens) > 8:
            orientation = int(listTokens[8])
            displayId = int(listTokens[9])
            if len(listTokens) > 11:
                appId = int(listTokens[10])
                launcherId = listTokens[11]
                    
        # now update the app properties... or create a new one
        windowId = int( listTokens[ 1 ] )
        if windowId in self.hashAppStatusInfo:   # app exists
            self.hashAppStatusInfo[ windowId ].setAll( listTokens[0], int(listTokens[1]),
                   int(listTokens[2]), int(listTokens[3]), int(listTokens[4]), int(listTokens[5]),
                   int(listTokens[6]), int(listTokens[7]), orientation, displayId, appId, launcherId) 
        else:
            # when new app is started it is assigned a z=0 but since previous app on top had z=0,
            # we set this one even higher temporarily (z=-1) so that it gets drawn on top
            # the new z order message comes right after the app is started so this -1 is temporary
            zValue = int(listTokens[7])
            for app in self.hashAppStatusInfo.itervalues():
                if app.getZvalue() == zValue:
                    zValue = -1
            self.hashAppStatusInfo[ windowId ] = SageApp( listTokens[0], int(listTokens[1]),
                   int(listTokens[2]), int(listTokens[3]), int(listTokens[4]), int(listTokens[5]),
                   int(listTokens[6]), zValue, orientation, displayId, appId, launcherId) 


        # (AKS 2004-10-23) Provide the app-id to the callback
        if ( 40001 in self.hashCallback ):
            self.hashCallback[ 40001 ]( self.hashAppStatusInfo[windowId] )

        if self.autosave:
            if self.__firstAutosave:
                self.__deleteOldAutosaves()
                self.__firstAutosave = False
            self.saveState("_autosave_LATEST", "")

        return

    #----------------------------------------------------------------------
    
    
    ##### ShutDown the sage application
    ##### prints invalid app ID if doesn't exist
    def sageAppShutDown(self, data):
        listTokens = string.split(data)
        windowId = int(listTokens[0])
        listApps = self.hashApps.keys()


        # do this first and then remove the app from the hash!!
        if ( 40003 in self.hashCallback ):
            self.hashCallback[ 40003 ]( self.hashAppStatusInfo[windowId]  )
            
        if windowId in self.hashAppStatusInfo :
            del self.hashAppStatusInfo[windowId]
        if windowId in self.hashAppPerfInfo :
            del self.hashAppPerfInfo[windowId]

        if windowId in self.hashFileInfo :
            fileObject = self.hashFileInfo.get(windowId)
            fileObject.close()
            del self.hashFileInfo[windowId]
            

    #----------------------------------------------------------------------

    def getLogFileHash(self):
        return self.hashFileInfo


    #----------------------------------------------------------------------

       
    #### Set the SAGE app performance status
    #### prints Invalid app ID if does not exists
    def setSagePerfInfo(self, data):
        #print ">>> SET SAGE PERF INFO <<<"
        listTokens = string.split(data, '\n', 1)
        windowId = int(listTokens[0])
        data = listTokens[1]

        
        if not windowId in self.hashAppPerfInfo:
            self.hashAppPerfInfo[windowId] = sageAppPerfInfo()

        appPerfInfo = self.hashAppPerfInfo.get(windowId)
        
        if (appPerfInfo):
            lineTokens = string.split(data, '\n')

            displayItemTokens = string.split(lineTokens[0])
            appPerfInfo.setDisplayPerfInfo(float(displayItemTokens[1]), float(displayItemTokens[2]),\
                                           float(displayItemTokens[3]), int(displayItemTokens[4]))

            renderItemTokens = string.split(lineTokens[1])
            # FIX: this is just a hack for now.. there actually is no data coming in for the last
            # two entries in the array but we fill it with 0s so that we dont have to change everything
            # (it might be used later as well)
            # in Graph.py we jsut decide not to print the last two values
            #renderItemTokens.append(0.0)
            #print "renderItemTokens = ", renderItemTokens
            renderItemTokens.append(0)
            appPerfInfo.setRenderPerfInfo(float(renderItemTokens[1]), float(renderItemTokens[2]),\
                                           float(renderItemTokens[3]), int(renderItemTokens[4]))


        # Now open a file and log the data on it
            try:   #in case the file and directory permissions are not right

                if not windowId in self.hashFileInfo:
                    sageApp = self.hashAppStatusInfo[ windowId ]
                    stAppName = sageApp.getName()
                    stDateTime = time.strftime("%Y%m%d-%H%M%S", time.localtime())
                    stFilename = stAppName + '-' + str(windowId) + '-' + stDateTime
                    
                    stPath = opj(DATA_DIR, stFilename + ".txt")
                    stFilename = os.path.normpath( stPath )
                    fileObject = open(stFilename, "w")

                    fileObject.write( stAppName + ":" + str(windowId) + " >> " + time.asctime() + "\n" )
                    fileObject.write( '-' * 120 + "\n\n" )
                    tempString = (' Disp BW    Disp FR    Packet Loss    Num Receivers        Rend BW    Rend FR    Packet Loss    Num Receivers\n')
                    fileObject.write(tempString)
                    fileObject.write( '-' * len(tempString) + "\n" )
                    self.hashFileInfo[windowId] = fileObject
                    fileObject.flush()
                # end of initialization

                if ( self.__bPerformanceLogging == True ):

                    fileObject = self.hashFileInfo.get(windowId)
                    tempString = "%8.3f    %7.3f    %3.2f    %8d       " % (float(displayItemTokens[1]), float(displayItemTokens[2]),
                                                                            float(displayItemTokens[3]), int(displayItemTokens[4]))

                    fileObject.write(tempString)

                    tempString = "%8.3f    %7.3f    %3.2f    %8d\n" % (float(renderItemTokens[1]), float(renderItemTokens[2]),
                                                                       float(renderItemTokens[3]), int(renderItemTokens[4]))
                    fileObject.write(tempString)
                    fileObject.flush()
            # >>> end file writing...else, nothing
            except:
                pass  #do nothing if something fails (such as permissions)

                
            # calculate totals
        self.__hashAppPerfTotals[ self.I_RENDER_TOTAL_BANDWIDTH ] = 0.0
        self.__hashAppPerfTotals[ self.I_RENDER_AVG_FRAME_RATE ] = 0.0
        self.__hashAppPerfTotals[ self.I_RENDER_TOTAL_NODES ] = 0
        self.__hashAppPerfTotals[ self.I_DISPLAY_TOTAL_BANDWIDTH ] = 0.0
        self.__hashAppPerfTotals[ self.I_DISPLAY_AVG_FRAME_RATE ] = 0.0
        self.__hashAppPerfTotals[ self.I_DISPLAY_TOTAL_NODES ] = 0
        
        fSumRenderFrameRate = 0.0
        fSumDisplayFrameRate = 0.0
        
        #print "zeroing..."

        # (AKS 2005-04-05) This comment is de
        # For each application, I am taking each of its metrics and adding them to the
        # totals which are stored in the hash.  The hash is just a dictionary of totals
        # for metrics for *ALL* application instances.
        
        for iKey in self.hashAppPerfInfo:
            #print "hash perf: ", self.hashAppPerfInfo
            sapiAppStats = self.hashAppPerfInfo[ iKey ]
            
            self.__hashAppPerfTotals[ self.I_RENDER_TOTAL_BANDWIDTH ] = self.__hashAppPerfTotals[
                self.I_RENDER_TOTAL_BANDWIDTH ] + sapiAppStats.getRenderInformation(
                'bandWidth', 1 )[0]
            
            fSumRenderFrameRate = fSumRenderFrameRate + sapiAppStats.getRenderInformation(
                'frameRate', 1 )[0]
            
            self.__hashAppPerfTotals[ self.I_RENDER_TOTAL_NODES ] = self.__hashAppPerfTotals[
                self.I_RENDER_TOTAL_NODES ] + sapiAppStats.getRenderInformation(
                'nodes', 1 )[0]
            
            self.__hashAppPerfTotals[ self.I_DISPLAY_TOTAL_BANDWIDTH ] = self.__hashAppPerfTotals[
                self.I_DISPLAY_TOTAL_BANDWIDTH ] + sapiAppStats.getDisplayInformation(
                'bandWidth', 1 )[0]
            
            fSumDisplayFrameRate = fSumRenderFrameRate + sapiAppStats.getDisplayInformation(
                'frameRate', 1 )[0]             
            
            self.__hashAppPerfTotals[ self.I_DISPLAY_TOTAL_NODES ] = self.__hashAppPerfTotals[
                self.I_DISPLAY_TOTAL_NODES ] + sapiAppStats.getDisplayInformation(
                'nodes', 1 )[0]

            
            # Make sure to clear update flag since data has been processed
            #self.__hashAppGraphUpdateFlag[ iKey ] = False
            
        # end of loop

        iAppCount = len( self.hashAppStatusInfo )

        # Calculate averages for frame rates
        if ( iAppCount == 0 ):
            self.__hashAppPerfTotals[ self.I_RENDER_AVG_FRAME_RATE ] = 0.0
            self.__hashAppPerfTotals[ self.I_DISPLAY_AVG_FRAME_RATE ] = 0.0
                
        else:
            self.__hashAppPerfTotals[ self.I_DISPLAY_AVG_FRAME_RATE ] = fSumDisplayFrameRate / iAppCount
            self.__hashAppPerfTotals[ self.I_RENDER_AVG_FRAME_RATE ] = fSumRenderFrameRate / iAppCount
                    
        
        # (AKS 2005-01-24) Performance data information is not posted at this time
        #tempString = "%8.3f \t %4d \t %7.3f \n" % (float(dataItemTokens[1]), int(dataItemTokens[2]), float(dataItemTokens[3]))
        #fileObject.write(tempString)


        # (AKS 2005-05-07) Now that the total bandwidth metrics have been calculated,
        # update their respective PerformanceGraphs
        self.__sapiPerfTotals.setDisplayPerfInfo( float( self.__hashAppPerfTotals[
            self.I_DISPLAY_TOTAL_BANDWIDTH ] ), 0.0, 0.0, 0 )
        
        self.__sapiPerfTotals.setRenderPerfInfo( float( self.__hashAppPerfTotals[
            self.I_RENDER_TOTAL_BANDWIDTH ] ), 0.0, 0.0, 0 )

        self.__pgTotalRenderBandwidth.update( self.__sapiPerfTotals.getRenderInformation( 'bandWidth', 30 ) )
        self.__pgTotalDisplayBandwidth.update( self.__sapiPerfTotals.getDisplayInformation( 'bandWidth', 30 ) )


        self.__lastTotalsUpdate = time.time()
        
        # (AKS 2004-10-23): Changed to send ID back
        if ( 40002 in self.hashCallback ):
            self.hashCallback[ 40002 ]( windowId )


    #----------------------------------------------------------------------

    # saves the performance data totals into a file from this SAGE site only
    def saveSiteTotals(self, siteName):
        totalsID = -10 # save the totals as appId = -10
        try:
            if not totalsID in self.hashFileInfo and self.__bPerformanceLogging:  
                stDateTime = time.strftime("%Y%m%d-%H%M%S", time.localtime())
                stFilename = "SITE_TOTAL-" + siteName + '-' + stDateTime
                #stPath = "./data/" + stFilename + ".txt"
                stPath = opj(DATA_DIR, stFilename + ".txt")
                stFilename = os.path.normpath( stPath )
                fileObject = open(stFilename, "w")

                fileObject.write( siteName + "\n" + time.asctime() + "\n" )
                fileObject.write( '-' * 65 + "\n" )
                tempString = (' Timestamp(s)    Disp BW(Gbps)    Rend BW(Gbps)\n')
                fileObject.write(tempString)
                fileObject.write( '-' * len(tempString) + "\n" )
                self.hashFileInfo[totalsID] = fileObject
                fileObject.flush()
            # end of initialization
            
            if ( self.__bPerformanceLogging ):
                
                fileObject = self.hashFileInfo[totalsID]
                tempString = "%12d    %12.4f    %12.4f\n" % (getTimeStamp(),
                                                            self.getDisplayBWTotal(),
                                                            self.getRenderBWTotal())
                fileObject.write(tempString)
                fileObject.flush()

        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
        

    #----------------------------------------------------------------------


    def getRenderBWTotal(self):
        # zero out the totals if it hasn't been update in a while
        if time.time() - self.__lastTotalsUpdate > 2.0: 
            self.__hashAppPerfTotals[self.I_RENDER_TOTAL_BANDWIDTH] = 0.0
        return float(self.__hashAppPerfTotals[self.I_RENDER_TOTAL_BANDWIDTH])/1000.0


    def getDisplayBWTotal(self):
        # zero out the totals if it hasn't been update in a while
        if time.time() - self.__lastTotalsUpdate > 2.0: 
            self.__hashAppPerfTotals[self.I_DISPLAY_TOTAL_BANDWIDTH] = 0.0
        return float(self.__hashAppPerfTotals[self.I_DISPLAY_TOTAL_BANDWIDTH])/1000.0
    

    #----------------------------------------------------------------------
    
    #   Get the z value of the app
    #   @arg windowId App instance ID of the app
    #   @return Returns Z value if windowId exists else returns -1
    def getZvalue(self, windowId):
        if windowId in self.hashAppStatusInfo:
            return self.hashAppStatusInfo[windowId].getZvalue()
        else:
            print("getZvalue: Invalid app instance ID")
            return

            
    #----------------------------------------------------------------------


    #   Set the Z value of the app
    #   @arg windowId App instance ID of the app
    #   @arg value New Z value to be set
    def setZvalue(self, windowId, value):
        if (windowId in self.hashAppStatusInfo):
            self.hashAppStatusInfo[windowId].setZvalue(value)
        else:
            print ('setZvalue: Invalid app instance ID: %d'% (windowId))
        return


    #----------------------------------------------------------------------

    
    ### Get the sage app Info
    ### @arg windowId Instance id of the application
    ### @returns A list of format [<left>, <right>, <top>, <bottom>]
    def getAppInfo(self, windowId):
        if windowId in self.hashAppStatusInfo:
            return self.hashAppStatusInfo[windowId].getBounds()
        else:
            print("getAppInfo: Invalid app instance ID")
            return []


    #----------------------------------------------------------------------


    # returns the SageApp associated with the given windowId
    def getApp(self, windowId):
        return self.hashAppStatusInfo[windowId]
    

    #----------------------------------------------------------------------


    ### Get the sage app display information
    ### @arg windowId Instance id of the application
    ### @arg interval Total no of values required (max = 30)
    ### @returns A array   
    def getDisplayItem(self, stItemName, windowId, interval):
        if windowId in self.hashAppPerfInfo:
            appInfo = self.hashAppPerfInfo.get(windowId)
            return appInfo.getDisplayInformation( stItemName, interval )
        else:
            print("getDisplayItem: Invalid app instance ID")


    ### Get the sage app render information
    ### @arg windowId Instance id of the application
    ### @arg interval Total no of values required (max = 30)
    ### @returns A array
    def getRenderItem(self, stItemName, windowId, interval):
        if windowId in self.hashAppPerfInfo:
            appInfo = self.hashAppPerfInfo.get(windowId)
            return appInfo.getRenderInformation( stItemName, interval )
        else:
            print("Invalid app instance ID")


    #----------------------------------------------------------------------

    
    ### Get the sage app data service bandwidth
    ### @arg windowId Instance id of the application
    ### @arg interval Total no of values required (max = 30)
    ### @returns A array
    def getDataBandWidth(self, windowId, interval):
        if windowId in self.hashAppPerfInfo:
            appInfo = self.hashAppPerfInfo.get(windowId)
            return appInfo.getDataBandWidth(interval)
        else:
            print("Invalid app instance ID")


    #----------------------------------------------------------------------
    

    ### So that SageData knows what to call when a message arrives
    def registerCallbackFunction( self, iSageID, function ):
        self.hashCallback[ iSageID ] = function


    #----------------------------------------------------------------------

    
    ### (RJ 2005-01-15)
    ### Returns a list of all the available apps
    def getAvailableApps( self ):
        return self.hashApps

    #----------------------------------------------------------------------

    ### sage background color
    def setSAGEColor(self, (r,g,b)):
        self._sageColor = (r,g,b)

    #----------------------------------------------------------------------

    ### sage background color
    def getSAGEColor(self):
        return self._sageColor
    
    #----------------------------------------------------------------------

    # checks all the apps and reports whether any one of them was
    # hit with a click and which region was hit (corners or shape in general)
    # if more than one shape was hit it returns the one on the top
    def checkHits(self, x, y):
        zHash = {}  #key=z value, value=SageApp

        for windowId, sageApp in self.hashAppStatusInfo.iteritems():
            zHash[sageApp.getZvalue()] = sageApp

        zKeys = zHash.keys()
        zKeys.sort()

        for z in zKeys:
            hitResult = zHash[z].hitTest(x,y)
            if hitResult >= 0:
                return (zHash[z], hitResult)

        return (None, -1)


    #----------------------------------------------------------------------

    # returns (windowId, zValue) or (-1,sys.maxint) if no apps
    def getTopApp(self):
        minZ = (-1, sys.maxint)
        for windowId, app in self.hashAppStatusInfo.iteritems():
            if app.getZvalue() < minZ[1]:
                minZ = (windowId, app.getZvalue())
        return minZ

    #----------------------------------------------------------------------

    ### (AKS 2005-02-16)
    ### Return totals/averages for performance metrics for applications
    def getTotals( self ):
        return self.__hashAppPerfTotals
    
    #----------------------------------------------------------------------

    ### (AKS 2005-02-16)
    ### Set the logging flag and close all the logging files
    def setLoggingFlag( self, bFlag ):
        self.__bPerformanceLogging = bFlag

    def isLoggingEnabled(self):
        return self.__bPerformanceLogging

    #----------------------------------------------------------------------
    
    ### (RJ 2005-5-18)
    ### Stops the logging and closes all the logging files
    def stopLogging(self):
        for windowId, fileObject in self.getLogFileHash().iteritems():
            fileObject.close()

    #----------------------------------------------------------------------

    ### (AKS 2005-05-07)
    ### Get render bandwidth graph
    def getRenderBandwidthGraph( self ):
        return self.__pgTotalRenderBandwidth


    #----------------------------------------------------------------------

    ### (AKS 2005-05-07)
    ### Get display bandwidth graph
    def getDisplayBandwidthGraph( self ):
        return self.__pgTotalDisplayBandwidth


    #----------------------------------------------------------------------


    def saveState(self, stateName, description):
        appLaunchers = self.sageGate.getLaunchers()
        appList = []

        # gather all the data that needs to be saved for each app
        for app in self.hashAppStatusInfo.values():

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
                    continue   # skip this app... something went wrong
                configName, optionalArgs = res

                # get the other app parameters from sageApp object
                pos = (app.getLeft(), app.getBottom())
                size = (app.getWidth(), app.getHeight())
                
                # append the tuple of app's data to the list that will be saved
                appList.append( (app.getLauncherId(), app.getName(), configName,
                                 pos, size, optionalArgs) )

      
        # open the file and write to it
        try:   #in case the file and directory permissions are not right
            f = open( opj(SAVED_STATES_DIR, stateName+".state"), "w")
            pickle.Pickler(f, 0).dump( (description, appList) )
            f.close()
        except IOError:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            return False

        return True


    def __deleteOldAutosaves(self):
        # remove old autosave files...
        try:
            sd = SAVED_STATES_DIR
            saves = os.listdir(sd)
            if "_autosave_PREV.state" in saves:
                os.remove(opj(sd, "_autosave_PREV.state"))
            if "_autosave_LATEST.state" in saves:
                shutil.move(opj(sd, "_autosave_LATEST.state"), opj(sd, "_autosave_PREV.state"))
        except:
            print "ERROR while deleting old states:"
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))


    def loadState(self, stateName):
        """ tries to reload the apps from the saved state """
        appList = []
        description = ""

        # load the state from a file
        try:
            f = open( opj(SAVED_STATES_DIR, stateName+".state"), "r")
            (description, appList) = pickle.Unpickler(f).load()
            f.close()
        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            print "\nUnable to read saved state file: "+"saved-states/"+stateName+".state"
            return False

        # try re-running all the apps
        count = 0
        for appInfo in appList: 
            launcherId, appName, configName, pos, size, optionalArgs = appInfo
            self.sageGate.executeRemoteApp(launcherId, appName, configName, pos, size, optionalArgs)
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

        sd = SAVED_STATES_DIR
        
        # load all the states and read descriptions from them
        for fileName in os.listdir( sd ):
            filePath = opj(sd, fileName)
            if os.path.isfile(filePath) and os.path.splitext(filePath)[1] == ".state":
                try:
                    stateName = os.path.splitext( os.path.split(filePath)[1] )[0]
                    f = open(filePath, "rb")
                    (description, appList) = pickle.Unpickler(f).load()
                    f.close()
                    stateHash[stateName] = description
                except:
                    print "\nUnable to read saved state file: "+filePath
                    continue

        return stateHash



    def closeAllApps(self):
        for app in self.hashAppStatusInfo.itervalues():
            if app.getId() == -5:
                continue
            self.sageGate.shutdownApp(app.getId())
            time.sleep(0.5)



