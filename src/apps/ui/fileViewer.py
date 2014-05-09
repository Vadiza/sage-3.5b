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


import xmlrpclib, base64, sys, cStringIO, socket, pickle, copy, os.path, os
from httplib import HTTPException
import wx.lib.scrolledpanel
from wx import ProgressDialog
from globals import ConvertPath, Message, ChoiceDialog, appPanelColor, dialogColor
import help  #my own
import preferences as prefs  #my own
from math import ceil
from threading import *
from misc.imsize import imagesize  # reads the image header and gets the size from it
import wx.lib.throbber as throb
import urllib

XMLRPC_PORT = "8800"
FILE_GRABBER_PORT = "8801"
FILE_SERVER_BIN_PORT = 8802
PREVIEW_SIZE = (150,150)
DEFAULT_TIMEOUT = None  # no timeout because server side processing might take a while

# used so that we don't create two connections to the same server... no reason
# keyed by IP address of the server (host)
fileServers = {}
def GetFileServer(host, port):
    global fileServers
    if fileServers.has_key(host):
        return fileServers[host]
    else:
        fs = FileServer(host, port)
        fileServers[host] = fs
        return fileServers[host]



### stuff needed in order to customize the sending...
class MyConnection:
    def __init__(self, conn):
        self.conn = conn

        ### if data is bigger (>100KB), show the progress bar
    def send(self, data):
        dataLength = len(data)
        if dataLength > 100000:
            try:
                dlg = wx.ProgressDialog("Uploading File", "Uploading... (0/%.2f)"%(dataLength/1048576),
                                        style=wx.PD_SMOOTH | wx.PD_CAN_ABORT |
                                        wx.PD_ELAPSED_TIME | wx.PD_REMAINING_TIME | wx.PD_AUTO_HIDE)
                chunkSize = 8192
                for i in range (0, dataLength, chunkSize):  #send chunks of 8192 bytes
                    self.conn.send(data[i:i+chunkSize])
                    if not dlg.Update(ceil((float(i)/dataLength)*100), "Uploading... (%.2f/%.2f MB)"%
                                      (float(i)/1048576, float(dataLength)/1048576)):
                        self.conn.close()
                        break
                dlg.Destroy()
            except socket.error:
                dlg.Destroy()
                Message("Error while uploading file. This is most likely a server-side error and it should be possible to continue.", "Upload Failed")
        else:
            self.conn.send(data)

        ### overridden so that we can catch the exception when we press cancel during upload
    def getreply(self):
        ret = (None, None, None)
        try:
            ret = self.conn.getreply()
        except HTTPException:
            pass
        return ret

    
    def __getattr__(self, key):
        return getattr(self.conn, key)


class MyTransport(xmlrpclib.Transport):
    def make_connection(self, host):
        conn = xmlrpclib.Transport.make_connection(self, host)
        return MyConnection(conn)




class FileServer:
    
    def __init__(self, host, port):
        self.host = host  #where the FileLibrary is running
        self.port = port
        self.connected = False
        self.Connect()


        # try to connect to the server
        # if we failed, set the flag
        # if we succeed, get the path where the images are stored so that we can send it to the app
    def Connect(self):
        if self.connected: return True

        print "\nConnecting to XMLRPC server at: http://"+str(self.host)+":"+self.port
        socket.setdefaulttimeout(DEFAULT_TIMEOUT)
        self.server = xmlrpclib.ServerProxy("http://"+str(self.host)+":"+self.port, transport=MyTransport())
        #socket.setdefaulttimeout(3)
        try:
            self.connected = True
            global FILE_GRABBER_PORT
            FILE_GRABBER_PORT = self.server.TestConnection()[1]  #just see if the connection opened correctly
        except:
            print "Could not connect to the file server at: "+str(self.host)+":"+self.port
            self.connected = False
            return False
        else:
            print "Connected to the XMLRPC server at: http://"+str(self.host)+":"+self.port
            return True


    def IsConnected(self):
        return self.Connect()


        # uploads the file to the file library and returns information about the file
    def UploadFile(self, fullPath):
        try:
            
            # get the file info first
            fileInfo = self.GetNewFileInfo(fullPath)
            if not fileInfo: return False            # file type not supported
            else: (fileType, appName, fullRemotePath, size, params, fileExists) = fileInfo

            if fileExists:                           # if the file exists on the server, just show it (ie. dont send it)
                return (fileType, appName, fullRemotePath, size, params, fileExists) #no need to upload
            else:
                if self.__SendFile(fullPath):    # did upload fail for some reason?
                    return (fileType, appName, fullRemotePath, size, params, fileExists)
                else:                           
                    return False                     # upload failed so whatever you are doing after this (ShowFile maybe), don't do it

        except socket.error: 
            Message("Unable to upload file. There is no connection with the File Server.", "No connection")
            return False
        except xmlrpclib.ProtocolError:
            return False
    

        # gets the information about the file and the supporting app from the file library
    def GetNewFileInfo(self, fullPath):
        #c = wx.BusyCursor()
        (path, filename) = os.path.split(fullPath)
        filename = "_".join(filename.split())

        # check if the file type is supported
        # if it is, it will get some data about the file (type, viewerApp ...)
        fileSize = os.stat(fullPath).st_size  #get the size in bytes
        fileInfo = self.server.GetFileInfo( filename, fileSize )
        if not fileInfo:
            #del c
            extension = os.path.splitext(filename)[1].lower()
            Message("File type <"+extension+"> not supported.", "Invalid File Type")
            return False

        (fileType, size, fullRemotePath, appName, params, fileExists) = fileInfo

        # prepare for showing the file (filename, path, size if necessary...)
        if fileType == "image":
            try:
                imsize = imagesize(fullPath)
                size = (imsize[1], imsize[0])
            except:
                size = (-1,-1)
        #del c
        return (fileType, appName, fullRemotePath, size, params, fileExists)


    def __SendFile(self, fullPath):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((self.host, FILE_SERVER_BIN_PORT))

            #convert to a filename with _ instead of empty spaces
            convertedFilename = "_".join(os.path.basename(fullPath).split())
            
            # send the header first
            fileSize = os.path.getsize(fullPath)
            header = convertedFilename + " " + str(PREVIEW_SIZE[0]) + " " + str(PREVIEW_SIZE[1]) + \
                     " " + str(fileSize) + "\n"
            s.sendall(header)     

            # show the progress dialog if file is bigger
            doDlg = False
            if fileSize > 100000:
                dlg = wx.ProgressDialog("Uploading File", "Uploading... (0/%.2f)"%(fileSize/1048576),
                                        style=wx.PD_SMOOTH | wx.PD_CAN_ABORT |
                                        wx.PD_ELAPSED_TIME | wx.PD_REMAINING_TIME | wx.PD_AUTO_HIDE)
                doDlg = True
                
            # send the file data
            f=open(fullPath, "rb")
            t = 0
            for line in f:
                s.sendall(line)
                t += len(line)

                # update the dialog if needed
                if doDlg and not dlg.Update(ceil((float(t)/fileSize)*100), "Uploading... (%.2f/%.2f MB)"%
                                            (float(t)/1048576, float(fileSize)/1048576)):
                    s.close()
                    f.close()
                    dlg.Destroy()
                    return False

            if doDlg:  dlg.Destroy()
                
            s.sendall("\n")  # because the other side is reading lines
            f.close()
            s.recv(1)
        except:
            print "Error sending file to File Server: ", sys.exc_info()[0], sys.exc_info()[1]
            if doDlg:  dlg.Destroy()
            Message("Could not upload file "+str(os.path.basename(fullPath)), "Error")
            return False

        return True
                
        #return self.server.UploadFile( convertedFilename, data, PREVIEW_SIZE )


    
