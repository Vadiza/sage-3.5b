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


import overlays.splitter as splitterMod
#import overlays.menu as menu
import overlays.button as button
import overlays.thumbnail as thumbnail
#import overlays.label as label
import overlays.sizer as sizer
import overlays.panel as panel
from globals import *
import pickle
import time
import operator
import subprocess as sp
from sageGate import addGate, getGate

sys.path.append("../dim/hwcapture")
from managerConn import ManagerConnection

def makeNew():
    """ this is how we instantiate the object from
    the main program that loads this plugin """
    return Sections()


# must have this to load the plugin automatically
TYPE = UI_PLUGIN
LOAD_LAST = True  # loads the plugin last because it depends on others


# how many minimize sections do we make?
# the more we have, the closer the app will be 
# to its original position when minimized...
NUM_MINIMIZE_SECTIONS = 6



class Section:
    def __init__(self, bounds, parent, relPos=None, makeWidgets=True, tiled=False):
        self.addWidget = getOverlayMgr().addWidget
        self.removeWidget = getOverlayMgr().removeWidget
        self.org = getLayoutOrganization()
        self.sageData = getSageData()

        self.relPos = relPos
        self.splitter = None
        self.bounds = bounds
        self.children = []  # only two, left and right
        self.parent = parent
        self._isMinimizePanel = False
        self._collapsed = False

        # min distance we have to drag to minimize all or collapse a section... in pixels 
        self.localdisplay = self.sageData.getDisplayInfo()
        self.__minGestureDist = int(0.12 * min(self.localdisplay.sageW, self.localdisplay.sageH))

        # controlled by Organization ui
        self.tiled = tiled

        # is it a shared section
        self.isShared = False
        self.hostip = None
        # shared pointers info
        self.manager = None
        self.pointers = set()
        self.remotedisplay = None

        # widgets dont have to be made if we are loading this section 
        # from a saved state where this section had children
        self.p = None
        if makeWidgets:
            if SHOW_USER_INTERFACE:
                self.makeWidgets()

        writeLog(LOG_SECTION_NEW, self, self.bounds)


        # for proper pickling...
    def __getstate__(self):
        d = {}
        d['bounds'] = self.bounds
        d['parent'] = self.parent
        d['relPos'] = self.relPos
        d['tiled'] = self.tiled
        d['children'] = self.children
        d['splitter'] = self.splitter
        d['_collapsed'] = self._collapsed
        return d
    

    def makeWidgets(self):
        self._makePanel()
        
        #self.s = sizer.Sizer(direction=HORIZONTAL)
        #self.s.align(RIGHT, TOP)
        self.s = sizer.Sizer(direction=VERTICAL)
        self.s.align(RIGHT, CENTER)

        bs = 30
        alpha = 120

        # layout btn
        self.layoutB = self.org.makeLayoutButtons(bs, alpha)
        self.layoutB.setUserData(self)
        if self.tiled:
            self.layoutB.setState(BUTTON_DOWN)
        self.s.addChild(self.layoutB)

        # splitter buttons
        self.vb = thumbnail.Thumbnail()
        self.vb.setSize(bs,bs)
        self.vb.setUpImage(opj("images", "vertical_divider_btn_up.png"))
        self.vb.setDownImage(opj("images", "vertical_divider_btn_over.png"))
        self.vb.setOverImage(opj("images", "vertical_divider_btn_over.png"))
        self.vb.align(CENTER, CENTER, xMargin=10, yMargin=10)
        self.vb.setUserData(VERTICAL)
        self.vb.setScaleMultiplier(2.0)
        self.vb.onUp(self.__onSplitBtn)
        self.s.addChild(self.vb)

        self.hb = thumbnail.Thumbnail()
        self.hb.setSize(bs,bs)
        self.hb.align(CENTER, CENTER, xMargin=10, yMargin=10)
        self.hb.setUpImage(opj("images", "horizontal_divider_btn_up.png"))
        self.hb.setDownImage(opj("images", "horizontal_divider_btn_over.png"))
        self.hb.setOverImage(opj("images", "horizontal_divider_btn_over.png"))
        self.hb.setUserData(HORIZONTAL)
        self.hb.setScaleMultiplier(2.0)
        self.hb.onUp(self.__onSplitBtn)
        self.s.addChild(self.hb)

        self.pdf = thumbnail.Thumbnail()
        self.pdf.setSize(bs,bs)
        self.pdf.align(CENTER, CENTER, xMargin=10, yMargin=10)
        self.pdf.setUpImage(opj("images", "pdf_btn_up.png"))
        self.pdf.setDownImage(opj("images", "pdf_btn_over.png"))
        self.pdf.setOverImage(opj("images", "pdf_btn_over.png"))
        self.pdf.setUserData(HORIZONTAL)
        self.pdf.setScaleMultiplier(2.0)
        self.pdf.onUp(self.__onPDFBtn)
        self.s.addChild(self.pdf)

        self.edl = thumbnail.Thumbnail()
        self.edl.setSize(bs,bs)
        self.edl.align(CENTER, CENTER, xMargin=10, yMargin=10)
        self.edl.setUpImage(opj("images", "edl_btn_up.png"))
        self.edl.setDownImage(opj("images", "edl_btn_over.png"))
        self.edl.setOverImage(opj("images", "edl_btn_over.png"))
        self.edl.setUserData(HORIZONTAL)
        self.edl.setScaleMultiplier(2.0)
        self.edl.onUp(self.__onEDLBtn)
        self.s.addChild(self.edl)


        self.master = thumbnail.Thumbnail()
        self.master.setSize(bs,bs)
        self.master.align(CENTER, CENTER, xMargin=10, yMargin=10)
        self.master.setUpImage(opj("images", "master_btn_up.png"))
        self.master.setDownImage(opj("images", "master_btn_down.png"))
        self.master.setOverImage(opj("images", "master_btn_over.png"))
        self.master.setUserData(HORIZONTAL)
        self.master.setScaleMultiplier(2.0)
        self.master.setToggle(True)
        self.master.onUp(self.__onMasterBtnUp)
        self.master.onDown(self.__onMasterBtnDown)
        self.s.addChild(self.master)


        # i.e. this isn't root section...
        if self.parent:

            # collapse button
            self.collapseBtn = thumbnail.Thumbnail()
            self.collapseBtn.setSize(bs,bs)
            self.collapseBtn.setToggle(True)
            self.collapseBtn.align(CENTER, CENTER, xMargin=10, yMargin=10)
            self.collapseBtn.setUpImage(opj("images", "fast-forward.png"))
            self.collapseBtn.setDownImage(opj("images", "rewind.png"))
            self.collapseBtn.setScaleMultiplier(2.0)
            self.collapseBtn.onUp(self.__onCollapseBtn)
            self.collapseBtn.onDown(self.__onCollapseBtn)
            self.collapseBtn.setTransparency(alpha)
            #self.s.addChild(self.collapseBtn)

            # close button
            closeB = thumbnail.makeNew()
            closeB.setSize(bs,bs)
            closeB.setUpImage(opj("images", "close_up.png"))
            closeB.setDownImage(opj("images", "close_over.png"))
            closeB.setOverImage(opj("images", "close_over.png"))
            closeB.align(CENTER, CENTER, xMargin=10, yMargin=10)
            closeB.onUp(self.__onRemoveSection)
            closeB.setScaleMultiplier(2.0)
            closeB.setTransparency(alpha)
            self.s.addChild(closeB)
            

        self.p.addChild(self.s)
        self.addWidget(self.p)        

        
    def _setButtonCollapsed(self):
        """ used on reload of state to set the button properly """
        if self._collapsed and self.parent:
            self.collapseBtn.setState(BUTTON_DOWN)
            self.layoutB.hide()

                         
    def _makePanel(self):
        self.p = panel.Panel()
        self.p.setTransparency(1)
        #self.p.setBackgroundColor(255,255,255)
        self.p._canScale = False
        self.p._eventTransparent = True
        #self.p.z = BOTTOM_Z-5
        self.p.fitInWidth(False)
        self.p.fitInHeight(False)
        self._updateSectionBounds()


    def hideWidgets(self):
        if self.p:
            self.p.hide()


    def showWidgets(self):
        if self.p:
            self.p.show()


    def __onRemoveSection(self, btn):
        getLayoutSections().removeSection(self)

        
    def removeWidgets(self):
        writeLog(LOG_SECTION_REMOVE, self)
        self.removeWidget(self.p)
        

    def _updateSectionBounds(self):
        """ updates the widgets inside because the position and/or size changed """

        if self.p:
            self.p.setPos(self.bounds.left, self.bounds.bottom)
            self.p.setSize(self.bounds.getWidth(), self.bounds.getHeight())
            self.p._refresh()


    def _updateSplitter(self):
        """ update the position and size of the splitter if the section it splits changed """

        if self.splitter:
            if self.splitter.direction == HORIZONTAL:
                x = self.bounds.left
                y = self.bounds.bottom + int(round(self.bounds.getHeight() * self.splitter.relPos))
                w = self.bounds.getWidth()
                h = self.splitter.getHeight()
            else:
                y = self.bounds.bottom
                x = self.bounds.left + int(round(self.bounds.getWidth() * self.splitter.relPos))
                w = self.splitter.getWidth()
                h = self.bounds.getHeight()
            self.splitter.setPos(x,y)
            self.splitter.setSize(w,h)
            self.splitter._refresh()
        
    
        # is x,y inside the section?
    def isIn(self, x, y):  
        return self.bounds.isIn(x,y)


        # returns all apps that are in this section
    def getSectionApps(self):
        a = []
        for app in self.sageData.getRunningApps().values():
            if app.section == self:
                a.append(app)
        return a
    

    def fitAppInSection(self, app, cx=None, cy=None):

        # first check if the windows were tiled in the old section
        # if so, re-tile it because we just removed an app from there
        prevSec = app.section
        app.section = self
        
        # if this is a minimize panel, mark the app as minimized
        appOverlay = getOverlayMgr().getAppOverlay(app)
        if appOverlay:  
            appOverlay.minimize( self._isMinimizePanel )
        else:  
            # at least set this... then the appOverlay will set it once it initializes
            app.minimize( self._isMinimizePanel )


        # if the section is not shared and the app is shared, kill it remotely
        if not self.isShared and app.isShared:
            local_apps = getSageGate().getAppStatus()
            local_params = local_apps[str(app.appId)][1].split()
            directory, local_filename = os.path.split( local_params[-1] )
            for h in app.sharedHosts:
                rgate, rdata = getGate( h )
                # Get local information
                remote_apps  = rgate.getAppStatus()
                running_apps = rdata.getRunningApps()
                winner = None
                for k in remote_apps:
                    ra = remote_apps[k]
                    rparams = ra[1].split()
                    rdir, rfile = os.path.split( rparams[-1] )
                    if rfile == local_filename:
                        winner = int(k)
                for v in running_apps.values():
                    remote_filename = v.title
                    if v.appName == app.appName:
                        if v.appId == winner:
                            rgate.sendAppEvent(APP_QUIT, v.sailID)
            app.isShared = False
            app.sharedHosts.clear()
        else: 
            # if the section is shared and not the app, share it then
            if self.isShared and not app.isShared:
                if "imageviewer" in app.getName().lower() or "mplayer" in app.getName().lower() or "pdfviewer" in app.getName().lower():
                    gate = getSageGate()
                    addGate(self.hostip)
                    gate.showContentRemotely(app, self.hostip)
                    app.sharedHosts.add(self.hostip)
                    app.isShared = True


        # tile the old section once removed...
        #if prevSec and prevSec != self and prevSec.tiled:
        #    print "Tiling OLD, ", prevSec, " for app: ", app.getId()
        #    prevSec.tileWindows()

        # if we are in tile mode, tile instead
        if self.tiled:
            #print "Tiling section: ", self
            self.tileWindows()
            return
                        
        # margin for fitting
        margin = 10
        
        sx = self.getX() + margin
        sy = self.getY() + margin
        sw = self.getWidth() - margin*2
        sh = self.getHeight() - margin*2

        if not cx or not cy:
            cx = app.getCenterX()
            cy = app.getCenterY()
                
        newA = self.getArea()
        oldA = 1

        # the app is too big so make it smaller
        if app.getWidth() > self.getWidth() or \
               app.getHeight() > self.getHeight():
            app.fitWithin(sx, sy, sw, sh, margin=0, saveBounds=False, cx=cx, cy=cy)


        # the app could be bigger...
        
        elif False: #newA / float(oldA) > 1.6 and app.getArea() < app.getInitialArea():
            newW = app.getInitialWidth()
            newH = app.getInitialHeight()

            if newW > sw or newH > sh:  # the app is gonna be too big, so make it fit
                app.fitWithin(sx, sy, sw, sh, margin=0, saveBounds=False, cx=cx, cy=cy)
                
            else:              # just use the original (initial) window size
                if cx + newW/2 > sx+sw:
                    newX = sx+sw - newW
                elif cx - newW/2 < sx:
                    newX = sx
                else:
                    newX = cx - newW/2

                if cy + newH/2 > sy+sh:
                    newY = sy+sh - newH
                elif cy - newH/2 < sy:
                    newY = sy
                else:
                    newY = cy - newH/2

                app.resizeWindow(newX, newX+newW, newY, newY+newH)
        

        # nothing was fitted so just position the app inside the section
        else:
            newW = app.getWidth()
            newH = app.getHeight()

            if cx + newW/2 > sx+sw:
                newX = sx+sw - newW
            elif cx - newW/2 < sx:
                newX = sx
            else:
                newX = cx - newW/2

            if cy + newH/2 > sy+sh:
                newY = sy+sh - newH
            elif cy - newH/2 < sy:
                newY = sy
            else:
                newY = cy - newH/2

            app.resizeWindow(newX, newX+newW, newY, newY+newH)


    def tileWindows(self):
        self.org.tileWindows(sectionToTile=self)
        
    def unTileWindows(self):
        if not self.tiled:
            self.org.unTileWindows(self.getSectionApps())

    def getBounds(self):
        return self.bounds

    def getArea(self):
        return self.bounds.getArea()

    def getWidth(self):
        return self.bounds.getWidth()

    def getHeight(self):
        return self.bounds.getHeight()

    def getX(self):
        return self.bounds.left
    
    def getY(self):
        return self.bounds.bottom

    def _onSplitterMoved(self):
        getLayoutSections().recalculateSections(self)
        writeLog(LOG_SECTION_RESIZE, self, self.bounds)

    def _onMultiTouchSwipe(self, event):
        if event.lifePoint == GESTURE_LIFE_END:
            self.p._resetCurtain()

        else:   
            
            # what is the predominant direction of the gesture??
            """
            if muchGreater(abs(event.dX), abs(event.dY)):   # HORIZONTAL...  collapse a panel
                return
                if abs(event.dX) > self.__minGestureDist: #170*getGlobalScale():
                    self.collapse(True)
                    event.device._resetMultiTouch()
                    self.p._resetCurtain()
                else:
                    print "Drawing at: ", float(event.dX) / self.__minGestureDist
                    self.p._drawCurtain(HORIZONTAL, float(event.dX) / self.__minGestureDist )
            """
            #else:                                 # VERTICAL....   minimize all apps
            if event.dY < -1*self.__minGestureDist: #-170*getGlobalScale():
                self.__minimizeAllApps()
                event.device._resetMultiTouch()
                self.p._resetCurtain()
            else:
                #print "Drawing at: ", float(abs(event.dY)) / self.__minGestureDist
                self.p._drawCurtain(VERTICAL, float(abs(event.dY)) / self.__minGestureDist )
                

    def __minimizeAllApps(self):
        writeLog(LOG_SECTION_MINIMIZE_ALL, self)
        for app in self.getSectionApps():
            getLayoutSections().minimizeApp(app) 


    # ------------   COLLAPSE SECTION   --------------#


    def __onCollapseBtn(self, btn):
        self.collapse( btn.getState() == BUTTON_DOWN )


    def __findSuitableParent(self, sec=None):
        """ finds the top-most, direct parent of that has 
            the splitter of the same direction as ours 
        """

        if not sec: sec = self

        if sec.parent and sec.parent.splitter and sec.splitter and \
                sec.parent.splitter.direction == sec.splitter.direction:
            return self.__findSuitableParent(sec.parent)
        else:
            return sec
        

    def __distributeSpace(self, newSpace):
        parent = self.__findSuitableParent(self.parent)
        
        spl = parent.splitter
        if parent.relPos == RIGHT or parent.relPos == TOP:
            spl.setPos( spl.getX() - newSpace[0], spl.getY() - newSpace[1] )
        else:
            spl.setPos( spl.getX() + newSpace[0], spl.getY() + newSpace[1] )
        spl._refresh()
        return parent

        
    def collapse(self, doCollapse):
        if not self.parent: return   # no collapse on root...

        # find parent's splitter
        spl = self.parent.splitter
        
        if doCollapse:
            spl._preCollapsedPos = spl.relPos  # preserve the old position
            
            # we simulate collapse of the panel by moving the parent's splitter
            # so find the new position of the splitter that will make the panel look collapsed
            # also, find the right parent to give all the empty space to... 
            # if none found, give it to the sibling
            if self.relPos == TOP or self.relPos == RIGHT:
                newSpace = spl.setRelPos( 1-COLLAPSE_RATIO )  # 15% of the section
            else:
                newSpace = spl.setRelPos( COLLAPSE_RATIO )
            self.layoutB.hide()

        else:
            newSpace = spl.setRelPos(spl._preCollapsedPos)
            #newSpace = (-1*newSpace[0], -1*newSpace[1])  # invert the newspace since we are restoring
            self.layoutB.show()

        #resizedParent = self.__distributeSpace( newSpace )        
        
        # this will update everything based on the new splitter position
        spl._refresh()
        #getLayoutSections().recalculateSections( resizedParent )
        getLayoutSections().recalculateSections( self.parent )
        self._collapsed = doCollapse
        
        #if doCollapse:
        #    self.tileWindows()
        #else:
        #    self.unTileWindows()



    # ------------   EXPORT SECTION   --------------#

    def __onEDLBtn(self, btn):
        print("__onEDLBtn")
        movielist = {}
        for app in self.getSectionApps():
            if not app.isMinimized() and "mplayer" in app.getName().lower():
                appo = getOverlayMgr().getAppOverlay(app)
                print("App: %s" % app.getName())
                print("\t filename %s" % appo.fullpathname)
                print("\t center %d x %d" % ( app.getCenterX(), app.getCenterY() ) )
                print("\t l r t b %d %d %d %d" % ( app.getLeft(), app.getRight(), app.getTop(), app.getBottom() ) )
                movielist[appo.fullpathname] = [app.getCenterX(), app.getCenterY(), app.getLeft(), app.getRight(), app.getTop(), app.getBottom() ]
 
        if len(movielist) > 0:
            # sorting
            sorted_app = sorted(movielist.iteritems(), key=lambda (k,v): v[0])
           
            # name of the doc is based on the time
            docName = time.strftime("%a_%b%d_%Y_%X", time.localtime())
            pdir = opj(getFileServer().GetLibraryPath(), "playlist")
            vdir = opj(getFileServer().GetLibraryPath(), "video")
            # the image path saved by this function
            playlist = opj(pdir, "Playlist_"+docName+".m3u")
            fpl = open(playlist, 'w+')
            fpl.write("#EXTM3U\n")
            for im in sorted_app:
                fn = os.path.relpath( opj(vdir, im[0]) , vdir)
                fpl.write("#EXTINF: %s\n" % im[0])
                fpl.write("%s\n" % opj(opj("..","video"),fn) )
            fpl.close()
            showFile(playlist, self.bounds.left+20, self.bounds.bottom+20)
        print("________")

    def __onPDFBtn(self, btn):
        print("__onPDFBtn")
        imagelist = {}
        for app in self.getSectionApps():
            if not app.isMinimized() and "imageviewer" in app.getName().lower():
                appo = getOverlayMgr().getAppOverlay(app)
                print("App: %s" % app.getName())
                print("\t filename %s" % appo.fullpathname)
                print("\t center %d x %d" % ( app.getCenterX(), app.getCenterY() ) )
                print("\t l r t b %d %d %d %d" % ( app.getLeft(), app.getRight(), app.getTop(), app.getBottom() ) )
                #imagelist.append( appo.fullpathname )
                imagelist[appo.fullpathname] = [app.getCenterX(), app.getCenterY(), app.getLeft(), app.getRight(), app.getTop(), app.getBottom() ]
 
        if len(imagelist) > 0:
            # sorting
            sorted_app = sorted(imagelist.iteritems(), key=lambda (k,v): v[0])
            #sorted_two = sorted(sorted_app, key=lambda (k,v): v[1], reverse=True)
           
            # name of the doc is based on the time
            docName = time.strftime("%a_%b%d_%Y_%X", time.localtime())
            pdir = opj(getFileServer().GetLibraryPath(), "pdf")
            idir = opj(getFileServer().GetLibraryPath(), "image")
            # the image path saved by this function
            storyboard = opj(pdir, "Storyboard_"+docName+".pdf")
            mergeCmd = "convert -density 150 "
            for im in sorted_app:
                fn = opj(idir, im[0])
                mergeCmd += " -gravity None " + fn + " -box white -gravity North -annotate 0 '%f' "
            # in a Letter page
            #mergeCmd += " -gravity None -page Letter -density 72 %s " % storyboard
            mergeCmd += " -gravity None -page 792x612 -density 72 %s " % storyboard
            # full size (first image dictates the page size)
            #mergeCmd += " -gravity None -density 72 %s " % storyboard
            print("Command: ", mergeCmd)
            if hasattr(sp, "check_call"):
                sp.check_call(mergeCmd.split())
            else:
                sp.call(mergeCmd.split())
            showFile(storyboard, self.bounds.left+20, self.bounds.bottom+20)
        print("________")
        

    # ------------   Send pointer to remote location   --------------#
    def hidePointer(self, deviceName):
        dn = deviceName.split(":")[1]
        if deviceName in self.pointers:
            self.pointers.remove(deviceName)
            if self.manager.conn.connected:
                self.manager.disconnect()

    def forwardPointer(self, deviceName, x, y):
        if self.isShared:
            dn = deviceName.split(":")[1]
            if deviceName not in self.pointers:
                self.pointers.add(deviceName)
                self._sendInitialMsg(dn, 10) # 10 : HAS_DOUBLE_CLICK
                # Get handlers on the remote site
                rgate, rdata = getGate( self.hostip )
                # Get the remote display dimensions
                self.remotedisplay = rdata.getDisplayInfo()
            localw = float ( self.getWidth() )
            localh = float ( self.getHeight() )
            localx = float ( self.getX() )
            localy = float ( self.getY() )
            remotew = float ( self.remotedisplay.sageW )
            remoteh = float ( self.remotedisplay.sageH )
            lx = (x-localx) / localw
            ly = (y-localy) / localh
            offsetx = 0.0
            offsety = 0.0
            # adjust remote dimensions to match local aspect ratio
            if (remotew/remoteh) >= (localw/localh):
                remotew = remoteh * localw / localh
                offsetx = ( float ( self.remotedisplay.sageW ) - remotew ) / 2.0
            else:
                remoteh = remotew * localh / localw
                offsety = ( float ( self.remotedisplay.sageH ) - remoteh ) / 2.0
            lx = lx * remotew
            ly = ly * remoteh
            if offsetx > 0.0:
                lx += offsetx
            if offsety > 0.0:
                ly += offsety
            lx = lx / float( self.remotedisplay.sageW )
            ly = ly / float( self.remotedisplay.sageH )
            self._sendMsg(dn, 1, lx,ly)  # 1 : move

    def _sendMsg(self, dn, *data):
        msg = ""
        for m in data:
            msg+=str(m)+" "
        self.manager.sendMessage(dn+"@"+WALL_NAME, "mouse", msg)

    def _sendInitialMsg(self, dn, *data):
        msg = ""
        for m in data:
            msg+=str(m)+" "
        self.manager.initialMsg(dn+"@"+WALL_NAME, "mouse", msg)

    # ------------   MASTER SECTION   --------------#

    def __onMasterBtnUp(self, btn):
        if self.isShared:
            # section not shared anymore
            self.isShared = False
            # close the pointer connection
            if self.manager.conn.connected:
                self.manager.disconnect()
            self.manager.quit()
            self.pointers.clear()
            self.remotedisplay = None

            # flip the flag on the apps in the section
            local_apps = getSageGate().getAppStatus()
            for app in self.getSectionApps():
                if app.isShared:
                    local_params = local_apps[str(app.appId)][1].split()
                    directory, local_filename = os.path.split( local_params[-1] )
                    for h in app.sharedHosts:
                        rgate, rdata = getGate( h )
                        # Get remote information
                        remote_apps  = rgate.getAppStatus()
                        running_apps = rdata.getRunningApps()
                        winner = None
                        for k in remote_apps:
                            ra = remote_apps[k]
                            rparams = ra[1].split()
                            rdir, rfile = os.path.split( rparams[-1] )
                            if rfile == local_filename:
                                winner = int(k)
                        for v in running_apps.values():
                            remote_filename = v.title
                            if v.appName == app.appName:
                                if v.appId == winner:
                                    rgate.sendAppEvent(APP_QUIT, v.sailID)
                    app.isShared = False
                    app.sharedHosts.clear()


    def __onMasterBtnDown(self, btn):
        setMasterSiteMode(True)
        # Get the host to share with
        site = getMasterSite()
        if site:
            print("Master section: ", site, site )
            self.isShared = True
            self.hostip = site
            # open a device manager connection
            self.manager = ManagerConnection(site)
            # Share content of the section
            gate = getSageGate()
            addGate(site)
            for app in self.getSectionApps():
                if not app.isMinimized() and not app.isShared and ( "imageviewer" in app.getName().lower() or "mplayer" in app.getName().lower() or "pdfviewer" in app.getName().lower() ):
                    #print("App: %s" % app.getName())
                    gate.showContentRemotely(app, site)
                    app.sharedHosts.add(site)
                    app.isShared = True

    # ------------   SPLIT SECTION   --------------#

    def __onSplitBtn(self, btn):
        
        # make the splitter first
        d = btn.getUserData()   # direction
        if d == HORIZONTAL:
            x = self.bounds.left
            y = self.bounds.getCenterY()
        else:
            x = self.bounds.getCenterX()
            y = self.bounds.bottom

        self.split(x,y,d)



    def split(self, x, y, direct):
        """ splits the section at x,y in specified direction """

        # figure out where we split the section
        s = int(10*(getGlobalScale()/2.0))  # thickness of the splitter
        if direct == HORIZONTAL:
            x = self.bounds.left
            w = self.bounds.getWidth()
            h = s
        else:
            y = self.bounds.bottom
            h = self.bounds.getHeight()
            w = s

        # make the splitter
        self.splitter = splitterMod.Splitter(direction=direct)
        self.splitter.setUserData(self)
        self.splitter.setPos(x,y)
        self.splitter.setSize(w,h)
        self.addWidget(self.splitter)
        
        # actually make the two new sections
        self.removeWidgets()
        getLayoutSections()._splitSection(self, self.splitter)





