############################################################################
#
# AppLauncher - Application Launcher for SAGE
# Copyright (C) 2006 Electronic Visualization Laboratory,
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
# Direct questions, comments etc about AppLauncher to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#
############################################################################


from data import *
from request import CurrentRequests, SSHRequest
from SimpleXMLRPCServer import *
from SecureXMLRPCServer import SecureXMLRPCServer
import socket, os, sys, xmlrpclib, time, optparse, os.path
import traceback as tb
import subprocess as sp
from threading import RLock
from myprint import *   # handles the printing or logging

opj = os.path.join
sys.path.append( opj(os.environ["SAGE_DIRECTORY"], "bin" ) )
from sagePath import getUserPath, SAGE_DIR, getPath, getDefaultPath


APP_CONFIG_FILE = getPath("applications", "applications.conf")
print "AppConfigFile: ", APP_CONFIG_FILE
TILE_CONFIG_FILE = None
XMLRPC_PORT = 19010
SECURE_XMLRPC_PORT = XMLRPC_PORT+1
SAGE_SERVER_PORT = 8009    # sage server port for reporting to
SAGE_SERVER = "localhost" #"sage.sl.startap.net"
STOP_LAUNCHER = False    #used for stopping the applauncher remotely
DO_REPORT = True      # for determining whether to report to the sage server or not
REPORT_NAME = socket.gethostname()
DEFAULT_SYSTEM_IP = None   # for application pixel streaming
DEFAULT_SYSTEM_PORT = None


# change to the directory of this script
os.chdir(sys.path[0])


# read the SYSTEM_IP from the fsManager.conf
try:
    f = open( getPath("fsManager.conf"), "r")
    for line in f:               # read the fsManager line
        if line.strip().startswith("fsManager"):
            lineTokens = line.split()
            if len(lineTokens) >= 4:
                DEFAULT_SYSTEM_IP = lineTokens[2]
        elif line.strip().startswith("systemPort"):
            DEFAULT_SYSTEM_PORT = line.split()[1]
        elif line.strip().startswith("tileConfiguration"):
            TILE_CONFIG_FILE = getPath( line.split()[1] )
    f.close()
except:
    DEFAULT_SYSTEM_IP = None




