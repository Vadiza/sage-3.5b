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


import wx, os, os.path, socket, math
from globals import *
import wx.lib.foldpanelbar as fpb
import preferences as prefs


############################################################################
#
#  CLASS: MyImage
#  
#  DESCRIPTION: This is just a data storage class for decorative bitmaps used
#               in the ui. You need to pass it a bitmap and optionally position,
#               transparency and new size. If the new size is passed in, the
#               bitmap will be automatically resized to that size.
#
#  DATE:        March, 2005
#
############################################################################

class MyImage(wx.Object):

    def __init__(self, bitmap, x=0, y=0, transparent=False, resizeWidth=0, resizeHeight=0):
        self.image = wx.Image(ConvertPath(str(bitmap)))
        self.x = x
        self.y = y
        self.transparent = transparent
        if resizeHeight != 0:  # you can resize the image during creation
            self.Rescale(resizeWidth, resizeHeight)
        else:
            self.bitmap = wx.BitmapFromImage(self.image)#.ConvertToBitmap()

    def GetBitmap(self):
        return self.bitmap

    def GetWidth(self):
        return self.bitmap.GetWidth()

    def GetHeight(self):
        return self.bitmap.GetHeight()

    def SetX(self, x):
        self.x = x

    def SetY(self, y):
        self.y = y

    def GetY(self):
        return self.y

    def GetX(self):
        return self.x

    def IsTransparent(self):
        return self.transparent

    def Rescale(self, newWidth, newHeight):
        if newWidth < 1:
            newWidth = 1
        if newHeight < 1:
            newHeight = 1
        self.bitmap = wx.BitmapFromImage(self.image.Rescale( round(newWidth), round(newHeight) ))#.ConvertToBitmap()
        return self.bitmap


    def Recalculate(self, newWidth, newHeight, x=-1, y=-1):
        # set the position if it was passed in
        if x != -1:
            self.x = x
        if y != -1:
            self.y = y
            
        return self.Rescale(newWidth, newHeight)


    def Hide(self):
        if not self.IsTransparent():
            self.transparent = True

    def Show(self):
        if self.IsTransparent():
            self.transparent = False


############################################################################
#
#  CLASS: MyBitmapButton
#  
#  DESCRIPTION: A template class for a bitmap button. It handles the images,
#               resizes them and changes them based on the state of the button.
#               It also handles basic painting (just draws the button on the
#               screen). All bitmap buttons should extend this class and
#               override some methods for specific behavior. In most cases
#               OnLeftUp() will have to be overwritten to define what happens
#               on a mouse click. In your overwride a method make sure you call
#               the same named method of this class since that will visually
#               update the button.
#
#  DATE:        March, 2005
#
############################################################################

