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




from eventHandler import EventHandler
from globals import *
from events import WallClickedEvent
import overlays.app as appOverlayObj
from thumbnail import Thumbnail
import multiSelect
import os.path, xmlrpclib, time
import traceback as tb


def makeNew(display):
    """ this is how we instantiate the object from
        the main program that loads this plugin """
    return Wall(display)




class Wall(EventHandler):

    def __init__(self, display):
        EventHandler.__init__(self)
        self.displayId = display.getId()
        self.setSize(display.sageW, display.sageH)
        self.setPos(0,0)
        self._updateBounds()

        #self._allowSelection = True

        self.registerForEvent(EVT_ENTERED_WINDOW, self.__onEnteredWindow)
        self.registerForEvent(EVT_LEFT_WINDOW, self.__onLeftWindow)
        #self.registerForEvent(EVT_CLICK, self.__onClick)
        self.registerForEvent(EVT_MOVE, self.__onMove)
        #self.registerForEvent(EVT_DRAG, self.__onDrag)
        self.registerForEvent(EVT_MULTI_TOUCH_SWIPE, self.__onMultiTouchSwipe)

        self.z = BOTTOM_Z-1

        # to keep track of direction in which we are splitting a section by the dragging finger
        self.__splitDir = -1
        
        # min distance we have to drag to split a section... in pixels 
        self.__minSplitDist = int(0.12 * min(display.sageW, display.sageH))

        self.postEvent = getEvtMgr().postEvent

        # for recording device movement
        self.recording = False
        self.dev = ""
        self.positions = []        

        # for dropping stuff from media browser
        self.sageGate = getSageGate()
        self.sageData = getSageData()
        self.fileServer = getFileServer()
        self.onDrop(self._onDrop)

    

    def _onDrop(self, event):

        # first figure out if one or many were dropped
        thumbs = []
        if isinstance(event.object, multiSelect.MultiSelect) and \
               event.object.getSelectedOverlayType() == OVERLAY_THUMBNAIL:
            thumbs = event.object.getSelectedOverlays()
            
        elif isinstance(event.object, Thumbnail):
            thumbs.append(event.object)

        else:
            return
        
        # go through each one and show it
        c = 0
        d = 1   # direction in which to change the y when dropping multiple thumbs
        xo = int(getGlobalScale()*150)
        yo = int(getGlobalScale()*100)
        
        for thumb in thumbs:

            try:
                userData = thumb.getUserData()
                
                # load files
                if len(userData) == 2:
                    filePath, mediaType = userData
                    
                    if mediaType == "saved":  # saved states
                        self.sageData.loadState( os.path.splitext( os.path.basename(filePath) )[0] )
                        break
                    
                    else:   # media files
                        showFile(filePath, event.x + xo*c, event.y + yo*d )
                        c += 1
                        d = d*(-1)

                # load directories
                elif len(userData) == 3:
                    dirPath, mediaType, level = userData
                    for item in os.listdir(dirPath):
                        fullPath = os.path.join(dirPath,item)
                        if os.path.isdir(fullPath):
                            continue
                        showFile(fullPath, event.x + xo*c, event.y + yo*d )
                        c += 1
                        d = d*(-1)


            except:
                print "Error dropping overlay on wall, userData:", userData
                tb.print_exc()


        # deselect everything
        if isinstance(event.object, multiSelect.MultiSelect):
            event.object.deselectAllOverlays()

        event.droppedOnTarget = True
        

    def __onClick(self, event):
        #if event.isDown:
        #    event.device.toEvtHandler = self
        #    self.postEvent(WallClickedEvent("%d %d"%(event.x, event.y)))

        # split the section if we drag on the wall
        x,y = event.x, event.y
        ptr = event.device.pointer


        if event.isDown:  
            event.device.toEvtHandler = self  # dont allow others to gesture on the wall at the same time
            #self._captured = True
            self.__clickX, self.__clickY = x,y
            self.__splitDir = -1

        else:
            self._captured = False
            if event.device.toEvtHandler == self:
                distX = abs(x - self.__clickX)
                distY = abs(y - self.__clickY)

                sec = getLayoutSections().getSectionByXY(self.__clickX, self.__clickY)

                # dont allow sectioning of minimized or collapsed panels
                if sec._collapsed or sec._isMinimizePanel: return

                if self.__splitDir == HORIZONTAL:
                    dist = distX
                elif self.__splitDir == VERTICAL:
                    dist = distY
                else:   # havent moved enough...
                    return

                # we moved at least 10% of the section
                if dist > self.__minSplitDist:   
                    sec.split(x, y, self.__splitDir)
                    if ptr: ptr.showSplitter(0, 0, 0, 0, False)



    def __onDrag(self, event):
        x,y = event.x, event.y
        ptr = event.device.pointer
        if not ptr:  return

        # figure out if we moved a minimal distance, if so, draw the splitter
        distX = abs(x - self.__clickX)
        distY = abs(y - self.__clickY)

        if self.__splitDir == -1:
            if muchGreater(distX, distY):
                self.__splitDir = HORIZONTAL
            elif muchGreater(distY, distX):
                self.__splitDir = VERTICAL
            else:
                return

        # dont allow sectioning of minimized or collapsed panels
        sec = getLayoutSections().getSectionByXY(self.__clickX, self.__clickY)
        if sec._collapsed or sec._isMinimizePanel: return
        
        # show the temporary splitter indicator...
        if self.__splitDir == HORIZONTAL:
            if distX > self.__minSplitDist:   
                ptr.showSplitter(sec.getX(), y, sec.getX()+sec.getWidth(), y, True)
            else:
                ptr.showSplitter(0, 0, 0, 0, False)
                
        elif self.__splitDir == VERTICAL:
            if distY > self.__minSplitDist:   
                ptr.showSplitter(x, sec.getY(), x, sec.getY()+sec.getHeight(), True)
            else:
                ptr.showSplitter(0, 0, 0, 0, False)
                

                
    def __onMultiTouchSwipe(self, event):
        sec = getLayoutSections().getSectionByXY(event.startX, event.startY)
        sec._onMultiTouchSwipe(event)

    
    def __onMove(self, event):
        if self.recording:
            self.positions.append("%s %d %d\n"%(self.dev, event.x, event.y))
        

    def __onLeftWindow(self, event):
        pass


    ### record pointer movement data
    def startRecording(self, device):
        print "starting recording"
        self.dev = device
        self.positions = []
        self.recording = True


    def stopRecording(self):
        print "stopped recording"
        self.recording = False
        f = open("movementData.txt", "a")
        f.writelines(self.positions)
        f.flush()
        f.close()
        

    def __onEnteredWindow(self, event):
        pointer = event.device.pointer
        if pointer:
            pointer.resetPointer()
            pointer.showOutApp()
    
