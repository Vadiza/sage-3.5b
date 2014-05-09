#!/usr/bin/env python

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
#         Allan Spale
#
############################################################################




############################################################################
#
#  IMPORTS
#
############################################################################

# python and wx stuff
import string, sys, os, copy, optparse, time
from threading import Timer
import traceback as tb

# hyperlink module is not (yet) included with wxPython as a standard module
# so not all distributions have it
global use_hyperlink
use_hyperlink = True
try:
    import wx.lib.hyperlink as hyperlink    #for the about box
except:
    use_hyperlink = False

# if this UI should autosave
global autosave

# my stuff
import Graph
from sageGate import *
from sageData import *
from canvases import *
from globals import *
from users import * 
import preferences as prefs
import launcherAdmin as lAdmin
from sagePath import getUserPath


############################################################################
#
#  GLOBAL CONSTANTS
#
############################################################################

# current version of the UI
VERSION = "3.0a"
setUIVersion(VERSION)


### IDS used for the menu items:
SAVE_STATE = 10
LOAD_STATE = 1
ASPECT_RATIO = 2
SAGE_COLOR = 3
PERF_DATA = 4
LOG_PERF_DATA = 5
RENDER_BW = 6
DISPLAY_BW = 7
RECORD_SESSION = 8
READ_SESSION = 9
SAGE_SHUTDOWN = 11
TILE_WINDOWS = 12



############################################################################

class Log:
    def WriteText(self, text):
        if text[-1:] == '\n':
            text = text[:-1]
        wx.LogMessage(text)
    write = WriteText


assertMode = wx.PYAPP_ASSERT_DIALOG



############################################################################
#
#  CLASS: DisplayNotebook
#  
#  DESCRIPTION: This is the main container (notebook) for the UI. It holds
#               one DisplayPage for every connection to SAGE. It also handles
#               page changes and menu changes when switching pages.
#
#  DATE:        June, 2005
#
############################################################################

class DisplayNotebook(wx.Notebook):

    def __init__(self, parent):
        wx.Notebook.__init__(self, parent, -1, style=wx.NO_BORDER)
        self.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChange)

        # open the file for recording totals for all sites
        try:
            stDateTime = time.strftime("%Y%m%d-%H%M%S", time.localtime())
            stFilename = "ALL_SITES_TOTALS-" + stDateTime
            stPath = opj(DATA_DIR, stFilename + ".txt")
            self._totalsFile = open(stPath, "w")
            self._totalsFile.write( time.asctime() + "\n" )
            self._totalsFile.write( '-' * 40 + "\n" )
            tempString = (' Timestamp(s)    Disp BW(Gbps)    Rend BW(Gbps)   Num Sites\n')
            self._totalsFile.write(tempString)
            self._totalsFile.write( '-' * len(tempString) + "\n" )
            self._totalsFile.flush()
        except:
            print "\n\nERROR: Unable to record performance totals in a file:\n"
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
            self._totalsFile = None

        # start the timer to periodically record totals
        self.Bind(wx.EVT_TIMER, self.onPerfTimer)
        self.perfTimer = wx.Timer(self)
        self.perfTimer.Start(1000)


        ### this timer is called once per second to write the performance
        ### data totals to a file (for each sage connection)
    def onPerfTimer(self, event):
        totalR,totalD = 0,0  # totals for ALL sites... render and display
        recordTotals = False
        
        for pageIndex in range(0, self.GetPageCount()):
            p = self.GetPage(pageIndex)
            p.SaveSiteTotals()
            totalR += p.GetRenderTotal()
            totalD += p.GetDisplayTotal()

            # if no sites are logging data, dont record totals either
            if p.sageData.isLoggingEnabled():
                recordTotals = True
                
        
        # now save the total of all sites
        if recordTotals and self._totalsFile:
            temp = "%12d    %12.4f    %12.4f       %d\n" % (getTimeStamp(),
                                                            totalD,
                                                            totalR,
                                                            self.GetPageCount())
            self._totalsFile.write(temp)
            self._totalsFile.flush()


        ### test whether an application window was dropped on a notebook tab
        ### if it was, send a message to SAGE about the request
        ### used to request streaming of an application to multiple displays
    def DropTest(self, windowId, pt):
        tab_tuple = self.HitTest((pt[0], pt[1]*(-1)))  # *(-1) since tabs are above the canvas (negative y coordinate with respect to it)
        newTab = tab_tuple[0]
        currentTab = self.GetSelection()
        
        if newTab == wx.NOT_FOUND or newTab == currentTab:
            return False
        else:
            currentPage = self.GetPage(currentTab)
            newPageMachine = self.GetPage(newTab).GetMachine()
            fsmIP = newPageMachine.GetSystemIP()
            fsmPort = newPageMachine.GetSystemPort() 
            currentPage.sageGate.streamApp(windowId, fsmIP, fsmPort)  #send a message
            self.SetSelection( newTab )  #open the new tab
            return True



    def GetFrame(self):
        return self.GetParent()


        ### this takes care of setting the proper menu items in the frame when a page changes
        ### it basically pulls the state of all the menu check items from the newly selected
        ### page and copies that hash into the hash of menu check items of the frame
    def OnPageChange(self, event):
        currentPageIndex = event.GetSelection() #must use event.GetSelection() instead of self.GetSelection() because WXMSW and WXMAC report the old selection otherwise 
        if not currentPageIndex == -1:  #windows reports -1 for some reason during creation
            newPage = self.GetPage(currentPageIndex)
            self.GetFrame().UpdateMenuItems(newPage.GetCurrentMenuItems())  #refresh the menu now so that it reflects the menu Items from the different page
        event.Skip()


    def SetDefaultMenuCheckItems(self, hashItems):
        self.defaultMenuCheckItems = hashItems.copy()


    def GetDefaultMenuCheckItems(self):
        return self.defaultMenuCheckItems.copy()


    def CloseCurrentPage(self, closeUI=True):
        if self.GetPageCount() < 2 and closeUI:
            self.GetFrame().Close(True)
        elif self.GetPageCount() > 1:
            currentPageIndex = self.GetSelection()
            if not currentPageIndex == -1:  #windows reports -1 for some reason during creation
                page = self.GetPage(currentPageIndex)
                page.OnClose()
                self.DeletePage(currentPageIndex)

                #refresh the menu now so that it reflects the menu Items from the different page
                newPageIndex = self.GetSelection()
                newPage = self.GetPage(newPageIndex)
                self.GetFrame().UpdateMenuItems(newPage.GetCurrentMenuItems())
        else:   # this is the case if the user reconnected to SAGE when there was only one tab open previously
            currentPageIndex = self.GetSelection()
            if not currentPageIndex == -1:  #windows reports -1 for some reason during creation
                page = self.GetPage(currentPageIndex)
                page.OnClose()
                self.DeletePage(currentPageIndex)

    
    def OnClose(self, event):
        for pageIndex in range(0, self.GetPageCount()):
            self.GetPage(pageIndex).OnClose(event)

        if self._totalsFile: self._totalsFile.close()
        self.perfTimer.Stop()
        

    def OnKeyEvent(self, event):
        if event.GetKeyCode() == wx.WXK_F9:
            currentPage = self.GetPage(self.GetSelection())
            currentPage.GetDisplayCanvas().TileShapes()

 
    def OnMenuEvent(self, event):
        eventId = event.GetId()  
        menuItem = self.GetFrame().GetMenuBar().FindItemById(eventId)
        currentPage = self.GetPage(self.GetSelection())
        
        # this ONLY changes the specific page's state of the selected menu item
        if menuItem.IsCheckable():
            if menuItem.IsChecked():
                currentPage.CheckMenuItem(eventId)
            else:
                currentPage.UncheckMenuItem(eventId)
                
        # call the correct menu event handler
        if eventId == SAVE_STATE:
            currentPage.OnSaveState()
        elif eventId == LOAD_STATE:
            currentPage.OnLoadState()
        elif eventId == ASPECT_RATIO:
            currentPage.OnMaintainAspectRatio(menuItem)
        elif eventId == SAGE_COLOR:
            currentPage.OnSAGEColorChange()
        elif eventId == PERF_DATA:
            currentPage.OnReceivePerfData(menuItem)
        elif eventId == LOG_PERF_DATA:
            currentPage.OnLogPerfData(menuItem)
        elif eventId == RENDER_BW:
            currentPage.OnShowRenderBW(menuItem)
        elif eventId == DISPLAY_BW:
            currentPage.OnShowDisplayBW(menuItem)
        elif eventId == RECORD_SESSION:
            currentPage.OnRecordSession(menuItem)
        elif eventId == READ_SESSION:
            currentPage.OnReadSession()
        elif eventId == SAGE_SHUTDOWN:
            currentPage.OnSAGEShutdown()
        else:
            pass  #nothing... we dont know how to handle anything else





