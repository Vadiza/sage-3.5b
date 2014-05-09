#!/usr/bin/env python
############################################################################
#
# -= FileViewer =-
#
# FileServer - receives, provides and organizes multimedia files for use with SAGE
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
# Direct questions, comments etc about FileViewer to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#
############################################################################
 
import os, base64, os.path, socket, shutil, sys, SocketServer, wx, stat, urllib
import re
from SimpleXMLRPCServer import *
import subprocess as sp
from threading import Thread
from time import asctime, time, sleep
import makeThumbs

# miscellaneous stuff (3rd party supporting tools)
sys.path.append("misc")  #so that we can import packages from "misc" folder
from imsize import imagesize  # reads the image header and gets the size from it
import mmpython
import countPDFpages

# a shortcut
opj = os.path.join

# for loading the sagePath helper file
sys.path.append( opj(os.environ["SAGE_DIRECTORY"], "bin" ) )
from sagePath import getUserPath, getPath


## some globals defining the environment
SCRIPT_PATH = sys.path[0]
CONFIG_FILE = getPath("fileServer", "fileServer.conf")
CACHE_DIR = getUserPath("fileServer", "file_server_cache") 
FILES_DIR = opj(SCRIPT_PATH, "file_library")
REDIRECT = False
THUMB_DIR = opj(FILES_DIR, "thumbnails")
RUN_SERVER = True

## holds information about the types that we support (read from the config file)
dirHash = {}
viewers = {}
types = {}


def ParseConfigFile():
    global FILES_DIR
    global THUMB_DIR
    global dirHash
    global types
    global viewers

    # reinitialize everything
    FILES_DIR = ""
    THUMB_DIR = ""
    dirHash = {}
    types = {}
    viewers = {}

    # read the config file
    f = open(CONFIG_FILE, "r")
    for line in f:
        line = line.strip()
        if line.startswith("FILES_DIR"):
            FILES_DIR = line.split("=")[1].strip()
            if not os.path.isabs(FILES_DIR):
                FILES_DIR = getUserPath("fileServer", FILES_DIR)
            FILES_DIR = os.path.realpath(FILES_DIR)  #expand any symbolic links in the library directory
            THUMB_DIR = opj(FILES_DIR, "thumbnails")
        elif line.startswith("type:"):
            line = line.split(":",1)[1]
            (type, extensions) = line.split("=")
            types[type.strip()] = extensions.strip().split(" ")
            dirHash[type.strip()] = opj(FILES_DIR, type.strip())
        elif line.startswith("app:"):
            line = line.split(":", 1)[1].strip()
            (type, app) = line.split("=")
            tpl = app.strip().split(" ", 1)
            if len(tpl) == 1:  params = ""
            else:  params = tpl[1].strip()
            app = tpl[0].strip()
            viewers[type.strip()] = (app, params)
    f.close()


    # create the folders first if they dont exist
    if not os.path.isdir(FILES_DIR):
        os.makedirs(FILES_DIR)
    if not os.path.isdir(CACHE_DIR):
        os.makedirs(CACHE_DIR)
    for fileType, typeDir in dirHash.items():
        if not os.path.isdir( ConvertPath(typeDir) ):
            os.makedirs(typeDir)
        if not os.path.isdir( ConvertPath( opj(typeDir, "Trash")) ):
            os.makedirs( opj(typeDir, "Trash") )
        if fileType == "image" and not os.path.isdir( ConvertPath( opj(typeDir, "screenshots")) ):
            os.makedirs( opj(typeDir, "screenshots") )
    if not os.path.isdir(THUMB_DIR):
        os.makedirs(THUMB_DIR)



# convert the path between the systems (Windows vs. Linux/Mac)
def ConvertPath(path):
    return apply(opj, tuple(path.split('\\')))





import traceback as tb
from SocketServer import ThreadingTCPServer, StreamRequestHandler



