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


import copy
from threading import Timer
from time import time
from random import choice, randint
from globals import *
import overlays.menu as menu
import overlays.button as button
import overlays.icon as icon
import overlays.label as label
import overlays.sizer as sizer
import overlays.panel as panel


def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return FittsStudy()


# must have this to load the plugin automatically
# to disable the plugin, just comment it out
#TYPE = UI_PLUGIN


global addWidget, removeWidget, sageGate, registerEvent, unregisterEvent


# for running on a local machine
DEBUG = False


REPEAT_COMBO = 2  # how many times to repeat a combo in each trial
REPEAT_BLOCKS = 2 # how many blocks per device
 

if DEBUG:
    positions = {0:800, 1:1000, 2:1200, 3:1500}
    yPos = 600
    MSG_LABEL_SIZE = 20

    # sizes of targets used
    SMALL = 50
    MEDIUM = 90
    LARGE = 120
    LARGEST = 150

else:
    positions = {0:2500, 1:7100, 2:11700, 3:16300}
    yPos = 3200
    MSG_LABEL_SIZE = 32

    # sizes of targets used
    SMALL = 200
    MEDIUM = 400
    LARGE = 600
    LARGEST = 800



sizes = []  # a list of all the sizes
for i in range(0, REPEAT_COMBO):
    sizes.extend([SMALL, MEDIUM, LARGE, LARGEST])



global r1,r2,deviceLabel,bL,targetBtn,tL
tL=r1=r2=deviceLabel=bL = None




# the whole study for one participant
class FittsStudy:
    def __init__(self):
        global addWidget, removeWidget, sageGate, registerEvent, unregisterEvent
        addWidget = getOverlayMgr().addWidget
        removeWidget = getOverlayMgr().removeWidget
        registerEvent = getEvtMgr().register
        unregisterEvent = getEvtMgr().unregister
        sageGate = getSageGate()        
    
        self.practice = Practice()
        self.targetEval = TargetEval()

        self.devices = ["wiimote", "mouse", "gyro"]
        self.device = ""
        self.participant = 10

        self.makeWidgets()

    
    def makeWidgets(self):
        global bL
        s = sizer.makeNew(direction=VERTICAL)
        s.align(RIGHT, TOP)

        pB = button.makeNew()
        pB.setLabel("New Participant")
        pB.onUp(self.onNewParticipantBtn)
        self.pL = label.makeNew(label="Participant")
        self.pL.setSize(150,20)
        self.pL.setBackgroundColor(0,0,0)

        cB = button.makeNew()
        cB.setLabel("Restart Condition")
        cB.pos.yMargin = 70
        cB.onUp(self.onResetConditionBtn)
        self.cL = label.makeNew(label="Device")
        self.cL.setSize(150,20)
        self.cL.setBackgroundColor(0,0,0)
        bL = label.makeNew(label="Block")
        bL.setSize(150,20)
        bL.setBackgroundColor(0,0,0)

        global tL
        self.thanksLabel = label.makeNew(label="Thank you for participating!", fontSize=MSG_LABEL_SIZE)
        self.thanksLabel.setBackgroundColor(0,0,0)
        self.thanksLabel.alignX(CENTER)
        self.thanksLabel.hide()
        tL = self.thanksLabel

        global deviceLabel
        deviceLabel = label.makeNew(label="Please use %s"%self.device, fontSize=MSG_LABEL_SIZE)
        deviceLabel.setBackgroundColor(0,0,0)
        deviceLabel.alignX(CENTER)
        deviceLabel.hide()

        global r1, r2
        r1 = label.makeNew(label="You may rest now.",fontSize=MSG_LABEL_SIZE)
        r1.setBackgroundColor(0,0,0)
        r1.alignX(CENTER)
        r1.hide()
        r2 = label.makeNew(label="When you are ready, click the yellow circle",fontSize=MSG_LABEL_SIZE)
        r2.setBackgroundColor(0,0,0)
        r2.alignX(CENTER)
        r2.hide()


        self.practiceBtn = button.makeNew()
        self.practiceBtn.setLabel("Start Practice")
        self.practiceBtn.setToggle(True)
        self.practiceBtn.pos.yMargin = 70
        self.practiceBtn.onDown(self.startPractice)
        self.practiceBtn.onUp(self.stopPractice)
        addWidget(self.practiceBtn)

        global targetBtn
        targetBtn = button.makeNew()
        targetBtn.setLabel("Start target eval")
        targetBtn.setToggle(True)
        targetBtn.pos.yMargin = 70
        targetBtn.onDown(self.startTargetEval)
        targetBtn.onUp(self.stopTargetEval)

        msgSizer = sizer.makeNew(direction=VERTICAL)
        msgSizer.align(CENTER, CENTER)
        msgSizer.addChild(deviceLabel)
        msgSizer.addChild(r1)
        msgSizer.addChild(r2)
        msgSizer.addChild(self.thanksLabel)
        addWidget(msgSizer)

        s.addChild(pB)
        s.addChild(self.pL)
        s.addChild(cB)
        s.addChild(self.cL)
        s.addChild(bL)
        s.addChild(self.practiceBtn)
        s.addChild(targetBtn)
        addWidget(s)


    def startPractice(self, btn):
        self.practice.start()
        self.practiceBtn.setLabel("Stop Practice")
        tL.hide()

    def stopPractice(self, btn):
        self.practice.stop()
        self.practiceBtn.setLabel("Start Practice")


    def startTargetEval(self, btn):
        self.thanksLabel.hide()
        targetBtn.setLabel("Stop target eval")
        self.targetEval.start()

    def stopTargetEval(self, btn):
        targetBtn.setLabel("Start target eval")
        self.targetEval.stop()
        


    def onNewParticipantBtn(self, btn):
        self.reset()
        self.participant += 1
        self.devices = ["mouse", "Gyromouse", "Wiimote"]
        self.device = self.devices[0] #choice(self.devices)
        self.devices.remove(self.device)
        self.thanksLabel.hide()

        self.startCondition()


    def startCondition(self):
        self.pL.setLabel("Participant %d" % self.participant)
        self.cL.setLabel("Device %s" % self.device)

        deviceLabel.setLabel("Please use %s"%self.device)
        deviceLabel.show()
        r2.show()
        if len(self.devices) != 2:
            r1.show()

        self.condition = Condition(self.participant, self.device, self.onConditionComplete)
                    

    def onConditionComplete(self):
        print ">>>>>>>  CONDITION COMPLETE: ", self.device
        if len(self.devices) > 0:
            self.device = self.devices[0] #choice(self.devices)
            self.devices.remove(self.device)
            self.startCondition()
        else:
            r1.hide()
            r2.hide()
            self.thanksLabel.show()
            print "\n>>>>>>>>>>>>>  PARTICIPANT COMPLETE: ", self.participant
        

    def onResetConditionBtn(self, evt):
        self.reset()
        self.startCondition()

        
    def reset(self):
        if hasattr(self, "condition"):
            self.condition.reset()
            del self.condition