############################################################################
#
#  CLASS: DisplayPage
#  
#  DESCRIPTION: DisplayPage is a panel that holds all the controls associated
#               with one SAGE connection (DisplayCanvas and AppInfoPanel). There
#               is one for every connection and they are all placed in a notebook
#               (DisplayNotebook). 
#
#  DATE:        June, 2005
#
############################################################################

class DisplayPage(wx.Panel):
    def __init__(self, parent, sageGate, usersClient, machineId, title):
        wx.Panel.__init__(self, parent, -1, style=wx.NO_BORDER)

        self.parent = parent
        # create main data storage and conectivity objects
        self.machineId = machineId
        self.usersClient = usersClient
        self.usersData = getUsersData()
        self.sageGate = sageGate
        self.sageData = SageData(sageGate, autosave, title.split('@')[1])
        self.RegisterCallbacks()
        self.menuCheckItems = self.GetParent().GetDefaultMenuCheckItems()
        self.GetParent().GetFrame().UpdateMenuItems(self.GetCurrentMenuItems()) #update the menu items

        self.Bind(wx.EVT_SIZE, self.OnSize)

        # get the machine name
        self.siteName = machineId
        m = self.usersData.GetMachine(machineId)
        if m:
            self.siteName = m.GetName()
        
        # (AKS 2005-01-27) Performance graph object
        self.gmGraphManager = Graph.GraphManager( self, self.sageData )
        # (AKS 2005-05-07) Create constants to identify totals graphs
        self.I_RENDER_BW_GRAPH = 1
        self.I_DISPLAY_BW_GRAPH = 2
        
        self.displayCanvas = DisplayCanvas(Log(), self, self.sageGate, self.sageData, self.gmGraphManager, title)
        self.appPanel = AppInfoPanel(self, self.sageGate, self, -1, (0,0), (self.displayCanvas.GetCanvasWidth(), 200), wx.NO_BORDER | wx.NO_FULL_REPAINT_ON_RESIZE)
        self.sageGate.getAppList()  # this will get the list and fill the appPanel with it
        #self.Show()  < -- this is done in DisplayCanvas.OnStatusMessage()  (canvases.py)


    def GetFrame(self):
        return self.GetParent().GetParent()

    def GetNotebook(self):
        return self.GetParent()

    def GetNumPages(self):
        return self.GetNotebook().GetPageCount()

    def GetCurrentMenuItems(self):
        return self.menuCheckItems.copy()  #return the current state of menu items specific to this display

    def CheckMenuItem(self, menuItemId):
        self.menuCheckItems[menuItemId] = True

    def UncheckMenuItem(self, menuItemId):
        self.menuCheckItems[menuItemId] = False

    def GetMachineId(self):
        return self.machineId

    def GetMachine(self):
        return self.usersData.GetMachine(self.machineId)

    
    #----------------------------------------------------------------------    

    def RegisterCallbacks(self):
        # (AKS 2005-01-13)
        # Register callbacks from SageData with SageGate
        # Whenever SageGate receives an update message from SAGE,
        # the client-side "app database" stored in SageData
        # will be updated and then issue its own callback function
        # to tell the registering app to retrieve and process
        # the updated data

        self.sageGate.registerCallbackFunction( 40000, self.sageData.setSageStatus )
        self.sageGate.registerCallbackFunction( 40001, self.sageData.setSageAppInfo )
        self.sageGate.registerCallbackFunction( 40002, self.sageData.setSagePerfInfo )
        self.sageGate.registerCallbackFunction( 40003, self.sageData.sageAppShutDown )
        self.sageGate.registerCallbackFunction( 40004, self.sageData.setDisplayInfo )
        self.sageGate.registerCallbackFunction( 40005, self.sageData.setSageZValue )
        self.sageGate.registerCallbackFunction( 40006, self.sageData.setSageAppExecConfig )
        self.sageGate.registerCallbackFunction( 40007, self.sageData.setDisplayConnections )
        
        return self.sageData


    #----------------------------------------------------------------------
    

    def GetDisplayCanvas(self):
        return self.displayCanvas

    def GetAppInfoPanel(self):
        return self.appPanel

    def GetAppPanel(self):
        return self.appPanel.GetAppPanel()

    def GetInfoPanel(self):
        return self.appPanel.GetInfoPanel()

    def HideAppInfoPanel(self): 
        self.oldHeight = self.GetClientSize().height
        newHeight = self.oldHeight - self.GetAppInfoPanel().GetSize().height + 60
        self.GetFrame().SetClientSize( (self.GetFrame().GetClientSize().width, newHeight) )
        self.GetAppInfoPanel().OnSize()
            
    def ShowAppInfoPanel(self):
        self.GetFrame().SetClientSize( (self.GetFrame().GetClientSize().width, self.oldHeight) )
        self.GetAppInfoPanel().OnSize()
    

    def SaveSiteTotals(self):
        self.sageData.saveSiteTotals(self.siteName)

    def GetRenderTotal(self):
        """ returns total display bandwidth in gbps """
        return self.sageData.getRenderBWTotal()

    def GetDisplayTotal(self):
        """ returns total rendering bandwidth in gbps """
        return self.sageData.getDisplayBWTotal()
    

    #----------------------------------------------------------------------
    # EVENT HANDLERS
    #----------------------------------------------------------------------

    def OnDisplayInfo(self):
        pass
        
        
    def OnSize(self, event=None):
        self.SetSize(self.GetParent().GetClientSize())#event.GetSize())
        if "__WXMSW__" in wx.PlatformInfo:   #to bypass MS Windows' deferred resizing
            def doResize():
                self.GetDisplayCanvas().OnSize(event)
                self.GetAppInfoPanel().OnSize(event)
            wx.CallAfter(doResize)
        else:
            self.GetDisplayCanvas().OnSize(event)
            self.GetAppInfoPanel().OnSize(event)
        

    def OnClose(self, evt=None):
        self.GetDisplayCanvas().Close()
        self.gmGraphManager.shutdown()  # close all the graphs that might be open
        
        # close the "Totals" graphs (if open)
        #if self.menuCheckItems[RENDER_BW]:
        #    self.removeGraph(self.I_RENDER_BW_GRAPH)
        #if self.menuCheckItems[DISPLAY_BW]:
        #    self.removeGraph(self.I_DISPLAY_BW_GRAPH)

        # stop all the running features
        self.usersClient.Unregister(self.machineId)  #tell the server that we are not registered in this room anymore
        self.sageData.stopLogging()
        #self.sageGate.StopRecording()
        self.sageGate.disconnectFromSage()  #break the connection with SAGE


       # Menu -> Options -> Change SAGE Background Color
    def OnSAGEColorChange(self):
        (r,g,b) = self.sageData.getSAGEColor()
        colorData = wx.ColourData()
        colorData.SetColour(wx.Colour(r,g,b))
        dlg = wx.ColourDialog(None, colorData)
        dlg.GetColourData().SetChooseFull(True)
        if dlg.ShowModal() == wx.ID_OK:
            data = dlg.GetColourData()
            self.sageData.setSAGEColor(data.GetColour().Get())
            self.sageGate.setBgColor(self.sageData.getSAGEColor())
            self.GetDisplayCanvas().ChangeBackgroundColor(self.sageData.getSAGEColor())
        dlg.Destroy()


        # Menu -> File -> Shutdown SAGE
    def OnSAGEShutdown(self):
        dlg = wx.MessageDialog(self, "You are about to shutdown SAGE. All the currently connected "+
                                     "users will be disconnected and all applications closed. \nAre "+
                                     "you sure you want to continue?", "SAGE Shutdown", wx.YES_NO | wx.ICON_EXCLAMATION)
        if dlg.ShowModal() == wx.ID_YES:
            self.sageGate.shutdownSAGE()
            

    # Menu -> File -> Save State
    def OnSaveState(self):
        def OnStateSelected(evt):
            if nameText.GetValue() in stateHash:
                descriptionText.SetValue( stateHash[nameText.GetValue()] )
                
        def OnDeleteState(evt):
            stateName = nameText.GetValue()
            if stateName in stateHash:
                if self.sageData.deleteState(stateName):
                    descriptionText.Clear()
                    nameText.Delete(nameText.FindString(stateName))
                    nameText.SetValue("")
                else:
                    Message("Error deleting the saved state. Do you have the required permissions?", "Error Deleting")
                                 
        
        stateHash = self.sageData.getStateList()
        
        dlg = wx.Dialog(self, -1, "Save Display Session")
        okBtn = wx.Button(dlg, wx.ID_OK, "Save")
        delBtn = wx.Button(dlg, wx.ID_OK, " Delete ", style=wx.BU_EXACTFIT)
        delBtn.Bind(wx.EVT_BUTTON, OnDeleteState)

        states = stateHash.keys()
        states.sort()
        nameLabel = wx.StaticText(dlg, -1, "Session name:")
        nameText = wx.ComboBox(dlg, -1, "", choices=states, style=wx.CB_DROPDOWN)
        nameText.Bind(wx.EVT_COMBOBOX, OnStateSelected)
        nameText.SetFocus()
        descriptionLabel = wx.StaticText(dlg, -1, "Description:")
        descriptionText = wx.TextCtrl(dlg, -1, "", style=wx.TE_MULTILINE)
        
        nameSizer = wx.BoxSizer(wx.HORIZONTAL)
        nameSizer.Add(nameLabel, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border=5)
        nameSizer.Add(nameText, 1, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        nameSizer.Add(delBtn, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT, border=5)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(nameSizer, 0, wx.EXPAND | wx.ALIGN_LEFT | wx.ALL, border=10)
        sizer.Add(descriptionLabel, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT | wx.TOP, border=10)
        sizer.Add(descriptionText, 1, wx.EXPAND | wx.ALIGN_LEFT | wx.RIGHT | wx.LEFT, border=10)
        sizer.AddSpacer((15,15))
        sizer.Add(okBtn, 0, wx.ALIGN_CENTER)
        sizer.AddSpacer((5,5))

        dlg.SetSizer(sizer)
        dlg.SetSize((350, 300))

        stateName = ""
        description = ""
        while dlg.ShowModal() == wx.ID_OK:
            stateName = nameText.GetValue()
            description = descriptionText.GetValue()
            if stateName == "":
                Message("Please name the session you are saving", "Session name required")
                continue
            elif stateName in stateHash:
                if wx.MessageBox("Session with that name already exists. Overwrite?",
                                 "Overwrite?", style=wx.YES_NO) == wx.YES:
                    dlg.Destroy()
                else:
                    continue
            self.sageData.saveState(stateName, description)
            break
            
        return
        


    # Menu -> File -> Load State
    def OnLoadState(self):
        def OnStateSelected(evt):
            descriptionText.SetLabel( stateHash[stateList.GetStringSelection()] )
            
        def OnDeleteState(evt):
            stateName = stateList.GetStringSelection()
            if stateName in stateHash:
                if self.sageData.deleteState(stateName):
                    stateList.Delete(stateList.FindString(stateName))
                    if not stateList.IsEmpty():
                        stateList.Select(0)
                        descriptionText.SetLabel(stateHash[stateList.GetString(0)])
                    else:
                        descriptionText.SetLabel("")
                else:
                    Message("Error deleting the saved state. Do you have the required file permissions?", "Error Deleting")

                                 
        stateHash = self.sageData.getStateList()
        if len(stateHash) == 0:
            Message("No saved sessions exist", "")
            return
    
        dlg = wx.Dialog(self, -1, "Load Saved Session")
        okBtn = wx.Button(dlg, wx.ID_OK, "Load")
        okBtn.Disable()
        delBtn = wx.Button(dlg, wx.ID_OK, " Delete ", style=wx.BU_EXACTFIT)
        delBtn.Bind(wx.EVT_BUTTON, OnDeleteState)

        states = stateHash.keys()
        states.sort()
        states = stateHash.keys()
        states.sort()
        stateLabel = wx.StaticText(dlg, -1, "Saved Sessions:")
        stateList = wx.Choice(dlg, -1, choices=states)
        stateList.Bind(wx.EVT_CHOICE, OnStateSelected)
        descriptionBox = wx.StaticBox(dlg, -1, "Description:")
        descriptionText = wx.StaticText(dlg, -1, "", style=wx.ST_NO_AUTORESIZE)

        dbSizer = wx.StaticBoxSizer(descriptionBox, wx.VERTICAL)
        dbSizer.Add(descriptionText, 1, wx.EXPAND | wx.ALL, border=5)
        
        if len(stateHash) > 0:
            stateList.SetSelection(0)
            descriptionText.SetLabel( stateHash[stateList.GetString(0)] )
            okBtn.Enable()
        
        stateSizer = wx.BoxSizer(wx.HORIZONTAL)
        stateSizer.Add(stateLabel, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border=5)
        stateSizer.Add(stateList, 1, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
        stateSizer.Add(delBtn, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT, border=5)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(stateSizer, 0, wx.EXPAND | wx.ALIGN_LEFT | wx.ALL, border=10)
        sizer.Add(dbSizer, 1, wx.EXPAND | wx.ALIGN_LEFT | wx.RIGHT | wx.LEFT, border=10)
        sizer.AddSpacer((15,15))
        sizer.Add(okBtn, 0, wx.ALIGN_CENTER)
        sizer.AddSpacer((5,5))

        dlg.SetSizer(sizer)
        dlg.SetSize((350, 300))

        if dlg.ShowModal() == wx.ID_OK:
            stateName = stateList.GetStringSelection()
            dlg.Destroy()
            if stateName == "":
                Message("No saved sessions exist", "")
                return
            self.sageData.loadState(stateName)

        return
        
        

    # Menu -> Options -> RecordSession
    def OnRecordSession(self, menuItem):
        if menuItem.IsChecked():
            self.sageGate.StartRecording(menuItem)
        else:
            self.sageGate.StopRecording()


    # Menu -> Options -> ReadSession
    def OnReadSession(self):
        self.sageGate.PlaySession(self)
        
        
    # Menu -> Options -> MaintainAspectRatio
    def OnMaintainAspectRatio(self, menuItem):
        diagram = self.GetDisplayCanvas().GetDiagram()
        shapeList = diagram.GetShapeList()

        # store the flag for future apps
        self.GetDisplayCanvas().SetMaintainAspectRatio( menuItem.IsChecked() )

        # change all the current apps' flags
        for s in shapeList:
            if ( s.__class__.__name__ == MyRectangleShape.__name__ ):
                s.SetMaintainAspectRatio( self.displayCanvas.GetMaintainAspectRatio() )


    # Menu -> Performance -> Logging
    def OnLogPerfData(self, menuItem):
        self.sageData.setLoggingFlag( menuItem.IsChecked() )


    # Menu -> Performance -> Receive Performance Data
    def OnReceivePerfData(self, menuItem):
        self.GetDisplayCanvas().ReceivePerformanceData( menuItem.IsChecked() )


    # (AKS 2005-05-07)
    def OnShowRenderBW( self, menuItem ):
        if ( menuItem.IsChecked() == True ):
            self.__sgRenderBWGraph = Graph.SingleGraph(
                self, self.sageData.getRenderBandwidthGraph(), "Totals: Render Bandwidth",
                "Totals: Render Bandwidth", "Bandwidth (Mbps)", self.I_RENDER_BW_GRAPH )
            self.__sgRenderBWGraph.registerShutdownCallback( self.removeGraph )
            self.gmGraphManager.AddToUpdateList(self.__sgRenderBWGraph)
        else:
            self.gmGraphManager.RemoveFromUpdateList(self.__sgRenderBWGraph)
            self.__sgRenderBWGraph.manualShutdown()

        # end if


    # (AKS 2005-05-07)
    def OnShowDisplayBW( self, menuItem ):
        if ( menuItem.IsChecked() == True ):
            self.__sgDisplayBWGraph = Graph.SingleGraph(
                self, self.sageData.getDisplayBandwidthGraph(), "Totals: Display Bandwidth",
                "Totals: Display Bandwidth", "Bandwidth (Mbps)", self.I_DISPLAY_BW_GRAPH )
            self.__sgDisplayBWGraph.registerShutdownCallback( self.removeGraph )
            self.gmGraphManager.AddToUpdateList(self.__sgDisplayBWGraph)
        else:
            self.gmGraphManager.RemoveFromUpdateList(self.__sgDisplayBWGraph)
            self.__sgDisplayBWGraph.manualShutdown()

        # end if
        

    # (AKS 2005-05-07)
    def removeGraph( self, iGraphID ):
        if ( iGraphID == self.I_RENDER_BW_GRAPH ):
            self.gmGraphManager.RemoveFromUpdateList(self.__sgRenderBWGraph)
            self.__sgRenderBWGraph.manualShutdown()
            self.UncheckMenuItem(RENDER_BW)
            self.GetFrame().UpdateMenuItems(self.GetCurrentMenuItems())  #we must manually update the frame

        elif ( iGraphID == self.I_DISPLAY_BW_GRAPH ):
            self.gmGraphManager.RemoveFromUpdateList(self.__sgDisplayBWGraph)
            self.__sgDisplayBWGraph.manualShutdown()
            self.UncheckMenuItem(DISPLAY_BW)
            self.GetFrame().UpdateMenuItems(self.GetCurrentMenuItems())  #we must manually update the frame

        # end if; else, do nothing







############################################################################
#
#  CLASS: SAGEuiFrame
#  
#  DESCRIPTION: It describes the main frame that holds all the other controls.
#               It creates the two main canvases for the UI:
#                       - the tiled display representation (upper canvas)
#                       - the area for displaying app info (lower canvas)
#
#  DATE:        February 28, 2005
#
############################################################################

class SAGEuiFrame(wx.Frame):

    def __init__(self, sageGate, usersClient, firstMachineId, loadState, title=""):
        self.loadState = loadState  # for automatically loading a saved display state upon startup
        
        if prefs.visual.GetFramePos() != None:
            position = prefs.visual.GetFramePos()
        else:
            position = (50,0)
            if "__WXMAC__" in wx.PlatformInfo:
                position = (50,50)
                
        wx.Frame.__init__(self, None, -1, "SAGE UI",
                          pos=position,
                          size=(500,300),
                          style=wx.DEFAULT_FRAME_STYLE )#| wx.FULL_REPAINT_ON_RESIZE)
        
        # this is the only common thing between the pages
        self.usersClient = usersClient
        self.usersData = getUsersData()

        # so that we can kill SAGE from the UI
        self.sageGate = sageGate
        
        # make the menu
        self.SetMenuBar( self.CreateMenu() )

        # bind any events we need
        self.Bind(wx.EVT_CLOSE, self.OnCloseFrame)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_MOVE, self.OnMove)
        self.Bind(wx.EVT_ACTIVATE, self.OnActivated)
                  
        # This creates some pens and brushes that the OGL library uses.
        # It should be called after the app object has been created, but
        # before OGL is used.
        ogl.OGLInitialize()    
        
        self.notebook = DisplayNotebook(self)
        tempHash = {}    #extract the values of all the checkable menu items and send them to the notebook as default values
        for key, mi in self.menuCheckItems.iteritems():
            tempHash[key] = mi.IsChecked()
        self.notebook.SetDefaultMenuCheckItems(tempHash)
        self.notebook.AddPage(DisplayPage(self.notebook, sageGate, usersClient, firstMachineId, title), title.split('@')[1], True)
        self.usersPanel = UsersPanel(self, usersClient)
        #self.Show()  < -- this is done in DisplayCanvas.OnSAGEDisplaysConnection()  (canvases.py)


    #----------------------------------------------------------------------


    def CreateMenu(self):      
        self.menuBar = wx.MenuBar()

        # File
        menuFile = wx.Menu()

        miFileNew = menuFile.Append(-1, "&New Connection\tCtrl-N", "Connect to another display")
        self.Bind(wx.EVT_MENU, self.OnNewConnection, miFileNew)

        miFileClose = menuFile.Append(-1, "Close Connection\tCtrl-W", "Close connection with this display")
        self.Bind(wx.EVT_MENU, self.OnCloseConnection, miFileClose)

        menuFile.AppendSeparator()
        
        miFileSave = menuFile.Append(SAVE_STATE, "&Save Session\tCtrl-S", "Save current session")
        self.Bind(wx.EVT_MENU, self.OnMenuItem, miFileSave)

        miFileLoad = menuFile.Append(LOAD_STATE, "&Load Session\tCtrl-O", "Load previously saved session")
        self.Bind(wx.EVT_MENU, self.OnMenuItem, miFileLoad)

        menuFile.AppendSeparator()

        miFileSAGEShutdown = menuFile.Append(SAGE_SHUTDOWN, "Shutdown SAGE", "Shutdown SAGE")
        self.Bind(wx.EVT_MENU, self.OnMenuItem, miFileSAGEShutdown)

        menuFile.AppendSeparator()

        miFileExit = menuFile.Append(-1, "E&xit\tAlt-X", "Exit demo")
        self.Bind(wx.EVT_MENU, self.OnExitButton, miFileExit)
        
        self.menuBar.Append(menuFile, "&File")


        # Options
        menuOptions = wx.Menu()

        self.miOptionsLibrary = menuOptions.Append(-1, "&File Library\tF2" , "Show File Library")
        self.Bind(wx.EVT_MENU, self.OnShowLibrary, self.miOptionsLibrary)
        
        self.miOptionsAspectRatio = menuOptions.Append(ASPECT_RATIO, "Preserve Aspect Ratio" , "Maintain Aspect Ratio?", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miOptionsAspectRatio)
        self.miOptionsAspectRatio.Check(prefs.visual.GetKeepAspectRatio())

        self.miOptionsSAGEColor = menuOptions.Append(SAGE_COLOR, "Change SAGE Background Color" , "Change SAGE Background Color?")
        self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miOptionsSAGEColor)

        self.miOptionsShowChat = menuOptions.Append(-1, "Show Chat Window", "", wx.ITEM_CHECK)
        if self.usersClient.connected:
            if prefs.visual.IsChatShown() != None:
                self.miOptionsShowChat.Check(prefs.visual.IsChatShown())
        self.Bind(wx.EVT_MENU, self.OnChatShow, self.miOptionsShowChat)

        self.miOptionsOutput = menuOptions.Append(-1, "Show Console Output", "", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnShowConsoleOutput, self.miOptionsOutput)
        self.miOptionsOutput.Check(False)

        self.miOptionsLauncherAdmin = menuOptions.Append(-1, "AppLauncher Admin" , "AppLauncher Admin Tool")
        self.Bind(wx.EVT_MENU, self.OnLauncherAdmin, self.miOptionsLauncherAdmin)

        self.menuBar.Append(menuOptions, "&Options")


        # Performance
        menuPerformance = wx.Menu()

        self.miPerformancePerformance = menuPerformance.Append(PERF_DATA, "Receive Performance Data", "", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miPerformancePerformance)
        self.miPerformancePerformance.Check(prefs.visual.GetReceivePerformanceData())

        self.miPerformanceLogging = menuPerformance.Append(LOG_PERF_DATA, "Log Performance Data" , "Permit performance data logging to a file.", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miPerformanceLogging)
        self.miPerformanceLogging.Check(True)

        self.miPerformanceRenderBWGraph = menuPerformance.Append(RENDER_BW, "Show Total Rendering Bandwidth" , "Show the graph for Total Rendering Bandwidth.", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miPerformanceRenderBWGraph)
        self.miPerformanceRenderBWGraph.Check(False)

        self.miPerformanceDisplayBWGraph = menuPerformance.Append(DISPLAY_BW, "Show Total Display Bandwidth" , "Show the graph for Total Display Bandwidth.", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miPerformanceDisplayBWGraph)
        self.miPerformanceDisplayBWGraph.Check(False)

        self.menuBar.Append(menuPerformance, "&Performance")
        

        # Session
        ## menuSession = wx.Menu()
