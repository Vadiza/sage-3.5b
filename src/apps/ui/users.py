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


from threading import Thread, RLock
import time, socket, string, os.path, sys, wx

from globals import *
from Mywx import MyButton
from sageui import AboutDialog
import preferences as prefs

# some globals for this module
CHUNK_SIZE = 2048 #4096
SOCKET_TIMEOUT = 1
MSGLEN = CHUNK_SIZE
SEPARATOR = '\0'



#  MESSAGES:
#
#
#  Notes:
#  --------------------
#  All messages are sent in this format (as strings):
#  code
#  data
#
#  For example:
#  "2002"
#  "Ratko"
#
#
#  All machines are always keyed by machineId that the users should connect to to control SAGE
#
#
#  <<< UI  --->  SERVER >>>
#  CODE    FORMAT                 MESSAGE
#  ----------------------------------------------------------------
#  2000    username                register this user with the Users server
#          info
#          machine_id              the machines the user is connected to
#
#  2001    from={username}         send a chat message to one person or to all
#          to={"all" | id}         id = specific to users connected to a sepcific SAGE machine
#          message
#
#  2002    username                check username for duplicates
#
#  2003    username                unregister this username from the machine specified
#          machine_id
#
#          
#  <<< SERVER  --->  UI >>>
#  CODE    FORMAT                 MESSAGE
#  ----------------------------------------------------------------
# 
#  30000   machine_name            a status list of all the MACHINES registered with the server
#          ip
#          port
#          machineId
#          alive={"1" | "0"}       if the machine is alive, send 1, otherwise send 0
#          displayInfo             in this format: "Xtiles Ytiles tileWidth tileHeight"
#          "\x00"               < --- a break between different blocks of data
#          machine_name
#          ip
#          port
#          machineId
#          alive={"1" | "0"}       if the machine is alive, send 1, otherwise send 0
#          displayInfo             in this format: "Xtiles Ytiles tileWidth tileHeight"
#          "\x00"
#          ....
#
#
#  30001   username                receive a list of USERS that are connected and their info
#          info
#          machine_id              the machines the user is connected to
#          machine_id
#          "\x00"               < --- a break between different users' info
#          username
#          info
#          machine_id
#          "\x00"
#          ....
#
#  30002   from={username}         receive a chat message from one person,
#          to={"all" | id}        id = specific to users connected to a specific SAGE machine
#          message
#
#  30003   "1" | "0"              1=username OK, 0=username already in use
#
#  31000   message                an informative message... just any string
#
#
#
#  <<< SAGE  --->  SERVER >>>
#  CODE    FORMAT                 MESSAGE
#  ----------------------------------------------------------------
#  
#  100     machine_name           "i am alive" message from SAGE
#          ip                   < --- machine ip and port that SAGE UIs should connect to
#          port
#




class MachineListCtrl(wx.ListCtrl):
    
    def __init__(self, dialogClass, parent, pos, size):
        wx.ListCtrl.__init__(self, parent, -1, pos, size, style=wx.LC_SINGLE_SEL | wx.LC_REPORT | wx.LC_NO_HEADER)
        if "__WXMAC__" not in wx.PlatformInfo:
            self.SetTextColour(wx.WHITE)
            self.SetBackgroundColour(appPanelColor)
        self.parent = parent
        self.usersData = getUsersData()
        self.host = None
        self.port = 20001
        self.hostname = ""
        self.dialogClass = dialogClass
        
        # holds the two images for 
        imageList = wx.ImageList(16, 16)
        greenImg = wx.Image(ConvertPath("images/green_circle.gif"))
        redImg = wx.Image(ConvertPath("images/red_circle.gif"))
        greenBmp = greenImg.Rescale(16,16).ConvertToBitmap()
        redBmp = redImg.Rescale(16,16).ConvertToBitmap()
        self.greenIndex = imageList.Add(greenBmp)
        self.redIndex = imageList.Add(redBmp)
        self.AssignImageList(imageList, wx.IMAGE_LIST_SMALL)
        self.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnDoubleClick)
        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelectItem)
        self.Bind(wx.EVT_LIST_ITEM_DESELECTED, self.OnDeselectItem)
        
        self.Bind(wx.EVT_MOTION, self.OnMouseMotion)
        self.currentIndex = -1

        self.InsertColumn(0, "", wx.LIST_FORMAT_LEFT)
        self.SetColumnWidth(0, self.GetSize().width-2)
        
        self.machineHash = {}   #contains all the machines currently displayed in the list
        self.RefreshMachineList()  #this will fill the self.machineHash and fill the list with it
        

    def OnSelectItem(self, evt):
        self.dialogClass.EnableButtons()

    def OnDeselectItem(self, evt):
        self.dialogClass.EnableButtons(False)
        

            #returns the last selected SAGEMachine, if none was selected, returns None
    def GetSelectedMachine(self):
        index  = self.GetFirstSelected()
        if self.toolTipHash.has_key(index):
            return self.toolTipHash[index]
        return None


            # gets the hash of machines from the config file,
            # gets the hash of machines received from the server,
            # combines them into one hash (self.machineHash),
            # if there are overlaps, the one from the server gets precedence
            # everything is keyed by the ID of each machine and the data
            # stored in every hash is actually a bunch of SAGEMachines
    def RefreshMachineList(self, data=None):
        self.DeleteAllItems()     #first delete everything

        #now update the hash holding all the machines
        self.machineHash = {}
        self.machineHash = self.usersData.GetMachinesStatus().copy()

        # insert the sorted list of machines into the control
        self.toolTipHash = {}  # a hash of sageMachines indexed by their ListCtrl index
        index = 0
        machines = self.machineHash.values()
        machines.sort(lambda x, y: cmp(x.GetName().lower(), y.GetName().lower()))
        for sageMachine in machines: 
            self.toolTipHash[index] = sageMachine  #used for creation of correct tooltips
            self.InsertRow(sageMachine, index)
            index = index + 1

        self.dialogClass.EnableButtons(False)