# one condition (i.e. one interaction device)        
class Condition:
    def __init__(self, participant, device, onComplete):
        self.onComplete = onComplete

        self.device = device
        self.participant = participant
        self.blockNum = 1
        self.data = []

        print ">>>>>>>> STARTING CONDITION:", self.device

        self.startBlock()


    def startBlock(self):
        if self.blockNum <= REPEAT_BLOCKS:
            self.block = Block(self.participant, self.device, self.blockNum, self.onBlockComplete)
        else:
            self.saveToFile()
            self.onComplete()
        
    
    def onBlockComplete(self):
        print ">>>>  BLOCK COMPLETE:", self.blockNum
        getEvtMgr().getWallObj().stopRecording()
        self.blockNum += 1
        self.data.extend(self.block.data)
        del self.block
        if self.blockNum <= REPEAT_BLOCKS:
            r1.show()
            r2.show()
        self.startBlock()

        
    def saveToFile(self):
        f = open("fitts.txt", "a")
        f.writelines(self.data) 
        f.flush()
        f.close()


    def onRestartBlockBtn(self, evt):
        self.reset()
        self.startBlock()


    def reset(self):
        if hasattr(self, "block"):
            self.block.reset()
            del self.block



# one run of all the A/W combinations repeated n times
class Block:
    def __init__(self, participant, device, blockNum, onComplete):
        self.makeCombos()
        self.b = None  # this is our target widget
        self.numCombos = 0
        self.onComplete = onComplete
        self.__callbacks = {}

        # this is the stuff we record
        self.size = LARGE
        self.targetShownTime = 0.0
        self.device = device
        self.participant = participant
        self.dist = 0  
        self.pos = 0  # extreme position for the first target
        self.errors = 0
        self.blockNum = blockNum

        self.data = [] # a list of lists holding all the above information
        
        bL.setLabel("Block %d"%self.blockNum)
        print ">> STARTING BLOCK:", self.blockNum

        # start it off
        # TODO: discard the first target
        t = Timer(3.0, self.makeTarget)
        t.start()
        #self.makeTarget()


    def makeCombos(self):
        self.combos = {}  #key=dist, value=[sizes]
        for i in [1,2,3]:  # 1=closest, 3=farthest
            self.combos[i] = copy.copy(sizes)

    
    def registerForErrors(self):
        self.errors = 0
        self.__callbacks[EVT_WALL_CLICKED] = self.onError
        registerEvent(EVT_WALL_CLICKED, self.onError)

    def unregister(self):
        unregisterEvent(self.__callbacks)
        self.__callbacks.clear()
        

    
    def makeTarget(self):
        self.removeOldTarget()

        if not self.pickNextCombo():
            if self.pos == 0:
                self.unregister()
                self.tempSave()
                self.onComplete()
                return
            else:
                self.pos = 0

        # make new one
        self.error = 0
        x = positions[self.pos]-int(self.size/2.0)
        y = yPos - int(self.size/2.0)
        self.b = button.makeNew()
        self.b._canScale = False  # so that it doesn't change the size
        self.b._roundEvt = True
        self.b.setSize(self.size, self.size)
        self.b.setPos(x, y)
        self.b.setUpImage("images/target.png")
        self.b.onDown(self.onTargetClicked)
        addWidget(self.b)

        # make the target
        self.targetShownTime = time() # start the timer


    def onError(self, evt):
        self.errors += 1

    
    def onTargetClicked(self, btn):
        # TODO: add error count
        # record data and start next target
        timeTaken = time() - self.targetShownTime
        self.numCombos += 1
        if self.numCombos > 1:
            line = "%d %d %s %f %d %d %d\n" % (self.participant, self.blockNum, self.device, timeTaken, self.size, self.dist, self.errors)
            print line
            self.data.append(line)
        else:
            getEvtMgr().getWallObj().startRecording(self.device)
            self.registerForErrors()
            deviceLabel.hide()
            r1.hide()
            r2.hide()
        self.errors = 0
        self.makeTarget()

        
    def tempSave(self):
        f = open("fitts_bak.txt", "a")
        f.write("\n--------------- new block %d -------------\n"%self.blockNum)
        f.writelines(self.data) 
        f.flush()
        f.close()


    def pickNextCombo(self):
        ad = self.combos.keys()
        if len(ad) == 0:
            return False

        if self.numCombos % 2 == 1:  # odd ones, find random target
            self.pos = choice(ad)
            self.size = choice(self.combos[self.pos])
            self.combos[self.pos].remove(self.size)
            if len(self.combos[self.pos]) == 0:
                del self.combos[self.pos]
            
            self.dist = positions[self.pos] - positions[0]
        else:
            self.pos = 0  # size and distance stay the same
            
        return True
        

    def removeOldTarget(self):
        if self.b:
            removeWidget(self.b)
            self.b = None


    def reset(self):
        self.unregister()
        self.removeOldTarget()