### the base class for accepting the dropped files
class FileDropTarget(wx.PyDropTarget):
    def __init__(self, host=None, port=None):

        # set up data objects  (Composite works correctly on Windows)
        if "__WXMSW__" in wx.PlatformInfo:
            self.do = wx.DataObjectComposite()
            self.filedo = wx.FileDataObject()
            self.textdo = wx.TextDataObject()
            self.do.Add(self.textdo)
            self.do.Add(self.filedo, True)
        else:
            self.do = wx.FileDataObject()
        
        wx.PyDropTarget.__init__(self, self.do)
        
        # delayed server object creation for the CanvasDropTarget
        # when a file is dropped, we dont know which library to upload the file to so
        # that's when we decide which library to connect to
        if host!=None and port!=None:
            self.serverObj = GetFileServer(host, port)
            self.server = self.serverObj.server


        # this must be called if you passed in None to the constructor
    def SetLibrary(self, host, port):
        self.serverObj = GetFileServer(host, port)
        self.server = self.serverObj.server


    def GetServer(self):
        return self.server

    
    def OnData(self, x, y, d):
        if not self.serverObj.IsConnected():
            Message("Unable to upload/show file. There is no connection with the File Server.", "No connection")
            return False
        
        self.GetData() #get the dropped data

        if "__WXMSW__" in wx.PlatformInfo:
            if self.textdo.GetTextLength() > 1:    # empty text has length = 1
                droppedText = self.textdo.GetText()
                self.textdo.SetText("")   #reset the text
                return self.__DownloadURL(droppedText)
            else:
                files = self.filedo.GetFilenames()[0]  #extract the filenames from the dropped data
                #files = unicode(files, "utf_8")
                if os.path.isdir(files):
                    Message("You cannot upload directories. Only individual files.", "Upload Failed")
                    return False
                else:
                    return files
        else:
	    filenames = self.do.GetFilenames()
	    if not filenames: return False
	    files = filenames[0]  #extract the filenames from the dropped data
	    if os.path.isdir(files):
                Message("You cannot upload directories. Only individual files.", "Upload Failed")
                return False
            else:
                return files

            
        # checks if the file the url is pointing to is valid,
        # if so, it downloads it and returns the local path to it
    def __DownloadURL(self, url):
        if os.path.isfile(url):  # if it's a local file so just return it's path
            return url
        elif url.startswith("http://") or url.startswith("ftp://"):   #if it's a url
            filename = os.path.basename(url)
            if self.server.GetFileType(filename):   # is the file type even supported?
                urllib.urlcleanup()  #cleanup the cache
                urllib.urlretrieve(url, filename)
                return os.path.abspath(filename)
            else:                                # file type not supported
                extension = os.path.splitext(filename)[1].lower()
                Message("File type <"+extension+"> not supported.", "Invalid File Type")
                return False
        else:
            return False



class CanvasDropTarget(FileDropTarget):
    def __init__(self, canvas):
        self.canvas = canvas
        self.sageGate = self.canvas.sageGate
        self.sageHost = self.sageGate.sageHost  #for the filegrabber
        self.lastX = 0
        self.lastY = 0
        
        FileDropTarget.__init__(self) # dont provide library location right away since we will let the user choose that each time


    def OnDrop(self, x, y):
        self.lastX = self.canvas.ToSAGECoordsX(x, 0)
        self.lastY = self.canvas.ToSAGECoordsY(y, 0)
        return True


    def OnData(self, x, y, d):   # just upload the file to the libary on the machine we are connected to
        self.SetLibrary(self.sageHost, XMLRPC_PORT)  
        filePath = FileDropTarget.OnData(self,x,y,d)
        if filePath:
            uploadResult = self.serverObj.UploadFile(filePath)
        else: return False
            
        if uploadResult:  #if upload succeeded
            (fileType, appName, fullRemotePath, size, params, fileExists) = uploadResult
            self.ShowFile(fileType, appName, fullRemotePath, size, params, True)
        return d


            # run the show file in a thread
    def ShowFile(self, fileType, fullPath, appName, params, size, doPrep=True):
        t = Thread(target=self.DoShowFile, args=(fileType, fullPath, appName, params, size, doPrep))
        t.start()


    def DoShowFile(self, fileType, appName, fullRemotePath, size, params, doPrep=True):
        # first, make sure that the file exists
        if doPrep:
            prepResult = self.server.PrepareFile(fullRemotePath, self.sageHost)
            if not prepResult:
                Message("Unable to show file because the transfer of file from the FileServer to the SAGE machine failed.", "Error")
                return
            else:
                fullPath = prepResult #the path was returned by the FileGrabber
            
        totalDisplayWidth = self.canvas.tiledDisplayWidth
        totalDisplayHeight = self.canvas.tiledDisplayHeight

        if fileType == "image":
            imageWidth = size[0]
            imageHeight = size[1]
            imageAspectRatio = float( imageWidth/imageHeight )

            # figure out if the image is going to fit, if not, reposition and resize it as necessary
            # first resize if necessary
            widthRatio = float(totalDisplayWidth / float(imageWidth))
            heightRatio = float(totalDisplayHeight / float(imageHeight))
            if widthRatio < 1 or heightRatio < 1:
                if widthRatio > heightRatio:   #we'll resize based on height
                    resizeRatio = heightRatio
                else:
                    resizeRatio = widthRatio   #we'll resize based on width
                imageWidth = int(imageWidth * resizeRatio)
                imageHeight = int(imageHeight * resizeRatio)         

            # now reposition
            cornerX = self.lastX - int(imageWidth/2)
            cornerY = self.lastY - int(imageHeight/2)

            # is the image off the screen up or down?
            if (totalDisplayHeight - cornerY) < imageHeight:
                cornerY = int(totalDisplayHeight - imageHeight)
            elif cornerY < 0:
                cornerY = 0

            # is the image off the screen left or right?
            if (totalDisplayWidth - cornerX) < imageWidth:
                cornerX = int(totalDisplayWidth - imageWidth)
            elif cornerX < 0:
                cornerX = 0
            res = self.sageGate.executeApp(appName, pos=(cornerX, cornerY), size=(imageWidth,imageHeight), optionalArgs = fullRemotePath+" "+str(imageWidth)+" "+str(imageHeight)+" "+params)
            
        elif fileType == "video" or fileType == "pdf":
            res = self.sageGate.executeApp(appName, pos=(self.lastX, self.lastY), optionalArgs=params+" "+fullRemotePath)
            
        else:  #for other types
            res = self.sageGate.executeApp(appName, optionalArgs=fullRemotePath+" "+params)

        if res == -1:
            Message("Application not started. Either application failed, the application launcher is not running or the application  <<"+appName+">>  is not configured in application launcher.", "Application Launch Failed")
        