##     def GetMachineForAutoload(self, name):
##         self.machineHash = {}
##         print "usersData machinestatus: ", self.usersData.GetMachinesStatus()
##         self.machineHash = self.usersData.GetMachinesStatus().copy()
##         return self.GetMachineByName(name)
    
        
            # inserts an item into the ListCtrl and sets the appropriate icon for it
            # it compares the machines based on their IDs 
    def InsertRow(self, sageMachine, index=-1):
        if index == -1:  #if true, insert at the end
            index = self.GetItemCount()

        if self.usersData.HasMachine(sageMachine.GetId()):  # set the correct icon
            if sageMachine.IsAlive():
                self.InsertImageStringItem(index, sageMachine.GetName(), self.greenIndex )
            else:               #the machine is not running but the room is still open since there are users in it
                self.InsertImageStringItem(index, sageMachine.GetName(), self.redIndex )
        else:                   #the machine is not even running
            self.InsertImageStringItem(index, sageMachine.GetName(), self.redIndex )

        if self.GetItemCount() > self.GetCountPerPage():  # a visual fix 
            self.SetColumnWidth(0, self.GetSize().width - 25 - 2)
        else:
            self.SetColumnWidth(0, self.GetSize().width - 2)
            

    #---------------------------------------------------------------
    # for tooltips
    #---------------------------------------------------------------
    
    def OnMouseMotion(self, event):
        return
        (index, flag) = self.HitTest(event.GetPosition())
        if not index == wx.NOT_FOUND:
            if self.currentIndex == index:  # if the tooltip is already displayed, just return
                return
            self.currentIndex = index
            #sageMachine = self.GetMachineByName( self.GetItem(index).GetText() )
            self.MakeToolTip(self.toolTipHash[index])


    def GetMachineByName(self, name):    #returns the first machine with the specified name, or None
        for sageMachine in self.machineHash.itervalues():
            if sageMachine.GetName() == name:
                return sageMachine
        return None

        
    def MakeToolTip(self, sageMachine):
        name = string.upper(sageMachine.GetName())
        ip = "\nIP: "+sageMachine.GetIP()
        port = "\nPort: "+str(sageMachine.GetPort())
        if sageMachine.GetNumTiles()[0] == 0:
            size = "\nSize: ? x ?"
        else:
            size = "\nSize: "+str(sageMachine.GetNumTiles()[0])+" x "+str(sageMachine.GetNumTiles()[1])  
        ttString =  name + ip + port + size 
        tt = wx.ToolTip(ttString)
        tt.SetDelay(300)
        self.SetToolTip(tt)

    #---------------------------------------------------------------

    def OnDoubleClick(self, event):
        self.hostname = self.GetItemText(event.GetIndex())
        if hasattr(self.parent, "IsModal") and self.parent.IsModal():
            self.parent.EndModal(wx.ID_OK)


            # when the user presses "Add" button
    def AddMachine(self, name, host, port, sysIP, sysPort):
        newMachine = SAGEMachine(name, host, port, sysIP, sysPort, host+":"+str(port), False)
        prefs.machines.AddMachine(newMachine)    # add it to the preferences
        self.usersData.AddNewSAGEMachine(newMachine)  # and then to the list of all the machines
        self.RefreshMachineList()  #this will refresh the list


            # when the user presses "Delete" button
    def RemoveMachine(self, sageMachine):
        prefs.machines.RemoveMachine(sageMachine)   # remove from preferences
        self.usersData.RemoveMachine(sageMachine)   # now remove from the datastructure as well
        self.RefreshMachineList()   # refresh the list visually








