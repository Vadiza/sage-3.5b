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


import os.path
import os 
import time
from math import ceil
import string
import wx
from Mywx import MyBitmapButton
from threading import Timer
from globals import *

# some global vars
hashRecordingWords = {}  #RJ 2005-04-07
hashRecordingWords[1001] = "exec"
hashRecordingWords[1002] = "kill"
hashRecordingWords[1003] = "move"
hashRecordingWords[1004] = "resize"
hashRecordingWords[1007] = "bg"
hashRecordingWords[1008] = "depth"
hashRecordingWords[1100] = "shutdown"



############################################################################
#
#  CLASS: SessionRecorder
#
#  DESCRIPTION: A class for recording user actions to a file in a directory.
#               To use just create an instance of this class and call
#               RecordAction(code, data) to write a line to a file. "Code" is
#               the message code sent to SAGE and it gets mapped to a text string
#               according to a hash defined here. "Data" is whatever you want to
#               record in a file and it's code dependent. The last (optional)
#               parameter to RecordAction is a boolean signifying if you want
#               to insert a pause after an action. By default pauses are inserted
#               in the file with the value of the elapsed time since the last
#               recorded message. When done recording, just call Close() and
#               destroy the object.
#
#  DATE:        April, 2005
#
############################################################################

class SessionRecorder:

    def __init__(self, filename):
        #recDir = ConvertPath("./sessions/")  # a dir to write session files to
        print "Session recording started. Session will be saved in ", filename
        self.file = open(filename, "w")
        
        self.prevTime = 0
        


    def RecordAction(self, code, data, insertPause = True):
        # only record if the action is supported
        if hashRecordingWords.has_key( int(code) ):
            #record a pause, a session always starts with a 2 second pause!
            if self.prevTime == 0:
                    pauseTime = 2
            else:
                    pauseTime = int( ceil( time.time() - self.prevTime ) )
            self.prevTime = time.time()

            # finally, do the writing
            if insertPause:
                self.file.write("pause " + str(pauseTime) + "\n")
            self.file.write( hashRecordingWords[int(code)] +" "+ data+"\n" )


    def Close(self):
        print "Session recording stopped"
        self.file.close()
        

        


############################################################################
#
#  CLASS: SessionReader
#
#  DESCRIPTION: A class for reading user actions from a specified file.
#               To use, create an instance of this class and pass in a
#               filename to read from. Once created, keep calling ReadAction()
#               in order to read new lines from a file. It returns data in this
#               format: (code, data). It is your job to use this data then.
#               This class also has options to pause, stop, speed up and slow
#               down the playback of the session file. It does all that by
#               playing with the length of the pause (speed), repeatedly
#               returning pause (pausing) or returning a fake EOF (stopping
#               prematurely).
#
#
#  DATE:        April, 2005
#
############################################################################

class SessionReader:

    def __init__(self, sessionFile):           
        if os.path.isfile(sessionFile):
            self.file = open(sessionFile, "r")
        else:
            print "ERROR: Sessions file \"", sessionFile, "\" doesn't exist."

        # control variables
        self.speed = 1.0
        self.stop = False
        self.paused = False


    def SetPause(self, doPause):
        self.paused = doPause

        

    # reads the file one line at a time
    # it returns a tuple in this format: (code, data)
    # code is: -1 (paused), 0 (EOF) or **** (message code)
    # data depends on the message
    def ReadAction(self):

        # this is how we stop session playback
        if self.stop:
            return (0, "EOF")

        # this is how we pause playback (pretend that we encountered a
        # pause in the file. Instead we are pausing the playback and
        # not reading any new lines from the file
        if self.paused:
            return (-1, "0.5") 
        
        if not hasattr(self, "file"):
            return

        line = self.file.readline()
        line = string.strip(line)  # remove the newline char

        # EOF case
        if line == "":
            self.file.close()
            return (0, "EOF")
        
        # split the line into action and data
        action, data = string.split(line, " ", 1)

        # send a pause separately
        if action == "pause":
            data = float(data) * self.speed
            return (-1, str(data))

        # this is the actual message to be sent to SAGE
        code = -2 # any non-existent code
        for k, v in hashRecordingWords.iteritems():
            if v == action:
                code = int(k)   #extract the code            
        return (code, data)



    def GoSlower(self):
        self.speed = self.speed * 2
        if self.speed > 8:
            self.speed = 8

        if self.speed > 1:
            return "1/" + str(int(self.speed))
        elif self.speed == 1:
            return "1"
        elif self.speed < 1:
            return str(int((0.125 / self.speed) * 8))


    def GoFaster(self):
        self.speed = float(self.speed) / 2
        
        if self.speed < 0.125:
            self.speed = 0.125

        if self.speed > 1:
            return "1/" + str(int(self.speed))
        elif self.speed == 1:
            return "1"
        elif self.speed < 1:
            return str(int((0.125 / self.speed) * 8))
        

    def Stop(self):
        self.stop = True
        
    def Close(self):
        if hasattr(self, "file"):
           self.file.close() 
        
            