##         self.miSessionRecord = menuSession.Append(RECORD_SESSION, "Record Session" , "", wx.ITEM_CHECK)
##         self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miSessionRecord)
##         self.miSessionRead = menuSession.Append(READ_SESSION, "Read Session" , "")
##         self.Bind(wx.EVT_MENU, self.OnMenuItem, self.miSessionRead)
##         self.menuBar.Append(menuSession, "&Session")


        # Help
        menuHelp = wx.Menu()
        self.miAbout = menuHelp.Append(-1, "About SAGE UI\tF1", "")
        self.Bind(wx.EVT_MENU, self.OnAbout, self.miAbout)
        self.menuBar.Append(menuHelp, "&Help")


        # store all the menu check items that are display dependent
        self.menuCheckItems = {}
        self.menuCheckItems[ASPECT_RATIO] = self.miOptionsAspectRatio#.IsChecked()
        self.menuCheckItems[PERF_DATA] = self.miPerformancePerformance#.IsChecked()
        self.menuCheckItems[LOG_PERF_DATA] = self.miPerformanceLogging#.IsChecked()
        self.menuCheckItems[RENDER_BW] = self.miPerformanceRenderBWGraph#.IsChecked()
        self.menuCheckItems[DISPLAY_BW] = self.miPerformanceDisplayBWGraph#.IsChecked()
        #self.menuCheckItems[RECORD_SESSION] = self.miSessionRecord#.IsChecked()


        # bind keystrokes to the frame
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyEvent)

        return self.menuBar


    #----------------------------------------------------------------------

    def UpdateMenuItems(self, newMenuItems):
        # update the menu with the specific menu items from the current page
        for k,mi in self.menuCheckItems.iteritems():
            mi.Check(newMenuItems[k])
        self.menuBar.Refresh()
            
    
    def GetUsersPanel(self):
        return self.usersPanel