class ConnectionDialog:

    def __init__(self, sageGate, usersClient, usersServerIP="sage.sl.startap.net", usersServerPort=15558, firstConnection=True, autologinMachine=None):
        self.sageGate = sageGate
        self.client = usersClient
        self.usersData = getUsersData()
        self.selectedMachine = None
        self.firstConnection = firstConnection  # first SAGE connection for this UI
        self.__firstTry = True   # first try of this connection... to disable autologin after the first try
        self.connectedToServer = False
        self.autologinMachine = autologinMachine
        if self.firstConnection:
            if not self.client.Connect(usersServerIP, usersServerPort):
                dlg = wx.MessageDialog(None, "Could not connect to sage server \"" + usersServerIP + "\". Chat will be unavailable."
                                       , "Connection Error", style=wx.OK)
                dlg.ShowModal()
                dlg.Destroy()
            else:
                self.connectedToServer = True
        self.usernameOK = None  #it's None until we get a reply from the UsersServer about the validity of the chosen username
        
        # data produced by this dialog
        self.machine = None


        # the machine that we are trying to connect to
    def GetMachine(self):
        return self.machine


        # show the initial connection dialog
        # if "Connect" was pressed, check the username for duplicate and if that's OK,
        # try to connect to SAGE
    def ShowDialog(self):
        while True:  # loop until the user gets it right or quits
            if not self.ShowConfigDialog():  #user pressed "Quit"
                return False
            else:     # first check the username with the UsersServer and then try to connect to SAGE
                if self.TryStart():
                    if self.firstConnection:
                        self.usersData.UnregisterUICallback(30000)  #FIX, for multiple connections
                    return True
                else:  # something failed so show the dialog again
                    self.usernameOK = None  # so that we have to check the username again


        # this does all the checks
    def TryStart(self):
        if self.firstConnection and self.connectedToServer:
            usernameResult = self.CheckUsername() 
            if usernameResult == False:
                dlg = wx.MessageDialog(None, "Username \"" + self.usersData.GetMyUsername() + "\" already taken. Please choose another one."
                                 , "Username Invalid", style=wx.OK)
                dlg.ShowModal()
                dlg.Destroy()
                return False

            elif usernameResult == -1:  #connection failed but we'll let the user continue
                dlg = wx.MessageDialog(None, "Connection to sage server failed.\nChat will be unavailable."
                                 , "Connection Failed", style=wx.OK)
                dlg.ShowModal()
                dlg.Destroy()
            
        # if we got here, username checked out OK so try to connect to SAGE and Register
        if self.ConnectToSAGE():
            if self.firstConnection or (not self.usersData.AmIConnectedTo(self.machine.GetId())):
                self.RegisterUser()  #only register if the connection to SAGE succeeded
            return True
        else:
            return False


        # enables or disables the buttons that require that a selection in the list
        # is made before proceeding  (Connect and Delete buttons notably)
    def EnableButtons(self, doEnable=True):
        if hasattr(self, "okBtn"):
            if doEnable:
                self.okBtn.Enable(True)
                self.delButton.Enable(True)
                self.infoButton.Enable(True)
            else:
                self.okBtn.Enable(False)
                self.delButton.Enable(False)
                self.infoButton.Enable(False)


    def ShowConfigDialog(self):
        
        # UI elements
        yOffset = 7
        dialog = wx.Dialog(None, -1, "SAGE Connection")#, size=(200, 240))#, style = wx.SIMPLE_BORDER)
        if "__WXMAC__" not in wx.PlatformInfo:
            dialog.SetBackgroundColour(dialogColor)
            dialog.SetForegroundColour(wx.WHITE)
        dialog.SetClientSizeWH(200, 300+yOffset)
        
        #helpButton = MyButton(dialog, (170, 10+yOffset), (20,20), self.OnHelp, "images/help.png", tTip="Help")
        self.machineList = MachineListCtrl(self, dialog, (20,35+yOffset), (160,125))
        if self.firstConnection:
            self.usersData.RegisterUICallback( 30000, self.machineList.RefreshMachineList) #so that we receive updates even if we are not registered yet
        text = wx.StaticText(dialog, -1, "Connect To:", (10, 10+yOffset))
        addButton = wx.Button(dialog, -1, "Add",  (15, 160+yOffset), (50, 20))
        addButton.SetClientSize((50, 20))
        smallFont = addButton.GetFont()
        smallFont.SetPointSize(smallFont.GetPointSize() - smallFont.GetPointSize()/5)
        addButton.SetFont(smallFont)
        self.infoButton = wx.Button(dialog, -1, "Info", (75, 160+yOffset), (50, 20))
        self.infoButton.SetClientSize((50,20))
        self.infoButton.SetFont(smallFont)
        self.infoButton.Enable(False)
        self.delButton = wx.Button(dialog, -1, "Delete", (135, 160+yOffset), (50, 20))
        self.delButton.Enable(False)
        self.delButton.SetClientSize((50, 20))
        self.delButton.SetFont(smallFont)
        self.okBtn = wx.Button(dialog, wx.ID_OK, "Connect", (10, 264+yOffset))
        self.okBtn.Enable(False)
        if self.firstConnection:  #if it's a first connection, allow the user to enter a username
            cancelBtn = wx.Button(dialog, wx.ID_CANCEL, "Quit", (110, 264+yOffset))
            userLabel = wx.StaticText(dialog, -1, "Connect As:", (10, 195+yOffset))
            initialUsername = prefs.usernames.GetDefault()
            userText = wx.ComboBox(dialog, -1, initialUsername, (10,215+yOffset), (180,25), choices=prefs.usernames.GetUsernames(), style=wx.CB_DROPDOWN)
            userText.SetForegroundColour(wx.BLACK)
            userText.Bind(wx.EVT_KEY_DOWN, self.OnEnter, userText)
            userText.SetFocus()
        else:                   # login under the same name but to a another machine
            cancelBtn = wx.Button(dialog, wx.ID_CANCEL, "Cancel", (110, 264+yOffset))
            userLabel = wx.StaticText(dialog, -1, "Connect As: "+self.usersData.GetMyUsername(), (10, 205))

        dialog.Bind(wx.EVT_BUTTON, self.OnAddButton, addButton)
        dialog.Bind(wx.EVT_BUTTON, self.OnDelButton, self.delButton)
        dialog.Bind(wx.EVT_BUTTON, self.OnInfoButton, self.infoButton)
        #### - end UI elements


        # try autologging first... if that fails display the dialog
        if self.__firstTry and self.autologinMachine and self.usernameOK == None:
            print "Autologin to:", self.autologinMachine
            tries = 0
            while not self.machineList.GetMachineByName(self.autologinMachine) and tries < 10:
                time.sleep(0.5)
                tries += 1
                self.machineList.RefreshMachineList()
                print "Trying to autologin... try#", tries
            self.machine = self.machineList.GetMachineByName(self.autologinMachine)

        
        # so that we don't try to autologin after the first try...
        # go back to the manual mode
        if self.__firstTry:  self.__firstTry = False
        else: self.machine = None
    

        # display the dialog
        if not self.machine:
            while self.machineList.GetFirstSelected() == -1:  #loop until the user selects something
                if not dialog.ShowModal() == wx.ID_OK:   
                    return False
            self.machine = self.machineList.GetSelectedMachine()  #get the selected SAGEMachine
        
        if self.firstConnection:  #get the username... but only if this is our first connection
            if userText.GetValue() == "": #if user entered nothing, use the default
                self.usersData.SetMyUsername("No Name")
            else:
                self.usersData.SetMyUsername( userText.GetValue() )
                prefs.usernames.AddUsername(userText.GetValue())
            self.usersData.SetMyInfo("no info")
        return True


    def ShowNewConnectionDialog(self):
        dialog = wx.Dialog(None, -1, "New Connection", size=(260, 250))#, style = wx.SIMPLE_BORDER)
        dialog.SetClientSizeWH(250, 250)
        
        hostLabel = wx.StaticText(dialog, -1, "Host:", (20, 20))
        hostText = wx.TextCtrl(dialog, -1, "", (20, 40), (120, 22))
        portLabel = wx.StaticText(dialog, -1, "Port:", (160, 20))
        portText = wx.TextCtrl(dialog, -1, "20001", (160, 40), (70, 22))

        sysIPLabel = wx.StaticText(dialog, -1, "System IP (optional):", (20, 90))
        sysIPText = wx.TextCtrl(dialog, -1, "", (20, 110), (120, 22))
        sysPortLabel = wx.StaticText(dialog, -1, "System Port:", (160, 90))
        sysPortText = wx.TextCtrl(dialog, -1, "20002", (160, 110), (70, 22))

        labelLabel = wx.StaticText(dialog, -1, "Label this connection as:", (20, 160))
        labelText = wx.TextCtrl(dialog, -1, "", (20, 180), (140, 22))
        self.okBtn = wx.Button(dialog, wx.ID_OK, "Save", (40, 210))
        cancelBtn = wx.Button(dialog, wx.ID_CANCEL, "Cancel", (140, 210))

        # so that the user can just click ENTER when they are done entering the data
        labelText.Bind(wx.EVT_KEY_DOWN, self.OnEnter, labelText)
        
        if dialog.ShowModal() == wx.ID_OK:
            return (labelText.GetValue(), hostText.GetValue(), portText.GetValue(), sysIPText.GetValue(), sysPortText.GetValue())
        else:
            return (None, None, None, None, None)


    def OnHelp(self):
        AboutDialog(None)
    

        # sends a request to the usersServer asking if the username already exists
    def CheckUsername(self):
        if self.client.IsConnected():
            self.client.CheckUsername(self.usersData.GetMyUsername())
            self.totalTime = 0
            return self.WaitForUsernameReply()
        else:  #we are not even connected so dont bother
            return -1


        # returns: True if username is valid, False if invalid and0
        # -1 if something went wrong but we should still continue execution w/o the chat
    def RegisterUser(self):
        if self.client.IsConnected():
            self.client.Register( self.machine.GetId() )


        # waits for 5 seconds and sleeps every 1/2 second
        # once the message from UsersServer arrives informing us of validity of
        # the chosen username, this function returns True if username was accepted or False otherwise
    def WaitForUsernameReply(self):
        while self.client.usernameOK == None:
            time.sleep(0.5)
            self.totalTime = self.totalTime + 0.5
            if self.totalTime > 3:  #dont wait forever... if we waited longer than 5 secs... return -1 (means "failed")
                self.totalTime = 0
                return -1
        else:   # we got a reply back
            self.totalTime = 0
            return self.client.usernameOK


        # try to connect to the selected machine
    def ConnectToSAGE(self):
        retVal = self.sageGate.connectToSage( self.machine.GetIP(), self.machine.GetPort() )
        if retVal == 1: 
            return True
        elif retVal == -1:
            dlg = wx.MessageDialog(None, "Could not connect to the appLauncher on " + self.machine.GetName(), "AppLauncher Connection Failed", wx.OK)
            dlg.ShowModal()
            return True
        else:
            dlg = wx.MessageDialog(None, "Could not connect to SAGE on " + self.machine.GetName() + " (" + self.machine.GetId() + ")", "Connection Failed", wx.OK)
            dlg.ShowModal()
            return False

            
        # so that the user can press ENTER to connect (when focus is on the username text field)
    def OnEnter(self, evt):
        if evt.GetKeyCode() == wx.WXK_RETURN:
            obj =  evt.GetEventObject() 
            if hasattr(obj, "EndModal"):  #since we use this in NewConnection and Config dialogs
                obj.EndModal(wx.ID_OK) #OnEnter was bound to a dialog
            else:
                obj.GetParent().EndModal(wx.ID_OK)  #OnEnter was bound to a child of a dialog
        else:
            evt.Skip()


        # shows the info about a selected machine
    def OnInfoButton(self, evt):
        sageMachine = self.machineList.GetSelectedMachine()
        if sageMachine == None:
            return

        info = "MACHINE: " + str(sageMachine.GetName())
        info += "\n-----------------------------------------------------"
        info += "\nIP ADDRESS: " + str(sageMachine.GetIP())
        info += "\nPORT FOR SAGE UI CONNECTIONS: " + str(sageMachine.GetPort())
        info += "\nSYSTEM IP: " + sageMachine.GetSystemIP()
        info += "\nSYSTEM PORT: " + sageMachine.GetSystemPort()
        if sageMachine.IsAlive():
            info += "\nRUNNING:\tYes"
        else:
            info += "\nRUNNING:\tNo"
        info += "\nTILE ARRANGEMENT:\t"+str(sageMachine.GetNumTiles()[0])+" x "+str(sageMachine.GetNumTiles()[1]) 
        info += "\nDISPLAY SIZE:\t"+str(sageMachine.GetDisplaySize()[0])+" x "+str(sageMachine.GetDisplaySize()[1])+" pixels"
        info += "\nTILE SIZE:\t"+str(sageMachine.GetTileSize()[0])+" x "+str(sageMachine.GetTileSize()[1])+ " pixels"
        
        dialog = wx.MessageDialog(None, info, "Details for "+str(sageMachine.GetName()), style=wx.OK)
        dialog.ShowModal()
        dialog.Destroy()
        

    def OnAddButton(self, evt):
        while True:
            (name, host, port, sysIP, sysPort) = self.ShowNewConnectionDialog()
            if host==None:  #user pressed "Cancel"
                return
            break
        if len(sysIP) < 2:
            sysIP = host  #in case user hasnt filled the sysIP field
        self.machineList.AddMachine(name, host, port, sysIP, sysPort)  #inserts the item into the machineList and update the config file


    def OnDelButton(self, evt):
        selectedMachine = self.machineList.GetSelectedMachine()
        if not selectedMachine == None:   #if the user actually has something selected
            self.machineList.RemoveMachine(selectedMachine)





