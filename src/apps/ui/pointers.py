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



import wx, time
from SAGEDrawObject import PointerObject


class DeviceManager:
    """ An object that brings all the devices under one roof and manages them """
    def __init__(self, sageData, sageGate):
        self.sageData = sageData
        self.sageGate = sageGate
        self.minAppSize = 250
        self.pointers = {}  # key=pointerId, value=PointerObject


    def __addPointerObject(self, pointerId):
        if pointerId not in self.pointers:
            self.pointers[pointerId] = PointerObject(pointerId, self.sageGate)
        return self.pointers[pointerId]


    def processMouseMove(self, pointerId, x, y):
        ptr = self.__addPointerObject(pointerId)
        (app, regionId) = self.sageData.checkHits(x,y)
        if ptr.leftDown:
            # release the mouse button after 15 secs
            if time.time() - ptr.leftDownTime > 15:
                self.processMouseUp(pointerId, x, y)
            return
        
        ptr.updateRegion(regionId, app)
        
        if regionId > 0:
            if ptr.lastRegion != ptr.region:  # corner
                ptr.showResizePointer()
        elif ptr.lastRegion[0] > 0:  # it was on a corner previously and now it's not
            ptr.reset()
            ptr.lastRegion = [regionId, app]
            
        #self.sageGate.movePointer(pointerId, dx, dy)

        
    def processMouseDown(self, pointerId, x, y):
        ptr = self.__addPointerObject(pointerId)
        (app, regionId) = self.sageData.checkHits(x,y)
        if regionId < 0 or app.hasCapture():
            return
        
        # record the info about this click
        ptr.onMouseDown(app, regionId, x, y)

        # process the click
        if regionId == 0:
            app.captureMouse(pointerId)
            self.__handleAppWindowHit(pointerId, app.getId())
        else:
            app.captureMouse(pointerId)
            self.__handleAppCornerHit(pointerId, app.getId(), regionId)


        
    def processMouseUp(self, pointerId, x, y):
        ptr = self.__addPointerObject(pointerId)
        if not ptr.leftDown: return
        
        ptr.onMouseUp()

        if ptr.lastRegion[0] == 0:    # move
            ptr.lastRegion[1].releaseMouse()
            # bring the app to front if necessary and then move it
            if self.sageData.getTopApp()[0] != ptr.lastRegion[1].getId():
                self.sageGate.bringToFront(ptr.lastRegion[1].getId())
            self.sageGate.moveWindow(ptr.lastRegion[1].getId(), x-ptr.clickPos[0], y-ptr.clickPos[1])
            
        elif ptr.lastRegion[0] > 0:   # resize
            (l,r,t,b) = ptr.lastRegion[1].getBounds()
            w = r-l
            h = t-b
            aspectRatio = w/float(h)
            ptr.lastRegion[1].releaseMouse()
            
            # don't allow resizing below minAppSize
            dx = x-ptr.clickPos[0]
            if ptr.lastRegion[0] == 1:
                l+=dx; b=b+dx/aspectRatio
                if r-l < self.minAppSize: l = r - self.minAppSize
                if t-b < self.minAppSize: b = t - self.minAppSize
            elif ptr.lastRegion[0] == 2:
                l+=dx; t=t-dx/aspectRatio 
                if r-l < self.minAppSize: l = r - self.minAppSize
                if t-b < self.minAppSize: t = b + self.minAppSize
            elif ptr.lastRegion[0] == 3:
                r+=dx; t=t+dx/aspectRatio
                if r-l < self.minAppSize: r = l + self.minAppSize
                if t-b < self.minAppSize: t = b + self.minAppSize
            elif ptr.lastRegion[0] == 4:
                r+=dx; b=b-dx/aspectRatio
                if r-l < self.minAppSize: r = l + self.minAppSize
                if t-b < self.minAppSize: b = t - self.minAppSize      
           
            self.sageGate.resizeWindow(ptr.lastRegion[1].getId(), l, r, b, t)
        
        

    def __handleAppCornerHit(self, pointerId, windowId, cornerId):
        """ a corner of a window was hit """
        (l,r,t,b) = self.sageData.getApp(windowId).getBounds()
        self.sageGate.setMouseResizeState(pointerId, cornerId, l, r, t, b)
        

    def __handleAppWindowHit(self, pointerId, windowId):
        """ an app window was hit but not the corner """
        (l,r,t,b) = self.sageData.getApp(windowId).getBounds()
        self.sageGate.setMouseMoveState(pointerId, l, r, t, b)
        