class LibraryDropTarget(FileDropTarget):
    def __init__(self, parent, host, port=XMLRPC_PORT):
        self.parent = parent
        FileDropTarget.__init__(self, host, port)
        
        
    def OnData(self, x, y, d):
        filePath = FileDropTarget.OnData(self,x,y,d)
        if not filePath: return
        uploadResult = self.serverObj.UploadFile(filePath)
        if uploadResult:  #if upload succeeded
            (fileType, appName, fullRemotePath, size, params, fileExists) = uploadResult
            if not fileExists:  #only add a file to the tree if the file doesnt exist yet
                self.parent.OnNewFile(fullRemotePath, fileType)
            else:
                Message("File already exists in the file library.", "Duplicate Found")



dirTextColor = wx.Colour(204, 153, 102)#250,50,50)

class FileLibrary(wx.Frame):
    def __init__(self, parent, parentPos):
        self.canvas = parent 
        self.sageGate = self.canvas.sageGate
        self.sageHost = self.sageGate.sageHost  #for the filegrabber

        res = LibraryChoiceDialog(parent).ShowDialog()
        if not res:
            return  #user pressed cancel when choosing the library
        else:
            (self.libName, self.libHost) = res

        
        self.dt = LibraryDropTarget(self, self.libHost, XMLRPC_PORT)
        self.server = self.dt.GetServer()
        self.serverObj = self.dt.serverObj

        # we must be connected in order to see the library
        if not self.serverObj.IsConnected():
            Message("Unable to access file library. There is no connection with the File Server.", "No connection")
        else:
            x,y=parentPos
            wx.Frame.__init__(self, parent, help.FILE_LIBRARY, "File Library - "+self.libName, pos=(x+200, y+200), size=(550,550))
            self.previewSize = (150,150)
            self.panel = wx.lib.scrolledpanel.ScrolledPanel(self, wx.ID_ANY)
            self.__SetupLayout()
            
            # so that wherever we press F1 key it will bring up help for that window
            # the help.ShowHelp function will check for the correct key press
            self.Bind(wx.EVT_CHAR, help.ShowHelp)

            # make the tree
            self.tree = wx.TreeCtrl(self.panel, wx.ID_ANY, style=wx.TR_NO_LINES | wx.TR_SINGLE | wx.TR_HAS_BUTTONS)
            self.tree.SetDropTarget(self.dt)
            self.root = self.tree.AddRoot("Files")
            self.imList = wx.ImageList(16,16)
            self.tree.SetImageList(self.imList)  #image list must be created in order to have DnD capabiliy... weird
            self.MakeTree()
            self.tree.Bind(wx.EVT_RIGHT_DOWN, self.OnRightClick)
            self.tree.Bind(wx.EVT_TREE_SEL_CHANGED, self.OnSelChanged)
            self.tree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnShowButton)
            self.tree.Bind(wx.EVT_TREE_BEGIN_DRAG, self.OnBeginDrag)
            self.tree.Bind(wx.EVT_TREE_END_DRAG, self.OnEndDrag)
            self.tree.Bind(wx.EVT_ENTER_WINDOW, self.OnMouseEnter)
            self.tree.Bind(wx.EVT_LEAVE_WINDOW, self.OnMouseLeave)
            self.tree.Bind(wx.EVT_MOTION, self.OnMouseMove)
            self.currentItemId = None

            # set the colors
            self.tree.SetBackgroundColour(appPanelColor)
            self.tree.SetForegroundColour(wx.WHITE)
            self.panel.SetBackgroundStyle(wx.BG_STYLE_COLOUR)
            self.panel.SetBackgroundColour(dialogColor)
            self.SetForegroundColour(wx.WHITE)

            # add main things to the sizer
            self.mainSizer.Add(self.tree, 1, wx.EXPAND)
            self.mainSizer.Add(self.vertLine, 0, wx.EXPAND )
            self.mainSizer.Add(self.vertSizer, 0, wx.EXPAND)

            # attach the sizer to the panel
            self.panel.SetSizer(self.mainSizer)
            self.panel.SetAutoLayout(1)
            self.panel.SetupScrolling()

            wx.Frame.Show(self)


    def __SetupLayout(self):        
        self.mainSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.vertSizer = wx.BoxSizer(wx.VERTICAL)
        self.horSizer = wx.BoxSizer(wx.HORIZONTAL)

        self.vertLine = wx.StaticLine(self.panel, style=wx.LI_VERTICAL)

        self.showBtn = wx.Button(self.panel, wx.ID_ANY, "Show")
        self.showBtn.Bind(wx.EVT_BUTTON, self.OnShowButton)
        self.showBtn.Disable()
        self.__SetupButton(self.showBtn)
        
        self.shareCheckBox = wx.CheckBox(self.panel, wx.ID_ANY, "Shareable app? (uses sageBridge)")
        self.shareCheckBox.SetForegroundColour(wx.WHITE)
        self.shareCheckBox.SetBackgroundStyle(wx.BG_STYLE_COLOUR)
        self.shareCheckBox.SetBackgroundColour(dialogColor)

        self.dxtCheckBox = wx.CheckBox(self.panel, wx.ID_ANY, "Use DXT (faster, lower quality)")
        self.dxtCheckBox.SetValue(True)
        self.dxtCheckBox.SetForegroundColour(wx.WHITE)
        self.dxtCheckBox.SetBackgroundStyle(wx.BG_STYLE_COLOUR)
        self.dxtCheckBox.SetBackgroundColour(dialogColor)

        self.refreshBtn = wx.Button(self.panel, wx.ID_ANY, "Refresh")
        self.refreshBtn.Bind(wx.EVT_BUTTON, self.RefreshTree)
        self.__SetupButton(self.refreshBtn, smaller=True)
                
        self.browseBtn = wx.Button(self.panel, wx.ID_ANY, "Browse")
        self.browseBtn.Bind(wx.EVT_BUTTON, self.BrowseFiles)
        self.__SetupButton(self.browseBtn, smaller=True)
        
        self.helpBtn = wx.Button(self.panel, help.FILE_LIBRARY, "Help")
        self.helpBtn.Bind(wx.EVT_BUTTON, help.ShowHelp)
        self.__SetupButton(self.helpBtn, smaller=True)

        self.searchField = wx.TextCtrl(self.panel, wx.ID_ANY, style=wx.TE_PROCESS_ENTER)
        self.searchField.Bind(wx.EVT_TEXT_ENTER, self.OnSearch)
        self.searchField.Bind(wx.EVT_TEXT, self.OnSearch)
        tTip = wx.ToolTip("Search as you type...\npress ENTER to search again")
        tTip.SetDelay(200)
        self.searchMsg = wx.StaticText(self.panel, wx.ID_ANY, "", style=wx.ALIGN_CENTRE)
        self.searchMsg.SetForegroundColour(wx.WHITE)
        self.searchField.SetToolTip(tTip)
        self.searchBtn = wx.Button(self.panel, wx.ID_ANY, "Find Next")
        self.searchBtn.Bind(wx.EVT_BUTTON, self.OnSearch)
        self.__SetupButton(self.searchBtn)
        tTip = wx.ToolTip("Same as pressing ENTER in the search field")
        tTip.SetDelay(200)
        self.searchBtn.SetToolTip(tTip)
                              
        self.horSizer.Add(self.refreshBtn, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT | wx.TOP | wx.BOTTOM, border=5)
        self.horSizer.Add(self.browseBtn, 0, wx.ALIGN_CENTER | wx.TOP | wx.BOTTOM, border=10)
        self.horSizer.Add(self.helpBtn, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT | wx.TOP | wx.BOTTOM, border=5)
        
        self.no_preview = wx.Image(ConvertPath("images/no_preview.png")).ConvertToBitmap()
        self.retrieving_preview = wx.Image(ConvertPath("images/retrieving_preview.png")).ConvertToBitmap()
        self.currentImage = wx.StaticBitmap(self.panel, wx.ID_ANY, self.no_preview, size=self.previewSize)

        self.vertSizer.Add(self.horSizer, 0, wx.ALIGN_CENTER_HORIZONTAL | wx.ALIGN_TOP | wx.TOP | wx.EXPAND, border=10)
        self.vertSizer.Add(wx.StaticLine(self.panel, style=wx.LI_HORIZONTAL), 0, wx.EXPAND | wx.ALIGN_TOP | wx.TOP, border=5)
        self.vertSizer.AddSpacer((1,1),1)
        self.vertSizer.Add(self.currentImage, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT | wx.TOP, border=20)
        self.vertSizer.Add(self.showBtn, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT | wx.TOP | wx.BOTTOM, border=5)
        self.vertSizer.Add(self.shareCheckBox, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT | wx.TOP | wx.BOTTOM, border=5)
        self.vertSizer.Add(self.dxtCheckBox, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT | wx.TOP | wx.BOTTOM, border=5)
        self.vertSizer.AddSpacer((1,1),1)
        self.vertSizer.Add(wx.StaticLine(self.panel, style=wx.LI_HORIZONTAL), 0, wx.EXPAND | wx.ALIGN_BOTTOM | wx.BOTTOM, border=10)
        self.vertSizer.Add(self.searchField, 0, wx.ALIGN_BOTTOM | wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)
        self.vertSizer.Add(self.searchMsg, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, border=5)
        self.vertSizer.Add(self.searchBtn, 0, wx.ALIGN_CENTER_HORIZONTAL | wx.LEFT | wx.RIGHT | wx.TOP | wx.BOTTOM, border=10)

        
    def __SetupButton(self, btn, smaller=False):
        if "__WXMAC__" not in wx.PlatformInfo:
            if smaller:
                sz = btn.GetSize()
                btn.SetMaxSize((sz[0], sz[1]-5))
            btn.SetBackgroundColour(wx.Colour(153, 255, 255))
            btn.SetForegroundColour(wx.BLACK)
        

    def MakeTree(self):
        self._alreadyFound = []  # a list of already found itemIds
        self._searchString = ""  # a current search string
        self.branches = {}   #treeItemIds keyed by file types
        self.favoritesBranches = {}   #treeItemIds keyed by file types
        
        # get the newest list of files from the File Server 
        try:
            fileHash = self.server.GetFiles()
        except socket.error:
            Message("Unable to retrieve the file list. No connection with the File Server.", "No Connection")
            return

        # make the parents (main folders)
        for fType, tpl in fileHash.iteritems():
            tpl[2].sort(lambda x, y: cmp(x.lower(), y.lower()))
            self.MakeBranch(fType, tpl[0])  #include the dir path
            self.MakeNodes(self.branches[fType], tpl)

        # make the favorites folder
        self.MakeBranch("favorites", None)  #None dirPath (virtual directory)
        for fType in self.branches.iterkeys():
            if fType != "favorites":
                self.MakeFavoritesBranch(fType)

        self.LoadFavorites()

        # bind events and finish the tree
        self.tree.Expand(self.root)


        ### recursively go through the data retrieved from server.GetFiles
        ### and recreate the directory structure as a tree
    def MakeNodes(self, parentId, tpl):
        (dirPath, dirs, files, fileType) = tpl
        sortedDirs = dirs.keys()
        sortedDirs.sort(lambda x, y: cmp(x.lower(), y.lower()))
        
        for dirName in sortedDirs:
            dirTpl = dirs[dirName]
            dirId = self.tree.AppendItem(parentId, dirName)
            dirObj = Node(dirName, fileType, dirPath, False, False)
            self.tree.SetItemTextColour(dirId, dirTextColor)
            self.tree.SetPyData(dirId, dirObj)
            self.MakeNodes(dirId, dirTpl)

        files.sort(lambda x, y: cmp(x.lower(), y.lower()))
        for f in files:
            fileObj = Node(f, fileType, dirPath, True, False)
            fId = self.tree.AppendItem(parentId, f)
            self.tree.SetPyData(fId, fileObj)


        ### opens a file browser for your local machine so that you can add files to the library that way
    def BrowseFiles(self, event=None):
        dlg = wx.FileDialog(self, str("Add to the file library"), defaultDir=os.getcwd(), style=wx.OPEN | wx.FILE_MUST_EXIST)
        if dlg.ShowModal() == wx.ID_OK:
            uploadResult = self.serverObj.UploadFile(dlg.GetPath())  # do the checks, upload the file...
            if uploadResult:  #if upload succeeded
                (fileType, appName, fullRemotePath, size, params, fileExists) = uploadResult
                if not fileExists:  #only add a file to the tree if the file doesnt exist yet
                    self.OnNewFile(fullRemotePath, fileType)
                else:
                    Message("File already exists in the file library.", "Duplicate Found")
        dlg.Destroy()


        # called when a new file is dropped on the file library
    def OnNewFile(self, fullRemotePath, fileType):
        def AlreadyThere(filename, type):
            (siblingId, cookie) = self.tree.GetFirstChild(self.branches[type])
            while siblingId.IsOk():
                if self.tree.GetPyData(siblingId).GetName() == filename:
                    return True   # no need to add to favorites since it already exists in there
                siblingId = self.tree.GetNextSibling(siblingId)
            return False  #if we got here that means that there are no files with the same name already
            

        (path, filename) = os.path.split(fullRemotePath)
        if AlreadyThere(filename, fileType): return   # check if the file already exists in the tree... if it does no need to update it
        itemId = self.tree.AppendItem(self.branches[fileType], filename)
        self.tree.SetPyData(itemId, Node(filename, fileType, path, True, False))
      

    def MakeBranch(self, fType, path):
        if not self.branches.has_key(fType):
            self.branches[fType] = self.tree.AppendItem(self.root, fType)
            self.tree.SetItemTextColour(self.branches[fType], dirTextColor)
            self.tree.SetPyData(self.branches[fType], Node("", fType, path, False, fType=="favorites", True))


    def MakeFavoritesBranch(self, fType):
        if not self.favoritesBranches.has_key(fType):
            self.favoritesBranches[fType] = self.tree.AppendItem(self.branches["favorites"], fType)
            self.tree.SetItemTextColour(self.favoritesBranches[fType], dirTextColor)
            self.tree.SetPyData(self.favoritesBranches[fType], Node(fType, fType, "", False, True, True))


    def RefreshTree(self, event=None):  #does not refresh favorites
        self.tree.CollapseAndReset(self.root)
        self.MakeTree()
        self.currentImage.SetBitmap(self.no_preview)


    def OnSearch(self, event):
        self.searchMsg.SetLabel("")
        if event.GetEventType() == wx.EVT_TEXT_ENTER.evtType[0] or event.GetEventType() == wx.EVT_BUTTON.evtType[0]:
            selectedItemId = self.tree.GetSelection()
            self._alreadyFound.append(selectedItemId)
        elif event.GetEventType() == wx.EVT_TEXT.evtType[0]:
            self._alreadyFound = []  #if the user changed the search string, restart the search

        # start the search
        resultItemId = self.SearchAndSelect(self.root, self.searchField.GetValue())
        if resultItemId != None and resultItemId.IsOk():
            self.tree.SelectItem(resultItemId, True)
        else:
            if resultItemId == None:
                self.searchMsg.SetLabel("")  # display nothing for the empty string
            else:
                self.searchMsg.SetLabel("Nothing Found.")


        # search through a tree recursively
    def SearchAndSelect(self, root, searchString):
        if len(searchString) == 0:
            self._alreadyFound = []
            return  #dont search for an empty string

        (itemId, cookie) = self.tree.GetFirstChild( root )
        while itemId.IsOk():
            itemData = self.tree.GetPyData(itemId)
            if itemData.GetType() == "favorites":  #skip the favorites
                (itemId, cookie) = self.tree.GetNextChild( root, cookie )
                continue
            if searchString.lower() in itemData.GetName().lower():
                if not itemId in self._alreadyFound:
                    self._alreadyFound.append(itemId)
                    return itemId 
            if self.tree.ItemHasChildren( itemId ):
                itemId = self.SearchAndSelect( itemId, searchString )
                if itemId.IsOk():
                    return itemId
            (itemId, cookie) = self.tree.GetNextChild( root, cookie )

        return itemId

            
    def OnBeginDrag(self, evt):
        self.draggedItemId = evt.GetItem()
        itemData = self.tree.GetPyData(self.draggedItemId)
        if itemData != None and itemData.IsFile() and not itemData.IsFavorite():
            evt.Allow()
            

    def OnEndDrag(self, evt):
        onItemId = evt.GetItem() #, flags = self.tree.HitTest(evt.GetPoint())
        if onItemId.IsOk():  #make sure that we dropped it on a legal spot
            onItemData = self.tree.GetPyData(onItemId)
            draggedItemData = self.tree.GetPyData(self.draggedItemId)
            draggedFilename = draggedItemData.GetName()
            draggedType = draggedItemData.GetType()

            if onItemData == None:  # files can't be dropped on the root nor can we drag favorites
                return

            elif onItemData.IsFavorite():   # we dropped it in the "favorites" part
                if prefs.favorite_files.AlreadyFavorite(draggedItemData): return # check if the file already exists in favs
                favItemId = self.tree.AppendItem(self.favoritesBranches[draggedType], draggedFilename)
                favItemData = copy.copy(draggedItemData)
                favItemData.SetFavorite(True)  #we use the same data from the original file but mark it as a favorite
                self.tree.SetPyData(favItemId, favItemData)
                prefs.favorite_files.AddFavorite(favItemData)
            
            elif onItemData.GetType() == draggedType:  # dropped it in a non-favorites area
                parentId = onItemId
                newFullPath = os.path.join(onItemData.GetFullPath(), draggedFilename)
                oldFullPath = draggedItemData.GetFullPath()
                if onItemData.IsFile():  #if we dropped it on a file pretend it was the directory above
                    parentId = self.tree.GetItemParent(onItemId)
                    parentData = self.tree.GetPyData(parentId)
                    newFullPath = os.path.join( parentData.GetFullPath(), draggedFilename)
                if newFullPath == oldFullPath:  #if the file was dropped within the same dir, do nothing
                    return
                try:
                    res = self.server.MoveFile(oldFullPath, newFullPath)
                except socket.error:
                    Message("Error moving file. No connection with the File Server.", "No Connection")
                else:
                    if res != True:
                        Message("Error occured while moving the file. Does the file exist?", "Move Unsuccessful")
                        self.RefreshTree()
                    else: # move successful, rearrange the tree
                        self.MoveNode(parentId, self.draggedItemId, newFullPath, draggedType)


    def MoveNode(self, parentId, oldItemId, newFullPath, fileType):
        self.tree.Delete(oldItemId)  #first delete the old node
        (path, filename) = os.path.split(newFullPath)
        itemId = self.tree.AppendItem(parentId, filename)
        self.tree.SetPyData(itemId, Node(filename, fileType, path, True, False))


    def LoadFavorites(self):  #must be done after the favorites branches have been built already
        favoriteFilesData = prefs.favorite_files.GetFavorites()
        if not favoriteFilesData: return

        # insert the nodes into the tree
        for type, items in favoriteFilesData.iteritems():
            if self.favoritesBranches.has_key(type):
                for itemData in items:
                    itemId = self.tree.AppendItem(self.favoritesBranches[type], itemData.GetName())
                    self.tree.SetPyData(itemId, itemData)


    def OnMouseEnter(self, evt):
        if evt.Dragging():
            self.__DestroyPopup()
            evt.Skip()


    def OnMouseLeave(self, evt):
        self.__DestroyPopup()


    def __DestroyPopup(self):
        if hasattr(self, "pd"): self.pd.Destroy(); del self.pd
        self.currentItemId = None


        # when we move a mouse over a file, display its metadata
    def OnMouseMove(self, evt):
        if evt.Dragging():
            evt.Skip()
        pt = evt.GetPosition()
        itemId, flags = self.tree.HitTest(pt)
        if not itemId.IsOk():
            self.__DestroyPopup()
            return
        self.ShowMetadata(itemId)

        # shows the metadata for a given itemId inside a popup
    def ShowMetadata(self, itemId):
        itemData = self.tree.GetPyData(itemId)
        
        if itemData == None:
            self.__DestroyPopup()
            return    #don't do anything if the user moved over the root

        # get the metadata now if it's a file we are over
        if itemData.IsFile():
            if self.currentItemId == itemId:
                return   #we already have a tooltip for this one
            self.__DestroyPopup()
            self.currentItemId = itemId
            metadata = self.server.GetMetadata(itemData.GetFullPath())
            if metadata:
                md = itemData.GetName()+"\n"+metadata
                self.pd = PopupDialog(self.tree)#popup.PopupControl(self.tree)#, metadata)
                text = wx.StaticText(self.pd, wx.ID_ANY, md, pos=(2,2))# etadata.lstrip())
                self.pd.SetContent(text)
                self.pd.Display()
            else:
                self.__DestroyPopup()


    def OnRightClick(self, evt):
        pt = evt.GetPosition()
        itemId, flags = self.tree.HitTest(pt)
        itemData = self.tree.GetPyData(itemId)
        self.tree.SelectItem(itemId)
        
        if itemData == None: 
            return    #don't do anything if the user right-clicked on the root

        # show the menu now
        menu = wx.Menu()
        if itemData.IsFile():                     # FILES
            if itemData.IsFavorite():
                delMenuItem = menu.Append(1, "Remove From Favorites")
                menu.Bind(wx.EVT_MENU, self.OnRemoveFavoritesFile, delMenuItem)
            else:
                delMenuItem = menu.Append(1, "Delete File")
                menu.Bind(wx.EVT_MENU, self.OnDeleteFile, delMenuItem)
        elif not itemData.IsFile() and not itemData.IsFavorite():        # FOLDERS
            newMenuItem = menu.Append(1, "New Folder")
            menu.Bind(wx.EVT_MENU, self.OnNewFolder, newMenuItem)
            if not itemData.IsBranch():           # you cant delete branches
                delMenuItem = menu.Append(2, "Delete Folder")
                menu.Bind(wx.EVT_MENU, self.OnDeleteFolder, delMenuItem)
        
        self.tree.PopupMenu(menu)


    def OnNewFolder(self, evt):
        newFolderName = wx.GetTextFromUser("New Folder Name: ", "New Folder")
        newFolderName = "_".join(newFolderName.split())
        if newFolderName == "":
            return
        itemId = self.tree.GetSelection()
        itemData = self.tree.GetPyData(itemId)
        itemType = itemData.GetType()
        newFullPath = os.path.join( itemData.GetFullPath(), newFolderName)
        try:
            res = self.server.NewFolder(newFullPath)
        except socket.error:
            Message("Error creating the directory. No connection with the File Server.", "No Connection")
        else:
            if res != True:
                Message("Error occured while creating the directory. Does the path leading to it still exist?", "Directory Creation Unsuccessful")
                self.RefreshTree()
            else: # make successful, rearrange the tree
                self.MakeFolder(itemId, newFullPath, itemType)


    def MakeFolder(self, parentId, newFullPath, fileType):
        (path, name) = os.path.split(newFullPath)
        itemId = self.tree.AppendItem(parentId, name)
        self.tree.SetItemTextColour(itemId, dirTextColor)
        self.tree.SetPyData(itemId, Node(name, fileType, path, False, False))
        self.tree.SelectItem(itemId)
        

    def OnDeleteFolder(self, evt):
        itemId = self.tree.GetSelection()
        itemData = self.tree.GetPyData(itemId)
        fullPath = itemData.GetFullPath()
        if ChoiceDialog("This will delete the DIR and all the FILES in it permanently from the File Server which means that no " \
                        "user will be able to access it/them anymore.\nAre you sure you want to continue?", \
                        "Deletion Confirmation"):
            try:
                res = self.server.DeleteFolder(fullPath)
            except socket.error:
                Message("Error deleting the directory. No connection with the File Server.", "No Connection")
            else:
                if res != True:
                    Message("Error occured while deleting the directory. Are the permissions correct?", "Directory Deletion Unsuccessful")
                    self.RefreshTree()
                else: # delete successful, rearrange the tree
                    self.tree.Delete(itemId)


    def OnDeleteFile(self, evt):
        itemId = self.tree.GetSelection()
        itemData = self.tree.GetPyData(itemId)
        if itemData == None:
            return  #the user clicked on one of the root folders which are not modifiable
        if ChoiceDialog("This will delete the file permanently from the File Server which means that no " \
                        "user will be able to access it anymore.\nAre you sure you want to continue?", \
                        "Deletion Confirmation"):
            try:
                res = self.server.DeleteFile(itemData.GetFullPath())
            except socket.error:
                Message("Error deleting file. No connection with the File Server.", "No Connection")
            else:
                if res != True:
                    Message("Error occured while deleting file. Does the file exist?", "Delete Unsuccessful")
                    self.RefreshTree()
                else:  # delete that node from the tree
                    self.tree.Delete(itemId)
                    self.currentImage.SetBitmap(self.no_preview)


    def OnRemoveFavoritesFile(self, event):
        itemId = self.tree.GetSelection()
        prefs.favorite_files.RemoveFavorite(self.tree.GetPyData(itemId))  # you must do this before deleting the item
        self.tree.Delete(itemId)
        #self.SaveFavorites()



    def OnShowButton(self, evt):
        itemId = self.tree.GetSelection()
        if not itemId.IsOk():
            return
        itemData = self.tree.GetPyData( itemId )
  

        # check if the user double clicked on a parent and not a leaf
        # if so, expand/collapse the parent
        if itemData == None or not itemData.IsFile():
            if self.tree.IsExpanded(itemId):
                self.tree.Collapse(itemId)
            else:
                self.tree.Expand(itemId)
            return

        # show the file (check if it exists first)
        try:
            c = wx.BusyCursor()
            fileInfo = self.server.GetFileInfo(itemData.GetName(), 0, itemData.GetPath())
            del c
            if not fileInfo:   #file not supported
                Message("File type <"+os.path.splitext(itemData.GetName())[1]+"> not supported.", "Invalid File Type")
                return
            (fileType, size, fullPath, appName, params, fileExists) = fileInfo
            if not fileExists:  #file doesnt exist
                Message("File <"+itemData.GetName()+"> does not seem to exist anymore.\nTry closing the library and opening it again in order to refresh the file list.", "Invalid Filename")
                return
            self.ShowFile(fileType, fullPath, appName, params, size)
        except socket.error:
            del c
            Message("Unable to show file. There is no connection with the File Library.", "No connection")
        

    def OnSelChanged(self, evt):
        class PreviewTimer(wx.Timer):
            def __init__(self, action, itemId):
                self.action = action
                self.itemId = itemId
                wx.Timer.__init__(self)
            def Notify(self):
                c = wx.BusyCursor()
                try:
                    self.action(self.itemId)
                except socket.error, xmlrpclib.ProtocolError:
                    pass   # no preview retrieved... oh well
                del c

        def GetPreview(itemId):
            itemData = self.tree.GetPyData(itemId)
            if itemData == None or not itemData.IsFile():
                self.showBtn.Disable()
                return  # do nothing if a leaf wasn't selected
            self.showBtn.Enable()

            # no preview if we didnt select an image
            #if itemData.GetType() != "image":
            #    self.currentImage.SetBitmap(self.no_preview)
            #    return

            # get the preview from the server (and get other info like the size of the image)
            self.currentImage.SetBitmap(self.retrieving_preview)
            try:
                preview = self.server.GetPreview(itemData.GetFullPath(), self.previewSize)
            except:
                self.currentImage.SetBitmap(self.no_preview)
                return
            if not preview:  #preview retrieval failed for some reason
                self.currentImage.SetBitmap(self.no_preview)
            else:
                (previewData, isBinary) = preview
                if isBinary:
                    stream=wx.InputStream(cStringIO.StringIO(base64.decodestring(previewData))) #make a stream out of the image
                    im = wx.ImageFromStream(stream)
                    im.Rescale(PREVIEW_SIZE[0], PREVIEW_SIZE[1])
                else:
                    im = wx.EmptyImage(PREVIEW_SIZE[0], PREVIEW_SIZE[1])
                    im.SetData(base64.decodestring(previewData))

                if im.Ok():
                    self.currentImage.SetBitmap(im.ConvertToBitmap())
                else:
                    self.currentImage.SetBitmap(self.no_preview)
                
        # show the preview on a timer (done so that search-as-you-type is faster)
        if hasattr(self, "_timer"):
            if self._timer.IsRunning():
                self._timer.Stop()
        self._timer = PreviewTimer(GetPreview, evt.GetItem())
        self._timer.Start(300, True)


        # run the show file in a thread
    def ShowFile(self, fileType, fullPath, appName, params, size):
        t = Thread(target=self.DoShowFile, args=(fileType, fullPath, appName, params, size))
        t.start()

    
    def DoShowFile(self, fileType, fullPath, appName, params, size):
        prepResult = self.server.PrepareFile(fullPath, self.sageHost)
        bridge = self.shareCheckBox.GetValue()

        if self.dxtCheckBox.GetValue():
            useDXT = ""
        else:
            useDXT = "-show_original"
        
        if not prepResult:
            Message("Unable to show file because the transfer of file from the FileServer to the SAGE machine failed.", "Error")
            return
        else:
            fullPath = prepResult #the path was returned by the FileGrabber

        if fileType == "image":
            totalDisplayWidth = self.canvas.tiledDisplayWidth
            totalDisplayHeight = self.canvas.tiledDisplayHeight
            imageWidth = size[0]
            imageHeight = size[1]
            imageAspectRatio = float( imageWidth/imageHeight )

            # figure out if the image is going to fit, if not, reposition and resize it as necessary
            # first resize if necessary
            widthRatio = float(totalDisplayWidth / float(imageWidth))
            heightRatio = float(totalDisplayHeight / float(imageHeight))
            if widthRatio < 1 or heightRatio < 1:
                if widthRatio > heightRatio:   #we'll resize based on height
                    resizeRatio = heightRatio
                else:
                    resizeRatio = widthRatio   #we'll resize based on width
                imageWidth = int(imageWidth * resizeRatio)
                imageHeight = int(imageHeight * resizeRatio)         

            # now reposition
            cornerX = 0
            cornerY = 0
            
            res = self.sageGate.executeApp(appName ,pos=(cornerX, cornerY), size=(imageWidth,imageHeight), optionalArgs = fullPath+" "+str(imageWidth)+" "+str(imageHeight)+" "+useDXT+" "+params, useBridge=bridge)
        elif fileType == "video" or fileType=="pdf":
            res = self.sageGate.executeApp(appName, optionalArgs=params+" "+fullPath, useBridge=bridge)
        else:  #for other types
            res = self.sageGate.executeApp(appName, optionalArgs=fullPath+" "+params, useBridge=bridge)

        if res == -1:
            Message("Application not started. Either application failed, the application launcher is not running or the application  <<"+appName+">>  is not configured in application launcher.", "Application Launch Failed")
            