class Listener(ThreadingTCPServer):
    """ the main server listening for connections from HWCapture """
    
    allow_reuse_address = True
    request_queue_size = 2
    
    def __init__(self, port, onMsgCallback, fileServer):
        self.onMsgCallback = onMsgCallback
        self.fileServer = fileServer
        ThreadingTCPServer.__init__(self, ('', int(port)), SingleFileUpload)
        self.socket.settimeout(0.5)


    def serve_forever(self):
        while RUN_SERVER:
            try:
                self.handle_request()
            except socket.timeout:
                pass
            except KeyboardInterrupt:
                break
            except:
                WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
                break
 

class SingleFileUpload(StreamRequestHandler):      
    """ one of these per file upload """

    
    def handle(self):
        """ this is where we parse the incoming messages """
        self.request.settimeout(0.5)  # so that we can quit properly

        # read the header
        header = self.rfile.readline().strip()
        if len(header) == 0:
            return
        (fileName, previewWidth, previewHeight, fileSize) = header.strip().split()
        previewSize = (int(previewWidth), int(previewHeight))
        fileSize = int(fileSize)
        
        # make sure the filename is ok
        fileName = os.path.basename(fileName)   # strip any directory and just keep the filename (for security reasons)
        fileType = self.server.fileServer.GetFileType(fileName)
        if not fileType:
            return #dont allow upload of anything that doesn't have an extension
        
        # make the file
        fullPath = opj(dirHash[fileType], fileName)
        try:
            f=open(fullPath, "wb")
        except:
            WriteLog("Cannot create file (have write permissions?):" + filePath)
            return

        bytesRead = 0
        while RUN_SERVER and bytesRead < fileSize:
            try:
                line = self.rfile.readline()
                if not line:   # readline doesn't fail when socket is closed... it returns an empty string
                    f.close()
                    os.remove(fullPath)
                    break
                    
                bytesRead+=len(line)
                if bytesRead > fileSize:
                    line = line.rstrip()   # remove the newline char at the end of transmission
                f.write(line)
                
            except socket.timeout:
                continue
            except:
                WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
                f.close()
                os.remove(fullPath)
                break
        else:
            f.flush()
            f.close()

            # tell the client that all went well
            self.wfile.write("1")  
            
            # make the preview if necessary and everything went well
            self.server.onMsgCallback(fileName, previewSize)