class MyBitmapButton(wx.Window):

    def __init__(self, parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap=None, over_bitmap=None, tTip="", id=-1 ):        
        # call the superclass' constructor
        wx.Window.__init__(self, parent, id, (x,y), (resizeWidth, resizeHeight), wx.NO_BORDER | wx.FULL_REPAINT_ON_RESIZE)
        self.parent = parent

        self.tTip = tTip
        
        if self.tTip != "":
            self.toolTip = wx.ToolTip( self.tTip )
            self.toolTip.SetDelay( 100 )
            self.SetToolTip( self.toolTip )

        self.resizeWidth = resizeWidth
        self.resizeHeight = resizeHeight

        # get the images and rescale them
        if down_bitmap == None:
            down_bitmap = up_bitmap
        self.up_image = wx.Image(ConvertPath( str(up_bitmap) ))
        self.down_image = wx.Image(ConvertPath( str(down_bitmap) ))
        self.up_bitmap = self.RescaleBitmap(self.up_image, resizeWidth, resizeHeight)
        self.down_bitmap = self.RescaleBitmap(self.down_image, resizeWidth, resizeHeight)
        if over_bitmap:
            self.over_image = wx.Image(ConvertPath( str(over_bitmap) ))
            self.over_bitmap = self.RescaleBitmap(self.over_image, resizeWidth, resizeHeight)
        self.currentImage = self.up_image
        self.currentBitmap = self.up_bitmap

        # register events and the event handlers
        self.Bind( wx.EVT_LEFT_UP, self.OnLeftUp )
        self.Bind( wx.EVT_LEFT_DOWN, self.OnLeftDown )
        self.Bind( wx.EVT_RIGHT_UP, self.OnRightUp )
        self.Bind( wx.EVT_RIGHT_DOWN, self.OnRightDown )
        self.Bind( wx.EVT_ENTER_WINDOW, self.OnMouseEnter )
        self.Bind( wx.EVT_LEAVE_WINDOW, self.OnMouseLeave )
        self.Bind( wx.EVT_PAINT, self.OnPaint )
        self.Bind( wx.EVT_ERASE_BACKGROUND, self.Chillout)


    def Chillout(self, event):
        pass   # this intercepts the ERASE_BACKGROUND event and therefore minimizes the flicker


    def Rescale(self, newWidth, newHeight):
        self.up_bitmap = self.RescaleBitmap(self.up_image, newWidth, newHeight)
        self.down_bitmap = self.RescaleBitmap(self.down_image, newWidth, newHeight)
        self.currentBitmap = self.RescaleBitmap(self.currentImage, newWidth, newHeight)
        if hasattr(self, "over_bitmap"):
            self.over_bitmap = self.RescaleBitmap(self.over_image, newWidth, newHeight)
        self.SetSize( (newWidth, newHeight) )
            
    def RescaleBitmap(self, image, newWidth, newHeight):
        if newWidth < 1:
            newWidth = 1
        if newHeight < 1:
            newHeight = 1
        bitmap = wx.BitmapFromImage(image.Rescale( round(newWidth), round(newHeight) )) #.ConvertToBitmap()
        return bitmap


    def SetUpBitmap(self, newBitmap):
        self.up_image = wx.Image(ConvertPath( str(newBitmap) ))
        self.up_bitmap = self.RescaleBitmap(self.up_image, self.resizeWidth, self.resizeHeight)

    def SetDownBitmap(self, newBitmap):
        self.down_image = wx.Image(ConvertPath( str(newBitmap) ))
        self.down_bitmap = self.RescaleBitmap(self.down_image, self.resizeWidth, self.resizeHeight)

    def SetOverBitmap(self, newBitmap):
        self.over_image = wx.Image(ConvertPath( str(newBitmap) ))
        self.over_bitmap = self.RescaleBitmap(self.over_image, self.resizeWidth, self.resizeHeight)

        
    #  event handlers
    def OnPaint(self, event=None):
        if not event == None and event.GetEventType() == wx.EVT_PAINT:  
            dc = wx.PaintDC(self)
        else:
            #if "__WXMAC__" in wx.PlatformInfo:
            #    self.Refresh()
            #    return
            dc = wx.ClientDC(self)
        dc.DrawBitmap( self.currentBitmap, 0, 0, True )

    def OnMouseEnter(self, event):        
        if hasattr(self, "over_bitmap"):
            self.currentBitmap = self.over_bitmap
            self.currentImage = self.over_image
            self.OnPaint() 
        
    def OnMouseLeave(self, event):
        self.currentBitmap = self.up_bitmap
        self.currentImage = self.up_image
        self.OnPaint()
    
    def OnLeftUp(self, event):
        self.currentBitmap = self.up_bitmap
        self.currentImage = self.up_image
        self.OnPaint()

    def OnLeftDown(self, event):
        self.currentBitmap = self.down_bitmap
        self.currentImage = self.down_image
        self.OnPaint()

    def OnRightUp(self, event):
        self.currentBitmap = self.up_bitmap
        self.currentImage = self.up_image
        self.OnPaint()

    def OnRightDown(self, event):
        self.currentBitmap = self.down_bitmap
        self.currentImage = self.down_image
        self.OnPaint()
        
        
            


############################################################################
#
#  CLASS: AppButton
#  
#  DESCRIPTION: This class describes the buttons with which you start apps.
#               It extends MyBitmapButton and adds a few methods for specific
#               stuff. Also, all the app buttons are anchors for all the
#               instances of that particular application. This class creates,
#               positions and draws all of the InstanceButtons.
#
#  DATE:        March, 2005
#
############################################################################