class ChatRoom(wx.Panel):

    def __init__(self, parent, machineId, machine_name, usersClient):
        wx.Panel.__init__(self, parent, -1)
        self.SetBackgroundColour(dialogColor)
        self.SetForegroundColour(wx.WHITE)
        self.client = usersClient
        self.machineId = machineId  #the machine this chatRoom corresponds to
        self.machine_name = machine_name  #the machine this chatRoom corresponds to
        self.usersData = getUsersData()
        
        self.MakeControls()


    def GetId(self):
        return self.machineId

    def GetMachineName(self):
        return self.machine_name
    

    def MakeControls(self):
        # create the sizer and the panel
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)
        self.mainSizer.Fit(self)
        self.SetSizer(self.mainSizer)

        # create the users list
        self.userListLabel = wx.StaticText(self, -1, "Connected Users:")
        self.userList = wx.ListBox(self, -1, choices=self.usersData.GetUsernames(self.machineId))#, style=wx.LB_SINGLE)
        self.userList.SetSizeHints(-1, -1, -1, maxH = 200)
        
        # create the chat window
        self.chatLabel = wx.StaticText(self, -1, "Chat Window:")
        self.chatWindow = wx.SplitterWindow(self, -1, style = wx.SP_3DSASH )#| wx.SP_LIVE_UPDATE )
        self.typePanel = wx.TextCtrl(self.chatWindow, -1, "Type your message here and press ENTER to send", style=wx.TE_MULTILINE )
        self.typePanel.Bind(wx.EVT_KEY_DOWN, self.SendMessage)  #when you press enter, the message is sent
        self.typePanel.Bind(wx.EVT_SET_FOCUS, self.OnFocus)  #when it receives focus, remove the info
        self.chatPanel = wx.TextCtrl(self.chatWindow, -1, style=wx.TE_MULTILINE | wx.TE_RICH | wx.TE_LINEWRAP)
        self.chatPanel.SetEditable(False)
        
        self.chatWindow.SetMinimumPaneSize(45)
        self.chatWindow.SplitHorizontally(self.chatPanel, self.typePanel, -45)
        self.chatWindow.SetBackgroundColour(dialogColor)#appPanelColor)
        self.chatWindow.SetForegroundColour(wx.WHITE)
        # add them to the sizer in the right order
        self.mainSizer.Add(self.userListLabel, 0, wx.ALIGN_LEFT | wx.ALIGN_TOP | wx.TOP | wx.LEFT, border=10)
        self.mainSizer.Add(self.userList, 1, wx.ALIGN_LEFT | wx.ALIGN_TOP | wx.ALL | wx.EXPAND, border=5)
        self.mainSizer.Add(self.chatLabel, 0, wx.ALIGN_LEFT | wx.ALIGN_TOP | wx.TOP | wx.LEFT, border=10)
        self.mainSizer.Add(self.chatWindow, 2, wx.ALIGN_LEFT | wx.ALIGN_TOP | wx.ALL | wx.EXPAND, border=5)
        #self.mainSizer.Layout()  #causes  "QPixmap: Invalid pixmap parameters" error
        self.chatWindow.SetSashGravity(1.0) #must set the gravity here, otherwise default size gets screwed up

        if "__WXMSW__" in wx.PlatformInfo:   #windows has problems showing splitterWindow in the beginning
            e = wx.SizeEvent(self.chatWindow.GetSize())
            self.chatWindow.ProcessEvent(e)

        # send a message when user presses ENTER inside the typePanel
    def SendMessage(self, event):
        if event.GetKeyCode() == wx.WXK_RETURN or event.GetKeyCode() == wx.WXK_NUMPAD_ENTER:
            if not self.client.IsConnected():  #if we are not even connected, dont send anything
                dlg = wx.MessageDialog(None, "Message could not be sent since there is no connection to the sage server."
                             , "Message not sent", style=wx.OK)
                dlg.ShowModal()
                dlg.Destroy()
                return
            
            if self.typePanel.GetValue() != "":  #did the user actually type something
                if not self.usersData.AmIConnectedTo(self.GetId()):   #if the message was sent from a room that this user is not logged into
                    if self.usersData.HasMachine(self.usersData.GetMyConnections()[0]):
                        fromUser = self.usersData.GetMyUsername() + "@" + self.usersData.GetMachine(self.usersData.GetMyConnections()[0]).GetName()
                    else:
                        fromUser = self.usersData.GetMyUsername() + "@none"
                else:  #i am in the room that i am connected to so just print my name
                    fromUser = self.usersData.GetMyUsername()
                self.client.SendChatMessage(fromUser, self.GetId(), self.typePanel.GetValue())
                self.typePanel.Clear()
        else:
            event.Skip()

        #when it receives focus, remove the info "Type your message here and press ENTER to send"
    def OnFocus(self, event):
        self.typePanel.Clear()
        
    