class AppLauncher:
    def __init__(self):
        # read the tile config to get a list of all the nodes in this cluster available for running apps
        self._nodeHash = {}  #key=IP, value=[0...n]   --> how many apps are running on that node

        if TILE_CONFIG_FILE:
            self.__readNodeIPs()
        else:
            self._nodeHash = {"127.0.0.1":0}

        # read the sage app configuration file and create the container for all the requests
        WriteLog( "\nUsing config file: " + APP_CONFIG_FILE )
        self.configs = Configurations(APP_CONFIG_FILE)
        self.requests = CurrentRequests(self._nodeHash)

        # report ourselves to the sage server
        self.sageServer = xmlrpclib.ServerProxy("http://"+SAGE_SERVER+":8009")
        self.launcherId = ""
        self.reportLauncher()

                            

    def __readNodeIPs(self):
        f = open(TILE_CONFIG_FILE, "r")
        for line in f:
            previous = False  # True if previous line was an IP, False otherwise
            if previous:
                previous = False
                continue
            elif line.strip().startswith("IP"):
                self._nodeHash[line.strip().split()[1].split(':')[0]] = 0   #add the machine ip to the hash
        f.close()
        WriteLog( "\n\nNodeHash = \n" + str(self._nodeHash) + "\n\n" )
    

    def startDefaultApp(self, appName, fsIP, fsPort, useBridge, configName, pos=False, size=False, optionalArgs=""):
        """ starts the application based on the entry in applications.conf """
        return self.startApp(appName, fsIP, fsPort, useBridge, configName, optionalArgs=optionalArgs, pos=pos, size=size)
        

    def startApp(self, appName, fsIP, fsPort, useBridge, configName="", command="", optionalArgs="", pos=False, size=False, nodeNum=False, protocol=False):
        """ receive the request for starting an app
            either start the default app from a predefined config
            or start a completely custom app with dynamic config
        """

        # first split the optionalArgs and add quotes around each piece
        oaTokens = optionalArgs.split()
        optionalArgs = ""
        for arg in oaTokens:
            optionalArgs += "\""+arg.strip("\"")+"\" "

        if configName != "":   # this assumes a default config and app startup
            if configName == "default":
                config = self.configs.getDefaultConfig(appName)
            else:
                config = self.configs.getConfig(appName, configName)
        elif command != "":    # this assumes a completely custom app startup
            config = OneConfig(appName+"_temp", appName, True)
            config.setCommand(command + " " + optionalArgs)

        # common set of parameters that can be changed in either case
        if pos:      config.setPosition(pos)
        if size:     config.setSize(size)
        if nodeNum:  config.setNodeNum(nodeNum)
        if protocol: config.setProtocol(protocol)

        if not fsIP: fsIP = DEFAULT_SYSTEM_IP
        config.setFSIP(fsIP)

        if not fsPort:
            fsPort = DEFAULT_SYSTEM_PORT
        config.setFSPort(fsPort)
        
        config.setUseBridge(useBridge)
        config.setLauncherId(socket.gethostbyname(socket.gethostname())+":"+str(XMLRPC_PORT))
        config.setConfigFilename()
        if useBridge:
            if config.getBridgeIP() == "":   #means that there was no app-specific bridge info set so use the default
                config.setBridgeIP(self.configs.getBridgeIP())
                config.setBridgePort(self.configs.getBridgePort())

        config.setCommand(config.getCommand()+" "+optionalArgs)

        # create the request, set the appropriate appId, write the config file and submit it
        return self.requests.addRequest(config)

        
    def stopApp(self, appId):
        """ forcefully kills the application """
        return self.requests.stopRequest(appId)


    def appStatus(self):
        """ returns a list of currently running applications as a hash key=appId, value=appName-command """
        return self.requests.getStatus()
        

    def getAppList(self):
        """ return configurations for all apps in a hash of strings """
        self.configs.reloadConfigFile()
        return self.configs.getConfigHash()


    def getAppConfigInfo(self, appId):
        """ return the configuration information for the running app """
        if self.requests.getRequest(appId):
            s = self.requests.getRequest(appId).config.getConfigString()
            return s
        else:
            return -1
  

    def killLauncher(self):
        """ used for killing the appLauncher remotely """
        global STOP_LAUNCHER
        STOP_LAUNCHER = True

        # unregister from the sage server
        if self.launcherId:
            try: self.sageServer.UnregisterLauncher(self.launcherId)
            except: pass

        return 1


    def reportLauncher(self):
        """ only used to report ourselves to the sage server that we are still alive
            every time pretend like it's our first time connecting so that even if the
            sage server was started after the appLauncher, it will still be reported
        """
        if not DO_REPORT:
            return
        
        try:
            self.launcherId = self.sageServer.ReportLauncher( REPORT_NAME, socket.gethostbyname(socket.gethostname()),
                                                              XMLRPC_PORT, self.getAppList() )
        except socket.error:
            pass
        except:
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )


    def test(self):
        return 1

        

    # a mix-in class for adding the threading support to the XMLRPC server
class ThreadedXMLRPCServer(SocketServer.ThreadingMixIn, SimpleXMLRPCServer):
    allow_reuse_address = True

class MySimpleXMLRPCServer(SimpleXMLRPCServer):
    allow_reuse_address = True
    



def get_commandline_options():
    parser = optparse.OptionParser()
    h = "if set, prints output to console, otherwise to output.txt"
    parser.add_option("-v", "--verbose", action="store_true", help=h, dest="verbose", default=False)

    h = "change the port number that the server listens on (default is 19010)"
    parser.add_option("-p", "--port", help=h, type="int", dest="port", default=19010)

    h = "specify the application configuration file to use (default is applications.conf)"
    parser.add_option("-c", "--config", dest="config", help=h, default="")

    h = "which sage server to report to (default is sage.sl.startap.net)"
    parser.add_option("-s", "--server", dest="server", help=h, default="sage.sl.startap.net")

    h = "report yourself as *name* (default is the local machine name)"
    parser.add_option("-a", "--name", dest="name", help=h, default=socket.gethostname())

    h = "set this flag if you don't want the appLauncher to report to the sage server and become visible by other people"
    parser.add_option("-n", "--noreport", action="store_true", dest="report", help=h, default=False)

    return parser.parse_args()