class PollTimer(wx.Timer):
    def SetCallback(self, cb):
        self.callback = cb
    def Notify(self):
        self.callback()




class Pointers:
    """ main class that keeps track of and manages all the devices on this machine """
    def __init__(self, sageGate, sageData):
        self.sageGate = sageGate
        self.sageData = sageData
        self.dispSize = sageData.getSageDisplayInformation()[3:5]
        self.devManager = DeviceManager(sageData, sageGate)

        # out devices
        self.numJoy = wx.Joystick().GetNumberJoysticks()
        self.joysticks = {}
        self.__addJoystick()
##         self.mouse = None  # it will be MousePointer

        # start the timer that will periodically poll the devices
        self.t = PollTimer()
        period = 50  # poll X times/sec (roughly)
        self.t.SetCallback(self.poll)
        self.t.Start(1000/period)


    def __addJoystick(self):
        self.sageGate.registerCallbackFunction( 40018, self.onNewJoystickDevice )
        if len(self.joysticks) < self.numJoy:
            self.sageGate.addPointer()
##         else:
##             print "adding a mouse!"
##             self.sageGate.registerCallbackFunction( 40018, self.onNewMouseDevice )
##             self.sageGate.addPointer()


    def onNewJoystickDevice(self, data):
        pointerId = int(data.split()[0])
        self.joysticks[pointerId] = JoyPointer( self.dispSize, pointerId, \
                                                len(self.joysticks), \
                                                self.onButtonDown, self.onButtonUp )
        self.sageGate.showObject(pointerId)
        if len(self.joysticks) < self.numJoy:  #add another one
            self.sageGate.addPointer()
##         else:   # finally add a mouse
##             self.sageGate.registerCallbackFunction( 40018, self.onNewMouseDevice )
##             self.sageGate.addPointer()
            

##     def onNewMouseDevice(self, data):
##         pointerId = int(data.split()[0])
##         self.sageGate.showObject(pointerId)
##         self.mouse = MousePointer(self.dispSize, pointerId, self.onButtonDown, self.onButtonUp)
        

##     def shareMouse(self):
##         if self.mouse:
##             self.mouse.start()
        

    def stop(self):
        if self.t.IsRunning():
            self.t.Stop()

        for pointerId in self.joysticks.iterkeys():
            self.sageGate.removePointer(pointerId)
        ## if self.mouse:
##             self.sageGate.removePointer(self.mouse.getId())


    def onButtonDown(self, pointerId, btn, x, y):
        self.devManager.processMouseDown(pointerId, x, y)


    def onButtonUp(self, pointerId, btn, x, y):
        self.devManager.processMouseUp(pointerId, x, y)

        
    def poll(self):
        """ here we poll the device just for their position
            the button clicks are dealt with through event handling
        """
        for id, joy in self.joysticks.iteritems():
            dX, dY = joy.getPos()
            if dX != 0 or dY != 0: 
                self.devManager.processMouseMove(id, joy.getX(), joy.getY())
                self.sageGate.movePointer(id, dX, dY)

##         if self.mouse and self.mouse.isShared():
##             dX, dY = self.mouse.getPos()
##             if dX != 0 or dY != 0: 
##                 self.devManager.processMouseMove(self.mouse.getId(), self.mouse.getX(), self.mouse.getY())
##                 print "moving pointer to", dX, dY
##                 self.sageGate.movePointer(self.mouse.getId(), dX, dY)



