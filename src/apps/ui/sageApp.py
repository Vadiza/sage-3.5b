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



import sys, string

### Class to hold sage application info
### An instance of this class should exist for every app Id on SAGE
class SageApp :

        
    def __init__(self, name, windowId, left, right, bottom, top, sailID, zValue, orientation, displayId, appId, launcherId, title=""):
        self.left = left
        self.right = right
        self.top = top
        self.bottom = bottom
        self.sailID = sailID
        self.appName = name
        self.windowId = windowId
        self.appId = appId
        self.launcherId = launcherId
        self.zValue = zValue
        self.configNum = 0
        self.title = title
        self.capture = -1  # which pointer captured it
        self.orientation = 0  # in degrees
        self.displayId = 0
        

    def setAll(self, name, windowId, left, right, bottom, top, sailID, zValue, orientation, displayId, appId, launcherId):
        self.appName = name
        self.windowId = windowId
        self.appId = appId
        self.launcherId = launcherId
        self.left = left
        self.right = right
        self.bottom = bottom
        self.top = top
        self.sailID = sailID
        self.zValue = zValue
        self.orientation = orientation
        self.displayId = displayId
        #self.title = title
        

    def getLeft(self):
        return self.left

    def getRight(self):
        return self.right

    def getTop(self):
        return self.top

    def getBottom(self):
        return self.bottom

    def getId(self):
        return self.windowId

    def getAppId(self):
        return self.appId

    def getLauncherId(self):
        return self.launcherId

    def getConfigNum(self):
        return self.configNum

    def getTitle(self):
        return self.title

    def setTitle(self, newTitle):
        self.title = newTitle

    def getWidth(self):
        return self.right - self.left

    def getHeight(self):
        return self.top - self.bottom

    def getDisplayId(self):
        return self.displayId

    def getOrientation(self):
        return self.orientation
    
    def setName(self, appName):
        self.appName = appName

    def getName(self):
        return self.appName
    
    #### Get the application window values
    #### @return Returns a list Format: [<left>, <right>, <top>, <bottom>]
    def getBounds(self) :
        return [self.left, self.right, self.top, self.bottom]

    #### Get the app's sailID
    def getSailID(self):
        return self.sailID

    def setZvalue(self, value):
        self.zValue = value

    def getZvalue(self):
        return self.zValue

    def hasCapture(self):
        if self.capture == -1:
            return False
        else:
            return True

    def captureMouse(self, pointerId):
        self.capture = pointerId
        
    def releaseMouse(self):
        self.capture = -1
        

    # checks whether (x,y) hit something on the shape
    # calculations are done in the sage coordinate system (bottom left = 0,0)
    # 0 = everything else... inside the app
    # -2 = hit but not on the corners (BOTTOM)
    # -1 = hit but not on the corners (TOP)
    # 1 = bottom left corner
    # 2 = top left corner
    # 3 = top right
    # 4 = bottom right
    def hitTest(self, x, y, cornerSize=250):

        # utility function
        def __inRegion(r):
            if x >= r[0] and x <= r[1] and y<=r[2] and y>=r[3]:
                return True
            return False
            
        (l,r,t,b,cs) = (self.left, self.right, self.top, self.bottom, cornerSize)

        # define the regions we need to test
        testRegions = {}  # keyed by -2,-1,1,2,3,4
        testRegions[-2] = (l+cs, r-cs, b+cs, b)
        testRegions[-1] = (l+cs, r-cs, t, t-cs)
        testRegions[1] = (l, l+cs, b+cs, b)
        testRegions[2] = (l, l+cs, t, t-cs)
        testRegions[3] = (r-cs, r, t, t-cs)
        testRegions[4] = (r-cs, r, b+cs, b)
        
        # now loop through the regions and test whether x,y is in it
        result = 0
        for rid, r in testRegions.iteritems():
            if __inRegion(r):
                return rid
                                
        return result







### a class to hold the information about all the available apps that SAGE supports
class SageAppInitial:

    def __init__ (self, name, configNames):
        self._name = name
        self._configNames = configNames
        
    def GetName (self):
        return self._name

    def SetName (self, name):
        self._name = name

    def GetConfigNames(self):
        return self._configNames
    
    ## # execution configurations
##     def AddConfig (self, name, stConfig):
##         self._execConfigs[ len(self._execConfigs)+1 ] = (name, string.strip(stConfig))

##     def GetNumConfigs(self):
##         return self._numConfigs
    
##     def GetConfigHash (self):
##         return self._execConfigs

##     def GetConfig (self, configNum):
##         if self._execConfigs.has_key( configNum ):
##             return self._execConfigs[ configNum ]
##         else:
##             print "Error: No configuration number ", configNum
##             return "No Configuration"

##     # instances already started
##     def AddInstanceId (self, instanceId):
##         self._appInstanceIds.append( instanceId )

##     def GetInstanceIds (self):
##         return self._appInstanceIds

    
