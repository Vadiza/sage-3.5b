
from threading import Thread
from globals import *
import xmlrpclib, sys, socket, os.path, os, time
import traceback as tb


FILE_SERVER_PORT = 8800  # for the file server
FILE_SERVER_BIN_PORT = 8802
APP_LAUNCHER_PORT = 19010
DEFAULT_TIMEOUT = None  # no timeout because server side processing might take a while
PREVIEW_SIZE = (150,150)


opj = os.path.join



def getFileServer(host):
    return FileServer(host, FILE_SERVER_PORT)


def getAppLauncher(host):
    return xmlrpclib.ServerProxy("http://"+host+":"+str(APP_LAUNCHER_PORT))



class FileServer:
    
    def __init__(self, host, port):
        self.host = host  #where the FileLibrary is running
        self.port = str(port)
        self.connected = False
        self.Connect()


        # try to connect to the server
        # if we failed, set the flag
        # if we succeed, get the path where the images are stored so that we can send it to the app
    def Connect(self):
        if self.connected: return True

        #print "\nConnecting to XMLRPC server at: http://"+str(self.host)+":"+self.port
        socket.setdefaulttimeout(DEFAULT_TIMEOUT)
        self.server = xmlrpclib.ServerProxy("http://"+str(self.host)+":"+self.port)
        #socket.setdefaulttimeout(3)
        try:
            bla = self.server.TestConnection()[1]  #just see if the connection opened correctly
        except:
            print "Could not connect to the file server at: "+str(self.host)+":"+self.port
            self.connected = False
            return False
        else:
            #print "Connected to the XMLRPC server at: http://"+str(self.host)+":"+self.port
            self.connected = True
            return True


    def IsConnected(self):
        return self.Connect()
            

        # uploads the file to the file library and returns information about the file
    def UploadFile(self, fullPath):
        try:
            # get the file info first
            fileInfo = self.GetNewFileInfo(fullPath)
            if not fileInfo: return False            # file type not supported
            fileExists = fileInfo[5]
            if fileExists:                           # if the file exists on the server, just show it (ie. dont send it)
                return fileInfo #no need to upload
            else:
                if fullPath.startswith("http"):
                    res = self.server.UploadLink(fullPath)
                else:
                    res = self.__SendFile(fullPath)    # did upload fail for some reason?                   
                
                if res:
                    return self.GetNewFileInfo(fullPath)   # this forces the creation of thumbnail
                else:                           
                    return False                     # upload failed so whatever you are doing after this (ShowFile maybe), don't do it

        except socket.error: 
            self.connected = False
            print "Unable to upload file. There is no connection with the File Server."
            return False
        except xmlrpclib.ProtocolError:
            self.connected = False
            print "Protocol error"
            return False
    

        # gets the information about the file and the supporting app from the file library
    def GetNewFileInfo(self, fullPath):
        (path, filename) = os.path.split(fullPath)
        filename = "_".join(filename.split())
        filename = filename.replace("(", "_")
        filename = filename.replace(")", "_")

        # check if the file type is supported
        # if it is, it will get some data about the file (type, viewerApp ...)
        if not fullPath.startswith("http"):
            fileSize = os.stat(fullPath).st_size  #get the size in bytes
        else:
            fileSize = -1
        fileInfo = self.server.GetFileInfo( filename, fileSize )
        if not fileInfo:
            extension = os.path.splitext(filename)[1].lower()
            print "File type <"+extension+"> not supported... filename: ", filename
            return False

        return fileInfo


    def __SendFile(self, fullPath):
        doDlg = False
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((self.host, FILE_SERVER_BIN_PORT))

            #convert to a filename with _ instead of empty spaces
            convertedFilename = "_".join(os.path.basename(fullPath).split())
            convertedFilename = convertedFilename.replace("(", "_")
            convertedFilename = convertedFilename.replace(")", "_")
            
            # send the header first
            fileSize = os.path.getsize(fullPath)
            header = convertedFilename + " " + str(PREVIEW_SIZE[0]) + " " + str(PREVIEW_SIZE[1]) + \
                     " " + str(fileSize) + "\n"
            s.sendall(header)     

            # send the file data
            f=open(fullPath, "rb")
            t = 0
            for line in f:
                s.sendall(line)
                t += len(line)

            s.sendall("\n")  # because the other side is reading lines
            f.close()
            s.recv(1)
        except:
            print "Error sending file to File Server: ", sys.exc_info()[0], sys.exc_info()[1]
            return False

        return True
                
 


 
class FileShare:

    def shareContent(self, files, onHost):   # just upload the file to the libary on the machine we are connected to
        t = Thread(target=self.__startShow, args=[files, onHost])
        t.start()


    def shareVNC(self, args, onHost):
        self.__startApp("VNCViewer", oa=args, host=onHost)

    
    # runs in a thread, showing all dropped files a few seconds apart
    def __startShow(self, files, onHost):
        for f in files:
            self.__uploadAndShow(f, onHost)
            time.sleep(2)


    # first uploads the file and then shows it
    def __uploadAndShow(self, f, onHost):
        fileInfo = getFileServer(onHost).UploadFile(f)
        if fileInfo:  #if upload succeeded
            self.__showFile(fileInfo, onHost)
        else:
            print "Sending file to SAGE failed."
    
              

    # shows one file
    def __showFile(self, fileInfo, dest):
        fileType, size, fullRemotePath, appName, params, fileExists = fileInfo
        
        if fileType == "image":
            res = self.__startApp(appName, oa=fullRemotePath+" "+params, host=dest)
        elif fileType == "video" or fileType == "pdf" or fileType == "audio":
            res = self.__startApp(appName, oa=params+" "+fullRemotePath, host=dest)
        else:  #for other types
            res = self.__startApp(appName, oa=fullRemotePath+" "+params, host=dest)

        if res < 0:
            print "Application not started. Either application failed, the application launcher is not running or the application  <<"+appName+">>  is not configured in application launcher."

            


    def __startApp(self, appName, pos=False, size=False, oa="", host=None):
        return getAppLauncher(host).startDefaultApp(appName, host, 20002, False, "default", pos, size, oa)