class AppButton(MyBitmapButton):    

    def __init__(self, parent, up_bitmap, down_bitmap, over_bitmap, (x,y), (resizeWidth, resizeHeight), app):
        tTip = "-- "+app.GetName().upper()+" --\nLEFT click to start application normally.\nRIGHT click to start the application through sageBridge for sharing."
        if not os.path.isfile(over_bitmap):
            over_bitmap = up_bitmap
        MyBitmapButton.__init__(self, parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap, over_bitmap, tTip)#app.GetName() )
        self.SetName( app.GetName() )
        self.app = app
        self.leftClick = True   #a hack to detect which mouse button was clicked (used in config menu popup)
        self.instanceButtons = {}  #keyed by windowId

    
    def AddConfigMenu(self, app):
        # make the config popup menu (ids of the menuItems are the config numbers)
        self.configMenu = wx.Menu()
        num = 1
        configs = self.app.GetConfigNames()
        configs.sort(lambda x, y: cmp(x.lower(), y.lower()))
        for configName in configs:
            menuItem = self.configMenu.Append( num, configName )
            self.configMenu.Bind(wx.EVT_MENU, self.OnConfigMenuItem, menuItem)
            num+=1
            #self.configMenu.Bind(wx.EVT_MENU_HIGHLIGHT_ALL, self.OnMenuItemHighlight, menuItem)
        #self.configMenu.Bind(wx.EVT_MENU_CLOSE, self.OnCloseMenu)
            

    def AddInstanceButton(self, sageApp):
        size = (16, 30)
        x = self.GetPosition().x + (self.GetSize().GetWidth() - size[0]) / 2
        y = self.GetPosition().y + self.GetSize().GetHeight() + len(self.instanceButtons)*size[1]
        self.instanceButtons[ sageApp.getId() ] = InstanceButton(self, (x,y), size, "images/inst_up.jpg", "images/inst_up.jpg", "images/inst_down.jpg", sageApp)
        self.OnPaint()

    def RemoveInstanceButton(self, windowId):
        self.instanceButtons[ windowId ].Destroy()  #remove the InstanceButton
        del self.instanceButtons[ windowId ]  #remove it from the hash
        self.OnPaint()


    #  EVENT HANDLERS
    def OnPaint(self, event=None):
        if not event == None and event.GetEventType() == wx.EVT_PAINT:    
            dc = wx.PaintDC(self.parent)
            MyBitmapButton.OnPaint(self, event)
        else:
            #if "__WXMAC__" in wx.PlatformInfo:
            #    self.Refresh()
            #    return
            dc = wx.ClientDC(self.parent)
            MyBitmapButton.OnPaint(self)
        
        # then draw the instance buttons (update their position first)
        yOffset = 0
        for id, instance in self.instanceButtons.iteritems():
            instance.MoveXY( self.GetPosition().x + (self.GetSize().GetWidth() - instance.GetSize().GetWidth()) / 2,
                             self.GetPosition().y + self.GetSize().GetHeight() + yOffset*instance.GetSize().GetHeight())
            yOffset = yOffset + 1
            instance.Refresh()#OnPaint(event)

        
    def OnConfigMenuItem(self, event):
        self.currentBitmap = self.up_bitmap
        configName = self.configMenu.GetLabel(event.GetId())

	# check if the user pressed on a vnc button... if so, add some vnc specific params
	if self.GetName().lower().find("vnc") == -1:
            if self.leftClick:
                res = self.parent.parent.sageGate.executeApp(self.GetName(), configName)
            else:
                res = self.parent.parent.sageGate.executeApp(self.GetName(), configName, useBridge=True)
	else:
	    myIP = socket.gethostbyname(socket.gethostname())
	    displayNum = "0"
	    displaySize = wx.DisplaySize()
            res = self.__ShowSimpleVNCDialog(myIP, displayNum, displaySize)
            if res: params=res[1]; displaySize=res[0]
            else: return

            if self.leftClick:
                res = self.parent.parent.sageGate.executeApp(self.GetName(), configName, size=displaySize, optionalArgs=params)
            else:
                res = self.parent.parent.sageGate.executeApp(self.GetName(), configName, size=displaySize, optionalArgs=params, useBridge=True)

        if res == -1:
            Message("Application not started. Either application failed or the application launcher is not running", "Application Launch Failed")


