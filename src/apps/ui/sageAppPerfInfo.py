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

### Class to hold all the performace information
### Instance of this class has to be created for every app instance ID on SAGE
class sageAppPerfInfo:

    def __init__(self) :
        self.displayArray = {}
        self.renderArray = {}
        self.dataArray = {}
        
        self.displayArray['bandWidth'] = self.__zerosHash(30)
        self.displayArray['frameRate'] = self.__zerosHash(30)
        self.displayArray['nodes'] = self.__zerosHash(30)
        self.displayArray['cpu'] = self.__zerosHash(30)
        self.displayArrayIndex = 0

        self.renderArray['bandWidth'] = self.__zerosHash(30)
        self.renderArray['frameRate'] = self.__zerosHash(30)
        self.renderArray['nodes'] = self.__zerosHash(30)
        self.renderArray['cpu'] = self.__zerosHash(30)
        self.renderArrayIndex = 0

        self.dataArray['bandWidth'] = self.__zerosHash(30)
        self.dataArray['nodes'] = self.__zerosHash(30)
        self.dataArray['cpu'] = self.__zerosHash(30)
        self.dataArrayIndex = 0

    #### Set the display performance Info
    #### @arg bandwidth Bandwidth
    #### @arg frameRate Frame rate
    #### @arg nodes Number of nodes
    #### @arg cpuUsage CPU utilisation
    def setDisplayPerfInfo(self, bandWidth, frameRate, nodes, cpuUsage):
        if (self.displayArrayIndex >= 30):
            self.displayArrayIndex = 0

        self.displayArray['bandWidth'][self.displayArrayIndex % 30] = bandWidth
        self.displayArray['frameRate'][self.displayArrayIndex % 30] = frameRate
        self.displayArray['nodes'][self.displayArrayIndex % 30] = nodes
        self.displayArray['cpu'][self.displayArrayIndex % 30] = cpuUsage

        self.displayArrayIndex = self.displayArrayIndex + 1



    #### Set the rendering performance Info
    #### @arg bandwidth Bandwidth
    #### @arg frameRate Frame rate
    #### @arg nodes Number of nodes
    #### @arg cpuUsage CPU utilisation
    def setRenderPerfInfo(self, bandWidth, frameRate, nodes, cpuUsage):
        if (self.renderArrayIndex >= 30):
            self.renderArrayIndex = 0

        self.renderArray['bandWidth'][self.renderArrayIndex] = bandWidth
        self.renderArray['frameRate'][self.renderArrayIndex] = frameRate
        self.renderArray['nodes'][self.renderArrayIndex] = nodes
        self.renderArray['cpu'][self.renderArrayIndex] = cpuUsage

        self.renderArrayIndex = self.renderArrayIndex + 1



    #### Get display information based on the specified item
    #### @arg interval No of values required (max = 30)
    #### @return Returns an array
    def getDisplayInformation(self, stItemName, interval):

        if (interval > 30 or interval < 0):
            print ('Out of bound range specified')
            return 0
        newArray = self.__zerosHash(interval)    #zeros(type='Float32', shape=interval)
        newArrayIndex = self.displayArrayIndex - 1

        for x in range(0, interval):
            if newArrayIndex < 0:
                newArrayIndex = 29

            newArray[x] = self.displayArray[ stItemName ][newArrayIndex]
            newArrayIndex = newArrayIndex - 1

        return newArray



    #### Get rendering information based on the specified item
    #### @arg interval No of values required (max = 30)
    #### @return Returns an array
    def getRenderInformation(self, stItemName, interval):

        if (interval > 30 or interval < 0):
            print ('Out of bound range specified')
            return 0
        newArray = self.__zerosHash(interval)   #zeros(type='Float32', shape=interval)
        newArrayIndex = self.renderArrayIndex - 1

        for x in range(0, interval):

            if newArrayIndex < 0:
                newArrayIndex = 29

            newArray[x] = self.renderArray[ stItemName ][newArrayIndex]
            newArrayIndex = newArrayIndex - 1
  
        return newArray


    # creates a hash of size "size" and initializes all the values to 0
    def __zerosHash(self, size):
        h = {}
        for i in range(0, size):
            h[i] = 0
        return h
