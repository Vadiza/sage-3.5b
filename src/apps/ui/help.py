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

import wx.lib.dialogs
from globals import *
from wx.lib import layoutf

# a list of IDs for all the help text
FILE_LIBRARY = 9000




allHelpStrings = {}  # help strings keyed by the above IDs

# checks if the event that triggered this is a EVT_CHAR and it comes from an F1 key
def ShowHelp(event):
    if hasattr(event,"GetKeyCode") and not event.GetKeyCode() == wx.WXK_F1:
        event.Skip()  #propagate the event further cause someone else might need it
    else:
        dlg = ScrolledMessageDialog(None, allHelpStrings[event.GetId()], "Help", size=(500,400))
        dlg.ShowModal()



class ScrolledMessageDialog(wx.Dialog):
    def __init__(self, parent, msg, caption,
                 pos=wx.DefaultPosition, size=(500,300),
                 style=wx.DEFAULT_DIALOG_STYLE):
        wx.Dialog.__init__(self, parent, -1, caption, pos, size, style)
        self.SetBackgroundColour(dialogColor)
        self.SetForegroundColour(wx.WHITE)
        x, y = pos
        if x == -1 and y == -1:
            self.CenterOnScreen(wx.BOTH)

        text = wx.TextCtrl(self, -1, msg, 
                           style=wx.TE_MULTILINE | wx.TE_READONLY)
        #text.SetBackgroundColour(appPanelColor)
        #text.SetDefaultStyle(wx.TextAttr(wx.WHITE, appPanelColor))
        ok = wx.Button(self, wx.ID_OK, "OK")
        ok.SetDefault()
        lc = layoutf.Layoutf('t=t5#1;b=t5#2;l=l5#1;r=r5#1', (self,ok)) 
        text.SetConstraints(lc)

        lc = layoutf.Layoutf('b=b5#1;x%w50#1;w!80;h*', (self,))
        ok.SetConstraints(lc)
        
        self.SetAutoLayout(1)
        self.Layout()




allHelpStrings[ FILE_LIBRARY ]='''File Library Help\n-------------------------\n
All the files are organized by their type and for each type there is a directory. Under each directory you can have multiple sub-directories to help you organize the files better. The tree you see is a direct representation of the actual directory tree on the server, except from the "favorites" which is explained later. You can add files, delete them, move them, create and delete directories, view previews and show files on the tiled display.


NOTE: Most of the following actions directly affect the files and directories on the server.

SEARCH:
- you can search as you type. By pressing ENTER or RETURN in the search box you search for the next occurence of the same string. "Find Again" button performs the same action.\n
REFRESH:
- this refreshes the whole directory tree to bring it up to date with any changes other users made. The tree will be automatically refreshed if a file could not be found in any of the below operations.\n
ADD FILES:
- just drag and drop a file from your computer anywhere on the tree (or use the "Browse for Files). The file will be uploaded to the server automatically and initially stored in the directory that matches its type. If a file with the same filename and file size exists, it will not be uploaded.\n
DELETE FILES:
- right click on a file you wish to delete and click "Delete File". The file will be permanently deleted from the server.\n
MOVE FILES:
- to organize your files you can move them from one directory to another (but only within the same "type" folder). To move a file just drag and drop it where you want it. You can even drop the file on another file and it will be moved to the same folder as that file. Note however, that directories cannot be moved nor can favorite files. If you drop the file on a wrong place the move will be ignored.\n
CREATE DIRECTORY:
- to create a sub-directory right click on any directory and click "New Folder". A new directory will be created as a sub-dir of the clicked one. You can then populate this new directory with files.\n
DELETE DIRECTORY:
- to delete a directory and all the files in it right click on a directory and click "Delete Folder".\n
VIEW PREVIEW:
- previews are automatically shown when a file is clicked on. However, previews are only available for images.\n
SHOW FILE:
- Simply double-click on a file and it will be shown. Another way is when a file is selected the show button is enabled and you can click it to show the file. The execution information (app name, params...) used for showing the file is retrieved from the server which in turn reads it from a config file.\n\n
FAVORITES:
-----------------
- the last outermost folder is called "favorites" and this is where you can store your favorite files (think of them as bookmarks or symbolic links). This whole folder and the files in it are just virtual and do not really exist on the server or your computer. However, the list of files in there is automatically stored on your local computer whenever there is a change and they are automatically reloaded upon startup.\n
ADD FILES TO FAVORITES:
- drag and drop a file from anywhere to anywhere in the favorites part of the tree. The file link will be automatically put under the correct type. The file itself will not be moved on the server, just a symbolic link will be created so when original files location changes this link will be invalid.\n
REMOVE FILES FROM FAVORITES:
- right click on a file and select "Remove From Favorites". This action will not remove the file from the server.\n
SHOW A FILE FROM FAVORITES:
- the same procedure as above (double-click or "Show" button)

NOTE: You cannot create new folders or move files within the favorites folder. Also, these files might become invalid if the orginal files that they point to have been moved/removed. In that case remove the file from favorites and add it again.
'''