class JoyPointer:
    def __init__(self, dispSize, pointerId, deviceId, onDown, onUp):
        self.dispSize = dispSize
        self.pointerId = pointerId
        self.onDown = onDown
        self.onUp = onUp
        self.x = self.y = 100

        # the top frame will capture all our joystick events
        self.win = wx.GetApp().GetTopWindow()
        self.joy = wx.Joystick(deviceId)
        self.joy.SetCapture(self.win)
        self.win.Bind(wx.EVT_JOY_BUTTON_DOWN, self.onButton)
        self.win.Bind(wx.EVT_JOY_BUTTON_UP, self.onButton)
        
        # analog axis parameters and calibration values
        # we'll make an assumption that both axis have the
        # same range of values
        xmin = self.joy.GetXMin()
        xmax = self.joy.GetXMax()
        rng = abs(xmax - xmin)
        self.centerX = abs( int((xmin + xmax)/2)) #so that 0 <-> max and -max/2 <-> max/2 both work
        self.drift = int(0.08*rng)
        self.usefulRange = abs( int( rng/2 - self.drift) )

        # for converting the movement from joystick axis position
        # to pixel distance on the SAGE display
        # it depends on the joystick position AND on the display size
        joyToMouseRatio = 70
        displayFactor = dispSize[0] / 10000.0 + 0.2
        self.moveRatio = joyToMouseRatio * displayFactor


    def getId(self):
        return self.pointerId
    

    def getX(self):
        return self.x


    def getY(self):
        return self.y


    def onButton(self, evt):
        """ left=1, right=2, middle=3 """
        btn = -1
        
        if evt.GetButtonChange() == 0:                 btn = 1
        elif evt.GetButtonChange() == wx.JOY_BUTTON1:  btn = 2       
        elif evt.GetButtonChange() == wx.JOY_BUTTON2:  btn = 3
        elif evt.GetButtonChange() == wx.JOY_BUTTON3:  btn = 4
        if btn == -1: return  #dont act on any other button
        
        if evt.ButtonUp():
            self.onUp(self.pointerId, btn, self.x, self.y)
        else:
            self.onDown(self.pointerId, btn, self.x, self.y)
        
        
    def getPos(self):
        """ returns (x, y) """

        # get the analog axis position and map it to some
        # pixel distance in sage coordinates
        (dx,dy) = self.joy.GetPosition()
        (dx,dy) = self.toSAGECoords(dx,dy)
        
        dx = int(round(dx))
        dy = int(round(dy))
        
        # keep the pointer inside the display
        if self.x + dx < 0:
            dx = -self.x
        elif self.x + dx > self.dispSize[0]:
            dx = self.dispSize[0] - self.x
        if self.y + dy < 0:
            dy = -self.y
        elif self.y + dy > self.dispSize[1]:
            dy = self.dispSize[1] - self.y

        # accumulate the position
        self.x += dx
        self.y += dy
        
        return dx, dy

            
    def toSAGECoords(self, x, y):
        # first map the values from 0-max to -max/2 to max/2 (if needed)
        x -= self.centerX
        y -= self.centerX

        # apply the movement threshold and get the normalized crds
        if abs(x) <= self.drift:
            x = 0
        else:
            if x > 0:  x = float(x-self.drift)/self.usefulRange
            else: x = float(x+self.drift)/self.usefulRange
            x *= self.moveRatio #50

        if abs(y) <= self.drift:
            y = 0
        else:
            if y > 0:  y = float(y-self.drift)/self.usefulRange
            else: y = float(y+self.drift)/self.usefulRange
            y *= -self.moveRatio #(-50)

        return (x,y)



    
## class MousePointer:
##     def __init__(self, dispSize, pointerId, onDown, onUp):
##         self.dispSize = dispSize
##         self.pointerId = pointerId
##         self.onDown = onDown
##         self.onUp = onUp
##         self.screenSize = wx.GetDisplaySize()
##         self.frame = wx.GetApp().GetTopWindow()

