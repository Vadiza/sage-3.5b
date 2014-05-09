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




########################################################################
#
# Creates some global widgets for the whole display
#
# Author: Ratko Jagodic
#
########################################################################

import os, os.path, xmlrpclib, socket, sys
from math import ceil, sqrt
from threading import Thread
import traceback as tb

from globals import *
import overlays.menu as menu
import overlays.button as button
import overlays.thumbnail as thumbnail
import overlays.icon as icon
import overlays.label as label
import overlays.sizer as sizer
import overlays.panel as panel
import overlays.app as app

opj = os.path.join




##############  a hack to make it work on python versions < 2.6
def relpath(path, start="."):
    """Return a relative version of a path"""

    if not path:
        raise ValueError("no path specified")

    start_list = os.path.abspath(start).split(os.path.sep)
    path_list = os.path.abspath(path).split(os.path.sep)

    # Work out how much of the filepath is shared by start and path.
    i = len(os.path.commonprefix([start_list, path_list]))

    rel_list = [os.path.pardir] * (len(start_list)-i) + path_list[i:]
    if not rel_list:
        return os.path.curdir
    return os.path.join(*rel_list)


try:
    getRelativePath = os.path.relpath
except:
    getRelativePath = relpath

###########  end hack

    



def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return MediaBrowserUI()


# must have this to load the plugin automatically
TYPE = UI_PLUGIN

global FILES_DIR, THUMBS_DIR
THUMB_SIZE = 85
SAGE_BIN_DIR = opj(SAGE_DIR, "bin")
CONFIG_FILE = getPath("fileServer", "fileServer.conf")