### shows the dialog with a list of file libraries that you can
### connect to. ShowDialog returns the selected library (Actually it's
### IP address)
class LibraryChoiceDialog(wx.Dialog):
    def __init__(self, parent):
        wx.Dialog.__init__(self, parent, wx.ID_ANY, "Choose File Library", style=wx.CAPTION | wx.CLOSE_BOX | wx.RESIZE_BORDER)
        self.SetBackgroundColour(dialogColor)
        self.SetForegroundColour(wx.WHITE)
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)
        self.horSizer1 = wx.BoxSizer(wx.HORIZONTAL)
        self.SetMinSize((100, 200))
        self.__MakeControls()
        


    def __MakeControls(self):
        self.ID_ADD = 500
        self.ID_DEL = 501
        self.ID_UPLOAD = 502
        self.ID_SHOW = 503
        
        self.libList = wx.ListBox(self, wx.ID_ANY, choices=prefs.fileLib.GetLibraryList(), style=wx.LB_SINGLE)
        self.libList.SetStringSelection( prefs.fileLib.GetDefault()[0] )
        self.libList.Bind(wx.EVT_LISTBOX_DCLICK, self.__OnDoubleClick)
        #self.libList.SetTextColour(wx.WHITE)
        #self.libList.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        #self.libList.SetBackgroundColour(appPanelColor)        
        #self.libList.SetBackgroundColour(appPanelColor)
        #self.libList.SetForegroundColour(wx.WHITE)
        
        addBtn = wx.Button(self, self.ID_ADD, "Add", size=(60,20))
        smallFont = addBtn.GetFont()
        smallFont.SetPointSize(smallFont.GetPointSize() - smallFont.GetPointSize()/5)
        addBtn.SetFont(smallFont)
        delBtn = wx.Button(self, self.ID_DEL, "Remove", size=(60,20))
        delBtn.SetFont(smallFont)

        self.horSizer1.Add(addBtn, 0, wx.ALIGN_CENTER | wx.RIGHT, border=10)
        self.horSizer1.Add(delBtn, 0, wx.ALIGN_CENTER )
        self.mainSizer.Add(self.libList, 1, wx.ALIGN_CENTER | wx.ALIGN_TOP | wx.ALL | wx.EXPAND, border=20)
        self.mainSizer.Add(self.horSizer1, 0, wx.ALIGN_CENTER | wx.TOP | wx.BOTTOM, border=5)
        self.mainSizer.Add(self.CreateButtonSizer(wx.OK | wx.CANCEL), 0, wx.ALIGN_CENTER | wx.ALL, border=15)
        self.SetSizer(self.mainSizer)
        self.Fit()

        self.Bind(wx.EVT_BUTTON, self.__OnAdd, id=self.ID_ADD)
        self.Bind(wx.EVT_BUTTON, self.__OnRemove, id=self.ID_DEL)
        self.Bind(wx.EVT_BUTTON, self.__OnUpload, id=self.ID_UPLOAD)
        self.Bind(wx.EVT_BUTTON, self.__OnShow, id=self.ID_SHOW)
        

    def ShowDialog(self):
        res = self.ShowModal()
        if res==wx.ID_OK or res==self.ID_UPLOAD:
            libName = self.libList.GetStringSelection()
            libHost = prefs.fileLib.GetLibraryIP( libName )
            prefs.fileLib.SetDefault( libName, libHost )
            self.Destroy()
            return (libName, libHost)
        else:
            self.Destroy()
            return False


    def __OnDoubleClick(self, evt):
        self.EndModal(self.ID_UPLOAD)


    def __OnUpload(self, evt):
        self.EndModal(self.ID_UPLOAD)


    def __OnShow(self, evt):
        self.EndModal(self.ID_SHOW)

            
    def __OnAdd(self, evt):
        dlg = wx.Dialog(self, wx.ID_ANY, "Add New Library")
        sizer = wx.BoxSizer(wx.VERTICAL)
        nameLabel = wx.StaticText(dlg, wx.ID_ANY, "Library name (label):")
        nameField = wx.TextCtrl(dlg, wx.ID_ANY)
        ipLabel = wx.StaticText(dlg, wx.ID_ANY, "Library IP address (or hostname):")
        ipField = wx.TextCtrl(dlg, wx.ID_ANY)

        sizer.Add(nameLabel, 0, wx.ALIGN_LEFT | wx.ALL, border = 10)
        sizer.Add(nameField, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT | wx.EXPAND, border = 10)
        sizer.Add(ipLabel, 0, wx.ALIGN_LEFT | wx.ALL, border = 10)
        sizer.Add(ipField, 0, wx.ALIGN_LEFT | wx.LEFT | wx.BOTTOM | wx.RIGHT | wx.EXPAND, border = 10)
        sizer.Add(dlg.CreateButtonSizer(wx.OK|wx.CANCEL), 0, wx.ALIGN_CENTER|wx.ALL, border=10)
        dlg.SetSizer(sizer)
        dlg.Fit()
        if dlg.ShowModal() == wx.ID_OK:
            prefs.fileLib.AddLibrary(ipField.GetValue(), nameField.GetValue())
            self.libList.Set(prefs.fileLib.GetLibraryList())
            self.libList.SetStringSelection(nameField.GetValue())
            dlg.Destroy()
        else:
            dlg.Destroy()
        

    def __OnRemove(self, evt):
        selection = self.libList.GetStringSelection()
        if selection != "":
            prefs.fileLib.RemoveLibrary(selection)
            self.libList.Set(prefs.fileLib.GetLibraryList())
        return