##         # figure out the mapping between the local screen and the display
##         i = self.dispSize.index( max(self.dispSize[0], self.dispSize[1]) )
##         self.moveRatio = self.dispSize[i] / float(self.screenSize[i])
##         self.x = self.y = 100
##         self.__shared = False

        
##     def getX(self):
##         return self.x


##     def getY(self):
##         return self.y


##     def getId(self):
##         return self.pointerId


##     def isShared(self):
##         return self.__shared
    

##     def makeControls(self):
##         winSize = (400, 200)
##         winPos = (self.screenSize[0]/2 - winSize[0]/2, self.screenSize[1]/2 - winSize[1]/2)
##         self.dlg = wx.Frame(self.frame, wx.ID_ANY, "", \
##                              winPos, winSize)
##         self.dlg.Bind(wx.EVT_CLOSE, self.onClose)
##         self.dlg.Bind(wx.EVT_KEY_DOWN, self.onKey)
##         self.dlg.Bind(wx.EVT_LEFT_DOWN, self.onButton)
##         self.dlg.Bind(wx.EVT_RIGHT_DOWN, self.onButton)
##         self.dlg.Bind(wx.EVT_MIDDLE_DOWN, self.onButton)
##         self.dlg.Bind(wx.EVT_LEFT_UP, self.onButton)
##         self.dlg.Bind(wx.EVT_RIGHT_UP, self.onButton)
##         self.dlg.Bind(wx.EVT_MIDDLE_UP, self.onButton)
        
##         sizer = wx.BoxSizer(wx.VERTICAL)
##         label = wx.StaticText(self.dlg, wx.ID_ANY, "Press ESC to stop sharing your mouse")
##         sizer.Add(label, 0, wx.ALIGN_CENTER | wx.BOTTOM | wx.LEFT | wx.RIGHT, border=5)
##         self.dlg.SetSizer(sizer)
##         self.dlg.CaptureMouse()
##         self.__shared = True
##         self.dlg.Show()


##     def onClose(self, evt):
##         self.stop()
        

##     def onKey(self, evt):
##         if evt.GetKeyCode() == wx.WXK_ESCAPE:
##             self.Close(True)
##         else:
##             evt.Skip()


##     def onButton(self, evt):
##         btn = -1
##         if evt.Button(wx.MOUSE_BTN_LEFT):     btn = 1
##         elif evt.Button(wx.MOUSE_BTN_RIGHT):  btn = 2
##         elif evt.Button(wx.MOUSE_BTN_MIDDLE): btn = 3

##         if btn == -1: return  #dont act on any other button
        
##         if evt.ButtonUp():
##             self.onUp(self.pointerId, btn, self.x, self.y)
##         else:
##             self.onDown(self.pointerId, btn, self.x, self.y)
            

##     def start(self):
##         self.makeControls()
        

##     def stop(self):
##         self.__shared = False
##         if self.dlg.HasCapture():
##             self.dlg.ReleaseMouse()
##         self.dlg.Destroy()


##     def getPos(self):
##         # get the new mouse position and figure out how
##         # much it moved in sage coordinates
##         p = wx.GetMousePosition().Get()
##         newPos = [ int(p[0]*self.moveRatio), int(p[1]*self.moveRatio) ]
##         dx = newPos[0] - self.x
##         dy = newPos[1] - self.y
        
##         # keep the pointer inside the display
##         if self.x + dx < 0:
##             dx = -self.x
##         elif self.x + dx > self.dispSize[0]:
##             dx = self.dispSize[0] - self.x
##         if self.y + dy < 0:
##             dy = -self.y
##         elif self.y + dy > self.dispSize[1]:
##             dy = self.dispSize[1] - self.y
            
##         # accumulate the position
##         self.x += dx
##         self.y += dy
        
##         return dx, dy
            