class MediaBrowserUI:
    def __init__(self):
        if not SHOW_USER_INTERFACE:
            return

        self.addWidget = getOverlayMgr().addWidget
        self.removeWidget = getOverlayMgr().removeWidget
        self.sageGate = getSageGate()
        self.sageData = getSageData()
        self.fileServer = getFileServer()

        getEvtMgr().register(EVT_SCREENSHOT_SAVED, self.__onScreenshotSaved)

        # a thread for making thumbnails
        self.t = None
        self.__makingThumbs = False

        # already loaded panels with thumbnails (key=directory, value=panel)
        self.__loadedThumbPanels = {}    # already loaded panels... for faster showing

        self.__loadedDirPanels = {} # already loaded dirs... for faster showing
        self.__currentDir = None    # currently shown directory path

        # all currently shown directory levels (dirs, subdirs and thumbPanel)
        # stored as (key=level, value(dirBtn, subDirPanel, thumbPanel)
        self.__visibleDirPanels = {}  
        
        # figure out where the file library is
        self.__getFilesDir()

        d = self.sageData.getDisplayInfo()
        self.__displayAr = d.sageW / float(d.sageH)


        if os.path.exists(FILES_DIR):
            self.makeBrowser(FILES_DIR)

            # for click outside the browser... it closes the browser
            #self.__callbacks = {}
            #self.__callbacks[EVT_WALL_CLICKED] = self.__onWallClicked
            #getEvtMgr().register(EVT_WALL_CLICKED, self.__onWallClicked)
        else:
            print "\nERROR: cannot make SAGE MediaBrowser because the directory doesn't exist:", FILES_DIR

        
    def makeBrowser(self, forDir):
        # the main panel
        self.browserPanel = panel.makeNew()
        self.browserPanel.setTransparency(200)
        self.browserPanel.setBackgroundColor(20,50,50)
        self.browserPanel.drawBorder(True)
        self.browserPanel.align(RIGHT, CENTER)
        #self.browserPanel.allowSlide(True)
        #self.browserPanel.slideHide()
        self.browserPanel.hide()
        
        # the main sizer for the browser panel
        self.browserSizer = sizer.makeNew()
        
        # holds types buttons and browser panel
        mainSizer = sizer.makeNew(direction=HORIZONTAL)
        mainSizer.align(LEFT, CENTER)
        mainSizer.drawLast(True)   # a hack to keep it on top of section panels and the curtain effect

        # holds all the subdirectories' panels
        self.dirSizer = sizer.makeNew()
        self.dirSizer.align(LEFT, CENTER, xMargin=10, yMargin=10)
        self.browserSizer.addChild(self.dirSizer)

        # make level 0
        mediaTypesPanel, imgBtn = self.__makeMediaTypePanel(forDir)
        self.__loadedDirPanels[forDir] = mediaTypesPanel
        
        mainSizer.addChild(mediaTypesPanel)
        mainSizer.addChild(self.browserPanel)

        # add everything to corresponding parents
        self.browserPanel.addChild(self.browserSizer)
        self.browserPanel.addChild(self.__makeCloseBtn1())
        self.browserPanel.addChild(self.__makeCloseBtn2())
        self.addWidget(mainSizer)


    def __makeCloseBtn1(self):
        b = thumbnail.makeNew()
        b.setSize(int(THUMB_SIZE/2.5),int(THUMB_SIZE/2.5))
        b.setUpImage(opj("images", "close_up.png"))
        b.setDownImage(opj("images", "close_over.png"))
        b.setOverImage(opj("images", "close_over.png"))
        b.align(RIGHT, TOP)
        b.onDown(self.__onCloseBrowser)
        b.setScaleMultiplier(2.5)
        b.setTransparency(255)
        return b

    def __makeCloseBtn2(self):
        b = self.__makeCloseBtn1()
        b.align(RIGHT, BOTTOM)
        return b


    def __loadThumbs(self, panel, forDir, mediaType):
        """ make the actual thumbnails """
        
        try:
            # figure out the list of all thumbnails in this dir
            thumbs = {}

            dirItems = os.listdir(forDir)

            if mediaType != "saved":
                numItems = 0  # number of thumbnails to make
                for f in dirItems:
                    fullImage = opj(forDir, f)
                    if os.path.isfile(fullImage) and os.path.splitext( os.path.basename(fullImage) )[1] != ".dxt":
                        thumb = self.__GetThumbName(fullImage, mediaType)
                        if not os.path.exists(thumb):
                            numItems += 1

                # create a progress label for generating thumbs if there are many... and no thumbs exist yet
                progLabel = None
                if numItems > 3 and panel._getNumChildren() == 0:
                    progLabel = label.makeNew(label="Making thumbnails... ", fontSize=SMALLER)
                    progLabel.drawBackground(False)
                    progLabel.align(CENTER, CENTER)
                    panel.addChild(progLabel)


                # make the thumbnails
                numMade = 1
                for f in dirItems:
                    if not self.__makingThumbs:
                        if progLabel:
                            panel._removeChild(progLabel)
                            self.removeWidget(progLabel)
                        return  # cause we run in a thread so if we want to stop it...

                    fullImage = opj(forDir, f)
                    if os.path.isfile(fullImage) and os.path.splitext( os.path.basename(fullImage) )[1] != ".dxt":

                        thumb = self.__GetThumbName(fullImage, mediaType)
                        if not os.path.exists(thumb):
                            try:
                                if not self.fileServer.MakeThumbnail(fullImage):
                                    print "Thumbnail not made for: ", fullImage
                                if progLabel: progLabel.setLabel("Making thumb %d out of %d" % (numMade, numItems))
                                numMade += 1
                            except socket.timeout:
                                print "Thumbnail not made for: ", fullImage 
                                continue

                        if os.path.exists(thumb):
                            thumb = os.path.split(thumb)[1]
                            filename = os.path.split(fullImage)[1]
                            date = os.path.getmtime(fullImage)
                            thumbs[str(int(date))+filename] = (fullImage, thumb)

                if progLabel:
                    panel._removeChild(progLabel)
                    self.removeWidget(progLabel)


                # load the saved states from the saved-states directory
            else:
                for f in dirItems:
                    fullImage = opj(forDir, f)
                    name, ext = os.path.splitext( os.path.basename(fullImage) )
                    if os.path.isfile(fullImage) and ext == ".jpg" and not name.startswith("screen") :
                        date = os.path.getmtime(fullImage)
                        filename = os.path.split(fullImage)[1]
                        thumbs[str(int(date))+filename] = (fullImage, fullImage)
                

            # make the image browser the same aspect ratio as the display
            cols = int(ceil(sqrt( len(thumbs)*self.__displayAr )))

            # get the main vertical sizer for all the rows of thumbnails... or create one
            if panel._getNumChildren() > 0:
                vertSizer = panel._getFirstChild()
                cols = vertSizer.getUserData()
                horSizer = vertSizer._getLastChild()
            else:
                vertSizer = sizer.makeNew(direction=VERTICAL)
                vertSizer.setUserData(cols)
                horSizer = sizer.makeNew()
                vertSizer.addChild(horSizer)
                panel.addChild(vertSizer)


            c = horSizer._getNumChildren()
            keys = thumbs.keys()
            keys.sort()
            for key in keys:
                (fullImage, thumb) = thumbs[key]

                # skip thumbnails that have already been loaded
                if fullImage in panel.getUserData():
                    continue
                else:
                    panel.getUserData().append(fullImage)

                # make new row if needed
                if c == cols: 
                    horSizer = sizer.makeNew()
                    vertSizer.addChild(horSizer)
                    c = 0

                b = thumbnail.makeNew()

                if mediaType == "saved":
                    b.setSize(int(THUMB_SIZE*self.__displayAr), THUMB_SIZE)
                    b.setUpImage( getRelativePath( fullImage, SAGE_DIR) )
                    b.setDownImage( getRelativePath( fullImage, SAGE_DIR) )
                    b.onUp(self.__onLoadState)
                    b.preventAccidentalClicks()
                    b.pos.xMargin = int(10.0*getGlobalScale())
                    b.pos.yMargin = int(10.0*getGlobalScale())
                else:
                    b.setSize(THUMB_SIZE, THUMB_SIZE)
                    b.setUpImage( getRelativePath( opj(THUMBS_DIR, thumb), SAGE_DIR) )
                    b.onUp(self.__onThumbnail)
                    b.preventAccidentalClicks()
                    b.pos.xMargin = int(5.0*getGlobalScale())
                    b.pos.yMargin = int(5.0*getGlobalScale())

                b.setUserData((fullImage, mediaType))
                b.setTransparency(255)
                b.drawOutline(True)
                b.allowDrag(True)
                #b.registerForEvent(EVT_MULTI_TOUCH_HOLD, self.__onMultiTouchHold)  # was for closing the browser...
                b.setTooltip(os.path.basename(fullImage), fontSize=SMALLEST)
                horSizer.addChild(b)
                c += 1

        except:
            print "\n\nERROR occured while loading thumbnails:"
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))



    
    def __makeMediaTypePanel(self, forDir):
        """ makes a list of buttons on the left side of the display """
        ## p = panel.makeNew()