class FileLibrary:
    def __init__(self):
        ParseConfigFile()
        
        # keep a list of all files around so that we dont re-upload an existing file
        self.__IndexFiles()

        # start the server that will accept the file data
        self.fServer = Listener(8802, self.__MakePreview, self)
        fServerThread = Thread(target=self.fServer.serve_forever)
        fServerThread.start()


        ### used by clients to test the connection with the server
    def TestConnection(self):
        return (FILES_DIR, 0)


    def GetLibraryPath(self):
        return FILES_DIR


         ### gets the file info:
        ### full path, file type, application info, size (for images...), file size
    def GetFileInfo(self, filename, fileSize=-1, path=None):
        #ParseConfigFile()
        try:
            fileType = self.GetFileType(filename)
            size = (-1, -1)
            fileExists = False
            self.__IndexFiles()
            
            if fileType:  #is file even supported
                appName = viewers[fileType][0]
                params_re = viewers[fileType][1]

                # match/replace %f by the filename (cna be useful to build command lines)
		params = re.sub("%f", filename, params_re)

                if path == None:  # the user dropped a new file on there

                    # if we are uploading a web link, the size will be -1
                    # but we may already have this file in the cache so use that
                    if fileSize == -1 and filename in self.__cachedFiles:
                        cachedPath = self.__cachedFiles[filename]
                        fileSize = os.stat(cachedPath).st_size
                        fullPath = self.FindFile(filename, fileSize)
                        if not fullPath:  # the file isnt in the library, so use the one from the cache
                            shutil.move(cachedPath, opj(dirHash[fileType], filename))
                        else:   # the file is in the library already so remove the cached one
                            os.remove(cachedPath)
                    else:
                        fullPath = self.FindFile(filename, fileSize)

                    #if the file was found in the library, try to get its size if its an image
                    if fullPath:  
                        if fileType == "image":   #if the file is an image, return its size
                            try:
                                imsize = imagesize(fullPath)
                                size = (imsize[1], imsize[0])
                            except:
                                size = (-1,-1)
                        
                        fileExists = True
                    else:
                        fullPath = opj(self.__GetFilePath(fileType), filename)  # return a default path if not passed in
                        
                elif not self.__LegalPath(path):
                    return False
                else:             # the user is trying to show an existing file
                    path = ConvertPath(path)
                    fullPath = opj(path, filename)
                    ## get the image size... FIX later (remove)
                    if os.path.isfile(fullPath):
                        fileExists = True
                        if fileType == "image":   #if the file is an image, return its size
                            try:
                                imsize = imagesize(fullPath)
                                size = (imsize[1], imsize[0])
                            except:
                                size = (-1,-1)
                return (fileType, size, fullPath, appName, params, fileExists) 
            else:
                return False  #file type not supported
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


        ### gets the metadata information about the file
    def GetMetadata(self, fullPath):
        fullPath = ConvertPath(fullPath)
        if not self.__LegalPath(fullPath):
            return False

        try:
            fileType = self.GetFileType(fullPath)
            if not fileType:
                return False
            elif fileType == "pdf":    # for pdfs
                return "Pages: "+str(countPDFpages.getPDFPageCount(fullPath))
            else:                      # for all other types
                metadata = mmpython.parse(fullPath)
                if metadata:   # metadata extracted successfully
                    return unicode(metadata).encode('latin-1', 'replace')
                else:
                    return False
        except:
            WriteLog( str(sys.exc_info()[0])+" "+str(sys.exc_info()[1]) )
            return False
        

        ### construct a hash of all the files keyed by the directory in which the files
        ### are and return it
    def __IndexFiles(self):
        self.__indexedFiles = {}  #key=filename, value=directory
        for root, dirs, files in os.walk(FILES_DIR):
            if root.endswith("Trash"): continue   # dont cache the trash...
            for name in files:       
                self.__indexedFiles[name] = opj(root, name)

        self.__cachedFiles = {}  #key=filename, value=directory
        for root, dirs, files in os.walk(CACHE_DIR):
            for name in files:       
                self.__cachedFiles[name] = opj(root, name)


        ### construct a hash of all the files keyed by the directory in which the files
        ### are and return it
    def GetFiles(self):
        #ParseConfigFile()
        numDirs = 0
        numFiles = 0
        
        fileHash = {}
        for fileType in dirHash.iterkeys():
            fileHash[fileType] = self.__TraverseDir( self.__GetFilePath(fileType), fileType )
        return fileHash
    

        ### deletes a folder and all files and dirs in it
    # FIX to delete the thumbnails for all the images in this folder
    def DeleteFolder(self, fullPath):
        try:
            if self.__LegalPath(fullPath):  # prevent users from deleting anything
                shutil.rmtree(ConvertPath(fullPath))
                return True
            else:
                return False
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


        ### makes a new folder
    def NewFolder(self, fullPath):
        try:
            if self.__LegalPath(fullPath):  # prevent users from deleting anything
                os.mkdir(ConvertPath(fullPath))
                self.__SetWritePermissions(ConvertPath(fullPath))
                return True
            else:
                return False
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


        ### move the file to Trash
    def DeleteFile(self, fullPath):
        try:
            fullPath = ConvertPath(fullPath)
            if not self.__LegalPath(fullPath):  # restrict the users to the FILES_DIR folder
                return False
            fileType = self.GetFileType(fullPath)
            if not fileType:  # no extension so don't allow deletion
                return False

            trashPath = opj( opj(self.__GetFilePath(fileType), "Trash"), os.path.split(fullPath)[1] )
            self.MoveFile( fullPath, trashPath )
            return True
        
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


        ### deletes the file from the library (also deletes it's thumbnail preview if it exists)
    def DeleteFilePermanently(self, fullPath):
        try:
            fullPath = ConvertPath(fullPath)
            if not self.__LegalPath(fullPath):  # restrict the users to the FILES_DIR folder
                return False
            fileType = self.GetFileType(fullPath)
            if not fileType:  # no extension so don't allow deletion
                return False

            previewPath = self.__GetPreviewName(fullPath)
            if os.path.isfile(fullPath): # prevent users from deleting anything 
                os.remove( fullPath )      #remove the file itself
                dxtFilePath = fullPath.rstrip(os.path.splitext(fullPath)[1])+".dxt"
                if os.path.isfile(dxtFilePath):
                    os.remove( dxtFilePath )  # remove the dxt file if it exists
            if os.path.isfile( previewPath ):
                os.remove( previewPath )  #remove its preview thumbnail
            return True
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


        ### moves the file from one directory to another
    def MoveFile(self, oldFullPath, newFullPath):
        oldFullPath = ConvertPath(oldFullPath)
        newFullPath = ConvertPath(newFullPath)
        if not self.__LegalPath(oldFullPath, newFullPath):  # restrict the movement to the FILES_DIR
            return False
        if os.path.isfile(oldFullPath):
            try:
                shutil.move(oldFullPath, newFullPath)
                oldDxtFilePath = oldFullPath.rstrip(os.path.splitext(oldFullPath)[1])+".dxt"
                newDxtFilePath = newFullPath.rstrip(os.path.splitext(newFullPath)[1])+".dxt"
                if os.path.isfile(oldDxtFilePath):
                    shutil.move(oldDxtFilePath, newDxtFilePath)
                return True
            except:
                WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
                return False
        else:
            return False


        ### accepts a file encoded as a base64 string and puts it in
        ### the default directory for that file type
    def UploadFile(self, name, data, previewSize=(150, 150)):
        name = os.path.basename(name)   # strip any directory and just keep the filename (for security reasons)
        fileType = self.GetFileType(name)
        if not fileType:
            return False  #dont allow upload of anything that doesn't have an extension
        try:
            # write the file
            f=open( opj(dirHash[fileType], name), "wb")
            f.write( base64.decodestring(data) )
            f.close()
            del data

            self.__MakePreview(name, previewSize)

            return True
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


    def UploadLink(self, url):
        name = os.path.basename(url)   # strip any directory and just keep the filename (for security reasons)
        fileType = self.GetFileType(name)
        if not fileType:   
            return False

        try:
            if fileType == "video":  # online video... no extension
		# old way
                #downloadCmd = "python youtube-dl.py -q --title --no-part --no-overwrites --dir " +self.__GetFilePath(fileType)+" "+url
		# new way (luc)
		#    modified youtube-dl to not skip download when using get-filename
                downloadCmd = "youtube-dl -q --prefer-free-formats --get-filename --no-part --no-overwrites -o " + self.__GetFilePath(fileType) + "/%(title)s-%(id)s.%(ext)s " + url
                #print downloadCmd
                print "Downloading: ",  url
                dl = sp.Popen(downloadCmd.split(), bufsize=1, stdout=sp.PIPE)
                sleep(2)
                
                # the child process will print the filename so read it from the pipe
                msg = dl.stdout.readline()
                tt = 0.0
                while not msg and tt<5:
                    msg = dl.stdout.readline()
                    sleep(0.5)
                    tt += 0.5
                if tt > 5:
                    return False
                else:
		    # make it a legal filename
		    dlfilename = msg.strip()
		    filename = msg.strip()
		    filename = "_".join(filename.split())
		    filename = filename.replace("(", "_")
		    filename = filename.replace(")", "_")
		    try:
		    	print "Downloaded file: ", filename
		    	os.rename(dlfilename, filename)
                        return filename
		    except:
			return False
        
            else:   # image links...
                downloadPath = opj(CACHE_DIR, name)
                filename, headers = urllib.urlretrieve(url, downloadPath)
                
            return True
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False
        

        ### looks if there is a preview already saved... if not, create one and send it back
    def GetPreview(self, fullPath, previewSize=(150,150)):
        try:
            res = self.MakeThumbnail(fullPath)
            if res:
                return (self.__ReadFile(res), True)
            else:
                return False
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False

        
##         fullPath = ConvertPath(fullPath)
##         if not self.__LegalPath(fullPath):
##             return False
##         fileType = self.GetFileType(fullPath)