class Practice:
    def __init__(self):
        pass


    def makeButtons(self):
        for i in range(0,5):
            self.makeNewBtn()


    def makeNewBtn(self):
        x = randint(positions[0], positions[3])
        y = randint(int(yPos/3.0), int(yPos + yPos*0.6))

        b = button.makeNew()
        b._canScale = False  # so that it doesn't change the size
        b._roundEvt = True
        b.setSize(LARGE, LARGE)
        b.setPos(x, y)
        b.setUpImage("images/target.png")
        b.onDown(self.onBtn)
        addWidget(b)
        self.widgets.append(b)


    def onBtn(self, btn):
        removeWidget(btn)
        self.widgets.remove(btn)
        self.makeNewBtn()


    def start(self):
        self.widgets = []
        self.makeButtons()


    def stop(self):
        for b in self.widgets:
            removeWidget(b)
        self.widgets = []



        

class TargetEval:
    def __init__(self):
        self.pos = [2568,4327,5991,7680,9403,11139,12769,14628,16298]
        #self.pos = [900, 1000]
        self.lastX = 0
        

    def makeNewBtn(self):
        if self.numButtons == 10:
            if self.size == LARGE:
                self.size = MEDIUM
            elif self.size == MEDIUM:
                self.size = SMALL
            else:
                targetBtn.setLabel("Start target eval")
                tL.show()
                return
            self.numButtons = 0
                        

        x = choice(self.pos)
        while x == self.lastX:    
            x = choice(self.pos)
        self.lastX = x

        self.b = button.makeNew()
        self.b._canScale = False  # so that it doesn't change the size
        self.b._roundEvt = True
        self.b.setSize(self.size, self.size)
        self.b.setPos(x-int(self.size/2.0), yPos - int(self.size/2.0))
        self.b.setUpImage("images/target.png")
        self.b.onDown(self.onBtn)
        addWidget(self.b)


    def onBtn(self, btn):
        self.numButtons += 1
        removeWidget(self.b)
        self.makeNewBtn()


    def start(self):
        self.numButtons = 0
        self.size = LARGE
        self.makeNewBtn()


    def stop(self):
        removeWidget(self.b)
        self.numButtons = 0
        self.size = LARGE


