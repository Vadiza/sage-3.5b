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



import sys, string
from globals import *


### Class to hold sage application info
### An instance of this class should exist for every app Id on SAGE
class SageApp:

        
    def __init__(self, name, windowId, left, right, bottom, top, sailID, zValue, orientation, displayId, appId, launcherId, resX, resY):
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
        self.title = ""
        self.orientation = 0  # in degrees
        self.displayId = 0
        self.maximized = False
        self.sectionMaximized = False
        self.__minimized = False
        self.aspectRatio = float(self.getWidth()) / self.getHeight()

        # resolution of the buffer being streamed to SAGE
        if resX < 0:
            resX = right-left
        if resY < 0:
            resY = top-bottom
        self.resX = resX
        self.resY = resY

        # For sharing support
        self.isShared = False
        self.sharedHosts = set()

        # this shouldn't ever change
        self.startBounds = self.getBounds()

        self.restoreBounds = self.getBounds()
        self.__beforeMininmizeBounds = [100, 100+self.resX, 100+self.resY, 100]  # the very default
        self.freeBounds = self.getBounds()
        self.tileBounds = self.getBounds()

        # which section the app belongs to (stored as section object)
        self.section = None
        #print "\n=====> NEW APP: winID %d   sailID %d" % (windowId, sailID)

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
        self.aspectRatio = float(self.getWidth()) / self.getHeight()
        #self.title = title

    def getInitialWidth(self):
        l = self.startBounds[0]
        r = self.startBounds[1]
        return r-l

    def getInitialHeight(self):
        t = self.startBounds[2]
        b = self.startBounds[3]
        return t-b

    def getInitialArea(self):
        return self.getInitialHeight() * self.getInitialWidth()

    def getArea(self):
        return self.getWidth() * self.getHeight()

    def getAspectRatio(self):
        """ width/height """
        return self.aspectRatio

    def getLeft(self):
        return self.left

    def getRight(self):
        return self.right

    def getTop(self):
        return self.top

    def getBottom(self):
        return self.bottom

    def getCenterX(self):
        return int(self.right - (self.getWidth() / 2.0))

    def getCenterY(self):
        return int(self.top - (self.getHeight() / 2.0))

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

    def getPos(self):
        return self.left, self.bottom

    def getSize(self):
        return self.getWidth(), self.getHeight()
    
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
        

    # checks whether (x,y) hit something on the shape
    # calculations are done in the sage coordinate system (bottom left = 0,0)
    # 0 = everything else... inside the app
    # -2 = hit but not on the corners (BOTTOM)
    # -1 = hit but not on the corners (TOP)
    # 1 = bottom left corner
    # 2 = top left corner
    # 3 = top right
    # 4 = bottom right
    # 5 = top right x
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
        testRegions[-1] = (l+cs, r-cs-cs, t, t-cs) # extra cs for top-right-x
        testRegions[1] = (l, l+cs, b+cs, b)
        testRegions[2] = (l, l+cs, t, t-cs)
        testRegions[3] = (r-cs, r, t, t-cs)
        testRegions[4] = (r-cs, r, b+cs, b)
        testRegions[5] = (r-cs-cs, r-cs, t, t-cs) # extra cs for top-right-x
        
        # now loop through the regions and test whether x,y is in it
        result = 0
        for rid, r in testRegions.iteritems():
            if __inRegion(r):
                return rid
                                
        return result


    def isIn(self, x, y):
        """ returns True if the (x,y) is in Bounds, False otherwise """
        if self.left <= x and self.right >= x and self.bottom <= y and self.top >= y:
            return True
        else:
            return False


    def bringToFront(self):
        getSageGate().bringToFront(self.windowId)


    def toggleMaximized(self):
        if self.maximized:
            self.restore()
        else:
            self.maximize()

    
    def isMaximized(self):
        return self.maximized

    
    def isSectionMaximized(self):
        return self.sectionMaximized


    def fitWithin(self, x, y, width, height, margin=0, saveBounds=True, cx=None, cy=None):
        # centerApp = False will still fit the app within the bounds but closer to the
        # original position of the app when this function was called
        # otherwise, it will be centered within the bounds
        
        if saveBounds:
            self.restoreBounds = self.getBounds()

        # take buffer into account
        x += margin
        y += margin
        width = width - 2*margin
        height = height - 2*margin
        
##        appWidgets = getOverlayMgr().getAppWidgets(self)

       ##  # find the max extents of the app and it's widgets
##         maxX = maxY = 0
##         minX, minY = width, height
        
##         for w in appWidgets:
##             if not w._isAppWidget() and w.getType() != OVERLAY_APP:
##                 continue
##             l,r,t,b = w.getBounds().getAll()
##             if maxX < r:  maxX = r
##             if minX > l:  minX = l
##             if maxY < t:  maxY = t
##             if minY > b:  minY = b
            
##         # total bounds of the app considering all the widgets
##         preMaxWidth, preMaxHeight = maxX-minX, maxY-minY
        
##         # what portion is the actual app
##         appPortionW = float(self.getWidth()) / preMaxWidth
##         appPortionH = float(self.getHeight()) / preMaxHeight
        