##         try:
##             if fileType != "image" or not os.path.isfile(fullPath):
##                 return False
##             previewPath = self.__GetPreviewName(fullPath)
##             wxImageType = self.__GetWxBitmapType(os.path.basename(fullPath))
##             if not os.path.isfile(previewPath):  #if the preview doesnt exist, make it 
##                 if wxImageType:               # is writing this file type supported?
##                     im = wx.Image(fullPath)  # read the original image
##                     if not im.Ok():   
##                         return False         # the image is probably corrupted
##                     preview = im.Rescale(previewSize[0], previewSize[1])        # resize it
##                     im.SaveFile(previewPath, wxImageType)    # save it back to a file
##                     self.__SetWritePermissions(previewPath)
##                 else:     #if writing is not supported, try and read it and resize it on the fly
##                     im = wx.Image(fullPath)  # read the original image
##                     if not im.Ok():   
##                         return False         # the image is probably corrupted
##                     preview = im.Rescale(previewSize[0], previewSize[1])        # resize it
##                     data = preview.GetData()
##                     return (base64.encodestring(data), False)   #return the pixels themselves (isBinary=False)
            
##             return (self.__ReadFile(previewPath), True)
##         except:
##             WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
##             return False


    def MakeThumbnail(self, fullPath):
        fullPath = ConvertPath(fullPath)
        if not self.__LegalPath(fullPath):
            return False
        fileType = self.GetFileType(fullPath)
        
        try:
            if not os.path.isfile(fullPath):
                return False

            previewPath = self.__GetPreviewName(fullPath)
            if not os.path.isfile(previewPath):  #if the preview doesnt exist, make it
                makeThumbs.makeThumbnail(fullPath, fileType, previewPath)
                if not os.path.isfile(previewPath):
                    return False
                
            return previewPath
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])))
            return False



        ### get the file ready for loading by the app on the "host" machine
    def PrepareFile(self, fullPath, host):
        if not self.__LegalPath(fullPath):
            return False
        
        # make the connection with the remote FileServer
        fileServer = xmlrpclib.ServerProxy("http://"+str(host)+":8800")

        try:
            # if the file exists already, there's no need to transfer it 
            filename = os.path.basename(fullPath)
            filePath = fileServer.FindFile(filename, os.stat(fullPath).st_size)
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False
        if filePath:
            return filePath  # if the file is already cached, just send the path back
        else:
            try:
                # file is not cached so read it and send it to the remote FileServer
                fileData = self.__ReadFile(fullPath)
            except:
                WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
                return False
            else:
                return fileServer.PutFile(os.path.basename(fullPath), fileData)
 

        ### gets the extension of the file that was passed in
    def GetFileType(self, thefile):
        try:
            filename = os.path.basename( thefile )
            extension = os.path.splitext(filename)[1].lower()
            if extension == "":
                return "video"
            fileType = False
            for fType in types.iterkeys():
                    if extension in types[ fType ]:
                        fileType = fType
            return fileType
        except:
            return False
        
    """
        ### does file exist in the cache already?? if not, someone will have to send it here
    def FileExists(self, fullPath, size):
        fullPath = ConvertPath(fullPath)
        if os.path.isfile(fullPath):  # if the file server is on the same machine then we can 
            return fullPath           # just read the file directly from the file_library
        filename = "_".join(os.path.basename(fullPath).split())
        if filename == "":  # if it's a directory
            return False
        path = os.path.abspath( opj(CACHE_DIR, filename) )
        if os.path.isfile(path) and os.stat(path).st_size==size:  #compare the sizes as well
            return path
        else:
            return False
    """

        ### called by the remote FileServer to upload the file 
    def PutFile(self, filename, data):
        try:
            filename = "_".join(os.path.basename(filename).split()) # strip the directory and keep just the name (security reasons)
            if filename == "":
                return False
            f=open( opj(CACHE_DIR, filename), "wb")
            f.write( base64.decodestring(data) )
            f.close()
            self.__SetWritePermissions(opj(CACHE_DIR, filename))

            del data
            return os.path.abspath( opj(CACHE_DIR, filename) )  #return the path of the file to be passed to the app
        except:
            del data
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            return False


        ### so that the server can be quit remotely
    def Quit(self, var):
        global RUN_SERVER
        RUN_SERVER = False
        return 1



        