##     def Reconnect(self):
##         dlg = wx.MessageDialog(None, "Connection to SAGE has closed.\nYou will have to reconnect when SAGE becomes available again.", "SAGE Connection Closed", style=wx.OK)
##         #btn = wx.Button(dlg, 999, "Reconnect")
##         if dlg.ShowModal() == wx.ID_OK:
##             self.notebook.CloseCurrentPage(closeUI=False)
##             sageGate = SageGate()
##             cd = ConnectionDialog(sageGate, self.usersClient, firstConnection=False)
##             title = self.usersData.GetMyUsername() + " @ " + cd.hostname
##             self.notebook.AddPage(DisplayPage(self.notebook, sageGate, self.usersData, self.usersClient, cd.host, title), title, True)
                         
##         dlg.Destroy()

    #----------------------------------------------------------------------
    # EVENT HANDLERS
    #----------------------------------------------------------------------


    def OnActivated(self, evt):
        """ happens only when the frame is shown because we unbind the event below """
        self.Disconnect(-1,-1,wx.wxEVT_ACTIVATE)
        
        # load the saved state of the display if requested
        if self.loadState:
            sageData = self.notebook.GetPage(self.notebook.GetSelection()).sageData
            t = Timer(4, sageData.loadState, [self.loadState])
            t.start()
        

    def OnSize(self, event=None):
        if "__WXMSW__" in wx.PlatformInfo:   #to bypass windows' deferred resizing
            def doResize():
                #self.notebook.OnSize(event)
                if self.usersClient.connected:
                    self.usersPanel.OnFrameMove()
            wx.CallAfter(doResize)
        else:
            #self.notebook.OnSize(event)
            if self.usersClient.connected:
                self.usersPanel.OnFrameMove()
        event.Skip()  #resize the notebook
        

    def OnMove(self, event):
        if self.usersClient.connected:
            self.usersPanel.OnFrameMove()


    def OnKeyEvent(self, event):
        #self.notebook.OnKeyEvent(event)
        event.Skip()

        
    def OnCloseFrame(self, evt):
        # save the visual preferences (frame size, position, chat panel)
        prefs.visual.SetAll(self.GetClientSize(), self.GetPosition(), self.usersPanel.IsShown())
        
        # close all connections to SAGEs
        self.notebook.OnClose(evt)
        
        #if self.usersClient.connected:
        self.usersPanel.Disconnect()

        
        # (AKS 2005-05-06) Delete all temp files of size 0 created when ignoring logging function
        # Instead of allowing for the possibility that new apps start and their respective log
        # files are not made causing an error, allow the new app's file to be created but do
        # not write to it.  Later, delete the 0-size file.
        try:   #in case the file and directory permissions are not right
            listFilenames = os.listdir( DATA_DIR )

            # remove files that were never written or minimally written (i.e. <= 1K)
            for stFilename in listFilenames:
                stFilename = opj(DATA_DIR, stFilename)
                stCompleteFilename = os.path.normcase( stFilename )
                structStats = os.stat( stCompleteFilename )

                if ( structStats.st_size <= 1024 ):
                    os.remove( stCompleteFilename )
                    print "Deleting", stCompleteFilename
                # end if
            # end loop
        except:
            pass
        
        self.DestroyChildren()
        self.Destroy()
        wx.GetApp().ExitMainLoop()


    #----------------------------------------------------------------------
    # MENU HANDLERS
    #----------------------------------------------------------------------


    ### this catches all the menu events that are display specific
    def OnMenuItem(self, event):
        self.notebook.OnMenuEvent(event)

    # Menu -> File -> Exit
    def OnExitButton(self, evt):
        self.Close(True)

    # Menu -> Help -> About
    def OnAbout(self, evt):
        AboutDialog(self)

    # Menu -> Chat -> Show
    def OnChatShow(self, evt):
        if self.miOptionsShowChat.IsChecked():
            self.usersPanel.Show()
        else:
            self.usersPanel.Hide()

    # Menu -> Options -> AppLauncher Admin
    def OnLauncherAdmin(self, evt):
        lAdmin.MainFrame(getSAGEServer())

    # Menu -> Options -> Show File Library
    def OnShowLibrary(self, evt):
        self.notebook.GetPage(self.notebook.GetSelection()).displayCanvas.ShowLibrary() 

    # Menu -> Options -> Show Console Output
    def OnShowConsoleOutput(self, evt):
        if self.miOptionsOutput.IsChecked():
            wx.GetApp().RestoreStdio()
        else:
            try:
                wx.GetApp().RedirectStdio( LOG_FILE )
            except IOError:
                message = "\nCould not redirect output to a log file. Probably because of file permissions."
                message = message+"\nInstead, output will be printed in the console"
                dlg = wx.MessageDialog(self, message, "Error redirecting output", style=wx.OK)
                dlg.ShowModal()
                dlg.Destroy()
                wx.GetApp().RestoreStdio()
                self.miOptionsOutput.Check(True)

    # Menu -> File -> New
    def OnNewConnection(self, event):
        sageGate = SageGate()

        # initialize the dialog and show it. If it returns False, the user pressed quit
        # otherwise the connection to SAGE was successful and the username was accepted
        # so create the tab, show it and continue with execution
        cd = ConnectionDialog(sageGate, self.usersClient, firstConnection=False)
        if cd.ShowDialog():        
            title = self.usersData.GetMyUsername() + " @ " + cd.GetMachine().GetName()
            self.notebook.AddPage(DisplayPage(self.notebook, sageGate, self.usersClient, cd.GetMachine().GetId(), title), cd.GetMachine().GetName(), True)
                         
        return True

    # Menu -> File -> Close
    def OnCloseConnection(self, event):
        self.notebook.CloseCurrentPage()


