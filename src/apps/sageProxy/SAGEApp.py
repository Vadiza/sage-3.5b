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




import sys, string

### Class to hold sage application info
### An instance of this class should exist for every app Id on SAGE
class SAGEApp :

        
    def __init__(self, name, id, left=0, right=0, bottom=0, top=0, sailID=0, zValue=0, configNum=1, title="untitled"):
        self.left = left
        self.right = right
        self.top = top
        self.bottom = bottom
        self.sailID = sailID
        self.appName = name
        self.id = id
        self.zValue = zValue
        self.configNum = configNum
        self.title = title

    def setAll(self, name, id, left, right, bottom, top, sailID, zValue):#, configNum, title):
        self.appName = name
        self.id = id
        self.left = left
        self.right = right
        self.bottom = bottom
        self.top = top
        self.sailID = sailID
        self.zValue = zValue
        #self.configNum = configNum
        #self.title = title

    def getAll(self):
        return [self.appName, self.id, self.left, self.right,
                self.bottom, self.top, self.sailID, self.zValue,
                self.configNum, self.title]

    #### Set the values of the application
    #### @arg left Left position of the window
    #### @arg right right position of the window
    #### @arg top top position of the window
    #### @arg bottom bottom position of the window
    #### @arg sailID sailID of the app
    def setValues(self, id, left, right, top, bottom, sailID, zValue) :
        self.id = id
        self.left = left
        self.right = right
        self.top = top
        self.bottom = bottom
        self.sailID = sailID
        self.zValue = zValue

    def getLeft(self):
        return self.left

    def getRight(self):
        return self.right

    def getTop(self):
        return self.top

    def getBottom(self):
        return self.bottom

    def getId(self):
        return self.id

    def getConfigNum(self):
        return self.configNum

    def getTitle(self):
        return self.title

    def setTitle(self, newTitle):
        self.title = newTitle
    
    #   Set the app name
    #   @arg appName Name of the app
    def setName(self, appName):
        self.appName = appName

    #   Get the app name
    #   @return Returns the app name
    def getName(self):
        return self.appName
    
    #### Get the application window values
    #### @return Returns a list Format: [<left>, <right>, <top>, <bottom>]
    def getValues(self) :
        return [self.left, self.right, self.top, self.bottom]

    #### Get the app's sailID
    def getSailID(self):
        return self.sailID

    #   Set the Z value of the app
    #   @arg value Z value of the app
    def setZvalue(self, value):
        self.zValue = value

    #   get the Z value of the app
    def getZvalue(self):
        return self.zValue

    def getWidth(self):
        return self.right - self.left

    def getHeight(self):
        return self.top - self.bottom




### a class to hold the information about all the available apps that SAGE supports
class SAGEAppInitial:

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

    