#=======================================#
########    Auxiliary methods    ########


        ### constructs the preview name from the filename
        ### preview name is: filename+fileSize+original_extension
        ### (such as trees56893.jpg where 56893 is file size in bytes)
    def __GetPreviewName(self, fullPath):
        fileType = self.GetFileType(fullPath)

        fileSize = os.stat(fullPath).st_size
        (root, ext) = os.path.splitext( os.path.basename(fullPath) )
        previewName = root+str(fileSize)+ext        #construct the preview name

        # all the files have .jpg tacked onto the end
        previewName += ".jpg"
            
        return opj( THUMB_DIR, previewName )


    def __MakePreview(self, name, previewSize):
        try:
            wxImageType = self.__GetWxBitmapType(name)
            if wxImageType:     # some types just cant be saved by wxPython
                fileType = self.GetFileType(name)
                # write the preview but name it with a filename and filesize so that they are unique
                previewPath = self.__GetPreviewName(opj(dirHash[fileType], name))
                im = wx.Image(opj(dirHash[fileType], name))  # read the original image

                if im.Ok():  #the image may be corrupted...
                    im.Rescale(previewSize[0], previewSize[1])        # resize it
                    im.SaveFile(previewPath, wxImageType)    # save it back to a file
                self.__SetWritePermissions(previewPath, opj(dirHash[fileType], name))
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            pass    # no preview was saved... oh well



        ### change the permission of the settings file to allow everyone to write to it
    def __SetWritePermissions(self, *files):
        for filePath in files:
            try:
                flags = stat.S_IWUSR | stat.S_IRUSR | stat.S_IWGRP | stat.S_IRGRP | stat.S_IROTH
                os.chmod(filePath, flags)
            except:
                WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
                
        
        ### reads a file and encodes it in a string (for transfer over the network)
    def __ReadFile(self, fullPath):
        f = open(fullPath, "rb")
        fileData = base64.encodestring( f.read() )
        f.close()
        return fileData

    
    def FindFile(self, filename, size):
        for name, fullPath in self.__indexedFiles.items():
            if filename == name:
                if os.stat(fullPath).st_size==size:
                    return fullPath
        
        return False


       ### recursively traverses the given directory
       ### it return a tuple of (dir-path, list-of-dirs, files)
    def __TraverseDir(self, dirPath, fileType):
        dirPath = ConvertPath(dirPath)
        items = os.listdir(dirPath)
        files = []
        dirs = {}
        for item in items:
            if os.path.isfile( opj(dirPath, item) ):
                if self.GetFileType(item):  #if the file is supported, add it to the list, otherwise disregard it
                    files.append(item)
            elif os.path.isdir( opj(dirPath, item) ):
                dirs[item] = self.__TraverseDir( opj(dirPath, item), fileType )
        return (dirPath, dirs, files, fileType)


       ### you pass it in a filename and it figures out the wx.BITMAP_TYPE and returns it
    def __GetWxBitmapType(self, name):
        ext = os.path.splitext(name)[1]
        ext = ext.lower()
        if ext==".jpg": return wx.BITMAP_TYPE_JPEG
        elif ext==".mpo": return wx.BITMAP_TYPE_JPEG
        elif ext==".jps": return wx.BITMAP_TYPE_JPEG
        elif ext==".png": return wx.BITMAP_TYPE_PNG
        elif ext==".pns": return wx.BITMAP_TYPE_PNG
        elif ext==".gif": return wx.BITMAP_TYPE_GIF
        elif ext==".bmp": return wx.BITMAP_TYPE_BMP
        elif ext==".pcx": return wx.BITMAP_TYPE_PCX
        elif ext==".pnm": return wx.BITMAP_TYPE_PNM
        elif ext==".tif": return wx.BITMAP_TYPE_TIF
        elif ext==".tiff": return wx.BITMAP_TYPE_TIF
        elif ext==".xpm": return wx.BITMAP_TYPE_XPM
        elif ext==".ico": return wx.BITMAP_TYPE_ICO
        elif ext==".cur": return wx.BITMAP_TYPE_CUR
        else:
            return False


        ### gets the default path for a certain file type
    def __GetFilePath(self, type=None):
        if type == None:
            return FILES_DIR 
        else:
            return dirHash[type]



        ### Checks whether all the paths are still within FILES_DIR (for security issues)
    def __LegalPath(self, *pathList):
        try:
            normedPathList = []
            normedPathList.append(FILES_DIR)
            for path in pathList:
                if path != "":
                    normedPathList.append( os.path.normpath( os.path.realpath(path) ) )  # normalize all the paths (remove ../ kinda stuff)
            return (os.path.commonprefix(normedPathList) == FILES_DIR)
        except:
            return False   #if anything fails, it's safer to assume that it's an illegal action


    def __IsFolder(self, fullPath):
        return (os.path.splitext(fullPath)[1] == '')
    



