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



import os, os.path
import overlays.pointer as pointer
from globals import *
import traceback as tb


class DeviceManager:
    
    def __init__(self):
        self.devices = {}  # key=deviceId, value=deviceObj

        # load all the plugins now
        self.__devicePlugins = {}  # key=deviceType, value=devicePlugin
        self.__loadAllPlugins()

        # for special devices and assigning unique ids
        self.__specialDevices = []  # a list of special devices <--- what is this??? i forgot :(
        
        # pointers already created for touch devices... key=deviceName, value=pointerObject
        self.__existingPointers = {}  
        

    def createTouchPointers(self):
        """ creates a bunch of pointers for a certain type of devices... like touch devices.
        This is done so that they can be reused instead of constantly recreating pointers """
        overlayMgr = getOverlayMgr()

        for i in range(0, 35):
            ptrToAdd = pointer.makeNew()
            ptrToAdd.setImage("images/touch.png")
            ptrToAdd.hide()
            name = "pqlabs"+str(i)
            ptrToAdd.setTooltip(" ".join(name.split("%")))
            ptrToAdd.displayId = 0
            overlayMgr.addOverlay(ptrToAdd, self.onPointerAdded)
                    
            
    def onPointerAdded(self, ptr):
        self.__existingPointers[ptr.getTooltip()] = ptr


    def getPointerForDevice(self, deviceName):
        return None  # BYPASSED!!!!
        if deviceName in self.__existingPointers:
            return self.__existingPointers[deviceName]
        else:
            return None


    def onHWMessage(self, deviceId, deviceType, data):
        """ this gets called when a message arrives from any hw device """

        self.cleanup()
        
        # check whether the device already exists, if so pass the message to it
        deviceObj = None
        if deviceId in self.devices:
            deviceObj = self.devices[ deviceId ]
            try:
                deviceObj.onMessage(data, False)
            except:
                tb.print_exc()
            
        else:
            if deviceType not in self.__devicePlugins:  # devicePlugin isn't loaded yet 
                if not self.__loadDevicePlugin( deviceType ):
                    return   # couldn't load the plugin
            # at this point we have a plugin loaded so create a device object to do the conversion

            newDeviceObj = self.__devicePlugins[ deviceType ].makeNew(deviceId)
            self.devices[ deviceId ] = newDeviceObj

            # set a special id if the device is special
            if newDeviceObj.specialDevice:
                specId = len(self.__specialDevices)
                newDeviceObj.setSpecialId(specId)
                self.__specialDevices.append(deviceId)

            writeLog(LOG_POINTER_NEW, deviceType, deviceId)
            
            try:
                newDeviceObj.onMessage(data, firstMsg=True)
            except:
                tb.print_exc()

            deviceObj = newDeviceObj

        # if the device requested to be destroyed... do it now
        if deviceObj.destroyMe:
            self.removeDevice(deviceId)


    def cleanup(self):
        """ cleans up old devices that can be destroyed
            ... mostly old touch strokes that expired
        """
        for deviceId, device in self.devices.items():
            if not device.isAlive() or device.destroyMe:
                self.removeDevice(deviceId)
                

    def removeDevice(self, deviceId):
        # remove from the list of devices
        if deviceId in self.devices:
            writeLog(LOG_POINTER_REMOVE, self.devices[deviceId].deviceType, deviceId)
            self.devices[ deviceId ].destroy()
            del self.devices[ deviceId ]

        # remove from the list of special devices if necessary
        if deviceId in self.__specialDevices:
            self.__specialDevices.remove(deviceId)

        # remove the device from the eventManagers list of devices and prevHandlers for each
        getEvtMgr()._removeDevice(deviceId)
        

    def __loadAllPlugins(self):
        """ goes into the 'devices' directory and loads all the plugins from there """
        loaded = []
        
        for entry in os.listdir("devices"):
            name, ext = os.path.splitext(entry)
            if (ext==".py" or ext==".pyc") and not name.startswith("__") and name not in loaded:
                loaded.append(name)
                self.__loadDevicePlugin( name )
            

    def __loadDevicePlugin(self, deviceType):
        """ try to load the devicePlugin for the requested deviceType """
        try:
            #(f, path, desc) = imp.find_module(deviceType, "devices")
            #devicePlugin = imp.load_module(deviceType, f, path, desc)
            devicePlugin = __import__("devices."+deviceType, globals(), locals(), [deviceType])
            self.__devicePlugins[ deviceType ] = devicePlugin
            print "Device plugin loaded: ", deviceType
            return True
                
        except ImportError:
            return False   # failed to load the module

    
