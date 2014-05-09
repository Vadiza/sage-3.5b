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

# for starting gadgets
import subprocess as sp


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return DemoUI()


# must have this to load the plugin automatically
#TYPE = UI_PLUGIN


class DemoUI:
    def __init__(self):

        # shortcuts for less typing
        self.addWidget = getOverlayMgr().addWidget
        self.removeWidget = getOverlayMgr().removeWidget
        self.sageGate = getSageGate()

        self.makeDemoMenu()


    def makeDemoMenu(self):
        m = menu.Menu()
        m.setLabel("Options")
        m.align(LEFT, TOP)
        m.addMenuItem("Global widgets", self.__makeWidgetDemo)
        #m.addMenuItem("App widgets", self.__makeAppWidgetDemo)
        m.addMenuItem("Dynamic widget size", self.__makeDynamicWidgets)
        m.addMenuItem("Dynamic app widget size", self.__makeDynamicAppWidgets)
        #m.addMenuItem("Show Gadgets", self.__makeGadgetDemo)
        #m.addMenuItem("Close panel", self.__closePanel)
        self.addWidget(m)




    #########    CALLBACKS FOR MENU ITEMS    ################

    def __makeWidgetDemo(self, mi):

        l = label.Label()
        l.setLabel("small label")
        
        labelBtn = button.Button()
        labelBtn.setLabel("Larger")
        labelBtn.alignX(RIGHT, 5)
        labelBtn.onDown(self.__biggerLabel)
        labelBtn.onUp(self.__smallerLabel)
        labelBtn.setToggle(True)
        labelBtn.setUserData(l)  # so that we can change the label on click

        sizerBtn = button.Button()
        sizerBtn.setLabel("Show Sizers")
        sizerBtn.align(LEFT, BOTTOM)
        sizerBtn.onDown(self.__onShowSizers)
        sizerBtn.onUp(self.__onHideSizers)
        sizerBtn.setToggle(True)
        
        vs = sizer.Sizer(direction=VERTICAL)
        vs.addChild(labelBtn)
        vs.addChild(l)
        vs.addChild(sizerBtn)

        w1 = button.Button()
        w1.setLabel("vertTop")

        w2 = button.Button()
        w2.setLabel("vertBottom")
        w2.alignX(RIGHT)

        w3 = button.Button()
        w3.setLabel("horLeft")

        w4 = button.Button()
        w4.setLabel("horRight")

        cb = button.Button()
        cb.setSize(50,50)
        cb.alignY(TOP)
        cb.setUpImage("images/close.png")
        cb.onUp(self.__closePanel)

        horSizer = sizer.Sizer()
        horSizer.addChild(w3)
        horSizer.addChild(w4)

        mainSizer = sizer.Sizer(direction=VERTICAL)
        mainSizer.align(LEFT, CENTER)
        mainSizer.addChild(w1)
        mainSizer.addChild(horSizer)   
        mainSizer.addChild(w2)

        hs = sizer.Sizer(direction=HORIZONTAL)
        hs.addChild(vs)
        hs.addChild(mainSizer)
        hs.addChild(cb)

        self.p = panel.Panel()
        self.p.setBackgroundColor(200,0,0)
        self.p.setTransparency(100)
        self.p.setX(0.1)
        self.p.alignY(CENTER)
        self.p.addChild(hs)
        
        # only for demo purposes...
        # wont work when panels are using aligned position... only absolute
        self.p._allowDrag = True 

        # add the parent now (it will add all the children)
        self.addWidget(self.p)



    def __makeAppWidgetDemo(self, mi):
        self.sageGate.executeApp("atlantis")

    def __makeGadgetDemo(self, mi):
        # a hack to start gadgets from the command line
        scriptPath = os.environ["SAGE_DIRECTORY"]+"/bin/run_screenlets"
        sp.call([scriptPath])

    def __makeDynamicWidgets(self, mi):
        doEnlarge( not getEnlargeWidgets() )  # negate whats already there

    def __makeDynamicAppWidgets(self, mi):
        doEnlargeAppWidgets( not getEnlargeAppWidgets() ) # negate whats already there



    #########    CALLBACKS FOR WIDGETS    ################
        
    def __closePanel(self, mi):
        if self.p:
            self.removeWidget(self.p)
    
    def __smallerLabel(self, btnObj):
        btnObj.getUserData().setLabel("small label")  # we stored the label as user data for this button
        btnObj.setLabel("Larger")

    def __biggerLabel(self, btnObj):
        btnObj.getUserData().setLabel("loooooooooooong label")
        btnObj.setLabel("Smaller")

    def __onShowSizers(self, btnObj):
        btnObj.setLabel("Show Sizers")
        self.sageGate.sendOverlayMessage(0, SHOW_SIZERS, 0)

    def __onHideSizers(self, btnObj):
        btnObj.setLabel("Hide Sizers")
        self.sageGate.sendOverlayMessage(0, SHOW_SIZERS, 1)