############################################################################
#
#  CLASS: PlaybackDialog
#
#  DESCRIPTION: This class describes the dialog box that controls the session
#               playback. It's a modal dialog so the UI can't be interacted
#               with during the playback. However, you can speed up or slow
#               down the playback, pause it or quit it prematurely.
#
#  NOTE:        This dialog is not neccessary for the operation of playback
#               but it gives user more control over it.
#
#  DATE:        April, 2005
#
############################################################################

class PlaybackDialog(wx.Dialog):

    def __init__(self, parent, sessionReader):
        wx.Dialog.__init__(self, parent, -1, "Session Playback")#, style = wx.STATIC_BORDER)
        self.sessionReader = sessionReader
        self.SetSize((250, 120))
        self.SetBackgroundColour(wx.Colour(0, 51, 52))  #003334
        self.SetForegroundColour(wx.Colour(255, 255, 255))
        self.CenterOnParent(wx.BOTH)
        self.SetFont(StandardFont())
        wx.StaticText(self, -1, "Playback in progress...", (50, 20))
        #self.CaptureMouse()
        self.pause = False

        # create the controls for the dialog
        self.pauseBtn = MyBitmapButton( self, (30, 50), (30, 30), "images/pause_up.jpg", "images/pause_down.jpg", "images/pause_over.jpg")
        self.stopBtn = MyBitmapButton( self, (70, 50), (30, 30), "images/stop_up.jpg", "images/stop_down.jpg", "images/stop_over.jpg")
        self.slowerBtn = MyBitmapButton( self, (135, 50), (30, 30), "images/slower_up.jpg", "images/slower_down.jpg", "images/slower_over.jpg")
        self.speedText = wx.StaticText( self, -1, "x 1", (172, 60), style = wx.ALIGN_CENTRE)
        self.fasterBtn = MyBitmapButton( self, (208, 50), (30, 30), "images/faster_up.jpg", "images/faster_down.jpg", "images/faster_over.jpg")

        # bind the events and event handlers for the buttons
        self.pauseBtn.Bind( wx.EVT_LEFT_UP, self.OnPause)
        self.stopBtn.Bind( wx.EVT_LEFT_UP, self.OnClose)
        self.fasterBtn.Bind( wx.EVT_LEFT_UP, self.OnFaster)
        self.slowerBtn.Bind( wx.EVT_LEFT_UP, self.OnSlower)
        self.Bind( wx.EVT_CLOSE, self.OnClose)
        
        self.Show()

    
    def OnPause(self, evt):
        if self.pause:
            self.pauseBtn.SetUpBitmap("images/pause_up.jpg")
            self.pauseBtn.SetDownBitmap("images/pause_down.jpg")
            self.pauseBtn.SetOverBitmap("images/pause_over.jpg")
            self.pause = False
            self.sessionReader.SetPause(False)
        else:
            self.pauseBtn.SetUpBitmap("images/play_up.jpg")
            self.pauseBtn.SetDownBitmap("images/play_down.jpg")
            self.pauseBtn.SetOverBitmap("images/play_over.jpg")
            self.pause = True
            self.sessionReader.SetPause(True)

        MyBitmapButton.OnLeftUp(evt.GetEventObject(), evt)
        

    # this just hides the window since sageGate will close it when it's done
    # it also tells the sessionReader to stop the playback
    def OnClose(self, evt):
        self.sessionReader.Stop()
        #self.Destroy()
        self.Show(False) # SAGEGate is the one that destroys the window so just hide it for now
        

    # sageGate calls this function when EOF has been reached
    def Close(self):
        #self.ReleaseMouse()
        self.Destroy()
        

    # this calls SessionReader and sets the speed of playback (basically it
    # changes the multiplier of the pauses)
    def OnFaster(self, evt):
        MyBitmapButton.OnLeftUp(evt.GetEventObject(), evt)
        newSpeed = self.sessionReader.GoFaster()
        self.speedText.SetLabel("x " + newSpeed)


    # this calls SessionReader and sets the speed of playback (basically it
    # changes the multiplier of the pauses)
    def OnSlower(self, evt):
        MyBitmapButton.OnLeftUp(evt.GetEventObject(), evt)
        newSpeed = self.sessionReader.GoSlower()
        self.speedText.SetLabel("x " + newSpeed)