############################################################################
#
#  CLASS: AboutDialog
#  
#  DESCRIPTION: It just displays the about box.
#
#  DATE:        June 2005
#
############################################################################

class AboutDialog(wx.Dialog):
    def __init__(self, parent):
        titleText = "SAGE Graphical User Interface"
        contentText = "version " + str(VERSION) + "\n\n\n" + "For more information go to:"
        linkUrl = "www.evl.uic.edu/cavern/sage"
        link2Text = "Direct questions or comments to:"
        link2Url = "www.evl.uic.edu/cavern/forum"
                
        wx.Dialog.__init__(self, parent, -1, "About SAGE UI", style=wx.CLOSE_BOX)
        okBtn = wx.Button(self, wx.ID_OK, "OK")
        title = wx.StaticText(self, -1, titleText, style=wx.ALIGN_CENTER)
        title.SetFont(BoldFont(title))
        
        if use_hyperlink: 
            link = hyperlink.HyperLinkCtrl(self, wx.ID_ANY, linkUrl, URL="http://"+linkUrl)
            link.AutoBrowse(True)
            link2 = hyperlink.HyperLinkCtrl(self, wx.ID_ANY, link2Url, URL="http://"+link2Url)
            link2.AutoBrowse(True)
        else:
            link = wx.StaticText(self, -1, linkUrl, style=wx.ALIGN_CENTER)
            link2 = wx.StaticText(self, -1, link2Url, style=wx.ALIGN_CENTER)

        text = wx.StaticText(self, -1, contentText, style=wx.ALIGN_CENTER)
        text2 = wx.StaticText(self, -1, link2Text, style=wx.ALIGN_CENTER)
        

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.AddSpacer((20, 20))
        sizer.Add(title, 0, wx.ALIGN_CENTER | wx.RIGHT | wx.LEFT, border=15)
        sizer.Add(text, 0, wx.ALIGN_CENTER)
        sizer.Add(link, 0, wx.ALIGN_CENTER)
        sizer.AddSpacer((15,15))
        sizer.Add(text2, 0, wx.ALIGN_CENTER)
        sizer.Add(link2, 0, wx.ALIGN_CENTER)
        sizer.AddSpacer((15,15))
        sizer.Add(okBtn, 0, wx.ALIGN_CENTER)
        sizer.AddSpacer((5,5))
        self.SetSizer(sizer)
        self.Fit()
        self.ShowModal()
        self.Destroy()
                