##         appPortionX = (float(self.getLeft()) - minX) / preMaxWidth
##         appPortionY = (float(self.getBottom()) - minY) / preMaxHeight

        # figure out by how much to multiply the size of the shape in order
        # to fill the tiled display and not go over all while preserving the aspect ratio (if needed)
        widthRatio = (width-4) / float(self.getWidth())   # -4 for showing the app border
        heightRatio = (height-4) / float(self.getHeight())  # add the top buttons' size
        if widthRatio > heightRatio:
            maximizeRatio = heightRatio
        else:
            maximizeRatio = widthRatio

##         # bounding box after maximizing
##         postMaxWidth = int(round( preMaxWidth*maximizeRatio ))
##         postMaxHeight = int(round( preMaxHeight*maximizeRatio ))

##         # figure out the maximized app size (w/o the widgets)
##         newAppWidth = int(round( appPortionW*postMaxWidth ))
##         newAppHeight = int(round( appPortionH*postMaxHeight ))

        # figure out the maximized app size (w/o the widgets)
        newAppWidth = int(round( maximizeRatio*self.getWidth() ))
        newAppHeight = int(round( maximizeRatio*self.getHeight() ))

        # figure out the maximized app position (with the widgets)
        postMaxX = int(round( width/2.0 - newAppWidth/2.0 ))
        postMaxY = int(round( height/2.0 - newAppHeight/2.0 )) 

        # the new position of the app considering the maximized state and
        # all the widgets around it
        newAppX = x + postMaxX #+ postMaxWidth * appPortionX
        newAppY = y + postMaxY #+ postMaxHeight * appPortionY

        # if the app shouldnt be centered, position it as
        # close to the original position as possible
        if cx and cy:
            if widthRatio > heightRatio:  # height diff is greater... so adjust x
                newHalfW = int(newAppWidth/2.0)
                if cx+newHalfW > x+width - margin:
                    newAppX = x+width - newAppWidth - margin
                elif cx-newHalfW < x + margin:
                    newAppX = x + margin
                else:
                    newAppX = cx-newHalfW
                    
            else:       # width diff is greater... so adjust y
                newHalfH = int(newAppHeight/2.0)
                if cy+newHalfH > y+height - margin:
                    newAppY = y+height - newAppHeight - margin
                elif cy-newHalfH < y + margin:
                    newAppY = y + margin
                else:
                    newAppY = cy-newHalfH
                

        # tell SAGE to resize the app
        self.resizeWindow(newAppX, newAppX+newAppWidth,
                          newAppY, newAppY+newAppHeight)
        
        return [newAppX, newAppX+newAppWidth, newAppY, newAppY+newAppHeight]
        

    def fitInTile(self, x, y, width, height, margin=0, saveBounds=True):
        self.tileBounds = self.fitWithin(x, y, width, height, margin, saveBounds=saveBounds)
                

    def maximize(self):
        if self.maximized: return
        self.maximized = True
       
        # get display size
        d = getSageData().getDisplayInfo()  
        self.fitWithin(d.usableZeroX, d.usableZeroY, d.usableW, d.usableH)

        playSound(MAXIMIZE_WINDOW)


    def hasMovedAfterTile(self):
        r = (self.getBounds() == self.tileBounds)
        print not r
        return not r


    #-------------   MINIMIZED STUFF  ---------------#
    
    def minimize(self, doMin):
        self.__minimized = doMin
        self.__beforeMininmizeBounds = self.getBounds()
        
        if doMin:
            writeLog(LOG_WINDOW_MINIMIZE, self.getId())
            getSageGate().changeAppFrameRate(self.getId(), 10)
        else:
            getSageGate().changeAppFrameRate(self.getId(), -1)
        

                
    def isMinimized(self):
        return self.__minimized


    def restoreFromMinimze(self):
        self.maximized = False
        self.sectionMaximized = False
        self.resizeWindow(self.__beforeMininmizeBounds[0],
                          self.__beforeMininmizeBounds[1],
                          self.__beforeMininmizeBounds[3],
                          self.__beforeMininmizeBounds[2])
        self.bringToFront()
    

    def _getMinimizedBounds(self):
        return self.__beforeMininmizeBounds


    def _setMinimizedBounds(self, b):
        self.__beforeMininmizeBounds = b


    #------------------------------------------------#


    def restore(self):
        self.maximized = False
        self.sectionMaximized = False
        self.resizeWindow(self.restoreBounds[0],
                          self.restoreBounds[1],
                          self.restoreBounds[3],
                          self.restoreBounds[2])
        
        # update section membership
        #self.section.updateAppSection(self)

        playSound(RESTORE_WINDOW)


    def saveBoundsBeforeTile(self):
        self.freeBounds = self.getBounds()


    def saveBoundsBeforeMax(self):
        self.restoreBounds = self.getBounds()
        

    def toFreeLayout(self):
        self.resizeWindow(self.freeBounds[0],
                          self.freeBounds[1],
                          self.freeBounds[3],
                          self.freeBounds[2])
        
    def resizeWindow(self, l, r, b, t):
        getSageGate().resizeWindow(self.getId(), l, r, b, t)
        
    
    def moveWindow(self, x, y):
        getSageGate().moveWindow(self.getId(), x, y)


        

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
    
    
