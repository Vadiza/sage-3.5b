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

from globals import *
import overlays.menu as menu
import overlays.button as button
import overlays.icon as icon
import overlays.label as label
import overlays.sizer as sizer
import overlays.panel as panel
import overlays.app as app
import time, os, ConfigParser
from threading import Thread


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return WallUI()


# must have this to load the plugin automatically
TYPE = UI_PLUGIN

class WallUI:
    def __init__(self):
        if not SHOW_USER_INTERFACE:
            return

	# Read the configuration file
	self.config = ConfigParser.ConfigParser()
	self.config.read(DIM_CONFIG)

	# parse background options
	self.file_background = getValue(self.config,'background','file',"images/backgrounds/pattern.jpg")
	self.show_background = getValue(self.config,'background','show',True)

	# parse logo options
	self.file_logo   = getValue(self.config,'logo','file', "images/evl-logo.png")
	self.show_logo   = getValue(self.config,'logo','show',True)
	self.logo_trans  = getValue(self.config,'logo','transparency',16)
	self.logo_width  = getValue(self.config,'logo','width',1500)
	self.logo_height = getValue(self.config,'logo','height',567)

	# parse corner picture options
	self.file_corner   = getValue(self.config,'corner','file', "images/evl-logo.png")
	self.show_corner   = getValue(self.config,'corner','show',True)
	self.corner_trans  = getValue(self.config,'corner','transparency',200)
	self.corner_width  = getValue(self.config,'corner','width',185)
	self.corner_height = getValue(self.config,'corner','height',70)

        self.addWidget = getOverlayMgr().addWidget
        self.sageGate = getSageGate()

	# Make a quit button
	has_quit = getValue(self.config, 'buttons', 'quit_button', False)
	if has_quit:
		self.makeShutdownBtn()

	# Make application buttons
	has_appbutton = getValue(self.config,'buttons','application_buttons',False)
	if has_appbutton:
		self.makeAppButtons(self.sageGate.getAppList())

	# Make a clock
	self.has_label = getValue(self.config, 'buttons', 'label', "")
	has_clock = getValue(self.config, 'buttons', 'clock', True)
	if has_clock:
		self.makeClock()

        self.makeLogo()
   

    def makeClock(self):
        l = time.strftime("%I:%M %p", time.localtime()).lower().lstrip("0")
        self.__oldTime = l
        self.clock = label.Label(label=l, fontSize=LARGEST)
        self.clock.align(LEFT, TOP)
        self.clock.drawBackground(False)
        self.clock.setTransparency(250)
        self.addWidget(self.clock)
	if self.has_label:
		self.iplabel = label.Label(label=self.has_label, fontSize=LARGEST)
		self.iplabel.align(LEFT, TOP, 300,0)
		self.iplabel.drawBackground(False)
		self.iplabel.setTransparency(220)
		self.addWidget(self.iplabel)
        self.t = Thread(target=self.runClock)
        self.t.start()
        

    def makeLogo(self):
        if self.show_background:
		self.bg = icon.Icon(app=None, imageFile=self.file_background)
		self.bg.align(LEFT, BOTTOM)
		self.bg.z = BOTTOM_Z
		self.bg.setSize(1.0,1.0)  # 1.0,1.0 means full width and full height
		self.addWidget(self.bg)

        if self.show_logo:
		self.logo = icon.Icon(app=None, imageFile=self.file_logo)
		self.logo.align(CENTER, CENTER)
		self.logo.z = BOTTOM_Z+1
		self.logo.setSize(self.logo_width,self.logo_height)
		self.logo.setTransparency(self.logo_trans)
		self.addWidget(self.logo)

        if self.show_corner:
		self.corner = icon.Icon(app=None, imageFile=self.file_corner)
		self.corner.align(RIGHT, BOTTOM, xMargin=10, yMargin=0)
		#self.corner.setSize(self.corner_width,self.corner_height)
		self.corner.setSize(self.corner_width,self.corner_height)
		self.corner.z = BOTTOM_Z
		self.corner.setTransparency(self.corner_trans)
		self.addWidget(self.corner)
        

    def runClock(self):
        while doRun():
            time.sleep(2)
            l = time.strftime("%I:%M %p", time.localtime()).lower().lstrip("0")
            if l != self.__oldTime:
                self.clock.setLabel(l)
                self.__oldTime = l
        
        
    def makeShutdownBtn(self):
        sb = button.makeNew()
        sb.setSize(65,65)
        sb.align(RIGHT, TOP)
        sb.setUpImage("images/on_off.png")
        sb.onUp(self.__onSageShutdownBtn)
        self.addWidget(sb)
    

    def __onSageShutdownBtn(self, btn):
        self.sageGate.shutdownSAGE()
    

    def makeAppButtons(self, appList):
        p = panel.makeNew()
        p.setBackgroundColor(200,0,0)
        p.setTransparency(100)
        p.align(CENTER, BOTTOM)

        appBtnSizer = sizer.makeNew()
        appBtnSizer.align(CENTER, BOTTOM)
    
        # add 4K movies from fatboy
        launcherId, movies = self.__getFatboyMovies()
        if launcherId:
            m = menu.makeNew()
            m.setSize(150, 30)
            m.alignLabel(CENTER)
            m.setLabel("4K movies")
            m.pos.xMargin = 5
            for movie in movies:
                m.addMenuItem(movie, self.__on4KMovie, launcherId)
            appBtnSizer.addChild(m)                
        
        # add the rest of the apps
        media_apps = [ "mplayer", "imageviewer", "pdfviewer", "VNCViewer", "officeviewer", "Stereo3D" ]
        # add the rest of the apps
        for appName, data in appList.iteritems():
            if appName not in media_apps:
                m = menu.makeNew()
                m.setSize(150, 30)
                m.alignLabel(CENTER)
                m.setLabel(appName)
                m.pos.xMargin = 5
                for cfgName in data:
                    m.addMenuItem(cfgName, self.__onAppBar, appName)

                appBtnSizer.addChild(m)                

        p.addChild(appBtnSizer)
        self.addWidget(p)


    def __on4KMovie(self, mi):
        self.sageGate.executeRemoteApp(mi.getUserData(), "bitplayer", mi.getLabel())

            
    def __onAppBar(self, mi):
        self.sageGate.executeApp(mi.getUserData(), mi.getLabel())


    def __getFatboyMovies(self):
        launcherHash = self.sageGate.getLaunchers()
        launcher = None
        launcherId = None
        movieList = []

        for lId, l in launcherHash.items():
            if "fatboy" in l.getName():
                launcher = l
                launcherId = launcher.getId()
                break
        
        if launcher:
            appHash = l.getAppList()
            appList = appHash.keys()
            if "bitplayer" in appList:
                movieList = appHash["bitplayer"]
                movieList.sort(lambda x, y: cmp(x.lower(), y.lower()))
                
        return launcherId, movieList
    
