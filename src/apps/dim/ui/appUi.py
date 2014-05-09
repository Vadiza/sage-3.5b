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
# Creates a close button for each application.
#
# Author: Ratko Jagodic
#
########################################################################


from globals import *
import overlays.button as button
import overlays.sizer as sizer
import overlays.app as app
import overlays.icon as icon
import overlays.label as label
import overlays.multiSelect as multiSelect


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return AppUI()


# must have this to load the plugin automatically
TYPE = UI_PLUGIN


class AppUI:
    def __init__(self):
        self.addWidget = getOverlayMgr().addWidget
        self.sageGate = getSageGate()  # connection with sage for closing apps
        self.evtMgr = getEvtMgr()
        self.appWidgets = {} # key=appId, value=list of widget objects

        # register for some app events so that we can create widgets for them
        self.evtMgr.register(EVT_NEW_APP, self.onNewApp)
        self.evtMgr.register(EVT_APP_KILLED, self.onCloseApp)

        # go through existing apps and add widgets for them
        for app in getSageData().getRunningApps().values():
            self.addAppWidgets(app)

        # make the widgets on top of the display for all the apps to use
        #self.makeAppMenu()


    def onNewApp(self, event):
        self.addAppWidgets(event.app)
        

    def addAppWidgets(self, app):
        self.appWidgets[app.getId()] = []
        self.makeAppOverlay(app)
        #self.makeAppCloseBtn(app)


    def onCloseApp(self, event):
        """ overlays will be removed by overlayManager so just 
            empty the local list of app related widgets
        """
        appId = event.app.getId()
        if appId in self.appWidgets:
            del self.appWidgets[appId]
        

    def __onAppCloseBtn(self, btn):
        if btn.app:
            self.sageGate.shutdownApp(btn.app.getId())


    def makeAppOverlay(self, appObj):
        a = app.App(appObj)
        self.addWidget(a)

        # store the pointer to the sizer so the app object has access to it
        #a.setUserData(self.appMenuSizer)
        

    def addWidget(self, obj, appObj):
        self.addWidget(obj)
        self.appWidgets[appObj.getId()].append(obj)