##     def __EncryptPassword(self, passwd):
##         key = chr(23)+chr(82)+chr(107)+chr(6)+chr(35)+chr(78)+chr(88)+chr(7)
##         obj = pyDes.des(key, pyDes.CBC, "\0\0\0\0\0\0\0\0")
##         string = obj.encrypt(passwd, "\0")
##         #print "string = ",string.decode("ascii")#, 'replace')
##         return string.decode("utf-16")#, 'replace')


    def __ShowSimpleVNCDialog(self, myIP, displayNum, displaySize):#, password):
        def __CaptureReturn(event):
            if event.GetKeyCode() == wx.WXK_RETURN or event.GetKeyCode() == wx.WXK_NUMPAD_ENTER:
                dialog.EndModal(wx.ID_OK)
            else:
                event.Skip()
                
        # make the dialog
        if "__WXMSW__" in wx.PlatformInfo:
            dialog = wx.Dialog(self, wx.ID_ANY, "Desktop Sharing")
        else:
            dialog = wx.Dialog(self, wx.ID_ANY, "Desktop Sharing", style=wx.RESIZE_BORDER | wx.CLOSE_BOX)
        dialog.SetBackgroundColour(dialogColor)
        dialog.SetForegroundColour(wx.WHITE)

        # make the control
        ipText = wx.StaticText(dialog, wx.ID_ANY, "IP Address:")
        ipField = wx.TextCtrl(dialog, wx.ID_ANY, myIP)
        displayNumText = wx.StaticText(dialog, wx.ID_ANY, "Display Number:")
        displayNumField = wx.TextCtrl(dialog, wx.ID_ANY, displayNum)
        sizeText = wx.StaticText(dialog, wx.ID_ANY, "Screen Size:")
        widthText = wx.StaticText(dialog, wx.ID_ANY, "W")
        widthField = wx.TextCtrl(dialog, wx.ID_ANY,str(displaySize[0]))
        heightText = wx.StaticText(dialog, wx.ID_ANY, "H")
        heightField = wx.TextCtrl(dialog, wx.ID_ANY,str(displaySize[1]))
        passwordText = wx.StaticText(dialog, wx.ID_ANY, "Password:")
        passwordField = wx.TextCtrl(dialog, wx.ID_ANY, style=wx.TE_PASSWORD)
        passwordField.SetFocus()
        passwordField.Bind(wx.EVT_KEY_DOWN, __CaptureReturn)
        okBtn = wx.Button(dialog, wx.ID_OK, "Share Desktop")

        # layout (sizers)
        horSizer = wx.BoxSizer(wx.HORIZONTAL)
        horSizer.Add(widthText, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT | wx.TOP, border=3)
        horSizer.Add(widthField, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT, border=3)
        horSizer.Add(heightText, 0, wx.ALIGN_RIGHT | wx.LEFT | wx.RIGHT | wx.TOP, border=3)
        horSizer.Add(heightField, 0, wx.ALIGN_RIGHT | wx.LEFT | wx.RIGHT, border=3)
        
        gridSizer = wx.FlexGridSizer(4, 2, 2, 4)
        gridSizer.AddMany([ (ipText, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.TOP, 3), (ipField, 0, wx.EXPAND),
                     (displayNumText, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.TOP, 3), (displayNumField, 0, wx.EXPAND),
                     (sizeText, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.TOP, 3), (horSizer, 0),
                     (passwordText, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.TOP, 3), (passwordField, 0, wx.EXPAND)
                   ])
        gridSizer.AddGrowableCol(0)

        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(gridSizer, 1, wx.ALIGN_LEFT | wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(okBtn, 0, wx.ALIGN_CENTRE | wx.BOTTOM | wx.TOP, border=10)

        # do the size stuff with some corrections because of the FoldPanelBar complications
        dialog.SetSizer(mainSizer)
        dialog.Fit()

        # show it and get the result
        if dialog.ShowModal() == wx.ID_OK:
            ip = ipField.GetValue().strip()
            dispNum = displayNumField.GetValue().strip()
            w = widthField.GetValue().strip()
            h = heightField.GetValue().strip()
            passwd = passwordField.GetValue().strip()
            return ((int(w),int(h)), (ip+" "+dispNum+" "+w+" "+h+" "+ passwd))
        else:
            return False




    def __ShowVNCDialog(self, myIP, displayNum, displaySize, password):
        def __OnStateChange(evt):
            isExpanded = evt.GetFoldStatus()
            s = dialog.GetSize()
            if isExpanded:
                dialog.SetSize((s[0], s[1]-150))
            else:
                dialog.SetSize((s[0], s[1]+150))
            evt.Skip()

        def __GetControlValues():
            ip = ipField.GetValue()
            dispNum = displayNumField.GetValue()
            size = sizeField.GetValue().lower()
            if "x" in size:
                size = size.split("x")
            elif "*" in size:
                size = size.split("*")
            else:
                size = (1024, 768)
            passwd = passwordField.GetValue()
            profile = profileField.GetValue()
            return (ip, dispNum, (int(size[0]), int(size[1])), passwd, profile)

        def __FillControls(profile):#, ip, dispNum, size, passwd):
            (myIP, displayNum, displaySize, password) = prefs.vnc.GetProfile(profile)
            ipField.SetValue(myIP)
            displayNumField.SetValue(displayNum)
            sizeField.SetValue(str(displaySize[0])+" x "+str(displaySize[1]))
            passwordField.SetValue(password)
            profileField.SetValue(profile)
            
        def __OnComboBox(evt):
            __FillControls( profileCombo.GetStringSelection() )
            

        def __OnDelete(evt):
            sel = profileCombo.GetStringSelection()
            if sel == "My Desktop":
                return  # do not allow the deletion of the default profile
            prefs.vnc.DeleteProfile( sel )
            profileCombo.Delete( profileCombo.GetSelection() )
            profileCombo.SetSelection(0)
            __FillControls( profileCombo.GetStringSelection() )
                
        if "__WXMSW__" in wx.PlatformInfo:
            dialog = wx.Dialog(self, wx.ID_ANY, "VNC Setup")#, style=wx.CLOSE_BOX)
        else:
            dialog = wx.Dialog(self, wx.ID_ANY, "VNC Setup", style=wx.RESIZE_BORDER | wx.CLOSE_BOX)#, style=wx.CLOSE_BOX | wx.THICK_FRAME)


        foldBar = fpb.FoldPanelBar(dialog, wx.ID_ANY)#, wx.DefaultPosition, wx.DefaultSize, fpb.FPB_DEFAULT_STYLE | fpb.FPB_VERTICAL)
        foldBar.Bind(fpb.EVT_CAPTIONBAR, __OnStateChange)
        foldPanel = foldBar.AddFoldPanel("Advanced Options", collapsed=True)
        p = wx.Panel(foldPanel, wx.ID_ANY)

        # store the default vnc values... or if the default already exists, load it
        if prefs.vnc.ProfileExists("My Desktop"):
            # compare the IPs of the current machine and the stored profile
            # if they are the same, use those values, otherwise use the detected values
            defaultProfile = prefs.vnc.GetProfile("My Desktop")
            if defaultProfile[0] == myIP:
                (myIP, displayNum, displaySize, password) = defaultProfile
        prefs.vnc.AddProfile("My Desktop", myIP, displayNum, displaySize, password)
        
        # controls
        text = wx.StaticText(dialog, wx.ID_ANY, "VNC Profile:")
        profileCombo = wx.ComboBox(dialog, wx.ID_ANY, "My Desktop", choices=prefs.vnc.GetProfileList(), style=wx.CB_DROPDOWN|wx.CB_READONLY)
        profileCombo.SetMinSize((130, profileCombo.GetSize()[1]))
        dialog.Bind(wx.EVT_COMBOBOX, __OnComboBox)
        
        ipText = wx.StaticText(p, wx.ID_ANY, "The IP ADDRESS of the machine you are trying to show:")
        ipField = wx.TextCtrl(p, wx.ID_ANY)
        ipField.SetMinSize((100, ipField.GetSize()[1]))
        displayNumText = wx.StaticText(p, wx.ID_ANY, "The DISPLAY NUMBER where the VNC server is running (Linux or Mac):")
        displayNumField = wx.TextCtrl(p, wx.ID_ANY)
        sizeText = wx.StaticText(p, wx.ID_ANY, "The SIZE of the desktop you are trying to show (width x height):")
        sizeField = wx.TextCtrl(p, wx.ID_ANY)
        passwordText = wx.StaticText(p, wx.ID_ANY, "The PASSWORD for your vnc server:")
        passwordField = wx.TextCtrl(p, wx.ID_ANY, style=wx.TE_PASSWORD)
        profileText = wx.StaticText(p, wx.ID_ANY, "Save this profile as: ")
        profileField = wx.TextCtrl(p, wx.ID_ANY)
        profileField.SetMinSize((100, profileField.GetSize()[1]))
        deleteBtn = wx.Button(p, wx.ID_ANY, "Delete")
        p.Bind(wx.EVT_BUTTON, __OnDelete)
        okBtn = wx.Button(dialog, wx.ID_OK, "Show")
        __FillControls("My Desktop")

        # layout (sizers)
        horSizer2 = wx.BoxSizer(wx.HORIZONTAL)
        horSizer2.Add(text, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT, border=10)
        horSizer2.Add(profileCombo, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        
        gridSizer = wx.FlexGridSizer(4, 2, 2, 0)
        gridSizer.AddMany([ (ipText, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL), (ipField, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL),
                     (displayNumText, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL), (displayNumField, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL),
                     (sizeText, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL), (sizeField, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL),
                     (passwordText, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL), (passwordField, 0, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
                   ])
        gridSizer.AddGrowableCol(0)

        horSizer = wx.BoxSizer(wx.HORIZONTAL)
        horSizer.Add((0,0), 1)
        horSizer.Add(profileText, 0, wx.ALIGN_CENTER)
        horSizer.Add(profileField, 0, wx.ALIGN_CENTER)
        horSizer.Add((0,0), 1)
        horSizer.Add(deleteBtn, 0, wx.ALIGN_RIGHT)

        vertSizer = wx.BoxSizer(wx.VERTICAL)
        vertSizer.Add(gridSizer, 1, wx.EXPAND | wx.ALIGN_TOP)
        vertSizer.Add(horSizer, 0, wx.EXPAND | wx.ALIGN_CENTER | wx.ALIGN_TOP | wx.TOP, border=5)
        vertSizer.Add(wx.StaticLine(p), 0, wx.ALIGN_TOP | wx.EXPAND | wx.TOP, border=10)
        
        p.SetSizer(vertSizer)
        p.Fit()
        foldBar.AddFoldPanelWindow(foldPanel, p, fpb.FPB_ALIGN_LEFT)
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(horSizer2, 0, wx.ALIGN_LEFT | wx.LEFT | wx.TOP | wx.BOTTOM, border=10)
        mainSizer.Add(foldBar, 1, wx.ALIGN_LEFT | wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(okBtn, 0, wx.ALIGN_CENTRE | wx.BOTTOM | wx.TOP, border=10)

        # do the size stuff with some corrections because of the FoldPanelBar complications
        dialog.SetSizer(mainSizer)
        dialog.Fit()
        minS = vertSizer.GetMinSize()
        s=dialog.GetSize()
        if s[0] < minS: dialog.SetSize((minS[0]+20, s[1]+10))
        
        
        # show it and get the result
        if dialog.ShowModal() == wx.ID_OK:
            (ip, dispNum, size, passwd, profile) = __GetControlValues()
            (w, h) = size
            prefs.vnc.AddProfile(profile, ip, dispNum, size, passwd)
            return (size, (ip+" "+str(dispNum)+" "+str(w)+" "+str(h)+" "+ str(passwd))) #self.__EncryptPassword(str(passwd))))
        else:
            return False

        
    def OnLeftDown(self, event):
        MyBitmapButton.OnLeftDown(self, event)
        if "imageviewer" in self.GetName() or "mplayer" in self.GetName():
            Message("You cannot start these directly. Please use the file library to show multimedia files (F2)", "")
        else:
            self.leftClick = True #a hack to detect which mouse button was clicked
            self.PopupMenu(self.configMenu)#, event.GetPosition())
            self.OnPaint()
        

    def OnRightDown(self, event):
        MyBitmapButton.OnRightDown(self, event)
        if "imageviewer" in self.GetName() or "mplayer" in self.GetName():
            Message("You cannot start these directly. Please use the file library to show multimedia files (F2)", "")
        else:
            self.leftClick = False  #a hack to detect which mouse button was clicked
            self.PopupMenu(self.configMenu)#, event.GetPosition())
            self.OnPaint()




class RemoteAppButton(MyBitmapButton):
    def __init__(self, parent, up_bitmap, down_bitmap, over_bitmap, (x,y), (resizeWidth, resizeHeight), sageGate):
        tTip = "-- REMOTE APPLICATIONS --\nLEFT click to start application normally.\nRIGHT click to start the application through sageBridge for sharing."
        if not os.path.isfile(over_bitmap):
            over_bitmap = up_bitmap
        MyBitmapButton.__init__(self, parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap, over_bitmap, tTip)
        self.SetName("REMOTE APPLICATIONS")
        self.sageGate = sageGate
        self.leftClick = True   #a hack to detect which mouse button was clicked (used in config menu popup)
        #self.AddLauncherMenu()

        
    def AddLauncherMenu(self):
        self.launcherMenu = wx.Menu()
        launcherHash = self.sageGate.getLaunchers()

        # appLaunchers
        k=1
        
        # sort the appLaunchers by name
        launchers = launcherHash.values()
        launchers.sort(lambda x, y: cmp(x.getName().lower(), y.getName().lower()))
        
        for l in launchers:   

            # applications
            i = k*100
            appMenu = wx.Menu()
            appHash = l.getAppList()
            appList = appHash.keys()
            appList.sort(lambda x, y: cmp(x.lower(), y.lower()))
            for appName in appList:   
                configList = appHash[appName]
                configList.sort(lambda x, y: cmp(x.lower(), y.lower()))
                
                # app configs
                j=i*100
                configMenu = wx.Menu()
                for appConfig in configList:
                    item = configMenu.Append(j, appConfig, help=l.getId())
                    j+=1
                    self.Bind(wx.EVT_MENU, self.OnConfigMenu, item)
                    
                mi = appMenu.AppendMenu(i, appName, configMenu, l.getId())
                i+=1
                
            menuItem = self.launcherMenu.AppendMenu(k, l.getName(), appMenu, l.getId())
            k+=1
            
    

    #  EVENT HANDLERS
    def OnPaint(self, event=None):
        if not event == None and event.GetEventType() == wx.EVT_PAINT:    
            dc = wx.PaintDC(self.parent)
            MyBitmapButton.OnPaint(self, event)
        else:
            #if "__WXMAC__" in wx.PlatformInfo:
            #    self.Refresh()
            #    return
            dc = wx.ClientDC(self.parent)
            MyBitmapButton.OnPaint(self)


    def OnConfigMenu(self, event):
        configId = event.GetId()
        windowId =  int(math.floor(configId/100))
        lId = int(math.floor(windowId/100))

        launcherMenu = self.launcherMenu.FindItemById(lId).GetSubMenu()
        appMenuItem = launcherMenu.FindItemById(windowId)
        appMenu = appMenuItem.GetSubMenu()
        configMenuItem = appMenu.FindItemById(configId)
        configMenu = configMenuItem.GetSubMenu()
        
        appName = appMenuItem.GetLabel() 
        configName = configMenuItem.GetLabel()
        appLauncherId = appMenuItem.GetHelp()
        
        if self.leftClick:
            res = self.sageGate.executeRemoteApp(appLauncherId, appName, configName)
        else:
            res = self.sageGate.executeRemoteApp(appLauncherId, appName, configName, useBridge=True)

        if res == -1:
            Message("Application not started. Either application failed or the application launcher "+str(appLauncherId)+" is not running", "Application Launch Failed")
        

    def OnLeftDown(self, event):
        MyBitmapButton.OnLeftDown(self, event)
        self.leftClick = True #a hack to detect which mouse button was clicked

        # get a new list from the sage server and make the menu
        self.AddLauncherMenu()

        self.PopupMenu(self.launcherMenu)#, event.GetPosition())
        self.OnPaint()
                    

    def OnRightDown(self, event):
        MyBitmapButton.OnRightDown(self, event)
        self.leftClick = False  #a hack to detect which mouse button was clicked

        # get a new list from the sage server and make the menu
        self.AddLauncherMenu()
        
        self.PopupMenu(self.launcherMenu)#, event.GetPosition())
        self.OnPaint()


        



############################################################################
#
#  CLASS: InstanceButton
#  
#  DESCRIPTION: This class describes the buttons that represents instances of
#               one application. It extends MyBitmapButton and adds a few
#               methods for specific stuff. These buttons are created when a
#               40001 message comes back and the app doesn't exists yet. They
#               are anchored at the appButton for that application so that
#               they are drawn underneith it. Also they are all positioned
#               relative to one another so that when one instance is closed,
#               the button for it goes away and all the other ones are
#               repositioned.
#
#  DATE:        March, 2005
#
############################################################################

class InstanceButton(MyBitmapButton):
    
    def __init__(self, parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap, over_bitmap, app):        
        MyBitmapButton.__init__(self, parent.parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap, over_bitmap, app.getName()+" "+str(app.getId()), app.getId() )
        self.SetName( self.GetToolTip().GetTip() )
        self.app = app  #SAGEApp
        self.anchor = parent  # parent is the AppButton that this instance corresponds to
        self.displayCanvas = self.anchor.parent.parent.GetDisplayCanvas()
        self.drawX = x
        self.drawY = y
        self.infoPanel = self.anchor.parent.parent.GetInfoPanel()
        self.Bind(wx.EVT_RIGHT_UP, self.OnRightUp)
        

    def SetDrawX(self, newX):
        self.drawX = newX

    def SetDrawY(self, newY):
        self.drawY = newY

    
    #  EVENT HANDLERS
    def OnRightUp(self, event):
        menu = wx.Menu()
        closeMenuItem = menu.Append( self.GetId()+1, "Close "+self.GetName() )  # +1 because Mac doesn't like 0 menuItem id
        menu.Bind(wx.EVT_MENU, self.OnCloseMenuItem, closeMenuItem)
        self.PopupMenu(menu, event.GetPosition())
        menu.Destroy()

    def OnCloseMenuItem(self, event):
        # (AKS 2005-04-05) Stop performance monitoring that began "by default"
        # when a new application had started.
        self.displayCanvas.sageGate.shutdownApp(self.GetId())  
        
    def OnPaint(self, event=None):
        MyBitmapButton.OnPaint(self, event)
           
    def OnMouseEnter(self, event):
        shape = self.displayCanvas.GetShape(self.GetId())
        
        # update the tooltip first because the name might have changed
        if self.tTip != "":
            self.tTip = shape.GetName()+" "+str(shape.GetId())+" - "+shape.GetTitle()
            self.toolTip = wx.ToolTip( self.tTip )
            self.toolTip.SetDelay( 200 )
            self.SetToolTip( self.toolTip )
            
        shape.Highlight(True)
        MyBitmapButton.OnMouseEnter(self, event)
        
    def OnMouseLeave(self, event):
        self.displayCanvas.GetShape(self.GetId()).Highlight(False)
        MyBitmapButton.OnMouseLeave(self, event)        
        
    def OnLeftUp(self, event):
        MyBitmapButton.OnLeftUp(self, event)
        self.displayCanvas.GetShape(self.GetId()).BringToFront()
        self.infoPanel.ShowData(self.app.getName(), self.app.getId())
        #self.infoPanel.ShowPerformanceData(self.app.getName(), self.app.getId())
        #self.PopupMenu(self.configMenu, event.GetPosition())
    


class PerfToggleButton(MyBitmapButton):    

    def __init__(self, parent, btnId, onClick, onDeClick, up_bitmap, down_bitmap, over_bitmap, (x,y), (resizeWidth, resizeHeight)):
        if not os.path.isfile(over_bitmap):
            over_bitmap = up_bitmap
        MyBitmapButton.__init__(self, parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap, over_bitmap )
        self.__pressed = False  #the current state of the button
        self.__id = btnId
        self.__clickAction = onClick
        self.__deClickAction = onDeClick

    def OnLeftUp(self, event):
        if self.__pressed:  #the button was pressed
            self.__pressed = False
            self.currentBitmap = self.up_bitmap
	    self.currentImage = self.up_image
            self.__deClickAction( self.__id )
        else:
            self.__pressed = True
            self.currentBitmap = self.down_bitmap
	    self.currentImage = self.down_image
            self.__clickAction( self.__id )
        self.OnPaint()


    def OnLeftDown(self, event):
        pass  # override the superclass handler

    def OnMouseLeave(self, event):
        if self.__pressed:
            self.currentBitmap = self.down_bitmap
	    self.currentImage = self.down_image
        else:
            self.currentBitmap = self.up_bitmap
	    self.currentImage = self.up_image
        self.OnPaint()

    def GetValue(self):
        return self.__pressed

    def SetValue(self, press):
        if self.__pressed == press:  #dont do anything if the button is already at that state
            return
        self.__pressed = press
        if self.__pressed:
            self.currentBitmap = self.down_bitmap
	    self.currentImage = self.down_image
        else:
            self.currentBitmap = self.up_bitmap
	    self.currentImage = self.up_image
        self.OnPaint()
        


class MyButton(MyBitmapButton):
    
    def __init__(self, parent, (x,y), (resizeWidth, resizeHeight), function, up_bitmap, down_bitmap=None, over_bitmap=None, tTip="", id=-1 ):        
        # call the superclass' constructor
        MyBitmapButton.__init__(self, parent, (x,y), (resizeWidth, resizeHeight), up_bitmap, down_bitmap=None, over_bitmap=None, tTip="", id=-1 )        
        self.function = function  # this is the function to be called when user presses the button
                
##     def OnPaint(self, event=None):
##         if not event == None:    
##             dc = wx.PaintDC(self)
##         else:
##             dc = wx.ClientDC(self)
        
##         #mask = wx.Mask(self.currentBitmap, wx.Colour(0,0,0))
## 	#self.currentBitmap.SetMask(mask)
##         print "painting the help button"
##         dc.DrawBitmap( self.currentBitmap, 0, 0, True )

    def OnLeftDown(self, event):
        MyBitmapButton.OnLeftDown(self, event)
        self.function()
