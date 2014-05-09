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


import wx, xmlrpclib, string, socket
import traceback as tb




class App:
    """ describes the app that's currently running and that has been executed by an appLauncher"""
    def __init__(self, name, windowId, command, machine, launcher):
        self.name = name
        self.windowId = windowId
        self.command = command
        self.machine = machine
        self.launcher = launcher  #the appLauncherId that started this app

    def getName(self):
        return self.name

    def getId(self):
        return self.windowId

    def getCommand(self):
        return self.command

    def getRenderMachine(self):
        return self.machine

    def getLauncher(self):
        return self.launcher

    def getStringForm(self):
        info = "APPLICATION NAME: " + self.name
        info = info + "\n------------------------------------------------"
        info = info + "\nEXECUTION COMMAND: " + str(self.command)
        info = info + "\nRENDERING MACHINE:\t"+str(self.machine) 
        return info
    

class AppLauncher:

    def __init__(self, launcherId, name, ip, port, appList):
        self.port = port
        self.appList = appList
        self.ip = ip
        self.launcherId = launcherId
        self.name = name
	self.connected = False


    def connect(self):
	if not self.connected:
	    socket.setdefaulttimeout(2)  #set the timeout to 3 seconds so that we dont wait forever
	    self.server = xmlrpclib.ServerProxy("http://" + self.ip + ":" + str(self.port))
	    try:
		self.server.test() #just use this as a way of testing whether the server is running or not
		self.connected = True
	    except socket.error:
		return False
	    except:
		tb.print_exc()
		return False
	return True


    def stopApp(self, windowId):
        try:
            self.connect()  #connect if necessary
            return self.server.stopApp(windowId)
        except socket.error:
            wx.MessageBox("No connection to the appLauncher: "+str(self.launcherId), "Failed")
            self.connected = False
        except:
            wx.MessageBox("There was an error stopping application:\n\n"+
                          str("".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))),
                          "Failed")
            
    def appStatus(self):
        try:
            self.connect()  #connect if necessary
            return self.server.appStatus()
        except socket.error:
            wx.MessageBox("No connection to the appLauncher: "+str(self.launcherId), "Failed")
            self.connected = False
        except:
            wx.MessageBox("There was an error getting application list:\n\n"+
                          str("".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))),
                          "Failed")
        return {}


    def killLauncher(self):
        try:
            self.connect()  #connect if necessary
            self.server.killLauncher()
        except socket.error:
            wx.MessageBox("No connection to the appLauncher: "+str(self.launcherId), "Failed")
            self.connected = False
        except:
            wx.MessageBox("There was an error killing application launcher:\n\n"+
                          str("".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))),
                          "Failed")



    def getId(self):
        return self.launcherId

    def getIP(self):
        return self.ip

    def getAppList(self):
        return self.appList

    def setAppList(self, appList):
        self.appList = appList

    def getPort(self):
        return self.port

    def getName(self):
        return self.name