#-------------------------------------------------------
#  MESSAGE CALLBACKS
#-------------------------------------------------------

    def OnUsersStatus(self):
        # update the ListBox control with the new data from UsersDatastructure
        self.userList.Set(self.usersData.GetUsernames(self.GetId()))
        

        #just write the message to the screen with the right font properties
    def OnChatMessage(self, fromUser, message):
        self.chatPanel.SetDefaultStyle(wx.TextAttr(wx.BLUE))
        self.chatPanel.SetInsertionPointEnd()
        self.chatPanel.AppendText(fromUser + ": ")
        self.chatPanel.SetDefaultStyle(wx.TextAttr(wx.BLACK))
        self.chatPanel.AppendText(message+"\n") 




############################################################################
#
#  CLASS: UsersPanel
#  
#  DESCRIPTION: This is the panel on the side of SAGE UI that contains the
#               list of users logged in to this SAGE environment. It also
#               enables users to chat.
#
#  DATE:        May, 2005
#
############################################################################

class UsersPanel(wx.Frame):

    def __init__(self, parent, usersClient):
        wx.Frame.__init__(self, parent, -1, "SAGE UI Chat")#, style= wx.SIMPLE_BORDER | wx.NO_FULL_REPAINT_ON_RESIZE)
        #self.SetBackgroundColour(wx.Colour(155,200,200))
        #self.SetBackgroundColour(dialogColor)
        #self.SetForegroundColour(wx.WHITE)
        self.Bind(wx.EVT_CLOSE, self.OnCloseFrame)
        self.Bind(wx.EVT_MOVE, self.OnMove)
        self.frame = parent
        self.sticky = True   #if it moves together with the main frame

        #resize and position it correctly
        self.SetSize( (300, 500) )
        self.OnFrameMove()
        
        # register functions that will be called when messages arrive
        self.client = usersClient
        self.usersData = getUsersData()
        self.usersData.RegisterUICallback( 30000, self.OnMachinesStatus )
        self.usersData.RegisterUICallback( 30001, self.OnUsersStatus )
        self.usersData.RegisterUICallback( 30002, self.OnChatMessage )

        # local data storage
        self.CreateNotebook()
        #self.Show()   < -- this is done in DisplayCanvas.OnStatusMessage()  (canvases.py)
                

    def CreateNotebook(self):
        self.notebook = wx.Notebook(self, -1)
        #self.notebook.SetBackgroundColour(appPanelColor)
        #self.notebook.SetForegroundColour(wx.WHITE)
        self.chatRooms = {} # a collection of ChatRooms objects (basically pages in self.notebook)
        
        # first make the image list:
        il = wx.ImageList(16, 16)
        aliveImage = wx.Image(ConvertPath("images/green_circle.gif"))
        aliveBmp = aliveImage.Rescale(16,16).ConvertToBitmap()

        deadImage = wx.Image(ConvertPath("images/red_circle.gif"))
        deadBmp = deadImage.Rescale(16,16).ConvertToBitmap()
        il.Add( aliveBmp )
        il.Add( deadBmp )
        self.notebook.AssignImageList(il)

        # create the pages (chat rooms) for every SAGE machine that's currently running
        # but first, insert the "All" page in the beginning
        self.chatRooms["all"] = ChatRoom(self.notebook, "all", "all", self.client)
        self.notebook.AddPage(self.chatRooms["all"], "All")
        
        # now add the rest of them
        for machineId in self.usersData.GetMachinesStatus():
            if self.usersData.AmIConnectedTo(machineId):
                self.AddChatRoom(machineId, doSelect=True)
            else:
                self.AddChatRoom(machineId)


            #make this frame follow the main frame if sticky
    def OnMove(self, evt):
        frameX = self.frame.GetSize().width + self.frame.GetPosition().x
        frameY = self.frame.GetPosition().y
        if abs(evt.GetPosition().x - frameX) < 40:
            self.sticky = True
            self.OnFrameMove()
        else:
            self.sticky = False


        # called whenever the main frame moves so that this Users Frame follows it
    def OnFrameMove(self):
        if self.sticky:
            x = self.frame.GetSize().width + self.frame.GetPosition().x + 3
            y = self.frame.GetPosition().y + 20
            self.MoveXY(x,y)


    def OnCloseFrame(self, evt):
        self.Hide()
        self.frame.miOptionsShowChat.Check(False)


    def Disconnect(self):
        self.client.Disconnect()


    def IsConnected(self):
        return self.client.IsConnected()


    def AddChatRoom(self, machineId, machine_name="All", doSelect=False):
        if self.usersData.HasMachine(machineId):  #if false, assign a custom name that was passed in
            machine_name = self.usersData.GetMachine(machineId).GetName()

        # now create the page and add it to the notebook
        self.chatRooms[machineId] = ChatRoom(self.notebook, machineId, machine_name, self.client)
        self.notebook.AddPage(self.chatRooms[machineId], machine_name, select=doSelect)

        # set the appropriate icon for it
        if not machine_name == "All":   #"All" has no icon
            if self.usersData.HasMachine(machineId) and self.usersData.GetMachine(machineId).IsAlive():
                self.notebook.SetPageImage(self.notebook.GetPageCount()-1, 0)
            else:
                self.notebook.SetPageImage(self.notebook.GetPageCount()-1, 1)

    
