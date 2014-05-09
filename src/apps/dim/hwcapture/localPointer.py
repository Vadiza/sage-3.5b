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



import sys, wx
sys.path.append("../dim/hwcapture")
from managerConn import ManagerConnection
from threading import Timer
import time

SHOW_WINDOW = True

MOVE = 1
CLICK = 2
WHEEL = 3
COLOR = 4
DOUBLE_CLICK = 5

# just used by the desktop mouse capture app
HAS_DOUBLE_CLICK = 10


LEFT=1
RIGHT=2
MIDDLE = 3  

global lastX, lastY
lastX = lastY = 0
global frame, manager, panel, buttons, w, h, label, autoCapture
buttons = {}  #key=BTN_ID, value=True|False
autoCapture = False

# for 
global rightClick
rightClick = []  # [a list of last right click times... up to 3 max]
TRIPLE_CLICK_RELEASE_TIME = 1.0  # seconds for triple click to happen



############    MESSAGING   ###################

def sendMsg(*data):
    msg = ""
    for m in data:
        msg+=str(m)+" "
    manager.sendMessage("", "mouse", msg)

def sendInitialMsg(*data):
    msg = ""
    for m in data:
        msg+=str(m)+" "
    manager.initialMsg("", "mouse", msg)



############    MOUSE CAPTURE / RELEASE   ###################

def captureMouse():
    if not panel.HasCapture():
        label.SetLabel("Press SPACE to release mouse")
        panel.SetCursor(wx.StockCursor(wx.CURSOR_BLANK))
        panel.CaptureMouse()


def releaseMouse():
    if panel.HasCapture():
        label.SetLabel("Press SPACE to capture mouse")
        panel.SetCursor(wx.NullCursor)
        panel.ReleaseMouse()
        

def toggleMouseCapture():
    if panel.HasCapture():
        releaseMouse()
    else:
        captureMouse()


############    EVENTS   ###################

def onKey(event):
    if event.GetKeyCode() == wx.WXK_ESCAPE:
        if panel.HasCapture():
            panel.ReleaseMouse()
        frame.Close()
    elif event.GetKeyCode() == wx.WXK_SPACE:
        global label
        toggleMouseCapture()
    

def onMove(event):
    global lastX, lastY
    ms = wx.GetMouseState()
    mx = ms.GetX()
    my = ms.GetY()
    x = float(mx/float(w))
    y = 1-float(my/float(h))
    if lastX != x and lastY != y:
        sendMsg(MOVE, x,y)
        lastX = x
        lastY = y


def onDoubleClick(event):
    global rightClick
    btnId = None
    if event.GetButton() == wx.MOUSE_BTN_LEFT:
        btnId = LEFT
    elif event.GetButton() == wx.MOUSE_BTN_RIGHT:
        btnId = RIGHT
        
        # triple click will also release mouse pointer
        rightClick.append(time.time())
        if len(rightClick) > 3:
            del rightClick[0]
            
        if len(rightClick) == 3:
            if (rightClick[2] - rightClick[0]) > TRIPLE_CLICK_RELEASE_TIME:
                releaseMouse()
                    
    elif event.GetButton() == wx.MOUSE_BTN_MIDDLE:
        btnId = MIDDLE

    if btnId:
        sendMsg(DOUBLE_CLICK, btnId)

    
def onClick(event):
    global rightClick
    btnId = None
    isDown=False
    if event.ButtonDown():  isDown=True

    if event.GetButton() == wx.MOUSE_BTN_LEFT:
        btnId = LEFT
    elif event.GetButton() == wx.MOUSE_BTN_RIGHT:
        btnId = RIGHT
        
        # triple click will also release mouse pointer
        if isDown:
            rightClick.append(time.time())
            if len(rightClick) > 3:
                del rightClick[0]
            
            if len(rightClick) == 3:
                if (rightClick[2] - rightClick[0]) < TRIPLE_CLICK_RELEASE_TIME:
                    releaseMouse()
            
    elif event.GetButton() == wx.MOUSE_BTN_MIDDLE:
        btnId = MIDDLE

    if btnId:
        sendMsg(CLICK, btnId, int(isDown))


def onWheel(event):
    numSteps = -1*event.GetWheelRotation()/event.GetWheelDelta()
    sendMsg(WHEEL, numSteps)
            

def onClose(event):
    manager.quit()
    frame.Destroy()
    wx.GetApp().ExitMainLoop()





############    MAIN   ###################

def doCapture():
    wx.CallAfter(captureMouse)
    

# runs in a thread, captures mouse position and sends to dim regularly
def main(mgr):
    app = wx.App()
    
    global manager,w,h, autoCapture
    manager = mgr

    sendInitialMsg(HAS_DOUBLE_CLICK)
     
    (w,h) = wx.DisplaySize()

    global frame, panel, label
    frame = wx.Frame(None, size=(300,80), pos=(100,100))
    frame.Bind(wx.EVT_CLOSE, onClose)
        
    panel = wx.Panel(frame, size=(300,80), pos=(0,0))
    panel.Bind(wx.EVT_KEY_DOWN, onKey)
    panel.Bind(wx.EVT_LEFT_DOWN, onClick)
    panel.Bind(wx.EVT_LEFT_UP, onClick)
    panel.Bind(wx.EVT_RIGHT_DOWN, onClick)
    panel.Bind(wx.EVT_RIGHT_UP, onClick)
    panel.Bind(wx.EVT_LEFT_DCLICK, onDoubleClick)
    panel.Bind(wx.EVT_RIGHT_DCLICK, onDoubleClick)
    panel.Bind(wx.EVT_MIDDLE_DCLICK, onDoubleClick)
    panel.Bind(wx.EVT_MOTION, onMove)
    panel.Bind(wx.EVT_MOUSEWHEEL, onWheel)

    label = wx.StaticText(panel, wx.ID_ANY, "Press SPACE to capture mouse")
    label2 = wx.StaticText(panel, wx.ID_ANY, "TRIPLE RIGHT CLICK also releases mouse!!!")
    
    s = wx.BoxSizer(wx.VERTICAL)
    s.Add(label, 0, wx.ALIGN_CENTER | wx.ALL, border = 15)
    s.Add(label2, 0, wx.ALIGN_CENTER | wx.ALL, border = 5)

    panel.SetSizer(s)
    panel.Fit()
    panel.SetFocus()

    if autoCapture:
        t = Timer(2.0, doCapture)
        t.start()
              
    frame.Show()
    app.MainLoop()

    
if len(sys.argv) < 2:
    print "USAGE: python localPointer.py SAGE_IP [-a]  ... (-a automatically captures the pointer after 2 seconds)"
    sys.exit(0)
elif len(sys.argv) > 2:
    autoCapture = True

print "Connecting to: ", sys.argv[1]
main(ManagerConnection(sys.argv[1], 20005))