def ShowLoadSessionDialog():
    if not os.path.isdir("sessions"):  #make the directory if it doesnt exist
        try:   #in case the file and directory permissions are not right
            os.mkdir("sessions")    
        except:
            dlg = wx.FileDialog(None, "Session to load:", os.path.abspath("."), "", "*.ses", style=wx.OPEN )
        else:
            dlg = wx.FileDialog(None, "Session to load:", os.path.abspath("sessions"), "", "*.ses", style=wx.OPEN )
    else:
        dlg = wx.FileDialog(None, "Session to load:", os.path.abspath("sessions"), "", "*.ses", style=wx.OPEN )


    if dlg.ShowModal() == wx.ID_OK:
        if "__WXMSW__" in wx.PlatformInfo:
            chosenFile = ConvertPath(dlg.GetPath())
        elif not ConvertPath(dlg.GetPath())[0] == "/":
            chosenFile = "/"+ConvertPath(dlg.GetPath())
        else:
            chosenFile = ConvertPath(dlg.GetPath())

        #make sure that the extension is ".ses"
        (path, ext) = os.path.splitext(chosenFile)
        if not ext == ".ses":
            chosenFile = chosenFile+".ses"

    else:
        chosenFile = None
    dlg.Destroy()
    return chosenFile


def ShowSaveSessionDialog():
    if not os.path.isdir("sessions"):  #make the directory if it doesnt exist
        try:   #in case the file and directory permissions are not right
            os.mkdir("sessions")    
        except:
            dlg = wx.FileDialog(None, "Save session as:", os.path.abspath("."), "", "*.ses", style=wx.SAVE | wx.OVERWRITE_PROMPT )
        else:
            dlg = wx.FileDialog(None, "Save session as:", os.path.abspath("sessions"), "", "*.ses", style=wx.SAVE | wx.OVERWRITE_PROMPT )
    else:
        dlg = wx.FileDialog(None, "Save session as:", os.path.abspath("sessions"), "", "*.ses", style=wx.SAVE | wx.OVERWRITE_PROMPT )

    
    if dlg.ShowModal() == wx.ID_OK:
        if "__WXMSW__" in wx.PlatformInfo:
            chosenFile = ConvertPath(dlg.GetPath())
        elif not ConvertPath(dlg.GetPath())[0] == "/":
            chosenFile = "/"+ConvertPath(dlg.GetPath())
        else:
            chosenFile = ConvertPath(dlg.GetPath())

        #make sure that the extension is ".ses"
        (path, ext) = os.path.splitext(chosenFile)
        if not ext == ".ses":
            chosenFile = chosenFile+".ses"

        # check if we have a permission to write to this folder
        (head, tail) = os.path.split(chosenFile)
        if not os.access(head, os.W_OK):
            ShowWriteFailedDialog(tail)
            chosenFile = None
    else:
        chosenFile = None
    dlg.Destroy()
    return chosenFile




