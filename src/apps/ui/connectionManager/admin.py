############################################################################
#
# SAGE UI Users Server - A server that handles users, fsManagers and chat for SAGE
#
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


import wx
import xmlrpclib
import string

server = None
REGISTERED_USERS = 1
CONNECTED_USERS = 2


class MyFrame(wx.Frame):

    def __init__(self):
        wx.Frame.__init__(self, None, -1, "SAGE Server Admin", pos = (100,100), size=(450,500))
        self.CreateControls()
        self.usersType = REGISTERED_USERS
        
        self.Show(True)


    def CreateControls(self):
        userLabel = wx.StaticText(self, -1, "Registered Users", style=wx.ALIGN_CENTER)
        #self.userCombo = wx.ComboBox(self, -1, "Registered Users", choices=["Registered Users", "Connected Users"], style = wx.CB_DROPDOWN | wx.CB_READONLY)
        machineLabel = wx.StaticText(self, -1, "Registered Machines", style=wx.ALIGN_CENTER)
        self.userList = wx.ListBox(self, -1, choices=server.GetRegisteredUsers(), style=wx.LB_SINGLE)
        self.machineList = wx.ListBox(self, -1, choices=server.GetRegisteredMachines(), style=wx.LB_SINGLE)
        self.disconnectUserBtn = wx.Button(self, -1, "Disconnect User")
        self.disconnectMachineBtn = wx.Button(self, -1, "Disconnect Machine")
        self.infoArea = wx.TextCtrl(self, -1, "", style=wx.TE_READONLY | wx.TE_MULTILINE)
        self.refreshBtn = wx.Button(self, -1, "Refresh")


        # registering event handlers
        self.userList.Bind(wx.EVT_LISTBOX, self.OnUserListSelect)
        self.machineList.Bind(wx.EVT_LISTBOX, self.OnMachineListSelect)
        self.disconnectUserBtn.Bind(wx.EVT_BUTTON, self.OnUserDisconnectBtn)
        self.disconnectMachineBtn.Bind(wx.EVT_BUTTON, self.OnMachineDisconnectBtn)
        self.refreshBtn.Bind(wx.EVT_BUTTON, self.UpdateLists)
        #self.userCombo.Bind(wx.EVT_COMBOBOX, self.OnUserComboBox)
        
        # placement of the controls
        gridSizer = wx.FlexGridSizer(3,2,0,20)
        gridSizer.Add(userLabel, 0, wx.ALIGN_CENTER | wx.BOTTOM | wx.LEFT | wx.EXPAND, border=5)
        gridSizer.Add(machineLabel, 0, wx.ALIGN_CENTER | wx.BOTTOM, border=5)
        gridSizer.Add(self.userList, 1, wx.EXPAND | wx.LEFT, border=15)
        gridSizer.Add(self.machineList, 1, wx.EXPAND | wx.RIGHT, border=15)
        gridSizer.Add(self.disconnectUserBtn, 0, wx.ALIGN_CENTER | wx.TOP, border=5)
        gridSizer.Add(self.disconnectMachineBtn, 0, wx.ALIGN_CENTER | wx.TOP, border=5)
        gridSizer.AddGrowableRow(1)
        gridSizer.AddGrowableCol(0)
        gridSizer.AddGrowableCol(1)
        
        boxSizer = wx.BoxSizer(wx.VERTICAL)
        boxSizer.AddSpacer((10,10))
        boxSizer.Add( gridSizer, 1, wx.ALIGN_CENTER | wx.EXPAND)
        boxSizer.AddSpacer((10,10))
        boxSizer.Add( self.infoArea, 1, wx.ALIGN_CENTER | wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 15)
        boxSizer.AddSpacer((20,20))
        boxSizer.Add( self.refreshBtn, 0, wx.ALIGN_CENTER | wx.BOTTOM, border=10)
        self.SetSizer(boxSizer)


##     def OnUserComboBox(self, event):
##         if self.userCombo.GetValue() == "Registered Users":
##             self.usersType = REGISTERED_USERS
##         else:
##             self.usersType = CONNECTED_USERS
##         self.UpdateLists()

    def OnUserListSelect(self, event):
        if event.IsSelection():
            if self.usersType == REGISTERED_USERS:
                machines = server.GetUserInfo( event.GetString() )
                if machines == -1:
                    machines = "Unable to retrieve a list of connections"
            else:
                machines = "This user is not even registered with the server so he\nis not connected to any machines yet"
            info = "USER: " + event.GetString() + "\nCONNECTED TO: " + str(machines)
            self.infoArea.Clear()
            self.infoArea.WriteText(info)

    def OnMachineListSelect(self, event):
        if event.IsSelection():
            (name, ip, port, machineId, alive, displayString, userList) = server.GetMachineInfo( event.GetString().split(" - ", 1)[1].strip() )
            info = "MACHINE: " + str(event.GetString())
            info = info + "\n---------------------------------------------------------"
            if name == -1:
                info = info + str("\nCould not retrieve additional information")
            else:
                displayInfo = displayString.split(" ", 5)
                info = info + "\nPORT FOR SAGE UI CONNECTIONS:" + str(port)
                if alive == 1:
                    info = info + "\nRUNNING:\tYes"
                else:
                    info = info + "\nRUNNING:\tNo"
                info = info + "\nTILE ARRANGEMENT:\t"+str(displayInfo[0])+" x "+str(displayInfo[1]) 
                info = info + "\nDISPLAY SIZE:\t"+str(displayInfo[2])+" x "+str(displayInfo[3])+" pixels"
                info = info + "\nTILE SIZE:\t"+str(displayInfo[4])+" x "+str(displayInfo[5])+ " pixels"
                info = info + "\nUSERS CONNECTED:\t"+str(userList)
            
            self.infoArea.Clear()
            self.infoArea.WriteText(info)
            

    def UpdateLists(self, event=None):
        if self.usersType == REGISTERED_USERS:
            self.userList.Set(server.GetRegisteredUsers())
        else:
            self.userList.Set(server.GetConnectedUsers())
        self.machineList.Set(server.GetRegisteredMachines())
        self.infoArea.Clear()


    def OnUserDisconnectBtn(self, event):
        selection = self.userList.GetStringSelection()
        if not selection == "":
            if self.usersType == REGISTERED_USERS:
                server.DisconnectUser(selection)
            else:
                server.DisconnectConnectedUser(selection)
            self.UpdateLists()

    def OnMachineDisconnectBtn(self, event):
        selection = self.machineList.GetStringSelection()
        if not selection == "":
            server.DisconnectMachine(selection.split(" - ", 1)[1].strip())
            self.UpdateLists()



class MyApp(wx.App):
    def __init__(self):
        wx.App.__init__(self, redirect=False)


    def OnInit(self):
        self.frame = MyFrame()
        self.SetTopWindow(self.frame)
        return True

        

def main(argv):
    name, ext  = os.path.splitext(argv[1])
    module = __import__(name)

    if len(argv) == 2:
        serverIP = "sage.sl.startap.net"
        serverPort = 8009
    elif len(argv) == 3:
        print "Usage: python status.py [SERVER_IP] [SERVER_PORT]\n"
        sys.exit(0)
    else:
        serverIP = argv[2]
        serverPort = int(argv[3])

    global server
    server = xmlrpclib.ServerProxy("http://"+serverIP+":"+str(serverPort)) 
    print "\nServer running:", server

    app = MyApp()
    app.MainLoop()



if __name__ == '__main__':
    import sys, os
    main(['', os.path.basename(sys.argv[0])] + sys.argv[1:])