class PopupDialog(wx.Dialog):
    def __init__(self,parent,content = None):
        wx.Dialog.__init__(self,parent,wx.ID_ANY,'', style = wx.NO_BORDER | wx.STAY_ON_TOP)

        self.ctrl = parent
        self.win = wx.Window(self,-1,pos = (0,0),style = wx.NO_BORDER)

        if content:
            self.SetContent(content)


    def SetContent(self,content):
        self.content = content
        self.content.Reparent(self.win)
        self.win.SetBackgroundColour(dialogColor)#102, 153, 153))#appPanelColor)
        self.content.SetForegroundColour(wx.WHITE)
        self.content.Show(True)
        sz = self.content.GetSize()
        self.win.SetClientSize((sz[0]+6, sz[1]+6))
        self.SetSize(self.win.GetSize())
        

    def Display(self):
        class InfoTimer(wx.Timer):
            def __init__(self, action):
                self.action = action
                wx.Timer.__init__(self)
            def Notify(self):
                try:
                    self.action()
                except socket.error, xmlrpclib.ProtocolError:
                    pass   # no info retrieved... oh well
            
        def __ActuallyShow():
            pos = wx.GetMousePosition()
            posX = pos[0]-(30+self.GetSize()[0])
            posY = pos[1]
            self.Move((posX, posY))
            self.Show()
        
        # show the info on a timer 
        if hasattr(self, "_timer"):
            if self._timer.IsRunning():
                self._timer.Stop()
        self._timer = InfoTimer(__ActuallyShow)
        self._timer.Start(500, True)

    

        
    
class Node:
    def __init__(self, name, fileType, path, isFile, isFavorite, isBranch=False):
        self.name = name
        self.fileType = fileType
        self.path = path
        self.isFavorite = isFavorite
        self.isFile = isFile
        self.isBranch = isBranch
    
    def GetName(self):
        return self.name

    def GetPath(self):
        return self.path

    def GetFullPath(self):
        return os.path.join(self.path, self.name)

    def GetType(self):
        return self.fileType

    def IsFavorite(self):
        return self.isFavorite

    def SetFavorite(self, fav):
        self.isFavorite = fav

    def IsFile(self):
        return self.isFile

    def IsBranch(self):
        return self.isBranch

    def __eq__(self, other):
        if hasattr(other, "GetName"):
            if self.GetName() == other.GetName():
                return True
            else:
                return False
        return False
    
    def __ne__(self, other):
        if hasattr(other, "GetName"):
            if self.GetName() != other.GetName():
                return True
            else:
                return False
        return True
    
    