############################################################################
#
#  CLASS: SAGEui
#  
#  DESCRIPTION: This is a starting point for wx. It extends wx.App and
#               creates the SAGEuiFrame to place the canvases in. It also displays
#               the initial dialogs for the User profile and SAGE connection.
#
#  DATE:        October 2004
#
############################################################################

class SAGEui(wx.App):
    def __init__(self, usersServerIP, usersServerPort, verbose, autologinMachine, loadState):
        self.usersServerIP = usersServerIP
        self.usersServerPort = usersServerPort
        self.autologinMachine = autologinMachine
        self.loadState = loadState
        
        if verbose:
            wx.App.__init__(self, redirect=False)
        else:
            try:
                wx.App.__init__(self, redirect=True, filename=LOG_FILE)
            except IOError:
                print "\nError: Could not redirect output to a log file. Possibly because of file permissions."
                print "Instead, output will be printed in the console"
                wx.App.__init__(self, redirect=False)

    def Skip( self, evt ):
        pass

    def OnInit(self):
        wx.Log_SetActiveTarget(wx.LogStderr())
        self.SetAssertMode(assertMode)
        socket.setdefaulttimeout(2)
    
        # initialize network communication with:
        # sageGate...   the connection to SAGE
        # usersClient...the connection to UsersServer
        # usersData...  the datastructure used in conjunction with usersClient
        usersData = UsersDatastructure()
        setUsersData(usersData) #store it in a global scope
        sageGate = SageGate()
        usersClient = UsersClient()

        # initialize the dialog and show it. If it returns False, the user pressed quit
        # otherwise the connection to SAGE was successful and the username was accepted
        # so show the frame and continue with execution
        cd = ConnectionDialog(sageGate, usersClient, self.usersServerIP, usersServerPort = self.usersServerPort, autologinMachine=self.autologinMachine)
        if cd.ShowDialog():        
            title = usersData.GetMyUsername() + " @ " + cd.GetMachine().GetName()
            self.frame = SAGEuiFrame(sageGate, usersClient, cd.GetMachine().GetId(), self.loadState, title)       
            self.SetTopWindow(self.frame)
        else:
            usersClient.Disconnect()
            sys.exit(0)
                         
        return True




