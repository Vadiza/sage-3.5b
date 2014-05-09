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


###   MODULE FOR SAVING SAGE UI PREFERENCES   ###

import os, os.path, cPickle, copy, sys

from globals import *
from sagePath import getUserPath

# where we store all the prefs
PREFS_DIR = getUserPath("sageui", "prefs")
FILE_LIB_FILE = opj(PREFS_DIR, "file_libs.pickle")
USERNAMES_FILE = opj(PREFS_DIR, "usernames.pickle")
VISUAL_FILE = opj(PREFS_DIR, "visuals.pickle")
FAVORITES_FILE = opj(PREFS_DIR, "favorite_files.pickle")
MACHINES_FILE = opj(PREFS_DIR, "machines.pickle")
VNC_FILE = opj(PREFS_DIR, "vnc.pickle")

global fileLib
global usernames
global visual
global favorite_files
global machines
global vnc


#-------------------------------------------------------
### for saving and loading preferences
#-------------------------------------------------------    

def readAllPreferences():
    global fileLib
    global usernames
    global visual
    global favorite_files
    global machines
    global vnc

    
    p = readPreferences(FILE_LIB_FILE)
    if not p: fileLib = FileLibraryPrefs()
    else: fileLib = p
    
    p = readPreferences(USERNAMES_FILE)
    if not p: usernames = Usernames()
    else: usernames = p
    
    p = readPreferences(VISUAL_FILE)
    if not p: visual = VisualPrefs()
    else: visual = p

    p = readPreferences(FAVORITES_FILE)
    if not p: favorite_files = FavoriteFiles()
    else: favorite_files = p
    
    p = readPreferences(MACHINES_FILE)
    if not p: machines = MachinePrefs()
    else: machines = p
    
    p = readPreferences(VNC_FILE)
    if not p: vnc = VNCPrefs()
    else: vnc = p



### reads the preferences from a file
def readPreferences(sourceFile):
    try:
        f = open(sourceFile, "rb")
        (objToLoad) = cPickle.Unpickler(f).load()
        f.close()
        return objToLoad
    except:
        #print sys.exc_info()[0], sys.exc_info()[1]
        return False


### saves the provided object into a specified file
def savePreferences(objToSave, targetFile):
    try:
        f = open(targetFile, "wb")
        cPickle.Pickler(f, cPickle.HIGHEST_PROTOCOL).dump(objToSave)
        f.close()
    except:
        #print sys.exc_info()[0], sys.exc_info()[1]
        return False




#-------------------------------------------------------    
# datastructures for preferences
#-------------------------------------------------------


class VNCPrefs:
    def __init__(self):
        self.__profiles = {}  #keyed by profile name

    def AddProfile(self, profileName, ip, displayNum, size, passwd):
        self.__profiles[profileName] = (ip, displayNum, size, passwd)
        self.__Save()

    def DeleteProfile(self, profileName):
        if self.__profiles.has_key(profileName):
            del self.__profiles[profileName]
            self.__Save()

    def ProfileExists(self, profileName):
        return self.__profiles.has_key(profileName)

    def GetProfileList(self):
        return copy.deepcopy(self.__profiles.keys())

    def GetProfile(self, profileName):
        return self.__profiles[profileName]

    def __Save(self):
        savePreferences(self, VNC_FILE)

        
            
class Usernames:
    def __init__(self):
        self.__usernames = ["New-user"]
        self.__default = "New-user"

    def AddUsername(self, username):
        if not username in self.__usernames:
            self.__usernames.append(str(username))
        self.__default = str(username)
        self.__Save()

    def GetDefault(self):
        return self.__default

    def GetUsernames(self):
        return copy.deepcopy(self.__usernames)

    def __Save(self):
        savePreferences(self, USERNAMES_FILE)



class MachinePrefs:
    def __init__(self):
        self.__machines = {}
     
    def GetMachineHash(self):
        return copy.deepcopy(self.__machines)
    
    def AddMachine(self, sageMachine):
        self.__machines[sageMachine.GetId()] = sageMachine
        self.__Save()

    def RemoveMachine(self, sageMachine):
        if self.__machines.has_key(sageMachine.GetId()):
            del self.__machines[sageMachine.GetId()]
            self.__Save()

    def __Save(self):
        savePreferences(self, MACHINES_FILE)

            

class VisualPrefs:
    def __init__(self):
        self.__frameSize = None
        self.__framePos = None 
        self.__chatShown = None
        self.__receivePerfData = True
        self.__keepAspectRatio = True

    def SetFrameSize(self, size):
        self.__frameSize = size
        self.__Save()

    def SetFramePos(self, pos):
        self.__framePos = pos
        self.__Save()

    def ShowChat(self, doShow):
        self.__chatShown = doShow
        self.__Save()

    def SetReceivePerformanceData(self, doReceive):
        self.__receivePerfData = doReceive
        self.__Save()

    def SetKeepAspectRatio(self, doKeep):
        self.__keepAspectRatio = doKeep
        self.__Save()

    def GetKeepAspectRatio(self):
        return self.__keepAspectRatio

    def GetReceivePerformanceData(self):
        return self.__receivePerfData
        
    def GetFramePos(self):
        return self.__framePos

    def GetFrameSize(self):
        return self.__frameSize

    def IsChatShown(self):
        return self.__chatShown

    def SetAll(self, size, pos, chatShow):
        self.__frameSize = size
        self.__framePos = pos
        self.__chatShown = chatShow
        self.__Save()

    def __Save(self):
        savePreferences(self, VISUAL_FILE)



class FavoriteFiles:
    def __init__(self):
        self.__favorites = {}

    def AddFavorite(self, itemData):
        type = itemData.GetType()
        if not self.__favorites.has_key(type):
            self.__favorites[type] = []
        self.__favorites[type].append(itemData)
        self.__Save()

    def RemoveFavorite(self, itemData):
        type = itemData.GetType()
        if self.AlreadyFavorite(itemData):
            self.__favorites[type].remove(itemData)
            self.__Save()

    def AlreadyFavorite(self, itemData):
        type = itemData.GetType()
        if self.__favorites.has_key(type) and (itemData in self.__favorites[type]):
            return True
        else:
            return False

    def GetFavorites(self):
        return copy.deepcopy(self.__favorites)

    def __Save(self):
        savePreferences(self, FAVORITES_FILE)



class FileLibraryPrefs:
    def __init__(self):
        self.__libs = {"local":"localhost"}  #keyed by name
        self.__default = ("local", "localhost")

    def AddLibrary(self, ip, name):
        if self.__libs.has_key(name):
            return False
        else:
            self.__libs[name] = ip
            self.__Save()

    def RemoveLibrary(self, name):
        if self.__libs.has_key(name):
            del self.__libs[name]
            self.__Save()

    def GetLibraries(self):
        return copy.deepcopy(self.__libs)  #prevent the user from modifying it directly

    def GetLibraryIP(self, name):
        if self.__libs.has_key(name):
            return self.__libs[name]
        else:
            return False

    def GetLibraryList(self):
        return self.__libs.keys()

    def SetDefault(self, name, ip):
        if not ip:
            return
        self.__default = (str(name), str(ip))
        self.__Save()

    def GetDefault(self):
        return self.__default

    def __Save(self):
        savePreferences(self, FILE_LIB_FILE)





