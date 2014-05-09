############################################################################
#
# DIM - A Direct Interaction Manager for SAGE
# Copyright (C) 2007 Electronic Visualization Laboratory,
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




# python imports
import optparse, time, os, sys
from threading import Timer

# my imports
from deviceManager import DeviceManager
from eventManager import EventManager
from overlayManager import OverlayManager
from listener import Listener
from sageGate import SageGate
from sageData import SageData
from globals import *


# ------------------------------------------------------------------------------
#
#                         GLOBALS   
#
# ------------------------------------------------------------------------------

VERBOSE = False
LISTENER_PORT = 20005   # for HW capture modules (ie HW devices sending events)


os.chdir(sys.path[0])


class DIM:
    
    def __init__(self, host, port, loadState, autosave, doLog):
       
        print "\n\n=========   Starting DIM  ============\n"

        # the log object
        if doLog:
            setLog(Log())
            print "Logging user interaction into: ", getLog().getLogFilename(), "\n\n"

        # sageGate is the network connection with SAGE
        self.sageGate = SageGate()
        setSageGate(self.sageGate)

        # the event manager takes care of properly dispatching events
        self.evtMgr = EventManager()
        setEvtMgr(self.evtMgr)
        self.evtMgr.addHandlers()

        # sageData keeps the current state of the SAGE windows/apps
        self.sageData = SageData(autosave, self.sageGate)
        setSageData(self.sageData)
        
        # overlay manager creates, destroys and updates overlays
        self.overlayMgr = OverlayManager()
        setOverlayMgr(self.overlayMgr)

        # contains all the devices and takes care of loading plugins for them
        # also, distributes HW messages to each device 
        self.devMgr = DeviceManager()
        setDevMgr(self.devMgr)

        # connect to SAGE
        if (DEBUG):
            print "\n\nRUNNING IN DEBUG MODE!!!!\n\n"
        else:
            time.sleep(2)
        
        for i in range(20):  # try to connect to SAGE for X seconds
            retval = self.sageGate.connectToSage(host, port)
            if retval == 1:
                self.sageGate.registerSage("dim")
                print "DIM: successfully connected to SAGE", (host, port)
                break
            elif retval == 0:
                print "DIM: SAGE", (host, port), "is not ready yet. Retrying..."
            elif retval == -1:
                print "DIM: Couldn't connect to appLauncher. appLauncher is not ready yet. Retrying..."
            time.sleep(1)
        else:  # we didn't manage to connect to sage in X seconds... so quit
            exitApp()

        # start listening for the device events
        time.sleep(2)   # wait till all the messages come in
        self.overlayMgr._addGlobalOverlays()   # add all the UI plugins for the display

        # automatically load a saved state if so desired
        if loadState:
            print "\n===> Autoloading state: ", loadState, "\n"
            t = Timer(3, self.sageData.loadState, [loadState])
            t.start()
        
        self.listener = Listener(LISTENER_PORT, self.devMgr.onHWMessage)
        self.listener.serve_forever()



### sets up the parser for the command line options
def get_commandline_options():
    parser = optparse.OptionParser()

    h = "if set, prints output to console, otherwise to output_log.txt"
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true", help=h, default=False)

    h = "if set, logs all user interaction into a file in ~/.sageConfig/log"
    parser.add_option("-l", "--log", dest="log", action="store_true", help=h, default=False)

    h = "interaction with which sage? (default=localhost)"
    parser.add_option("-s", "--sage_host", dest="host", help=h, default=getSAGEIP())

    h = "the UI port number for sage (default is 20001)"
    parser.add_option("-p", "--sage_port", dest="port", help=h, type="int", default=20001)

    h = "location of the Connection Manager (default is "+ getSAGEServer()+")"
    parser.add_option("-c", "--con_manager", dest="conManager", help=h, default=getSAGEServer())

    h = "the debug mode"
    parser.add_option("-d", "--debug", dest="debug", help=h, action="store_true", default=False)

    h = "upon startup load this saved state (write saved state name from saved-states directory)"
    parser.add_option("-o", "--load_state", dest="load_state", help=h, default=None)

    h = "cancel autosave?"
    parser.add_option("-a", "--no_autosave", action="store_false", help=h, dest="autosave", default=True)

    h = "share with which sage? (default=None)"
    parser.add_option("-b", "--shared_host", dest="shared_host", help=h, default=None)

    h = "master to remote site"
    parser.add_option("-m", "--master_site", dest="master_site", help=h, default=None)

    return parser.parse_args()




# ------------------------------------------------------------------------------
#
#                         MAIN
#
# ------------------------------------------------------------------------------


def main():
    global VERBOSE
    
    # parse the command line params
    (options, args) = get_commandline_options()
    VERBOSE = options.verbose
    sagePort = options.port
    sageHost = options.host
    setDebugMode(options.debug)
    loadState = options.load_state
    autosave = options.autosave
    setSAGEServer(options.conManager)
    doLog = options.log

    # parse the shared sage host parameter
    sh = options.shared_host
    
    if sh:
		# NAME:IP,NAME:IP,...
        hostnamepairs = sh.split(",")
        names = []
        hosts = []
        for pairitem in hostnamepairs:
            tokens = pairitem.split(":",1)
            names.append(tokens[0])
       	    if len(tokens) == 1:
                hosts.append(tokens[0])
            else:
                hosts.append(tokens[1])
        print "List of shared hosts are .. ", hosts
        setSharedHost(names, hosts)

    # parse the master sage host parameter
    ms = options.master_site
    if ms:
        setMasterSite(ms)

    # start everything off
    DIM(sageHost, sagePort, loadState, autosave, doLog)
    

if __name__ == '__main__':
    main()
