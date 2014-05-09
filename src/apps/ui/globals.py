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



# this file holds functions and constants used throughout the application

import os.path, time, wx, sys

# shortcut
opj = os.path.join
#sys.path.append( opj(os.environ["SAGE_DIRECTORY"], "bin" ) )
from sagePath import getUserPath, getPath, SAGE_DIR


#------------------------------------------------------------------------
#     COLORS  
#------------------------------------------------------------------------

# display
tileColor = wx.Colour(102, 204, 204)

# shapes
shapeColor          = wx.Colour(153, 255, 255)
shapeBorderColor    = wx.Colour(204, 153, 102)
shapeTextColor      = "grey"  
shapeSelectedTextColor = "white"
shapeBorderColor    = wx.Colour(51,153,153)
shapeHighlightColor = wx.Colour(255, 204, 102)
shapeShadowColor    = wx.Colour(51, 102, 102)
shapeSelectedBorderColor = wx.Colour(153,255,204)
shapeSelectedColor  = wx.Colour(51,153,153)

dialogColor = wx.Colour(51, 102, 102)

# app panel
appPanelColor = wx.Colour(0,51,52)

# info panel
infoPanelColor = wx.Colour(102,204,153)

# performance panel
perfButtonColor     = wx.Colour(0, 153, 153)
perfButtonTextColor = wx.Colour(255, 255, 255)
perfPanelColor = wx.Colour(102,204,153)



#------------------------------------------------------------------------
#     PATHS
#------------------------------------------------------------------------

def ConvertPath(path):
    """Convert paths to the platform-specific separator"""
    return apply(opj, tuple(path.split('\\')))


DATA_DIR = getUserPath("sageui", "data")
LOG_FILE = getUserPath("sageui", "output_log.txt")
SAVED_STATES_DIR = getUserPath("saved-states")


#------------------------------------------------------------------------
#     DIALOG UTILITY FUNCTIONS 
#------------------------------------------------------------------------

def ShowWriteFailedDialog(filename):
    dialog = wx.MessageDialog(None, "Unable to write to file <"+filename+">. Check write permissions for the file or directory.",
                              "Write Failed", wx.OK)
    dialog.ShowModal()


def Message(msg, title):
    """ for thread safety """
    wx.CallAfter(_ShowMessage, msg, title)

def _ShowMessage(msg, title):
    dlg = wx.MessageDialog(None, msg, title, style = wx.OK)
    dlg.ShowModal()

def ChoiceDialog(msg, title):
    dlg = wx.MessageDialog(None, msg, title, style = wx.OK | wx.CANCEL)
    if dlg.ShowModal() == wx.ID_OK:
        return True
    else:
        return False


def ShowColorDialog(previousColor=None):
    """ takes in (r,g,b) tuple and returns the same """
    if not previousColor == None:
        colorData = wx.ColourData()
        colorData.SetColour(previousColor)
        dlg = wx.ColourDialog(None, colorData)
    else:
        dlg = wx.ColourDialog(None)

    dlg.GetColourData().SetChooseFull(True)
    if dlg.ShowModal() == wx.ID_OK:
        color = dlg.GetColourData().GetColour()
        dlg.Destroy()
        return color
    else:
        dlg.Destroy()
        return (-1,-1,-1)




def doRun():
    return True


#------------------------------------------------------------------------
#     FONTS  
#------------------------------------------------------------------------

def StandardFont(window = None):
    if window == None:
        ss = wx.SystemSettings
        ssFont = ss.GetFont(wx.SYS_DEFAULT_GUI_FONT)
        return ssFont
    else:
        winFont = window.GetFont()
        return winFont
    

def BoldFont(window = None):
    if window == None:
        ss = wx.SystemSettings
        ssFont = ss.GetFont(wx.SYS_DEFAULT_GUI_FONT)
        ssFont.SetWeight(wx.BOLD)
        return ssFont
    else:
        winFont = window.GetFont()
        winFont.SetWeight(wx.BOLD)
        return winFont



#------------------------------------------------------------------------
#    LOCATION OF THE SAGE SERVER 
#------------------------------------------------------------------------
    
SAGE_SERVER = "sage.sl.startap.net"
def setSAGEServer(server):
    global SAGE_SERVER
    SAGE_SERVER = server

def getSAGEServer():
    return SAGE_SERVER
    



#------------------------------------------------------------------------
#    LOCATION OF THE APP LAUNCHER
#------------------------------------------------------------------------
    
APP_LAUNCHER = ""
def setAppLauncher(server):
    global APP_LAUNCHER
    APP_LAUNCHER = server

def getAppLauncher():
    return APP_LAUNCHER



#------------------------------------------------------------------------
#    SAGE UI VERSION 
#------------------------------------------------------------------------

global UI_VERSION
def setUIVersion(v):
    global UI_VERSION
    UI_VERSION = v

def getUIVersion():
    return UI_VERSION




#------------------------------------------------------------------------
#     USERS DATA (data from sage server about all the connections)
#------------------------------------------------------------------------

global USERSDATA
def setUsersData(d):
    global USERSDATA
    USERSDATA = d

def getUsersData():
    return USERSDATA



#------------------------------------------------------------------------
#    POINTER STATES AND TYPES
#------------------------------------------------------------------------
#### Sage Draw Object types
POINTER_TYPE = 1


#### MOUSE STATES
PTR_NORMAL_STATE = 1
PTR_MOVE_STATE   = 2
PTR_RESIZE_STATE = 3




#------------------------------------------------------------------------
#    FOR RECORDING PERFROMANCE DATA TOTALS 
#------------------------------------------------------------------------
global startTime
startTime = -1
def getTimeStamp():
    global startTime
    if startTime == -1:  # set the start time on first request... so that we start from 0
        startTime = time.time()
    return int(round(time.time() - startTime))



#------------------------------------------------------------------------
#    FOR MULTIPLE SAGE DISPLAYS ON ONE FSMANAGER
#------------------------------------------------------------------------

MIDDLE_DISP = -1
LEFT_DISP = 0
RIGHT_DISP = 1
BOTTOM_DISP = 2
TOP_DISP = 3


class Bounds:
    """ used in event conversion """
    def __init__(self, left=0, right=0, top=0, bottom=0):
        self.left = int(round(left))
        self.right = int(round(right))
        self.top = int(round(top))
        self.bottom = int(round(bottom))

    def setAll(self, left, right, top, bottom):
        self.left = int(round(left))
        self.right = int(round(right))
        self.top = int(round(top))
        self.bottom = int(round(bottom))

    def getAll(self):
        return self.left, self.right, self.top, self.bottom

    def getWidth(self):
        return self.right - self.left

    def getHeight(self):
        return self.bottom - self.top   # top is 0

##     def isIn(self, x, y):
##         """ returns True if the (x,y) is in Bounds, False otherwise """
##         if self.left <= x and self.right >= x and self.bottom <= y and self.top >= y:
##             return True
##         else:
##             return False
