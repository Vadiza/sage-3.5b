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
# Creates sound feedback for SAGE
#
# Author: Luc RENAMBOT
#
########################################################################


from globals import *
import overlays.button as button
import overlays.sizer as sizer
import overlays.app as app
import overlays.icon as icon
import overlays.label as label
import overlays.multiSelect as multiSelect

# OpenSoundControl library
import threading
import OSC


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return SoundUI()


# must have this to load the plugin automatically
TYPE = UI_PLUGIN


class SoundUI:
    def __init__(self):
        self.addWidget = getOverlayMgr().addWidget
        self.sageGate = getSageGate()  # connection with sage for closing apps
        self.evtMgr = getEvtMgr()
        self.sageData = getSageData()

        # register for some app events so that we can create widgets for them
        self.evtMgr.register(EVT_NEW_APP, self.onNewApp)
        self.evtMgr.register(EVT_APP_KILLED, self.onCloseApp)
        self.evtMgr.register(EVT_APP_INFO, self.onInfoApp)
        self.evtMgr.register(EVT_DOUBLE_CLICK, self.onDoubleClick)
        self.evtMgr.register(EVT_Z_CHANGE, self.onZChange)

        # self.registerForEvent(EVT_ENTERED_WINDOW, self.__onEnteredWindow)
        # self.registerForEvent(EVT_LEFT_WINDOW, self.__onLeftWindow)
        # self.registerForEvent(EVT_MOVE, self.__onMove)
        # self.registerForEvent(EVT_CLICK, self.__onClick)
        # self.registerForEvent(EVT_DRAG, self.__onDrag)
        # self.registerForEvent(EVT_ZOOM, self.__onZoom)
        # self.onDrop(self.__onDrop)

        # go through existing apps
        for app in getSageData().getRunningApps().values():
            self.addAppWidgets(app)

        # get display information
        self.displayInfos = self.sageData.getDisplayInfo().getValues()
        # return [tileNumber, dimX, dimY, desktopWidth, desktopHeight, tileWidth, tileHeight, displayId]
        print("Display: ", self.displayInfos)

        # OSC client creation
        self.oscClient = None
        self.ready = False
        
        # OSC server creation
        # Zone2
       	# self.oscServer = OSC.ThreadingOSCServer(("109.171.138.145", 22222))
       	# EVL sound server (lynch.evl.uic.edu)
       	self.oscServer = OSC.ThreadingOSCServer(("131.193.77.178", 22222))
       	# localhost, for testing
       	#self.oscServer = OSC.ThreadingOSCServer(("127.0.01", 22222))
	print self.oscServer
	self.oscServer.setSrvErrorPrefix("/error")
	self.oscServer.setSrvInfoPrefix("/serverinfo")
	self.oscServer.addDefaultHandlers()
	self.oscServer.addMsgHandler("/print", self.osc_printing_handler)
	self.oscServer.addMsgHandler("/connect", self.osc_connecting_handler)
	self.oscServer.addMsgHandler("/printed", self.oscServer.msgPrinter_handler)
	self.oscServer.addMsgHandler("/serverinfo", self.oscServer.msgPrinter_handler)
	self.st = threading.Thread(target=self.oscServer.serve_forever)
	self.st.start()

    # define a message-handler function for the server to call.
    def osc_printing_handler(self, addr, tags, stuff, source):
        msg_string = "%s [%s] %s" % (addr, tags, str(stuff))
        sys.stdout.write("OSCServer Got: '%s' from %s\n" % (msg_string, OSC.getUrlStr(source)))
        # send a reply to the client.
        msg = OSC.OSCMessage("/printed")
        msg.append(msg_string)
        return msg


    # define a message-handler function for the server to call.
    def osc_connecting_handler(self, addr, tags, stuff, source):
        msg_string = addr
        CLIENT_ip = source[0]
        CLIENT_port = source[1]
        sys.stdout.write("OSCServer connection: '%s' from %s port %s\n" % (msg_string, CLIENT_ip, CLIENT_port))

        # Force the port number for MAX/MSP
        self.oscServer.return_port = 12000

	# Message bundle (group of messages)
        bund = OSC.OSCBundle()

	# First message: info about the wall
        msg = OSC.OSCMessage()
	#	  /sageinfo: tileNumber, dimX, dimY, desktopWidth, desktopHeight, tileWidth, tileHeight
        msg.setAddress("/sageinfo")
        msg.append(self.displayInfos)
        bund.append(msg)

	# Then, one message per running application
	#	/appstart: appName (string), id (int), x (int), y (int), width (int), height, depth (int, 0: frontmost, N: back), title

        for app in self.sageData.getRunningApps().values():
            # get the application parameters
            appName = app.getName()
            appid = app.getId()
            appleft = app.getLeft()
            appright = app.getRight()
            appbottom = app.getBottom()
            apptop = app.getTop()
            appsailID = app.getSailID()
            appzValue = app.getZvalue()
            appconfigNum = app.getConfigNum()
            apptitle = app.getTitle()
		# build the message
            apm = OSC.OSCMessage()
            apm.setAddress("/appstart")
            apm.append( [ appName, appid, appleft, appbottom, appright-appleft, apptop-appbottom, appzValue, apptitle ] )
            bund.append(apm)
        print "Sending ", bund
        self.ready = True
        return bund

    def onNewApp(self, event):
	app = event.app
	appId = app.getId()
	print("New app: ", appId)
	if self.ready:
            # Application start message
            #	/appupdate: appName (string), id (int), x (int), y (int), width (int), height, depth (int, 0: frontmost, N: back)
            
            # get the application parameters
            appName = app.getName()
            appid = app.getId()
            appleft = app.getLeft()
            appright = app.getRight()
            appbottom = app.getBottom()
            apptop = app.getTop()
            appsailID = app.getSailID()
            appzValue = app.getZvalue()
            appconfigNum = app.getConfigNum()
            apptitle = app.getTitle()
            
		# build the message
            msg = OSC.OSCMessage("/appstart")
            msg.clearData()
            msg.append( [ appName, appid, appleft, appbottom, appright-appleft, apptop-appbottom, appzValue ] )
            print "Sending ", msg
            self.oscServer.client.send(msg)

    def onMaximize(self, app):
	if self.ready:
            # get the application parameters
            appName = app.getName()
            appid = app.getId()
            appleft = app.getLeft()
            appright = app.getRight()
            appbottom = app.getBottom()
            apptop = app.getTop()
            appsailID = app.getSailID()
            appzValue = app.getZvalue()
            appconfigNum = app.getConfigNum()
            apptitle = app.getTitle()

            # Application start message
            #	/appmaximize: appName, id, x, y, width, height, depth
            
		# build the message
            msg = OSC.OSCMessage("/appmaximize")
            msg.clearData()
            msg.append( [ appName, appid, appleft, appbottom, appright-appleft, apptop-appbottom, appzValue ] )
            print "Sending ", msg
            self.oscServer.client.send(msg)

    def onMinimize(self, app):
	if self.ready:
            # get the application parameters
            appName = app.getName()
            appid = app.getId()
            appleft = app.getLeft()
            appright = app.getRight()
            appbottom = app.getBottom()
            apptop = app.getTop()
            appsailID = app.getSailID()
            appzValue = app.getZvalue()
            appconfigNum = app.getConfigNum()
            apptitle = app.getTitle()

            # Application start message
            #	/appminimize: appName, id, x, y, width, height, depth
            
		# build the message
            msg = OSC.OSCMessage("/appminimize")
            msg.clearData()
            msg.append( [ appName, appid, appleft, appbottom, appright-appleft, apptop-appbottom, appzValue ] )
            print "Sending ", msg
            self.oscServer.client.send(msg)

    def onZChange(self, event):
	if self.ready:
            # Application start message
            #	/appzchange: appName (string), id (int), depth (int, 0: frontmost, N: back)
            
		# build the message
            msg = OSC.OSCMessage("/appzchange")
            msg.clearData()
            msg.append( event.zHash )
            print "Sending ", msg
            self.oscServer.client.send(msg)

    def onInfoApp(self, event):
        app = event.app
        appId = app.getId()
        val = event.myMsg
        if val:
	    # Compute centroid of application
            cx = event.app.getCenterX()
            cy = event.app.getCenterY()
            # Bound to 0 - MAX display size
            if cx < 0:
                cx = 0
            if cx > self.displayInfos[3]:
                cx = self.displayInfos[3]
            if cy < 0:
                cy = 0
            if cy > self.displayInfos[4]:
                cy = self.displayInfos[4]
            # Compute a ratio 0.0-1.0 of placement on the wall
            cx = float(cx) / self.displayInfos[3]
            cy = float(cy) / self.displayInfos[4]
            #print("Info app: ", appId, cx, cy)

            # Maximized window
            if app.isMinimized():
                self.onMinimize(app)
            else:
                # Minized window
                if app.isMaximized():
                    self.onMaximize(app)
                else:
                    # Regular window
                    if self.ready:
                        # Send application update
                        #	/appupdate: appName (string), id (int), x (int), y (int), width (int), height, depth (int, 0: frontmost, N: back)
                        
                        # get the application parameters
                        appName = app.getName()
                        appid = app.getId()
                        appleft = app.getLeft()
                        appright = app.getRight()
                        appbottom = app.getBottom()
                        apptop = app.getTop()
                        appsailID = app.getSailID()
                        appzValue = app.getZvalue()
                        appconfigNum = app.getConfigNum()
                        apptitle = app.getTitle()
                        
                        # build the message
                        msg = OSC.OSCMessage("/appupdate")
                        msg.clearData()
                        msg.append( [ appName, appid, appleft, appbottom, appright-appleft, apptop-appbottom, appzValue ] )
                        print "Sending ", msg
                        self.oscServer.client.send(msg)

    def onDoubleClick(self, event):
        app = event.app
        appId = app.getId()
        print ("onDoubleClick")
	if self.ready:
            # get the application parameters
            appName = app.getName()
            appid = app.getId()
            appleft = app.getLeft()
            appright = app.getRight()
            appbottom = app.getBottom()
            apptop = app.getTop()
            appsailID = app.getSailID()
            appzValue = app.getZvalue()
            appconfigNum = app.getConfigNum()
            apptitle = app.getTitle()
            print ("double click", appName)

    def onCloseApp(self, event):
        app = event.app
        appId = app.getId()
	if self.ready:
            # Send application update
            #       /appkill: id, x, y, width, height, depth
            
            # get the application parameters
            appName = app.getName()
            appid = app.getId()
            appleft = app.getLeft()
            appright = app.getRight()
            appbottom = app.getBottom()
            apptop = app.getTop()
            appsailID = app.getSailID()
            appzValue = app.getZvalue()
            appconfigNum = app.getConfigNum()
            apptitle = app.getTitle()
            
		# build the message
            msg = OSC.OSCMessage("/appkill")
            msg.clearData()
            msg.append( [ appid, appleft, appbottom, appright-appleft, apptop-appbottom, appzValue ] )
            print "Sending ", msg
            self.oscServer.client.send(msg)

        print("Closing app: ", appId)

    def addAppWidgets(self, app):
        print("Existing app: ", app.getId())