def generateSSLCertificate():
    print "Creating a new SSL certificate and key..."
    cmd = "openssl req -x509 -days 9999 -nodes -config certificate_defaults -newkey rsa:1024 -keyout key.pem -out cert.pem"
    if sp.call(cmd.split(' ')) == 0:
        print "Success!"
        return True
    else:
        print "Something went wrong... no SSL security today :("
        return False


def main(argv):
    global STOP_LAUNCHER
    global APP_CONFIG_FILE
    global XMLRPC_PORT
    global SECURE_XMLRPC_PORT
    global SAGE_SERVER
    global DO_REPORT
    global REPORT_NAME
    
    # parse the command line params
    (options, args) = get_commandline_options()
    doRedirect(not options.verbose)
    if options.config != "":
        APP_CONFIG_FILE = options.config
    XMLRPC_PORT = options.port
    SECURE_XMLRPC_PORT = XMLRPC_PORT+1
    SAGE_SERVER = options.server
    DO_REPORT = not options.report
    REPORT_NAME = options.name

    # set the default timeout so that we don't wait forever
    socket.setdefaulttimeout(0.1)  # 200ms... because both servers need to be running    
        
    # create the main object for handling requests
    appLauncher = AppLauncher()

    # start the insecure server and listen for sage ui connections
    server = MySimpleXMLRPCServer(("", XMLRPC_PORT), logRequests=False)
    server.register_function(appLauncher.startDefaultApp)
    server.register_function(appLauncher.startApp)
    server.register_function(appLauncher.stopApp)
    server.register_function(appLauncher.getAppList)
    server.register_function(appLauncher.getAppConfigInfo)
    server.register_function(appLauncher.killLauncher)
    server.register_function(appLauncher.test)
    server.register_function(appLauncher.appStatus)
    server.register_introspection_functions()
    print "Started insecure XML_RPC server on port " + str(XMLRPC_PORT)

    # create a new key and certificate upon startup
    use_ssl = generateSSLCertificate()

    # now start the SSL version of the server
    if use_ssl:
        secureServer = SecureXMLRPCServer(("", SECURE_XMLRPC_PORT), logRequests=False)
        secureServer.register_function(appLauncher.startDefaultApp)
        secureServer.register_function(appLauncher.startApp)
        secureServer.register_function(appLauncher.stopApp)
        secureServer.register_function(appLauncher.getAppList)
        secureServer.register_function(appLauncher.getAppConfigInfo)
        secureServer.register_function(appLauncher.killLauncher)
        secureServer.register_function(appLauncher.test)
        secureServer.register_function(appLauncher.appStatus)
        secureServer.register_introspection_functions()
        print "Started secure SSL XML_RPC server on port " + str(SECURE_XMLRPC_PORT)

    # now loop forever and serve the requests
    oldT = time.time()
    while not STOP_LAUNCHER:
        try:
            # report every 6 seconds that we are still alive 
            if (time.time() - oldT) > 6:
                appLauncher.reportLauncher()
                oldT = time.time()

            # accept and process any xmlrpc requests
            # this times out every few seconds as defined by the setdefaulttimeout above
            server.handle_request()
            if use_ssl:  secureServer.handle_request()
        except KeyboardInterrupt:
            break
        except:
            WriteLog( "\n=======> XML_RPC SERVER ERROR: ")
            WriteLog( "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])) )
            continue

    appLauncher.requests.stopSubmitThread()
    server.server_close()
    if use_ssl: secureServer.server_close()
    sys.exit(0)



if __name__ == '__main__':
    main(['', os.path.basename(sys.argv[0])] + sys.argv[1:])