#-------------------------------------------------------
#  MESSAGE CALLBACKS
#-------------------------------------------------------

    def OnMachinesStatus(self):
        # first create new pages for new machines
        for machineId in self.usersData.GetMachinesStatus():
            if not self.chatRooms.has_key(machineId): #if the room doesnt exist already...
               self.AddChatRoom(machineId)
               
        # now update the icons for pages that are no longer valid (ie the machine died)
        pageIndexToDelete = -1
        pageIdToDelete = -1
        for pageIndex in range(1, self.notebook.GetPageCount()): #start from 1 since the 0th page is "All" = no icon
            chatRoom = self.notebook.GetPage(pageIndex)
            machineId = chatRoom.GetId()
            if not self.usersData.HasMachine(machineId): #the machine died and there are no more people in that room, so remove the tab
                pageIndexToDelete = pageIndex  # perform delayed deletion since page indices
                pageIdToDelete = machineId            # would not be valid if we deleted a page now
            else:   #otherwise, the machine may not be running but there are still people in there so don't remove the tab
                if self.usersData.GetMachine(machineId).IsAlive():
                    self.notebook.SetPageImage(pageIndex, 0)
                else:
                    self.notebook.SetPageImage(pageIndex, 1)  #the machine died but there are still people in that room, so keep it open

        # finally delete the page (if there were any for deletion)
        if pageIndexToDelete > -1:
            self.notebook.DeletePage(pageIndexToDelete)
            del self.chatRooms[pageIdToDelete]
            

    def OnUsersStatus(self):
        # update all the chatRooms with the new data from UsersDatastructure
        try:
            for chatRoom in self.chatRooms.itervalues():
                chatRoom.OnUsersStatus()
        except wx.PyDeadObjectError:  # a fix so that the error doesnt show up on exit (it happens because some events might still be in the queue)
            pass

        
    def OnChatMessage(self, message):
        tokens = string.split(message, "\n", 2)
        fromUser = tokens[0]
        toRoom = tokens[1]
        message = tokens[2]

        # write the message to the appropriate window
        if toRoom == "all":
            for chatRoom in self.chatRooms.itervalues():
                chatRoom.OnChatMessage(fromUser, message)
        else:    #send a message to the specific room
            if self.chatRooms.has_key(toRoom):
                self.chatRooms[toRoom].OnChatMessage(fromUser, message)





############################################################################
#
#  CLASS: UsersClient
#  
#  DESCRIPTION: This is the connection to the SAGE users server. It connects
#               to the server, it receives and sends messages. The receiving
#               is done in a thread that constantly loops and checks for new
#               messages on a socket. It sends messages about the apps that
#               this UI started and also sends all the chat messages.
#
#  DATE:        May, 2005
#
############################################################################

class UsersClient:
    
    def __init__(self):
        self.connected = False
        self.threadKilled = False
        self.usersData = getUsersData()
        self.usernameOK = None