##         p.drawBorder(True)
##         p.align(LEFT, CENTER)
##         p.setTransparency(100)
##         p.setBackgroundColor(20,20,20)
        
        horSizer = sizer.makeNew(direction=VERTICAL)
        horSizer.align(LEFT, CENTER)
        #p.addChild(horSizer)

        firstBtn = None  # first media button

        l = label.makeNew(label="Media")
        l.drawBackground(False)
        l.pos.yMargin = 5
        l.pos.xMargin = 5
        l.setFontColor(255,255,255)
        horSizer.addChild(l)
        

        dirs = os.listdir(forDir)
        dirs.sort()
        for item in dirs:
            if item.endswith("lost+found"):
                continue
            if item.endswith("thumbnails"):
                continue
            
            fullPath = opj(forDir, item)
            if os.path.isdir(fullPath):
                b = button.makeNew()
                
                if item == "image":
                    icon = "image.png"
                    icon_down = "image_down.png"
                    firstBtn = b
                elif item == "video":
                    icon = "video.png"
                    icon_down = "video_down.png"
                elif item == "pdf":
                    icon = "pdf.png"
                    icon_down = "pdf_down.png"
                elif item == "audio":
                    icon = "audio.png"
                    icon_down = "audio_down.png"
                elif item == "office":
                    icon = "office.png"
                    icon_down = "office_down.png"
                elif item == "3dpano":
                    icon = "3d.png"
                    icon_down = "3d_down.png"
                else:
                    icon = "dir.png"
                    icon_down = "dir_down.png"
                    b.setLabel(item, fontSize=NORMAL)
                    b.setFontColor(0,0,0)

                b.setSize(THUMB_SIZE, THUMB_SIZE)
                b.setUpImage(opj("images", icon))
                b.setDownImage(opj("images", icon_down))
                b.setOverImage(opj("images", icon_down))
                b.alignLabel(CENTER)
                b.onDown(self.__onDirBtn)
                b.onUp(self.__onDirBtn)
                b.setToggle(True, doManual=True)
                b.setUserData((fullPath, item, 0))
                b.pos.xMargin = 5
                b.pos.yMargin = 5
                horSizer.addChild(b)


        # screenshot button
        """
        l = label.makeNew(label="Screenshot")
        l.drawBackground(False)
        l.pos.yMargin = 75
        l.pos.xMargin = 5
        l.setFontColor(255,255,255)
        horSizer.addChild(l)
        
        sbs = int(THUMB_SIZE - THUMB_SIZE*0.2)
        b = button.Button()
        b.setUpImage(opj("images", "screenshot_up.png"))
        b.setDownImage(opj("images", "screenshot_down.png"))
        b.setOverImage(opj("images", "screenshot_down.png"))
        b.setSize(sbs, sbs)
        b.align(LEFT, TOP)
        b.setTransparency(80)
        b.onUp(self.__onScreenshotBtn)
        b.pos.xMargin = 10
        b.pos.yMargin = 5
        horSizer.addChild(b)
        """


        l = label.makeNew(label="Session")
        l.drawBackground(False)
        l.pos.yMargin = 75
        l.pos.xMargin = 5
        l.setFontColor(255,255,255)
        horSizer.addChild(l)

        # make the save button
        self.__saveBtn = button.makeNew()
        self.__saveBtn.setLabel("Save", fontSize=NORMAL)
        self.__saveBtn.setFontColor(0,0,0)
        self.__saveBtn.setSize(THUMB_SIZE, THUMB_SIZE)
        self.__saveBtn.setUpImage(opj("images", "dir.png"))
        self.__saveBtn.setDownImage(opj("images", "dir_down.png"))
        self.__saveBtn.setOverImage(opj("images", "dir_down.png"))
        self.__saveBtn.alignLabel(CENTER)
        self.__saveBtn.preventAccidentalClicks()
        self.__saveBtn.onUp(self.__onSaveBtn)
        self.__saveBtn.onDown(self.__onSaveBtn)
        self.__saveBtn.setToggle(True, doManual=True)
        self.__saveBtn.pos.xMargin = 5
        self.__saveBtn.pos.yMargin = 5
        horSizer.addChild(self.__saveBtn)

        # make the load button
        b = button.makeNew()
        b.setLabel("Load", fontSize=NORMAL)
        b.setFontColor(0,0,0)
        b.setSize(THUMB_SIZE, THUMB_SIZE)
        b.setUpImage(opj("images", "dir.png"))
        b.setDownImage(opj("images", "dir_down.png"))
        b.setOverImage(opj("images", "dir_down.png"))
        b.alignLabel(CENTER)
        b.setToggle(True, doManual=True)
        b.onDown(self.__onDirBtn)
        b.setUserData((SAVED_STATES_DIR, "saved", 0))
        b.pos.xMargin = 5
        b.pos.yMargin = 5
        horSizer.addChild(b)
                
        return (horSizer, firstBtn)
        #return (p, firstBtn)



    def __makeSubDirPanel(self, forDir, mediaType, level):
        """ makes a list of subdirectories """
        def __makeDirThumb(pth, name, margin=5):
            b = thumbnail.makeNew()
            b.setSize(THUMB_SIZE*2, THUMB_SIZE/3)
            b.setUpImage(opj("images", "plain_button_wide_up.png"))
            b.setDownImage(opj("images", "plain_button_wide_down.png"))
            b.setOverImage(opj("images", "plain_button_wide_down.png"))
            b.setToggle(True, doManual=True)
            b.allowDrag(True)
            b.setLabel(name, fontSize=SMALLER)
            b.alignLabel(CENTER)
            b.onDown(self.__onDirBtn)
            b.onUp(self.__onDirBtn)
            b.setTransparency(255)
            b.setUserData((pth, mediaType, level))
            b.pos.xMargin = 25
            b.pos.yMargin = margin
            vertSizer.addChild(b)
            
        
        # if the panel already exists, don't re-make it... just unhide it
        self.__currentDir = forDir
        if forDir in self.__loadedDirPanels:
            return self.__loadedDirPanels[forDir]

        vertSizer = sizer.makeNew(direction=VERTICAL)

        numDirs = 0
        dirs = os.listdir(forDir)
        dirs.sort()
        trashDir = None
        for item in dirs:
            fullPath = opj(forDir, item)
            if os.path.isdir(fullPath):
                if item == "Trash":
                    trashDir = fullPath
                else:
                    __makeDirThumb(fullPath, item)
            numDirs += 1

        if trashDir:
            __makeDirThumb(trashDir, "Trash", margin=40)
            numDirs += 1
        
        p = None
        if numDirs > 0:
            p = panel.makeNew()
            p.setTransparency(1)
            p.setBackgroundColor(0,0,0)
            p.align(CENTER, CENTER)
            p.addChild(vertSizer)

        self.__loadedDirPanels[forDir] = p
        return p


    def __onDirBtn(self, clickedBtn):
        """ one of the directories was clicked...
            includes the main buttons (pdf, video, audio, image...)
        """
        dirPath, mediaType, level = clickedBtn.getUserData()

        if level == 0 and not self.browserPanel.isVisible(): 
            self.browserPanel.show()

        # opening the same directory... don't do anything
        if self.__currentDir == dirPath:
            return

        # hide old levels
        for i in range(max(level-1, 0), len(self.__visibleDirPanels)):
            dirBtn, subDirPanel, thumbPanel = self.__visibleDirPanels[i]

            # we need to hide the panel from one level above
            if thumbPanel: thumbPanel.hide()
            
            if i>=level: 
                if subDirPanel: subDirPanel.hide()
                if dirBtn:      dirBtn.setState(BUTTON_UP)
                del self.__visibleDirPanels[i]

        # load and show new levels
        subDirPanel, thumbPanel = self.__loadLevel(level, dirPath, mediaType)
        self.__visibleDirPanels[level] = (clickedBtn, subDirPanel, thumbPanel)
        clickedBtn.setState(BUTTON_DOWN)


    def __onSaveBtn(self, btn):
        writeLog(LOG_STATE_SAVE)
        self.__saveBtn.setState(BUTTON_DOWN)
        saveScreenshot(caller=self)   # in globals.py...
        #btn.setState(BUTTON_UP)  # set below


        # called after the screenshot is actually saved
    def __onScreenshotSaved(self, event):
        if event.caller == self and event.stateSaved:
            self.__saveBtn.setState(BUTTON_UP)
        

    def __makeThumbPanel(self, forDir, mediaType):
        # if the panel already exists, don't re-make it... just unhide it
        if forDir in self.__loadedThumbPanels:
            p = self.__loadedThumbPanels[forDir]
            p.show()

        else:   # otherwise, make a new one and make the thumbs
            p = panel.makeNew()
            p.setTransparency(1)
            p.setBackgroundColor(0,0,0)

            align = CENTER
            if mediaType == "saved":
                align = BOTTOM
            
            p.align(CENTER, align, xMargin=10, yMargin=10)
            p.setUserData([])  # for keeping a list of all shown thumbnails in this dir

            self.__loadedThumbPanels[forDir] = p
            self.browserSizer.addChild(p)
            
        # make the thumbs... if the panel exists already, just make the new ones
        self.t = Thread(target=self.__loadThumbs, args=(p, forDir, mediaType))
        self.__makingThumbs = True
        self.t.start()
        
        return p
    

    def __loadLevel(self, level, dirPath, mediaType):
        self.__makingThumbs = False
        if self.t: self.t.join()
        
        # make the sub dir panel
        subDirPanel = None
        if mediaType != "saved":
            subDirPanel = self.__makeSubDirPanel(dirPath, mediaType, level+1)
            if subDirPanel:
                subDirPanel.show()
                self.dirSizer.addChild(subDirPanel)
            
        
        # show the thumb panel
        thumbPanel = self.__makeThumbPanel(dirPath, mediaType)

        return subDirPanel, thumbPanel


        # called when the button is clicked
    def __onScreenshotBtn(self, btn):
        btn._lastDevice.setCaptureMode(True)


    def __onThumbnail(self, thumbBtn):
        fullPath, mediaType = thumbBtn.getUserData()
        d = self.sageData.getDisplayInfo()

        # test for wall= show file in center
        path, filename = os.path.split(fullPath)
        fileInfo = getFileServer().GetFileInfo(filename, 0, path)
        if not fileInfo:   #file not supported
            return
        (fileType, size, fullPath, appName, params, fileExists) = fileInfo
        if not fileExists:  #file doesnt exist
            return
        if fileType == "image":
            xx = (d.sageW/2) - (size[0]/2)
            yy = (d.sageH/2) - (size[1]/2)
        else:
            xx = (d.sageW/2)
            yy = (d.sageH/2)
        showFile(fullPath, xx, yy)
        #showFile(fullPath)


    def __onLoadState(self, thumbBtn):
        fullPath, mediaType = thumbBtn.getUserData()
        stateName = os.path.splitext( os.path.basename(fullPath) )[0]
        #print "Loading state: ", os.path.splitext( os.path.basename(fullPath) )[0]
        self.sageData.loadState(stateName)