# to output all the error messages to a file
def WriteLog(message):
    try:
        print message
    except:
        pass






# a mix-in class for adding the threading support to the XMLRPC server
class ThreadedXMLRPCServer(SocketServer.ThreadingMixIn, SimpleXMLRPCServer):
    allow_reuse_address = True


    # so that we can quit the server properly
class MyServer(ThreadedXMLRPCServer):
    def serve_forever(self):
	while RUN_SERVER:
	    self.handle_request()
    
    


def main(argv):

    ## global REDIRECT    
##     if len(argv) > 2:
##         if "-v" in argv: 
##             REDIRECT = False
##         else:

##             # redirect all the output to nothing
##             class dummyStream:
##                 ''' dummyStream behaves like a stream but does nothing. '''
##                 def __init__(self): pass
##                 def write(self,data): pass
##                 def read(self,data): pass
##                 def flush(self): pass
##                 def close(self): pass
##             # and now redirect all default streams to this dummyStream:
##             sys.stdout = dummyStream()
##             sys.stderr = dummyStream()
##             sys.stdin = dummyStream()
##             sys.__stdout__ = dummyStream()
##             sys.__stderr__ = dummyStream()
##             sys.__stdin__ = dummyStream()


    wx.InitAllImageHandlers()

    # start the XMLRPC server
    server = MyServer(("", 8800), SimpleXMLRPCRequestHandler, logRequests=False)
    server.register_instance(FileLibrary())
    server.serve_forever()



if __name__ == '__main__':
    import sys, os
    main(['', os.path.basename(sys.argv[0])] + sys.argv[1:])
    