#-------------------------------------------------------
#  CONNECTION
#-------------------------------------------------------

    def IsConnected(self):
        return self.connected
    

    def Connect(self, host, port):
        if self.connected: return False
        
        # make the socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if self.socket is None:
            print "\n\nUsersClient: socket could not be created"
            return False

        # set the timeout the first time to 5 secs so that we dont wait for too long when
        # you cant connect to the users server... the timeout will be reset below
        self.socket.settimeout(2)
        
        # try to connect
        try:
            self.socket.connect((host, port))
        except socket.error, socket.timeout:
            print "\n\nUsersClient: can't connect to SAGE UI Users Server."
            return False

        # set some socket options
        self.socket.settimeout(SOCKET_TIMEOUT)


        # start the receiver in a thread
        self.receiverThread = Thread(target=self.Receiver, args=())
        self.receiverThread.start()
        self.threadKilled = False
        self.connected = True

        return True

  

    def Disconnect(self):
        #if not self.connected: return False
		
        self.threadKilled = True
        self.connected = False
        
	if hasattr(self, "receiverThread"):
	    self.receiverThread.join()
	
        self.socket.close()
        del self.socket
        return True


#-------------------------------------------------------
#  SENDING
#-------------------------------------------------------

           # for formatting the messages correctly and sending them
    def MakeMsg(self,code,data):
        if not self.connected:
            print "UsersClient: not connected to server, message",code,"was not sent"
            return False
        
        msg = '%8s\n%s' %(code,data)
        msg = msg + ' ' * (MSGLEN-len(msg))
        self.Send(msg)
        return msg


           # for sending string data (used by MakeMsg)
    def Send(self, msg):
        try:
            self.socket.send(msg)
        except socket.error:
            print "UsersClient: Could not send message - socket error"
            self.connected = False
        

            # register with the UsersServer
    def Register(self, machineId):
        data = self.usersData.GetMyUsername() + "\n" + self.usersData.GetMyInfo() + "\n" + machineId
        self.usersData.AddMyConnection(machineId)
        self.MakeMsg(2000, data)


    def Unregister(self, machineId):
        data = self.usersData.GetMyUsername() + "\n" + machineId
        self.usersData.RemoveMyConnection(machineId)
        self.MakeMsg(2003, data)
        

    def SendChatMessage(self, fromUser, toRoom, message):
        data = fromUser + "\n" + toRoom + "\n" + message
        self.MakeMsg(2001, data)


    def CheckUsername(self, username):
        self.usernameOK = None
        self.MakeMsg(2002, username + "\n" + str(getUIVersion()))


#-------------------------------------------------------
#  RECEIVING
#-------------------------------------------------------
        
    # this runs in a thread, loops forever and receives messages
    def Receiver(self):

        while not self.threadKilled:
            try:
                
                msg = self.socket.recv(CHUNK_SIZE)  #retrieve the message from the socket
                if len( msg ) < 2:
                    print "UsersClient: connection closed"
                    break

                # since recv is not guaranteed to receive exactly CHUNK_SIZE bytes
                # so keep receiving until it gets the whole chunk
                while len( msg ) < CHUNK_SIZE:
                    msg = msg + self.socket.recv(CHUNK_SIZE - len( msg))

                if self.threadKilled:
                    break

                # strip all the empty spaces from the message and retrieve useful information
                cleanMsg = self.CleanBuffer( msg )
                cleanMsg = string.strip( cleanMsg )
                msgHeader = string.split(cleanMsg, "\n", 1)
                if len(msgHeader) < 1:
                    continue
                code = int(msgHeader[0])
                if len(msgHeader) == 1:
                    data = ""
                else:
                    data = msgHeader[1]

                
            except socket.timeout:
                continue
            except socket.error:
                print "UsersClient: socket error on socket.receive"
                break
	    #except:
                #print 'UsersClient: exception occured in Receiver: ', sys.exc_info()[0], sys.exc_info()[1]
            #    break	    


            ##################
            # Call function for updating ui-information
            # wx.CallAfter is used because we pass this onto the GUI thread
            # to do since wx functions need to be called from the main thread (the GUI thread)
            ###########################################
            
            # call the appropriate function to update the datastructure
            if code == 30000:
                self.usersData.OnMachinesStatus(data)
            if code == 30001:
                wx.CallAfter(self.usersData.OnUsersStatus, data)
            if code == 30002:
                if data == "": continue
                wx.CallAfter(self.usersData.OnChatMessage, data )
            if code == 30003:
                self.SaveUsernameOK(data)

        # exited the while loop so we are not connected anymore
        self.connected = False
        self.threadKilled = True
		

    # converts all non-printable characters from the buffer to white spaces
    # (so that they can be removed using string.strip() function)
    def CleanBuffer( self, stBuffer ):
        stNewBuffer = ""
        
        for ch in stBuffer:
            if ch == '\0':   #this is our message separator so handle it separately
                stNewBuffer += ch
            elif ch in string.printable:
                stNewBuffer += ch
            else:
                stNewBuffer += " "
        return stNewBuffer
        

        # when user tries to register, it first has to check the username so
        # this message comes back to confirm the validity of it
    def SaveUsernameOK(self, data):
        self.usernameOK = bool(int(data))