class MainFrame(wx.Frame):

    def __init__(self, serverIP, serverPort=8009):
        wx.Frame.__init__(self, None, -1, "AppLauncher Admin", pos = (100,100), size=(550,300))
        self.server = xmlrpclib.ServerProxy("http://"+serverIP+":"+str(serverPort))
        self.CreateControls()
        self.launcherHash = {}
        self.appHash = {}
        self.updateLauncherList()
        self.SetBackgroundColour(wx.Colour(51, 102, 102))
        self.Show(True)

        
    ### connects to the sage server and retrieves the list of all app launchers running
    def updateLauncherList(self):
        self.launcherList.ClearAll()  #clear the ListBox
        self.launcherHash = {}
	try:
	    # a hash comes back (key=launcherId, value=appList - that's another hash of appNames and configs)
	    launchers = self.server.GetRegisteredLaunchers()
	except socket.error:
            wx.MessageBox("No connection to the sage server.", "No connection")
	except:
	    tb.print_exc()
	else:
	    for launcherString, appList in launchers.iteritems():
		(name, launcherId) = launcherString.split(":",1)
		(ip, port) = launcherId.split(":",1)
                newId = wx.NewId()
                item = wx.ListItem()
                item.SetText(name)
                item.SetData(newId)
                self.launcherHash[newId] = AppLauncher(launcherId, name, ip, port, appList)
                self.launcherList.InsertItem(item)
		
	
    def CreateControls(self):
        self.launcherLabel = wx.StaticText(self, -1, "Registered AppLaunchers", style=wx.ALIGN_CENTER)
        self.appLabel = wx.StaticText(self, -1, "Apps Running On: ", style=wx.ALIGN_CENTER)
        self.appInfoLabel = wx.StaticText(self, -1, "Details for: ", style=wx.ALIGN_CENTER)

        self.launcherLabel.SetForegroundColour(wx.WHITE)
        self.appLabel.SetForegroundColour(wx.WHITE)
        self.appInfoLabel.SetForegroundColour(wx.WHITE)

        self.launcherList = wx.ListCtrl(self, -1, style=wx.LC_LIST | wx.LC_SINGLE_SEL )
        self.launcherList.SetMinSize((100,100))
        self.appList = wx.ListCtrl(self, -1, style=wx.LC_LIST | wx.LC_SINGLE_SEL )
        self.appList.SetMinSize((200,100))
        self.appInfo = wx.TextCtrl(self, -1, "", style=wx.TE_READONLY | wx.TE_MULTILINE | wx.TE_DONTWRAP)
        self.appInfo.SetMinSize((300,100))
        
        self.killAppBtn = wx.Button(self, -1, "Kill App")
        self.killLauncherBtn = wx.Button(self, -1, "Kill Launcher")
        self.refreshBtn = wx.Button(self, -1, "Refresh")


        # registering event handlers
        self.launcherList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnLauncherListSelect)
        self.appList.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnAppListSelect)
        self.killAppBtn.Bind(wx.EVT_BUTTON, self.OnKillAppBtn)
        self.killLauncherBtn.Bind(wx.EVT_BUTTON, self.OnKillLauncherBtn)
        self.refreshBtn.Bind(wx.EVT_BUTTON, self.UpdateLists)
        
        # placement of the controls
        gridSizer = wx.FlexGridSizer(2,3,0,20)
        gridSizer.Add(self.launcherLabel, 0, wx.ALIGN_CENTER | wx.TOP | wx.LEFT | wx.EXPAND, border=15)
        gridSizer.Add(self.appLabel, 0, wx.ALIGN_CENTER | wx.TOP, border=15)
        gridSizer.Add(self.appInfoLabel, 0, wx.ALIGN_CENTER | wx.TOP, border=15)

        gridSizer.Add(self.launcherList, 1, wx.EXPAND | wx.ALL, border=10)
        gridSizer.Add(self.appList, 1, wx.EXPAND | wx.ALL, border=10)
        gridSizer.Add(self.appInfo, 1, wx.EXPAND | wx.ALL, border=10)

        gridSizer.Add(self.killLauncherBtn, 0, wx.ALIGN_CENTER | wx.BOTTOM, border=15)
        gridSizer.Add(self.killAppBtn, 0, wx.ALIGN_CENTER | wx.BOTTOM, border=15)
        gridSizer.Add(self.refreshBtn, 0, wx.ALIGN_RIGHT | wx.BOTTOM | wx.RIGHT, border=15)

        gridSizer.AddGrowableRow(1)
        gridSizer.AddGrowableCol(2)
        
        self.SetSizer(gridSizer)
        gridSizer.Fit(self)

        
    def OnLauncherListSelect(self, event=None):
        selection = event.GetText()
        self.appList.ClearAll()
        self.appLabel.SetLabel("Apps Running On: " + str(selection))
        self.appInfo.Clear()
        self.appInfoLabel.SetLabel("Details for: ")
        appLauncher = self.launcherHash[event.GetData()] 
        for windowId, (appName, command, machine) in appLauncher.appStatus().iteritems():
            newId = wx.NewId()
            item = wx.ListItem()
            item.SetText(appName)
            item.SetData(newId)
            self.appHash[newId] = App(appName, int(windowId), command, machine, appLauncher)
            self.appList.InsertItem(item)
        self.GetSizer().Layout()


    def OnAppListSelect(self, event=None):
        selection = event.GetText()
        self.appInfo.Clear()
        self.appInfoLabel.SetLabel("Details for: " + str(selection))
        self.appInfo.WriteText(self.appHash[event.GetData()].getStringForm())
        self.GetSizer().Layout()
        

    def UpdateLists(self, event=None):
        self.updateLauncherList()
        self.appList.ClearAll()
        self.appLabel.SetLabel("Apps Running On: ")
        self.appInfo.Clear()
        self.appInfoLabel.SetLabel("Details for: ")
        self.GetSizer().Layout()


    def OnKillAppBtn(self, event):
        selection = self.appList.GetFirstSelected()
        if selection > -1:
            app = self.appHash[self.appList.GetItemData(selection)]
            app.getLauncher().stopApp(app.getId())
            self.UpdateLists()


    def OnKillLauncherBtn(self, event):
        selection = self.launcherList.GetFirstSelected()
        if selection > -1:
            msg = "Really stop this application launcher? You may be interfering with someone else's work."
            dlg = wx.MessageDialog(None, msg, "Confirm kill", style = wx.OK | wx.CANCEL)
            if dlg.ShowModal() == wx.ID_OK:
                appLauncher = self.launcherHash[self.launcherList.GetItemData(selection)]
                appLauncher.killLauncher()
                self.UpdateLists()




class MyApp(wx.App):
    def __init__(self):
        wx.App.__init__(self, redirect=False)

    def OnInit(self):
        return True




        

def main(argv):
    name, ext  = os.path.splitext(argv[1])

    if len(argv) == 2:
        serverIP = "sage.sl.startap.net"
        serverPort = 8009
    elif len(argv) == 3:
        serverIP = argv[2]
        serverPort = 8009
    else:
        print "Usage: python admin.py [SERVER_IP]\n"
        sys.exit(0)

    app = MyApp()
    frame = MainFrame(serverIP, serverPort)
    app.SetTopWindow(frame)
    app.MainLoop()



if __name__ == '__main__':
    import sys, os
    main(['', os.path.basename(sys.argv[0])] + sys.argv[1:])