############################################################################
#
# Main entry point for the application
#
############################################################################

### sets up the parser for the command line options
def get_commandline_options():
    parser = optparse.OptionParser()

    h = "if set, prints output to console, otherwise to ~/.sageConfig/sageui/output_log.txt"
    parser.add_option("-v", "--verbose", action="store_true", help=h, dest="verbose", default=False)

    h = "which sage server / connection manager to use (default is sage.sl.startap.net)"
    parser.add_option("-s", "--server", dest="server", help=h, default="sage.sl.startap.net")

    h = "change the port number of the sage server (default is 15558)"
    parser.add_option("-p", "--port", help=h, type="int", dest="port", default=15558)

    h = "override which application launcher to use (by default it looks for one on the same machine as sage master. Specify as machine:port)"
    parser.add_option("-l", "--launcher", dest="launcher", help=h, default="")

    h = "try autologin to this sage name (what fsManager reports to connection manager from fsManager.conf )"
    parser.add_option("-a", "--autologin", dest="autologin", help=h, default=None)

    h = "upon startup load this saved state (write saved state name from saved-states directory)"
    parser.add_option("-o", "--load_state", dest="load_state", help=h, default=None)

    h = "perform autosave?"
    parser.add_option("-t", "--autosave", action="store_true", help=h, dest="autosave", default=False)

    return parser.parse_args()