# a data structure to hold all the machines, users and so on
class UsersDatastructure:

    def __init__(self):
        self.usersStatusHash = {}
        self.machinesStatusHash = {}
        self.uiCallback = {}   # functions for updating the UI
        self.myconnections = []    # the machines this user is connected to (SAGEMachine objects keyed by machineId)
        self._username = "No Name"  # this user's username
        self._info = "No Info"     # this user's info
        self.machinesStatusHash.update(prefs.machines.GetMachineHash().copy())
        

        # for updating the UI once the datastructure has been updated
    def RegisterUICallback(self, code, func):
        self.uiCallback[code] = func

    def UnregisterUICallback(self, code):
        if code in self.uiCallback:
            del self.uiCallback[code]
        

    ##### UPDATE DATA  -----------------------------------------------------

            # updates the data structure when the new machine status comes in
    def OnMachinesStatus(self, data):
        machines = string.split(data, SEPARATOR)
        self.ClearMachinesStatus()

        # first add the machines from the preferences
        self.machinesStatusHash.update(prefs.machines.GetMachineHash().copy())
        if data != "":   # when no machines are reported we get an empty message from the server
            # now add the machines from the server (but they first need to be created)
            for machine in machines:
                machineData = machine.splitlines()
                name = machineData[0]
                ip = machineData[1]
                port = machineData[2]
                machineId = machineData[3]
                alive = bool( int(machineData[4]) )
                displayInfo = str(machineData[5])
                displayData = string.split(displayInfo)    #extract the data from the displayInfo string
                if len(machineData) > 6: # the first time we connect we dont get the system port/ip
                    (sysIP, sysPort) = machineData[6].split()
                else:
                    (sysIP, sysPort) = (ip, port+str(1))
                self.AddNewMachine(name, ip, port, sysIP, sysPort, machineId, alive, displayData) 

        # update the ui
        if 30000 in self.uiCallback:
                wx.CallAfter(self.uiCallback[30000])
        

        # updates the data structure when the new machine status comes in
    def OnUsersStatus(self, data):
        users = string.split(data, SEPARATOR)  #split it into individual client's status
        self.ClearUsersStatus()  #recreate the hash from scratch
        for user in users:
            if user == "":
                break
            userData = string.split(user, "\n",2)
            username = userData[0]
            info = userData[1]
            if len(userData) > 2:
                machineList = string.split(userData[2], "\n")  #split the machines into a list
            else:
                machineList = []
            self.AddNewUser(username, info, machineList)  #store info about all the users in a hash

        # update the ui
        if 30001 in self.uiCallback:
            self.uiCallback[30001]()


        # actually just relays the message to the appropriate UI component
    def OnChatMessage(self, data):
        # update the ui
        if 30002 in self.uiCallback:
            self.uiCallback[30002](data)
        

    ##### MY DATA  ---------------------------------------------------------

    def GetMyInfo(self):
        return self._info

    def SetMyInfo(self, newInfo):
        self._info = newInfo

    def GetMyUsername(self):
        return self._username

    def SetMyUsername(self, username):
        self._username = username
        
    def GetMyConnections(self):
        return self.myconnections

    def AddMyConnection(self, machineId):
        self.myconnections.append(machineId) 

    def RemoveMyConnection(self, machineId):
        if machineId in self.GetMyConnections():
            self.myconnections.remove(machineId)

    def AmIConnectedTo(self, machineId):
        return machineId in self.GetMyConnections()


    ##### MACHINES  ----------------------------------------------------------

    def HasMachine(self, machineId):
        return self.GetMachinesStatus().has_key(machineId)   #True of False

    def FindMachineByIP(self, ip):
        for machine in self.machinesStatusHash.itervalues():
            if machine.HasIP(ip):
                return machine
        return False

    def GetMachine(self, machineId):
        return self.GetMachinesStatus()[machineId]   #returns SAGEMachine

    def GetMachineNames(self):
        tempList = []
        for sageMachine in self.GetMachinesStatus().itervalues():
            tempList.append(sageMachine.GetName())
        return tempList

    def GetMachinesStatus(self):  #returns SAGEMachines hash
        return self.machinesStatusHash 

    def AddNewMachine(self, name, ip, port, sysIP, sysPort, machineId, alive, displayInfo=[]):
        self.machinesStatusHash[ machineId ] = SAGEMachine(name, ip, port, sysIP, sysPort, machineId, alive, displayInfo)

    def AddNewSAGEMachine(self, newMachine):
        self.machinesStatusHash[ newMachine.GetId() ] = newMachine

    def RemoveMachine(self, machine):
        if machine.GetId() in self.machinesStatusHash:
            del self.machinesStatusHash[ machine.GetId() ]

    def ClearMachinesStatus(self):
        del self.machinesStatusHash
        self.machinesStatusHash = {}

        
    ##### USERS  -------------------------------------------------------------
        
    def HasUsername(self, username):
        return self.GetUsersStatus().has_key(username)

    def GetUser(self, username):
        return self.GetUsersStatus()[username]

    def GetUsernames(self, machine="all"):  #if machine is specified, it returns a list of all the users connected to that machine
        if machine == "all":
            return self.GetUsersStatus().keys()
        else:
            tempList = []
            # loop through all the users and see if the machine they are connected to matches
            # the machine we passed in here
            for username, sageUser in self.GetUsersStatus().iteritems():
                if machine in sageUser.GetMachines():
                    tempList.append(username)
            return tempList

    def GetUsersStatus(self):   #returns SAGEUsers
        return self.usersStatusHash

    def AddNewUser(self, username, info, machineList=[]):
        self.usersStatusHash[ username ] = SAGEUser(username, info, machineList)
            
    def ClearUsersStatus(self):
        del self.usersStatusHash
        self.usersStatusHash = {}



    


# holds info about every connected user
class SAGEUser:

    def __init__(self, username, info, machineList=[]):
        self._username = username
        self._info = info
        self._machines = machineList[:]  #the list of machines this user is connected to

        
    def GetName(self):
        return self._username

    def SetName(self, name):
        self._username = name

    def GetInfo(self):
        return self._info

    def SetInfo(self, info):
        self._info = info

    def GetMachines(self):
        return self._machines



# holds info about every currently running SAGE machine
class SAGEMachine:

    def __init__(self, name, ip, port, sysIP, sysPort, machineId, alive, displayInfo=[] ):
        self.name = name
        self.ip = ip
        self.port = int(port)
        self.sysIP = sysIP
        self.sysPort = sysPort
        self.machineId = machineId
        self.alive = alive
        if not len(displayInfo) == 6:
            displayInfo = ["?","?","?","?","?","?"]
            
        self.xTiles = displayInfo[0]
        self.yTiles = displayInfo[1]
        self.displayWidth = displayInfo[2]
        self.displayHeight = displayInfo[3]
        self.tileWidth = displayInfo[4]
        self.tileHeight = displayInfo[5]

    def GetName(self):
        return self.name

    def GetIP(self):
        return self.ip

    def GetPort(self):
        return self.port

    def GetSystemIP(self):
        return self.sysIP

    def GetSystemPort(self):
        return self.sysPort

    def GetId(self):
        return self.machineId

    def IsAlive(self):
        return self.alive

    def GetNumTiles(self):
        return (self.xTiles, self.yTiles)   #returns a tuple of (x*y) number of tiles

    def GetDisplaySize(self):
        return (self.displayWidth, self.displayHeight)

    def GetTileSize(self):
        return (self.tileWidth, self.tileHeight)

    def HasIP(self, ip):
        return self.sysIP==ip or self.ip==ip