#    def __onWallClicked(self, evt):
#        #self.browserPanel.slideHide()


    def __onMultiTouchHold(self, event):
        if event.lifePoint == GESTURE_LIFE_BEGIN:
            self.__onCloseBrowser(None)


    def __onCloseBrowser(self, btn):
        self.browserPanel.hide()

        # force refresh of panels and thumbs
        self.__currentDir = None 

        self.__makingThumbs = False
        if self.t: self.t.join()
        
        if 0 in self.__visibleDirPanels:
            self.__visibleDirPanels[0][0].setState(BUTTON_UP)

        
        
#######################   AUXILIARY METHODS  ##########################

    def __getFilesDir(self):
        global FILES_DIR, THUMBS_DIR

        f = open(CONFIG_FILE, "r")
        for line in f:
            line = line.strip()
            if line.startswith("FILES_DIR"):
                FILES_DIR = line.split("=")[1].strip()
                if not os.path.isabs(FILES_DIR):
                    FILES_DIR = getUserPath("fileServer", FILES_DIR)
                FILES_DIR = os.path.realpath(FILES_DIR)  #expand any symbolic links in the library directory
                THUMBS_DIR = opj(FILES_DIR, "thumbnails")
        f.close()
        
  
    def __GetThumbName(self, fullPath, mediaType):
        fileSize = os.stat(fullPath).st_size
        (root, ext) = os.path.splitext( os.path.basename(fullPath) )
        thumbName = root+str(fileSize)+ext        #construct the thumb name

        # all the thumbs have .jpg appended to them
        thumbName += ".jpg"
        
        return opj( THUMBS_DIR, thumbName )