def main(argv):
    os.chdir(sys.path[0])  # change to the folder where script is running
    global autosave
    verbose = False
    usersServerIP = "74.114.99.36"
    usersServerPort = 15558
    
    # parse the command line params
    (options, args) = get_commandline_options()
    verbose = options.verbose
    usersServerPort = options.port
    usersServerIP = options.server
    appLauncher = options.launcher
    autologinMachine = options.autologin
    loadState = options.load_state
    autosave = options.autosave
    
    # set the overridden app launcher
    if appLauncher != "":
        setAppLauncher(appLauncher)
        
    # set the global variable for the SAGE SERVER
    setSAGEServer(usersServerIP)
    
    # print some information about the system currently running
    print "\nCurrently running:\n--------------------------"
    print "SAGE UI version: ", VERSION
    print "Python version:  ", string.split(sys.version, "(", 1)[0]
    print "wxPython version: "+str(wx.VERSION[0])+"."+str(wx.VERSION[1])+"."+str(wx.VERSION[2])+"."+str(wx.VERSION[3])+"\n"  

    # read all the preferences
    import preferences as prefs
    prefs.readAllPreferences()

    app = SAGEui(usersServerIP, usersServerPort, verbose, autologinMachine, loadState)
    app.MainLoop()
    
    


if __name__ == '__main__':
    import sys, os
    main(['', os.path.basename(sys.argv[0])] + sys.argv[1:])


    
    


