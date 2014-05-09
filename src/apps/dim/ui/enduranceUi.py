############################################################################
#
# UI for displaying a series of ENDURANCE images according to their UTM coords
#
# Copyright (C) 2009 Electronic Visualization Laboratory,
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




########################################################################
#
# Creates a demo of widget framework
#
# Author: Ratko Jagodic
#
########################################################################

from globals import *
import overlays.menu as menu
import overlays.button as button
import overlays.icon as icon
import overlays.label as label
import overlays.sizer as sizer
import overlays.panel as panel
import overlays.enduranceThumbnail as enduranceThumbnail
import os.path, os, sys
import traceback as tb
from math import sin, cos, radians


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return EnduranceUI()


# must have this to load the plugin automatically
#TYPE = UI_PLUGIN

# where the images are located relative to SAGE_DIRECTORY
IMAGES_DIR = "images/endurance" 
SCALE = 0.2


class EnduranceUI:
    def __init__(self):

        # shortcuts for less typing
        self.evtMgr = getEvtMgr()
        self.addWidget = getOverlayMgr().addWidget
        self.removeWidget = getOverlayMgr().removeWidget

        # display size
        d = getSageData().getDisplayInfo()
        self.dispW, self.dispH = d.sageW, d.sageH

        # register for some app events so that we can create widgets for them
        self.evtMgr.register(EVT_NEW_APP, self.onNewApp)
        self.evtMgr.register(EVT_APP_KILLED, self.onCloseApp)

        self.thumbs = {}  # key=appId, value=thumbs
        #self.makeShowButton()

        self.app = None



    def onNewApp(self, event):
        if event.app.getName() == "endurance":
            self.__loadImages(event.app)

    
    def onCloseApp(self, event):
        """ overlays will be removed by overlayManager so just 
            empty the local list of app related widgets
        """
        appId = event.app.getId()
        if appId in self.thumbs:
            self.app = None
            del self.thumbs[appId]


    def makeShowButton(self):
        b = button.Button()
        b.setLabel("Show Endurance Images")
        b.align(LEFT, TOP)
        b.onDown(self.__onShow)
        b.onUp(self.__onHide)
        b.setToggle(True)
        self.addWidget(b)
        

    def __loadImages(self, app):
        appId = app.getId()
        images = {}  # key=filename, value=[path, timestamp, UMTX, UMTY]
        self.thumbs[appId] = []
        maxX = maxY = -sys.maxint
        minX = minY = sys.maxint

        # read the IMAGES_DIR, parse the filenames and determine min/max UTM
        for filename in os.listdir( os.path.join(os.environ["SAGE_DIRECTORY"], IMAGES_DIR)):
            try:
                name, ext = os.path.splitext(filename)
                timestamp, x, y = name.split("_", 2)
                x,y = int(x),int(y)
                path = os.path.join(IMAGES_DIR, filename)

                # rotate CW
                rad = radians(-59)
                oldX, oldY = x, y
                x = int (oldX*cos(rad) - oldY*sin(rad))
                y = int (oldX*sin(rad) + oldY*cos(rad))

                # find min and max
                if x > maxX:  maxX = x
                elif x < minX: minX = x
                if y > maxY:  maxY = y
                elif y < minY: minY = y

                # store for later
                images[filename] = [path, timestamp, x, y]
                
            except:
                tb.print_exc()
                print "Skipping: ", filename
                continue  


        rangeX, rangeY = float(maxX - minX), float(maxY - minY)
        rangeAr = rangeX / rangeY

        
        maxHeight = self.dispH - 2200
        yOffset = int(2200/2.0)
        xOffset = 3300

        # create a thumbnail for each image at the right location
        for filename, imageInfo in images.iteritems():
            path, timestamp, x, y = imageInfo

            # find the pixel coordinates
            #xOffset = ((self.dispW - self.dispH) / 2.0)

            normX = rangeAr * ((x - minX) / rangeX)
            normY = 1 - ((y - minY) / rangeY)

            #pixelX = int(maxHeight * normX) + xOffset
            #pixelY = int(maxHeight * normY) + yOffset

            # make the thumbnail
            b = enduranceThumbnail.makeNew(app)
            b.setSize(32, 24) #int(320*SCALE), int(240*SCALE))
            b.setTransparency(255)
            #b.setPos(pixelX, pixelY)
            b.setPos(normX, normY)
            b._canScale = False
            b.setUpImage(path)
            b.allowDrag(True)
            b.setTooltip(filename, fontSize=SMALLEST)
            self.addWidget(b)
            self.thumbs[appId].append(b)



##     def __onShow(self, btn):
##         #btn.setLabel("Hide Endurance Images")
##         #self.__loadImages()


##     def __onHide(self, btn):
##         #btn.setLabel("Show Endurance Images")
##         #for thumb in self.thumbs:
##         #    self.removeWidget(thumb)