class MinimizeSection(Section):
    def __init__(self, bounds):
        Section.__init__(self, bounds, None, None, False, tiled=True)
        if SHOW_USER_INTERFACE:
            self._makePanel()
        self._isMinimizePanel = True

    
    def _makePanel(self):
        self.p = panel.Panel()
        self.p.setBackgroundColor(0,0,0)
        self.p.setTransparency(50)
        self.p._canScale = False
        self.p._eventTransparent = True
        self.p.fitInWidth(False)
        self.p.fitInHeight(False)
        #self.p.drawBorder(True)
        self._updateSectionBounds()  
        self.addWidget(self.p)
    

            

class Sections:
    def __init__(self):
        self.addWidget = getOverlayMgr().addWidget
        self.removeWidget = getOverlayMgr().removeWidget
        self.sageData = getSageData()
        self.evtMgr = getEvtMgr()

        # so that others can access it
        setLayoutSections(self)
        
        # make the main, root section
        self.disp = self.sageData.getDisplayInfo()
        l = 0
        r = self.disp.sageW
        b = self.disp.usableZeroY
        t = b + self.disp.usableH
        self.root = Section( Bounds(l, r, t, b), None )
        
        # make all the minimize sections...
        self.__minimizeSections = []
        minSectionWidth = int(self.disp.sageW / NUM_MINIMIZE_SECTIONS)  # the width of each section        
        for i in range(0, NUM_MINIMIZE_SECTIONS):
            minL = minSectionWidth*i
            minR = minSectionWidth*(i+1)
            self.__minimizeSections.append( MinimizeSection( Bounds(minL, minR, b, 0) ) )

        if SHOW_USER_INTERFACE:
            self.__makeMinimizePanel()  # just for the borders actually


        # event stuff
        self.evtMgr.register(EVT_NEW_APP, self.__onNewApp)
        self.evtMgr.register(EVT_APP_INFO, self.__onAppInfo)

        # update when remote app is created
        self.evtMgr.register(EVT_NEW_REMOTEAPP, self.__onNewRemoteApp)


    def __makeMinimizePanel(self):
        p = panel.Panel()
        p.setPos(0,0)
        p.setSize(self.disp.sageW, self.disp.usableZeroY)
        p.setBackgroundColor(0,0,0)
        p.setTransparency(50)
        p._canScale = False
        p._eventTransparent = True
        p.fitInWidth(False)
        p.fitInHeight(False)
        p.drawBorder(True)
        self.addWidget(p)


        # If a new remote app appears, just relayout
    def __onNewRemoteApp(self, event):
        self.recalculateSections(self.root)


    def __onNewApp(self, event):
        app = event.app
        newSec = self.getSectionByApp(app)
        newSec.fitAppInSection(app)
        #print("onNewApp: isShared ", newSec.isShared)


    def __onAppInfo(self, event):
        """ this is needed in case some other UI client moves the app... 
            otherwise, the appOverlay actually does this 
        """
        #if event.myMsg:  # this message came from us so don't do anything! (otherwise race conditions could happen)
        #    return
        
        if not event.app.maximized and not event.app.sectionMaximized:
            newSec = self.getSectionByApp(event.app)
            if event.app.section._isMinimizePanel: 
                if event.app.section != newSec:  # did the app actually change a section?
                    newSec.fitAppInSection(event.app)
                

    def recalculateSections(self, section):
        """ 
            recursively adjust the sizes and positions of all children of "section"
            - typically called after a splitter moves or a section is collapsed
        """

	# Root section
	if not section.parent:
                # resize and reposition the apps
                for app in section.getSectionApps():
                    if not app.isMinimized():
                        section.fitAppInSection(app)
		return

        # first figure out where the splitter is
        sp = section.splitter
        sDir = sp.direction        
        if sp and sDir == HORIZONTAL:
            sPos = sp.getY()
        else:
            sPos = sp.getX()

            
        if section._collapsed: return

            
        # adjust the two children based on the splitter
        for child in section.children:

            if sDir == HORIZONTAL:   # HORIZONTAL
                if child.relPos == TOP:
                    child.bounds.bottom = sPos
                    child.bounds.top = section.bounds.top
                else:
                    child.bounds.top = sPos
                    child.bounds.bottom = section.bounds.bottom
                child.bounds.left = sp.getBounds().left
                child.bounds.right = sp.getBounds().right

            else:                    # VERTICAL
                if child.relPos == LEFT:
                    child.bounds.right = sPos
                    child.bounds.left = section.bounds.left
                else:
                    child.bounds.left = sPos
                    child.bounds.right = section.bounds.right
                child.bounds.top = sp.getBounds().top
                child.bounds.bottom = sp.getBounds().bottom
                    
            child._updateSectionBounds()
            child._updateSplitter()

            
            if len(child.children) > 0:
                self.recalculateSections(child)
            else:
                # resize and reposition the apps since the sections resized
                for app in child.getSectionApps():
                    if not app.isMinimized():
                        child.fitAppInSection(app)

        
    def _splitSection(self, section, splitter):
        """
            actually makes the two new sections based on the passed in splitter 
        """
        section.tiled = False
        bounds = section.bounds
        
        if splitter.direction == HORIZONTAL:
            sPos = splitter.getY()

            # calculate the bounds for the new sections
            # bottom
            r = bounds.right
            t = sPos
            l = bounds.left
            b = bounds.bottom
            section1 = Section( Bounds(l, r, t, b), section, BOTTOM )

            # top
            t = bounds.top
            b = sPos
            section2 = Section( Bounds(l, r, t, b), section, TOP )
        
        else:
            sPos = splitter.getX()

            # calculate the bounds for the new sections
            # left
            r = sPos
            t = bounds.top
            l = bounds.left
            b = bounds.bottom
            section1 = Section( Bounds(l, r, t, b), section, LEFT )

            # right
            l = sPos
            r = bounds.right
            section2 = Section( Bounds(l, r, t, b), section, RIGHT )

        section.children = [section1, section2]

        # update app section memberships now
        maximizedApps = []
        for app in section.getSectionApps():

            # maximized apps should be brought to front... so save them here
            if app.maximized or app.sectionMaximized:
                maximizedApps.append(app)

            # if they arent minimized either, reassign their section membership
            elif not app.isMinimized():
                newSec = self.getSectionByApp(app, section)  # are we in section1 or section2 now?
                newSec.fitAppInSection(app)

        # bring the maximized apps to front
        for app in maximizedApps:
            app.bringToFront()
        

    def getSectionByApp(self, app, sec=None):

        # check the minimize sections first
        for minSection in self.__minimizeSections:
            if minSection.isIn(app.getCenterX(), app.getCenterY()):
                return minSection

        # now check the rest of the sections
        else:
            if not sec:
                sec=self.root
            
            for child in sec.children:
                if child.isIn(app.getCenterX(), app.getCenterY()):
                    return self.getSectionByApp(app, child)
            else:
                return sec


    def getSectionByXY(self, x, y, sec=None):

        # check the minimize sections first
        for minSection in self.__minimizeSections:
            if minSection.isIn(x, y):
                return minSection

        # now check the rest of the sections
        else:
            if not sec:
                sec=self.root

            for child in sec.children:
                if child.isIn(x,y):
                    return self.getSectionByXY(x, y, child)
            else:
                return sec

            
    def minimizeApp(self, app):
        """ find minimize section that's closest to the app and minimize the app there """
        minSection = self.getSectionByXY(app.getCenterX(), 5)  # 5 as if the app was down there already...
        minSection.fitAppInSection(app)

    
         # returns a new section if app is now in a different section or false otherwise
    def isAppInNewSection(self, app, x, y):
        newSec = self.getSectionByXY(x, y)
        if app.section != newSec:
            return newSec
        else:
            return False


    def __updateAppSection(self, app):
        app.section = self.getSectionByApp(app)
        return app.section
    

    def __removeChildren(self, parent, first=True, removingAll=False):
        for child in parent.children:
            self.__removeChildren(child, False, removingAll)
            child.removeWidgets()
        
        parent.children = []
        self.removeWidget(parent.splitter)
        parent.splitter = None
        

    def removeSection(self, section):
        if section == self.root: return

        self.__removeChildren(section.parent)

        # make parent's widgets now...
        section.parent.makeWidgets()

        # change the sections of all apps to the parent of the deleted section
        for app in self.sageData.getRunningApps().values():
            self.__updateAppSection(app)


    def __removeAllSections(self):
        # all except the root... but remove root's widgets
        self.__removeChildren(self.root, removingAll=True)
        self.root.removeWidgets()
        

        # returns the state of all the sections for saving
    def getState(self):
        return self.root


    def loadState(self, root):
        if not root:  # in case we are loading an older state where sections werent saved
            return
        
        # clear the current state first
        self.__removeAllSections()
        
        self.__recreateTree(root)

        # update app memberships now
        for app in self.sageData.getRunningApps().values():
            if not app.isMinimized():
                app.section = self.getSectionByApp(app)

        

    def __recreateTree(self, sec, parent=None):

        # make the splitter if necessary
        newSpl = sec.splitter
        splitter = None
        if newSpl:
            splitter = splitterMod.Splitter(direction=newSpl.direction)
            splitter.setSize(newSpl.size[0], newSpl.size[1])
            splitter.setPos(newSpl.pos[0], newSpl.pos[1])
            splitter.relPos = newSpl.relPos
            if hasattr(newSpl, "_preCollapsedPos"):
                splitter._preCollapsedPos = newSpl._preCollapsedPos


        # make the sections... the first one is root so we
        # use the default one created by Sections class
        if not parent:
            oldSpl = self.root.splitter
            section = self.root
            self.root.tiled = sec.tiled
            if sec.tiled:
                self.root.layoutB.setState(BUTTON_DOWN)
            self.root.splitter = splitter

            if not splitter:  # if it didn't have a splitter, we'll have to re-create the widgets
                self.root.makeWidgets()
             
        # all other sections beside root
        else:
            section = Section( sec.bounds, parent, relPos=sec.relPos, makeWidgets=(splitter==None), tiled=sec.tiled)
            section.splitter = splitter
            if hasattr(sec, "_collapsed"):            
                section._collapsed = sec._collapsed
                section._setButtonCollapsed()


        # assign the section to splitter after it has been created
        if splitter:
            splitter.setUserData(section)
            self.addWidget(splitter)

        # make the children recursively... if any
        for child in sec.children:
            section.children.append(self.__recreateTree(child, section))

        return section
        
