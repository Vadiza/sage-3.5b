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
#         Allan Spale
#
############################################################################



# my imports
from SAGEShape import *
from sageData import *
from Mywx import *
import Graph
from globals import *
#from pointers import Pointers
import fileViewer
import preferences as prefs

# python imports
import time
from threading import Timer
from time import sleep
from math import floor,ceil
import os.path
import ConfigParser
import wx.lib.buttons as btns


DEBUG = False
if DEBUG: import pdb




class OneDisplay:
    """ contains the shapes for on """
    def __init__(self, l, r, t, b, sageDisplay, canvas):
        self.sageDisplay = sageDisplay
        self.canvas = canvas
        self.diagram = canvas.GetDiagram()
        self.cols = self.sageDisplay.cols
        self.rows = self.sageDisplay.rows
        self.sageW = self.sageDisplay.sageW
        self.sageH = self.sageDisplay.sageH
        self.bounds = Bounds(l,r,t,b)   # these are the bounds of the display... we have to make these panels fit here
        
        # create all the panels (Shapes)
        self.__CreatePanels()


    def __CreatePanels(self):
        self.__panels = [ [None]*self.cols for i in range(self.rows) ]
        
        panelW = self.bounds.getWidth() / self.cols
        panelH = self.bounds.getHeight() / self.rows
        
        for xOffset in range(self.cols):
            for yOffset in range(self.rows):
                self.__panels[yOffset][xOffset] = self.__AddShape(
                    ogl.RectangleShape(panelW, panelH), 
                    xOffset*panelW + (panelW/2) + self.bounds.left, 
                    yOffset*panelH + (panelH/2) + self.bounds.top,
                    wx.Pen(wx.Colour(51, 102, 102), 1), wx.Brush(tileColor), "")


    # draw boxes in the background as the frames
    # which should not be choosable or clickable
    def __AddShape(self, shape, x, y, pen, brush, text):
        shape.SetDraggable(False, False)
        shape.SetCanvas(self.canvas)
        shape.SetX(x)
        shape.SetY(y)
        if pen:    shape.SetPen(pen)
        if brush:  shape.SetBrush(brush)
        if text:
            for line in text.split('\n'):
                shape.AddText(line)
        self.diagram.AddShape(shape)
        shape.Show(True)
        
        return shape


    def Resize(self, l, r, t, b):
        # resize and reposition the panels here
        self.bounds.setAll(l,r,t,b)

        # resize and reposition the display tiles
        panelW = self.bounds.getWidth() / self.cols
        panelH = self.bounds.getHeight() / self.rows
        
        for xOffset in range(self.cols):
            for yOffset in range(self.rows):
                self.__panels[yOffset][xOffset].SetX(xOffset*panelW + (panelW/2) + self.bounds.left)
                self.__panels[yOffset][xOffset].SetY(yOffset*panelH + (panelH/2) + self.bounds.top)
                self.__panels[yOffset][xOffset].SetSize(panelW, panelH)
                    
    


class Backdrop:
    """ consists of OneDisplays """
    def __init__(self, sageDisplayInfo, canvas):
        self.displays = {}  # key=displayId, value=OneDisplay
        self.bounds = Bounds() # what's the area we have available for drawing
        self.sageDisplayInfo = sageDisplayInfo
        self.canvas = canvas
        self.dispByPos = {}   # key=placement, value=OneDisplay
        self.widths = {}      # key=placement, value=width in UI coords
        self.heights = {}     # key=placement, value=height in UI coords
        self.positions = {}   # key=placement, value=tuple of (l,r,t,b)



    def GetDisplayBounds(self, displayId):
        return self.displays[displayId].bounds
    

    def SetupDisplays(self,l,r,t,b):
        self.bounds.setAll(l,r,t,b)
                
        # first create all the displays
        for sageDisplay in self.sageDisplayInfo.getAllDisplays():
            newDisp = OneDisplay(l,r,t,b,sageDisplay,self.canvas)
            self.displays[ sageDisplay.getId() ] = newDisp
            self.dispByPos[ sageDisplay.placement ] = newDisp
            
        self.SetSize(self.bounds.left, self.bounds.right, self.bounds.top, self.bounds.bottom)
        
            
    def SetSize(self, l, r, t, b):
        self.bounds.setAll(l,r,t,b)
        
        self.__CalculateDisplayWidths()
        self.__CalculateDisplayHeights()

        # loop through each display and set the correct bounds for each
        for pos, disp in self.dispByPos.iteritems():
            if pos == LEFT_DISP:
                l = self.bounds.left
                r = l + self.widths[LEFT_DISP]
                t = self.bounds.top + self.heights[TOP_DISP]
                b = t + self.heights[LEFT_DISP]
                
            elif pos == MIDDLE_DISP:
                l = self.bounds.left + self.widths[LEFT_DISP]
                r = l + self.widths[MIDDLE_DISP]
                t = self.bounds.top + self.heights[TOP_DISP]
                b = t + self.heights[MIDDLE_DISP]
                            
            elif pos == RIGHT_DISP:
                l = self.bounds.left + self.widths[LEFT_DISP] + self.widths[MIDDLE_DISP]
                r = l + self.widths[RIGHT_DISP]
                t = self.bounds.top + self.heights[TOP_DISP]
                b = t + self.heights[RIGHT_DISP]
                
            elif pos == TOP_DISP:
                l = self.bounds.left + self.widths[LEFT_DISP]
                r = l + self.widths[TOP_DISP]
                t = self.bounds.top
                b = t + self.heights[TOP_DISP]

            elif pos == BOTTOM_DISP:
                l = self.bounds.left + self.widths[LEFT_DISP]
                r = l + self.widths[BOTTOM_DISP]
                t = self.bounds.top + self.heights[TOP_DISP] + self.heights[MIDDLE_DISP]
                b = t + self.heights[BOTTOM_DISP]
            
            # finally apply the size
            disp.Resize(l,r,t,b)
            
                
                            

    def __CalculateDisplayWidths(self):
        self.widths = {MIDDLE_DISP:0, LEFT_DISP:0, RIGHT_DISP:0, BOTTOM_DISP:0, TOP_DISP:0}
        totalW = self.sageDisplayInfo.getTotalWidth()
        
        # in UI coords
        for pos, disp in self.dispByPos.iteritems():
            self.widths[pos] = (disp.sageW / float(totalW)) * self.bounds.getWidth()
            
    def __CalculateDisplayHeights(self):
        self.heights = {MIDDLE_DISP:0, LEFT_DISP:0, RIGHT_DISP:0, BOTTOM_DISP:0, TOP_DISP:0}
        totalH = self.sageDisplayInfo.getTotalHeight()
        
        # in UI coords
        for pos, disp in self.dispByPos.iteritems():
            self.heights[pos] = (disp.sageH / float(totalH)) * self.bounds.getHeight()





############################################################################
#
#  CLASS: DisplayCanvas
#  
#  DESCRIPTION: This class describes the canvas where the tiled display
#               representation is drawn. It draws all the shapes, updates
#               the visual state of the UI as a result of messages received
#               from SAGE, it draws all the decorative graphics pertaining
#               to it. It also contains all the coordinate conversion methods.
#               All the drawing is done in the OnPaint() and Redraw() methods
#               so that way when you call canvas.Refresh() or canvas.Redraw()
#               it will use these overloaded methods. Another reason for
#               overloading these methods is to force them to use double buffering
#
#  DATE:        October, 2004
#
############################################################################

class DisplayCanvas(ogl.ShapeCanvas):
    def __init__(self, log, parent, sageGate, sageData, gmGraphManager, title):
        # the size passed in here is being changed later so it doesnt really matter
        ogl.ShapeCanvas.__init__(self, parent, -1, (0,0), (710,300), wx.NO_BORDER | wx.NO_FULL_REPAINT_ON_RESIZE)


        # some global parameters
        displaySize = wx.DisplaySize()  #the resolution of the current screen... we will base our frame size on that
        if displaySize[1] > 900:
            self._maxCanvasWidth = 800
            self._borderWidth = 25
        else:
            self._maxCanvasWidth = 650
            self._borderWidth = 20
        self.frame = parent.GetFrame()#wx.GetApp().GetTopWindow() #parent
        self.parent = parent
        self._maxCanvasHeight = 0
        self._maxDisplayHeight = 0
        self._maxDisplayWidth = 0
        self._aspectRatio = 1   #the aspect ratio for the whole canvas... this is not for the apps
        self.title = title
        self.displayInfoArrived = False
        self._minimizedShapes = {}  # key=displayId, value=hash of minimized shapes
        self._shapesTiled = False
        
        # main data and communication objects
        self.sageGate = sageGate
        self.sageData = sageData
        self.__gmGraphManager = gmGraphManager

        # support for file viewing and drag-and-dropping inside the UI
        self.dt = fileViewer.CanvasDropTarget(self)
        self.SetDropTarget(self.dt)
        
        # register sageData callbacks
        self.sageData.registerCallbackFunction(40000, self.OnSAGEStatusMsg)
        self.sageData.registerCallbackFunction(40001, self.OnSAGEAppInfoMsg)
        self.sageData.registerCallbackFunction(40002, self.OnSAGEPerfInfoMsg)
        self.sageData.registerCallbackFunction(40003, self.OnSAGEAppShutdownMsg)
        self.sageData.registerCallbackFunction(40004, self.OnSAGEDisplayInfoMsg)
        self.sageData.registerCallbackFunction(40005, self.OnSAGEZChangeMsg)
        self.sageData.registerCallbackFunction(40006, self.OnSAGEAppExecConfig)
        self.sageData.registerCallbackFunction(40007, self.OnSAGEDisplaysConnection)

        # set up the canvas
        self.log = log
        self.SetBackgroundColour('#006666')
        self.diagram = ogl.Diagram()
        self.SetDiagram(self.diagram)
        self.diagram.SetCanvas(self)
        self.shapes = {}  #a hash of all the apps that are CURRENTLY RUNNING (keyed by windowId)
        self.availableApps = {}     #a hash of all the apps available for running
                                        # in this form  key:value = windowId:SAGEAppInitial
        # now tell SAGE that we exist so we can get the current SAGE status
        self.sageGate.registerSage()
        
        # at this point we know we succeeded in connecting to SAGE so load all the images
        self.LoadImages()   #creates a hash with the "Images" keyed by the name        
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.Chillout)
        self.Bind(wx.EVT_MOTION, self.OnMouseEnter)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.cursorSet = False
  


#----------------------------------------------------------------------
# DRAWING METHODS
#----------------------------------------------------------------------    
    
    def OnPaint( self, event ):
        if DEBUG: print "OnPaint --> DisplayCanvas"; sys.stdout.flush()
        if not self.displayInfoArrived:
            if "__WXMSW__" in wx.PlatformInfo:  #windows needs a PaintDC created even if we dont use it
                dc = wx.PaintDC(self)
            return
        dc = wx.BufferedPaintDC( self, self.swapBuffer )

    def Chillout(self, event):
        """ For intercepting the EVT_ERASE_BACKGROUND. Do nothing here on purpose
            in order to reduce the flicker during drawing"""
        pass


    def OnClose(self, evt):
        pass


#----------------------------------------------------------------------


    # overridden so that we can specify the order in which the drawing occurs (because of the decorative images)
    def Redraw( self, dc=None):
        if DEBUG: print "Redraw --> DisplayCanvas"; sys.stdout.flush()
        if not self.displayInfoArrived:
            return
        # create the right kind of DC for double buffering
        if dc == None: 
            cdc = wx.ClientDC(self)
            self.PrepareDC(cdc)
            dc = wx.BufferedDC(cdc, self.swapBuffer)
            dc.SetBackground(wx.Brush(self.GetBackgroundColour()))
            dc.Clear()

        if self.GetDiagram().GetShapeList():
            self.SetCursor(wx.HOURGLASS_CURSOR)

        # first draw the underlying shapes (LCD panels)
        for object in self.GetDiagram().GetShapeList():
            if ((object.__class__.__name__ != MyRectangleShape.__name__ ) and
               (object.__class__.__name__ != MyControlPoint.__name__ )):
                   object.Draw(dc)

        # then draw the decorative images
        for name, image in self.images.iteritems():
            dc.DrawBitmap( image.bitmap, image.GetX(), image.GetY(), image.IsTransparent() )

        # draw the title (user @ machine)
        dc.SetTextForeground(wx.WHITE)
        textSize = dc.GetTextExtent(self.title)
        dc.DrawText(self.title, self.GetCanvasWidth()/2 - textSize[0]/2, (self.GetBorderWidth() - textSize[1])/2) 

        # finally draw the app shapes on top of everything
        for object in self.GetDiagram().GetShapeList():
            if ((object.__class__.__name__ == MyRectangleShape.__name__ ) or
               (object.__class__.__name__ == MyControlPoint.__name__ )):
                   object.Draw(dc)
   
        self.SetCursor(wx.STANDARD_CURSOR)


    #----------------------------------------------------------------------


    def OnSize(self, event=None):
        if DEBUG: print "OnSize --> DisplayCanvas"; sys.stdout.flush()        
        if not self.displayInfoArrived:
            return
        # manually maintain the aspect ratio of the display canvas
        parentSize = self.parent.GetClientSize()
        self.SetCanvasWidth( parentSize.width )
        self.SetCanvasHeight( round(self.GetCanvasWidth() / self.GetAspectRatio()) )
        self.SetSize((self.GetCanvasWidth(), self.GetCanvasHeight()))
        
        # change the display width now
        self.SetDisplayWidth( self.GetCanvasWidth() - 2 * self.GetBorderWidth() )
        heightFactor = self.tiledDisplayWidth / float (self.GetDisplayWidth() )
        self.SetDisplayHeight( self.GetCanvasHeight() - 1.5*self.GetBorderWidth()) #round (self.tiledDisplayHeight / heightFactor ) )

        # update the size of the swap buffer
        self.swapBuffer = wx.EmptyBitmap(self.GetCanvasWidth(), self.GetCanvasHeight())
        
        # reposition and resize all of the following
        self.RecalculateImages() #decorative images
        self.RecalculateTiles()  
        self.RecalculateShapes() 
        
        self.Redraw()
        #event.Skip()

           
    #----------------------------------------------------------------------


    def LoadImages(self):
        # load all the decorative images
        wx.InitAllImageHandlers()
        self.images = {}
        self.imageNames = [ "top_left", "top", "top_right", "left", 
                            "right", "bottom_left", "bottom", "bottom_right" ]
        for i in range(1, 9):
            self.images[ self.imageNames[i-1] ] = MyImage( "images/SAGEapp_0"+str(i)+".jpg" )       


    def RecalculateImages(self):
        # figure out all the sizes and positions for the decorative images and apply them
        positions = [ (0, 0),                 (self.GetLeft(), 0),                 (self.GetRight(), 0),
                      (0, self.GetTop()),                                          (self.GetRight(), self.GetTop()),
                      (0, self.GetBottom()),  (self.GetLeft(), self.GetBottom()),  (self.GetRight(), self.GetBottom()) ]

        sizes = [ (self.GetBorderWidth(), self.GetBorderWidth()),    (self.GetDisplayWidth(), self.GetBorderWidth()),    (self.GetBorderWidth(), self.GetBorderWidth()),
                  (self.GetBorderWidth(), self.GetDisplayHeight()),                                                      (self.GetBorderWidth(), self.GetDisplayHeight()),
                  (self.GetBorderWidth(), self.GetBorderWidth()/2),  (self.GetDisplayWidth(), self.GetBorderWidth()/2),  (self.GetBorderWidth(), self.GetBorderWidth()/2) ]

        for i in range(0, 8):
            image = self.images[ self.imageNames[i] ]
            image.Rescale( sizes[i][0], sizes[i][1] )
            image.SetX( positions[i][0] )
            image.SetY( positions[i][1] )
            

    def RecalculateTiles(self):
        l = self.GetBorderWidth()
        r = self.GetCanvasWidth() - self.GetBorderWidth()
        t = self.GetBorderWidth()
        b = t + self.GetDisplayHeight()
        self.backdrop.SetSize(l,r,t,b)
        

    def RecalculateShapes(self):
        # resize and reposition all the shapes
        # get the SAGE coordinates for the shape and convert them to UI coords again
        for s in self.GetShapeList():
            sageApp = self.sageData.getApp(s.GetId())
            s.Recalculate(
                self.ToUICoordsX(sageApp.getLeft(), sageApp.getDisplayId()),
                self.ToUICoordsX(sageApp.getRight(), sageApp.getDisplayId()),
                self.ToUICoordsY(sageApp.getTop(), sageApp.getDisplayId()),
                self.ToUICoordsY(sageApp.getBottom(), sageApp.getDisplayId())
                )




#----------------------------------------------------------------------
# TILING SHAPES
#----------------------------------------------------------------------

    def TileShapes(self):
        if self._shapesTiled:  # restore the shape to their previous state
            for shape in self.GetShapeList():
                if not shape.GetMinimized():
                    shape.RestoreOldParams()
                    shape.UpdateSAGEParams()
            self._shapesTiled = False
            return



        # figure out by how much we need to size down each shape so that they have a chance
        # of fitting them inside the tiled display (if perfectly tiled... which is very unlikely to happen))
        displayArea = self.GetDisplayHeight() * self.GetDisplayWidth()
        totalShapeArea = 0
        for shape in self.GetShapeList():
            if not shape.GetMinimized():
                totalShapeArea = totalShapeArea + shape.GetWidth() * shape.GetHeight()

        resizeRatio = 1
        if displayArea <= totalShapeArea*0.8:
            resizeRatio = totalShapeArea*0.8 / displayArea

        # set aside the shapes that are eligible for tiling (meaning no minimized shapes)
        # and preserve their size and position for later restore
        hashByArea = {}
        for shape in self.GetShapeList():
            if shape.GetMinimized():
                continue       #skip minimized shapes
            if not shape.GetMaximized(): 
                shape.PreserveOldParams()  #preserve the current size and position
            shape.SetSize( shape.GetWidth()/resizeRatio, shape.GetHeight()/resizeRatio )
            shape.UpdateSAGEParams()
            hashByArea[ (shape.GetWidth() * shape.GetHeight()) ] = shape  #key the shapes eligible for tiling by their area


        # now the hardest part, reposition the shapes so that they fit nicely (or as nice as it gets)
        # actually, try the other way around... first reposition the shapes and then resize them
        # so that they all fit inside the display
        

        self._shapesTiled = True

        
#----------------------------------------------------------------------
# DEALING WITH THE SHAPES
#----------------------------------------------------------------------

    def GetShape(self, windowId):
        if self.shapes.has_key(windowId):
            return self.shapes[windowId]
        else:
            return None

    def GetShapeList(self):  # returns a copy of the shape list
        tempList = []
        for s in self.diagram.GetShapeList(): 
            if ( s.__class__.__name__ == MyRectangleShape.__name__ ):
                tempList.append(s)
        return tempList

    def GetTopShape(self):  # returns the windowId of the shape that's on top
        minZ = sys.maxint
        windowId = sys.maxint
        for s in self.shapes.values():
            if s.GetZ() < minZ or (s.GetZ() == minZ and s.GetId() < windowId):
                minZ = s.GetZ()
                windowId = s.GetId()
        return windowId
    
##     # draw boxes in the background as the frames
##     # which should not be choosable or clickable
##     def MyAddBackShape(self, shape, x, y, pen, brush, text):
##         # Composites have to be moved for all children to get in place
##         shape.SetDraggable(False, False)
##         shape.SetCanvas(self)
##         shape.SetX(x)
##         shape.SetY(y)
##         if pen:    shape.SetPen(pen)
##         if brush:  shape.SetBrush(brush)
##         if text:
##             for line in text.split('\n'):
##                 shape.AddText(line)
##         self.diagram.AddShape(shape)
##         shape.Show(True)

##         return shape


    #----------------------------------------------------------------------


    # creates the shape that corresponds to the app and adds it to the diagram.
    def MyAddShape(self, app):
        name = app.getName()
        left = self.ToUICoordsX(app.getLeft(), app.getDisplayId())
        right = self.ToUICoordsX(app.getRight(), app.getDisplayId())
        bottom = self.ToUICoordsY(app.getBottom(), app.getDisplayId())
        top = self.ToUICoordsY(app.getTop(), app.getDisplayId())
        width = right - left
        height = bottom - top
        id = app.getId()
        z = app.getZvalue()
        title = app.getTitle()
        
        shape = MyRectangleShape(width, height, app, self.sageGate, self, title) 
        text = name + " " + str(id)
                           
        shape.SetDraggable(True, True)
        shape.SetCanvas(self)
        shape.SetMyX(left)
        shape.SetMyY(top)
        shape.SetId(id) #this is windowId
        shape.SetZ(z) 
        shape.SetPen(wx.Pen(shapeBorderColor))#wx.BLACK_PEN)
        shape.SetBrush( wx.Brush(shapeColor) ) #wx.GREY_BRUSH )
        shape.SetTextColour("grey")
        shape.SetFont(wx.Font(14, wx.MODERN, wx.NORMAL, wx.BOLD))
            
        if text:
            shape.SetName(name)
            for line in text.split('\n'):
                shape.AddText(line)
            shape.AddText(shape.GetTitle())
        shape.SetShadowMode(ogl.SHADOW_NONE)#RIGHT) # uncomment for shadow
        shape.SetMaintainAspectRatio(prefs.visual.GetKeepAspectRatio()) 
        shape.SetCentreResize(False)   #resize the shape from one corner only
        self.diagram.AddShape(shape)  #add the shape to the end (draws on the top)
        shape.Show(True)

        evthandler = MyEvtHandler(self.log, self.frame, self.sageGate, self)
        evthandler.SetShape(shape)
        evthandler.SetPreviousHandler(shape.GetEventHandler())
        shape.SetEventHandler(evthandler)

        self.shapes[id] = shape #store the newly created shape in a hash
        return shape

    #----------------------------------------------------------------------

    # removes the shape from the canvas and deletes it
    def MyRemoveShape(self, id):
        shape = self.shapes[id]
        canvas = shape.GetCanvas()
        self.FreeMinimizedPosition(shape.minimizedPosition, shape.app)
        del self.shapes[id] #remove the shape from the hash
        shape.RemoveFromCanvas(self)
        del shape

    #----------------------------------------------------------------------

##      # here there's no need to refill the diagram since all the shapes are in order
##     # we just have to update their z values. No messages are sent to SAGE
##     def UpdateZsForAdd(self):
##         for s in self.GetShapeList(): #[:]:
##             s.SetZ(s.GetZ()+1)

    def OrderByZ(self):
        diagram = self.GetDiagram()
        shapeList = self.GetShapeList()
        tempHash = {}  #shapes keyed by Z value for easy ordering
        selectedShape = None

        # now go through all the shapes and update them visually according to the new z values
        for s in shapeList[:]:
            s.SetZ( self.sageData.getZvalue(s.GetId()) )
            tempHash[ s.GetZ() ] = s
            if s.Selected():
                selectedShape = s
            diagram.RemoveShape(s)
        
        # sort the shapes so that they are in the right order for drawing
        tempKeys = tempHash.keys()
        tempKeys.sort()
        tempKeys.reverse()

        # now add all the shapes back sorted by Z
        for z in tempKeys:
            tempHash[z].DeleteControlPoints()
            diagram.AddShape(tempHash[z])

        # select the previously selected shape
        if selectedShape is not None:
            selectedShape.MakeControlPoints()

       
    #----------------------------------------------------------------------

    # detects when the mouse pointer is over a control point and changes the pointer itself
    def OnMouseEnter(self, event):
        x = event.GetX()
        y = event.GetY()

        # stop this event here so that we can make other events
        # (clicks on the shape to resize it)
        event.StopPropagation()
        event.Skip()
        
        shape = self.FindShape(x,y)[0]
        if shape == None:
            return

        # set the correct cursor based on the location of the control point
        if shape.__class__.__name__ == MyControlPoint.__name__:
            if not self.cursorSet:   # set the cursor only once (ie, if it's set, don't set it again)
                if shape.location == "NE" or shape.location == "SW":
                    self.SetCursor(wx.StockCursor(wx.CURSOR_SIZENESW))
                else:
                    self.SetCursor(wx.StockCursor(wx.CURSOR_SIZENWSE))
                self.cursorSet = True

        elif self.cursorSet:    # we only want to reset the cursor if it was previously set
            self.cursorSet = False
            self.SetCursor(wx.STANDARD_CURSOR)


    #----------------------------------------------------------------------

        # this keeps track of where the minimized app should be located
        # the hash holds windowIds but the keys are actually more important
        # if a key exists, that means that there is an app in that spot so
        # return the first free spot
    def GetNextMinimizedPosition(self, app):
        for i in range(0,30):
            if not i in self._minimizedShapes[app.getDisplayId()]:
                self._minimizedShapes[app.getDisplayId()][i] = app
                return i

        # when the app is not minimized anymore, it has to free its spot
    def FreeMinimizedPosition(self, pos, app):
        if pos in self._minimizedShapes[app.getDisplayId()]:
            if self._minimizedShapes[app.getDisplayId()][pos] == app:
                del self._minimizedShapes[app.getDisplayId()][pos]
            

#----------------------------------------------------------------------
# CANVAS PREFERENCES AND PROPERTIES
#----------------------------------------------------------------------

    def ShowLibrary(self):
        #reload(fileViewer)  # for debugging only!
        fileViewer.FileLibrary(self, self.frame.GetPositionTuple())

    def ChangeBackgroundColor(self, (r,g,b)):
        return
    
    ##     tileColor = wx.Colour(r,g,b)
##         for xOffset in range(self.iTileColumns):
##             for yOffset in range(self.iTileRows):
##                 self._LCDPanels[yOffset][xOffset].SetBrush(wx.Brush(tileColor))
##         self.Redraw()
        

    def SetMaintainAspectRatio(self, ar):
        prefs.visual.SetKeepAspectRatio(ar)

    def GetMaintainAspectRatio(self):
        return prefs.visual.GetKeepAspectRatio()


        # stop or start receiving performance data
    def ReceivePerformanceData(self, doReceive):
        if doReceive:
            prefs.visual.SetReceivePerformanceData(True)
            for shape in self.GetShapeList():
                self.sageGate.startPerformance(shape.GetId())
        else:
            prefs.visual.SetReceivePerformanceData(False)
            for shape in self.GetShapeList():
                self.sageGate.stopPerformance(shape.GetId())


        # returns the bounds of the display in UI coords
    def GetUIDisplayBounds(self, displayId):
        return self.backdrop.GetDisplayBounds(displayId)
    
    
    # for getting the bounds of the area where the displays will be
    def GetLeft(self):
        return self.GetBorderWidth()

    def GetRight(self):
        return self.GetDisplayWidth() + self.GetBorderWidth()

    def GetTop(self):
        return self.GetBorderWidth()

    def GetBottom(self):
        return self.GetDisplayHeight() + self.GetBorderWidth()


    # tiled display are size (in the UI) - this is not for every individual display...
    def GetDisplayWidth(self):
        return self._maxDisplayWidth

    def GetDisplayHeight(self):
        return self._maxDisplayHeight

    def SetDisplayWidth(self, newWidth):
        self._maxDisplayWidth = newWidth
        
    def SetDisplayHeight(self, newHeight):
        self._maxDisplayHeight = newHeight


    # the top part of the UI (the window that holds the tiled display)
    def GetCanvasWidth(self):
        return self._maxCanvasWidth

    def GetCanvasHeight(self):
        return self._maxCanvasHeight

    def SetCanvasWidth(self, newWidth):
        self._maxCanvasWidth = newWidth

    def SetCanvasHeight(self, newHeight):
        self._maxCanvasHeight = newHeight

    def GetBorderWidth(self):
        return self._borderWidth

    def SetBorderWidth(self, newWidth):
        self._borderWidth = newWidth

    def SetAspectRatio(self, ratio):
        self._aspectRatio = ratio

    def GetAspectRatio(self):
        return self._aspectRatio
        

    def GetGraphManager(self):
        return self.__gmGraphManager



#----------------------------------------------------------------------
# SAGEGATE CALLBACKS
#----------------------------------------------------------------------

    ### 40000 ###
    def OnSAGEStatusMsg(self):
        # this is where we make all the buttons for all the apps that SAGE supports
        hashAvailableApps = self.sageData.getAvailableApps()

        # figure out the positions for all the app buttons so that they are evenly spaced and centered
        # then, create the SAGEAppInitial for every one of them and make the buttons on the bottom panel
        buttonOffset = 40
        buttonNum = 1  

        # add the remote app button first
        self.parent.GetAppPanel().AddRemoteButton("REMOTE APPS", (buttonNum*buttonOffset, 10), self.sageGate)

        # now add the rest of the buttons from the local appLauncher (local to our SAGE)
        apps = hashAvailableApps.keys()
        apps.sort(lambda x, y: cmp(x.lower(), y.lower()))
        
        for appName in apps:
            buttonNum = buttonNum + 2
            temp = hashAvailableApps[appName]
            self.availableApps[appName] = hashAvailableApps[appName]
            self.parent.GetAppPanel().AddButton(self.availableApps[appName], (buttonNum*buttonOffset, 10))
            self.parent.GetAppPanel().AddConfigMenu(self.availableApps[appName])

               
    #----------------------------------------------------------------------
    

    ### 40001 ###
    def OnSAGEAppInfoMsg(self, sageApp):
        # Here we just check if a shape for this "sageApp" exists and
        # if it does, well, then we dont wanna create a new one but modify the old one
        windowId = sageApp.getId()
        if not self.shapes.has_key(windowId):  # CREATING A NEW SHAPE
            newShape = self.MyAddShape(sageApp)
            self.parent.GetAppInfoPanel().AddInstanceButton(sageApp)
            self.OrderByZ()  # the shapes may arrive out of order
            
            # (AKS 2005-04-05) Only start performance monitoring when a new application
            # is generated.
            if prefs.visual.GetReceivePerformanceData():
                self.sageGate.startPerformance( windowId, 2 )
            
        else:                          # MODIFYING A SHAPE
            shape = self.shapes[windowId]
            newWidth = self.ToUIWidth( sageApp.getLeft(), sageApp.getRight(), sageApp.getDisplayId() )
            newHeight = self.ToUIHeight( sageApp.getBottom(), sageApp.getTop(), sageApp.getDisplayId() )
            shape.SetSize(newWidth, newHeight)          
            shape.SetX( round (self.ToUICoordsX( sageApp.getLeft(), sageApp.getDisplayId() ) + int(newWidth/2)) )
            shape.SetY( round (self.ToUICoordsY( sageApp.getTop(), sageApp.getDisplayId() ) + int(newHeight/2)) )
            shape.SetTitle( sageApp.getTitle() )
            shape.SetOrientation( sageApp.getOrientation() )
            shape.ResetControlPoints() 
            

        self.Redraw()
            
                
    #----------------------------------------------------------------------
    

    # 40002
    def OnSAGEPerfInfoMsg(self, windowId ):
        # (AKS 2005-04-05) Performance monitoring information
        ## print "**** sageui >> 40002 >> update graph manager"
        self.__gmGraphManager.update( windowId )
        #print
        #print "TOTALS"
        #print "------"
        stats = self.sageData.getTotals()
        #print "Render total bandwidth = ", stats[ self.sageData.I_RENDER_TOTAL_BANDWIDTH ]
        #print "Render average framerate = ", stats[ self.sageData.I_RENDER_AVG_FRAME_RATE ]
        #print "Render total nodes = ", stats[ self.sageData.I_RENDER_TOTAL_NODES ]
        #print "Display total bandwidth = ", stats[ self.sageData.I_DISPLAY_TOTAL_BANDWIDTH ]
        #print "Display average framerate = ", stats[ self.sageData.I_DISPLAY_AVG_FRAME_RATE ]
        #print "Display total nodes = ", stats[ self.sageData.I_DISPLAY_TOTAL_NODES ]



    #----------------------------------------------------------------------


    # 40003
    def OnSAGEAppShutdownMsg(self, sageApp):
        # turn the button off and close the app
        self.MyRemoveShape(sageApp.getId())
        self.parent.GetAppInfoPanel().RemoveInstanceButton(sageApp)
        self.Redraw()


        ## # (AKS 2005-04-05) Stop performance monitoring that began "by default"
        # when a new application had started.
        #self.sageGate.stopPerformance( windowId )  #this needs to be called BEFORE we close the app and not after...
        
        self.__gmGraphManager.removeGraph( sageApp.getId() )

        
    #----------------------------------------------------------------------
    

    # 40004
    def OnSAGEDisplayInfoMsg(self):
        self.displayInfo = self.sageData.getDisplayInfo()

        # create the hash for each of the displays that holds the minimized shapes
        for sageDisplay in self.displayInfo.getAllDisplays():
            self._minimizedShapes[sageDisplay.displayId] = {}
        
        # for one display and old SAGE there are no connection messages so just
        # pretend that one came in for drawing everything correctly
        if self.displayInfo.getNumDisplays() == 1:
            self.OnSAGEDisplaysConnection()

             
    #----------------------------------------------------------------------


    # 40007
    def OnSAGEDisplaysConnection(self):
        self.tiledDisplayWidth = self.displayInfo.getTotalWidth()
        self.tiledDisplayHeight = self.displayInfo.getTotalHeight()
        
        # now we have all the data needed to set up the sizes of the canvas and display part
        self.SetDisplayWidth( self.GetCanvasWidth() - 2 * self.GetBorderWidth() )
        heightFactor = self.tiledDisplayWidth / float (self.GetDisplayWidth() )
        self.SetDisplayHeight( round (self.tiledDisplayHeight / heightFactor ) )
        self.SetCanvasHeight( self.GetDisplayHeight() + 1.5 * self.GetBorderWidth())
        
        self.EnableScrolling(False, False)
        self.SetSize((self.GetCanvasWidth(), self.GetCanvasHeight()))
        self.SetAspectRatio( float(self.GetCanvasWidth()) / self.GetCanvasHeight() )


        # figure out the backdrop panels
        self.displayInfoArrived = True  #set the flag so that we can begin resizing
        l = self.GetBorderWidth()
        r = self.GetCanvasWidth() - self.GetBorderWidth()
        t = self.GetBorderWidth()
        b = self.GetCanvasHeight() - self.GetBorderWidth()

        self.backdrop = Backdrop(self.displayInfo, self)
        self.backdrop.SetupDisplays(l,r,t,b)


        # resize the parent to be of the right size for its contents
        if self.parent.GetNotebook().GetPageCount() < 2:  #if this is the first display, resize the frame
            displaySize = wx.DisplaySize()  #the resolution of the current screen
            if prefs.visual.GetFrameSize() != None:  # read the last state from the preferences
                self.frame.SetSizeHints(400, 350, displaySize[0], displaySize[1])
                self.frame.SetClientSize( prefs.visual.GetFrameSize() )
            else:                                    # there are no previous preferences saved so figure it out
                appCanvasWidth, appCanvasHeight = self.parent.GetAppInfoPanel().GetSize()
                self.frame.SetSizeHints(400, 350, displaySize[0], displaySize[1])  #minimum size = (400, 400)
                self.frame.SetClientSize((self.GetCanvasWidth(), self.GetCanvasHeight() + appCanvasHeight)) # set the initial size of the frame
                if self.frame.GetSize().GetHeight() > displaySize[1]:  #is the frame too big for the desktop??? if so, size it back down
                    heightDifference = self.frame.GetSize().GetHeight() - displaySize[1]
                    self.frame.SetClientSize((self.GetCanvasWidth(), self.GetCanvasHeight() + appCanvasHeight - heightDifference))
            
            # figure out the positions and the sizes of all the decorative images
            self.RecalculateImages()
            ##set up a swap buffer for double buffering
            self.swapBuffer = wx.EmptyBitmap(self.GetCanvasWidth(), self.GetCanvasHeight())
            dc = wx.BufferedDC(None, self.swapBuffer)
            dc.SetBackground(wx.Brush(self.GetBackgroundColour()))
            dc.Clear()
            self.Redraw(dc)
        else:   # since we didn't resize the frame, we have to fit the contents inside of it
            ##set up a swap buffer for double buffering
            self.swapBuffer = wx.EmptyBitmap(self.GetCanvasWidth(), self.GetCanvasHeight())
            dc = wx.BufferedDC(None, self.swapBuffer)
            dc.SetBackground(wx.Brush(self.GetBackgroundColour()))
            dc.Clear()
            self.parent.OnSize()
            #self.Redraw(dc)
        del dc

        # call the parent if something needs to be done
        self.parent.OnDisplayInfo()

        # add the pointers
        #self.ptrs = Pointers(self.sageGate, self.sageData)
        
        # show the frame when all the buttons have been created (just for visual purposes)
        self.frame.Show()
        if self.frame.usersClient.connected:
            if prefs.visual.IsChatShown() == None or prefs.visual.IsChatShown():
                self.frame.GetUsersPanel().Show()


    
        
    #----------------------------------------------------------------------
    

    # 40005
    def OnSAGEZChangeMsg(self):
        self.OrderByZ() # redraw the shapes based on the new order
        self.Redraw()

        
    #----------------------------------------------------------------------
    

    # 40006
    def OnSAGEAppExecConfig(self, appInitial):
        self.parent.GetAppPanel().AddConfigMenu(appInitial)



#----------------------------------------------------------------------
#  UI ---> SAGE coordinate conversion
#----------------------------------------------------------------------

    def ToSAGECoordsX (self, uiX, displayId, doRound=True):
        bounds = self.backdrop.GetDisplayBounds(displayId)
        uiX = uiX - bounds.left
        sageX = (float(uiX) / bounds.getWidth()) * self.displayInfo.getDisplay(displayId).sageW
        if doRound:
            sageX = round(sageX)
        return sageX

    def ToSAGECoordsY (self, uiY, displayId, doRound=True):
        bounds = self.backdrop.GetDisplayBounds(displayId)
        uiY = uiY - bounds.top
        sageY = (1-(float(uiY) / bounds.getHeight())) * self.displayInfo.getDisplay(displayId).sageH
        if doRound:
            sageY = round(sageY)
        return sageY

    def ToSAGEDistX (self, oldX, newX, displayId):   
        return round( self.ToSAGECoordsX( newX, displayId, False ) - self.ToSAGECoordsX( oldX, displayId, False ) )

    def ToSAGEDistY (self, oldY, newY, displayId):   
        return round( self.ToSAGECoordsY( newY, displayId, False ) - self.ToSAGECoordsY( oldY, displayId, False ) )
    

#----------------------------------------------------------------------
#  SAGE ---> UI coordinate conversion   
#----------------------------------------------------------------------

    def ToUICoordsX (self, sageX, displayId, doRound=True):
        bounds = self.backdrop.GetDisplayBounds(displayId)
        uiX = (float(sageX) / self.displayInfo.getDisplay(displayId).sageW) * bounds.getWidth()
        uiX += bounds.left
        
        if doRound:
            uiX = round(uiX)
        return uiX

    def ToUICoordsY (self, sageY, displayId, doRound=True):
        bounds = self.backdrop.GetDisplayBounds(displayId)
        uiY = (1 - (float(sageY) / self.displayInfo.getDisplay(displayId).sageH)) * bounds.getHeight()
        uiY += bounds.top
        
        if doRound:
            uiY = round(uiY)
        return uiY

    def ToUIWidth (self, leftSageX, rightSageX, displayId): 
        return round( abs(self.ToUICoordsX( leftSageX, displayId, False ) - self.ToUICoordsX( rightSageX, displayId, False )) )
    
    def ToUIHeight (self, bottomSageY, topSageY, displayId):
        return round( abs(self.ToUICoordsY( bottomSageY, displayId, False ) - self.ToUICoordsY( topSageY, displayId, False )) )





############################################################################
#
#  CLASS: AppInfoPanel
#  
#  DESCRIPTION: It describes the canvas that holds all the buttons for
#               starting and stopping SAGE apps from within the SAGE UI.
#               It also draws all the graphics that is used instead of the
#               regular wx controls. It handles button clicks and other app
#               specific actions. It does all the drawing with double buffering.
#
#  DATE:        December 26, 2004
#
############################################################################

class AppInfoPanel(wx.Panel):
    def __init__(self, frame, sageGate, parent, id, position, size, style):
        wx.Panel.__init__(self, parent, id, position, size, style)
        self.SetBackgroundColour('#888888')
        self.sageGate = sageGate
        self.parent = parent
        self.visible = True
        self.shrinkInfoPanel = False
        self.infoPanelMinimized = False

        # load the images for the arrows
        self.arrowLeft = MyBitmapButton(self, (self.GetDisplayCanvas().GetBorderWidth(), 10), (20,40), "images/scrollLeft_up.jpg", "images/scrollLeft_down.jpg", "images/scrollLeft_over.jpg") 
        self.arrowRight = MyBitmapButton(self, (self.GetDisplayCanvas().GetBorderWidth(), 10), (20,40), "images/scrollRight_up.jpg", "images/scrollRight_down.jpg", "images/scrollRight_over.jpg")
        self.arrowLeft.Bind(wx.EVT_LEFT_DOWN, self.OnLeftArrowDown)
        self.arrowLeft.Bind(wx.EVT_LEFT_UP, self.OnLeftArrowUp)
        self.arrowRight.Bind(wx.EVT_LEFT_DOWN, self.OnRightArrowDown)
        self.arrowRight.Bind(wx.EVT_LEFT_UP, self.OnRightArrowUp)

        # load the decorative images (edges)
        border = self.GetDisplayCanvas().GetBorderWidth()
        self.borderImages = []
        self.borderImages.append( MyImage( "images/SAGEapp_10.jpg", 0,0, False, border, size[1] - border/2))
        self.borderImages.append( MyImage( "images/SAGEapp_14.jpg", border,0, False, size[0]-2*border, size[1]-border/2))
        self.borderImages.append( MyImage( "images/SAGEapp_15.jpg", size[0]-border,0, False, border, size[1] - border/2))
        self.borderImages.append( MyImage( "images/SAGEapp_17.jpg", 0,size[1]-border/2, False, border, border/2))
        self.borderImages.append( MyImage( "images/SAGEapp_18.jpg", border,size[1]-border/2, False, size[0]-2*border,border/2))
        self.borderImages.append( MyImage( "images/SAGEapp_19.jpg", size[0]-border,size[1]-border/2, False, border,border/2))
        
        # bind events for the panel
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.Chillout)
        if "__WXMSW__" in wx.PlatformInfo:  # this is needed on windows for proper painting after resize
            self.Bind(wx.EVT_SIZE, self.OnSize)

        # set up the area for the buttons to be in
        self.appPanel = AppPanel(self, 20,20, border+self.arrowLeft.GetSize().width,10)
        self.infoPanel = InfoPanel(self, 50,20, 100,20)

        # the show/hide buttons
        self.hideAppPanelButton = MyBitmapButton(self, (0,0), (40,12), "images/handle1.jpg")
        self.hideAppPanelButton.Bind(wx.EVT_LEFT_UP, self.OnHideAppPanelButton)
        self.hideInfoPanelButton = MyBitmapButton(self, (0,0), (12,40), "images/handle.jpg")
        self.hideInfoPanelButton.Bind(wx.EVT_LEFT_UP, self.OnHideInfoPanelButton)

        

    #----------------------------------------------------------------------


    def OnPaint(self, event=None):
        if DEBUG: print "    OnPaint --> AppInfoPanel"; sys.stdout.flush()
        if not event == None and event.GetEventType() == wx.EVT_PAINT:
            dc = wx.PaintDC(self)
        else:
            dc = wx.ClientDC(self)
        #dc = wx.PaintDC(self)
        if not self.GetDisplayCanvas().displayInfoArrived:
            return
        self.Redraw(dc)  # redraw first and then swap the buffer
            

    #----------------------------------------------------------------------
    

    def Redraw(self, dc=None):
        if DEBUG: print "    Redraw --> AppInfoPanel"; sys.stdout.flush()
        if not self.GetDisplayCanvas().displayInfoArrived:
            return
        if dc == None:
            if "__WXMAC__" in wx.PlatformInfo:
                self.Refresh()
                return
            dc = wx.ClientDC(self)

        # decorative images
        for image in self.borderImages:
            dc.DrawBitmap( image.bitmap, image.GetX(), image.GetY(), image.IsTransparent() )        

        del dc # to stop mac os from complaining about nested wxDCs

        # redraw the children
        self.infoPanel.Redraw()
        self.appPanel.Redraw()

        # redraw the controls on the panel
        self.hideAppPanelButton.OnPaint()
        self.hideInfoPanelButton.OnPaint()
        self.arrowLeft.OnPaint()
        self.arrowRight.OnPaint()
        

        

    #----------------------------------------------------------------------

    
    def Chillout(self, event):
        """ For intercepting the EVT_ERASE_BACKGROUND. Do nothing here on purpose
            in order to reduce the flicker during drawing"""
        pass


    #----------------------------------------------------------------------


    def OnSize(self, event=None):
        if DEBUG: print "    OnSize --> AppInfoPanel"; sys.stdout.flush()
        if not self.GetDisplayCanvas().displayInfoArrived:
            return
        # move the panel first to the bottom of the canvas
        canvasHeight = self.parent.GetDisplayCanvas().GetCanvasHeight()
        self.MoveXY(0, canvasHeight - 1)
        
        # resize the main AppInfoPanel now
        parentSize = self.parent.GetClientSize()
        if "__WXMSW__" in wx.PlatformInfo:  # a visual hack for windows
            newHeight = parentSize.height - canvasHeight - 21#canvasSize.height - 21
        else:
            newHeight = parentSize.height - canvasHeight #canvasSize.height
	if newHeight < 1:
	   newHeight = 1
           
        self.SetSize( (parentSize.width, newHeight) )

        # rescale and reposition all the decorative images (edges)
        border = self.parent.GetDisplayCanvas().GetBorderWidth()
        size = self.GetSizeTuple()
        if size[0] > 0 and size[1] > 0:    # resize only if the parent has a reasonable size
            self.borderImages[0].Recalculate( border, size[1] - border/2, 0,0)
            self.borderImages[1].Recalculate( size[0]-2*border, size[1]-border/2, border,0)
            self.borderImages[2].Recalculate( border, size[1] - border/2, size[0]-border,0)
            self.borderImages[3].Recalculate( border, border/2, 0,size[1]-border/2)
            self.borderImages[4].Recalculate( size[0]-2*border,border/2, border,size[1]-border/2)
            self.borderImages[5].Recalculate( border,border/2, size[0]-border,size[1]-border/2)

        # as we are sizing down, make the infoPanel disappear if the frame size got small enough
        self.shrinkInfoPanel = parentSize.width < (border*2 + 2*self.arrowLeft.GetSize().width + self.infoPanel.GetCurrentMinSize() + 50)
                
        if (self.shrinkInfoPanel and self.infoPanel.visible):  # hide the info panel
            self.infoPanel.Minimize()
            self.hideInfoPanelButton.Hide()
        elif (not self.shrinkInfoPanel) and (not self.infoPanel.visible) and (not self.infoPanelMinimized):  #show it (once)
            self.infoPanel.Maximize()
            self.hideInfoPanelButton.Show()

        # now resize the children
        self.infoPanel.OnSize()
        self.appPanel.OnSize()

        # reposition the scrolling arrows for the appPanel
        self.arrowLeft.MoveXY(self.GetDisplayCanvas().GetBorderWidth(), 20)
        self.arrowRight.MoveXY(self.GetDisplayCanvas().GetBorderWidth()+self.appPanel.GetSize().width+self.GetArrowWidth(), 20)
        self.hideAppPanelButton.MoveXY( size[0]/2 - self.hideAppPanelButton.GetSize()[0]/2,  0 )
        self.hideInfoPanelButton.MoveXY( self.infoPanel.GetPosition().x - 16,  self.infoPanel.GetPosition().y + self.infoPanel.GetSize().height/2 - self.hideInfoPanelButton.GetSize()[1]/2 )

        self.Redraw()


    #----------------------------------------------------------------------


    def OnHideAppPanelButton(self, event=None):
        if self.visible:
            self.parent.HideAppInfoPanel()
            self.visible = False
            self.arrowRight.Hide()
            self.arrowLeft.Hide()
        else:
            self.parent.ShowAppInfoPanel()
            self.visible = True
            self.arrowRight.Show()
            self.arrowLeft.Show()

    def OnHideInfoPanelButton(self, event=None):
        if self.infoPanel.visible:  #hide it
            self.infoPanel.Minimize()
            self.infoPanelMinimized = True
            self.OnSize()
        else:              # it's already hidden so show it
            self.infoPanel.Maximize()
            self.infoPanelMinimized = False
            self.OnSize()

            
    def OnLeftArrowDown(self, event):
        self.arrowLeft.OnLeftDown(event)
        self.appPanel.ScrollLeft()

    def OnRightArrowDown(self, event):
        self.arrowRight.OnLeftDown(event)
        self.appPanel.ScrollRight()

    def OnLeftArrowUp(self, event):
        self.arrowLeft.OnLeftUp(event)
        self.appPanel.ScrollLeft()

    def OnRightArrowUp(self, event):
        self.arrowRight.OnLeftUp(event)
        self.appPanel.ScrollRight()
        

    #----------------------------------------------------------------------
    
    
    def AddInstanceButton(self, sageApp):
        # once the 40001 message arrives and the app doesnt exists yet, create an instance button for it
        self.appPanel.AddInstanceButton(sageApp)
	self.Refresh()

    def RemoveInstanceButton(self, sageApp):
        # once the 40003 message arrives and we close the app, remove the instance button for it
        self.appPanel.RemoveInstanceButton(sageApp)
        self.Refresh()

        
    #----------------------------------------------------------------------

    def GetDisplayCanvas(self):
        return self.parent.GetDisplayCanvas()

    def GetArrowWidth(self):
        return self.arrowLeft.GetSize().width

    def GetInfoPanel(self):
        return self.infoPanel
    
    def GetAppPanel(self):
        return self.appPanel



############################################################################
#
#  CLASS: AppPanel
#  
#  DESCRIPTION: It describes the canvas that holds all the buttons for
#               starting and stopping SAGE apps from within the SAGE UI.
#               It also draws all the graphics that is used instead of the
#               regular wx controls. It handles button clicks and other app
#               specific actions. It does all the drawing with double buffering.
#
#  DATE:        April, 2005
#
############################################################################

class AppPanel(wx.Panel):

    def __init__(self, parent, width, height, x, y):
        wx.Panel.__init__(self, parent, -1, (x,y), (width,height), wx.NO_BORDER | wx.NO_FULL_REPAINT_ON_RESIZE)
        self.SetBackgroundColour(appPanelColor)
        self.page = parent.parent
        self.parent = parent
        self.visible = True
        self._appButtonWidth = 40
        self.appButtons = {} # has to store all the buttons keyed by the app_inst_IDs
        self.appNames = [] # list of all app names that we got from sage
        self.buttonOffset = 0 #used for positioning the buttons
        self.scrollWidth = width  #the minimum width required to draw all the app buttons
        self.first = None  # used for scrolling... these are first and last app buttons
        self.last = None

        # background for the app buttons
        self.bar = MyImage( "images/menu_bar.jpg", 0,10, False, 1500, self.GetButtonWidth())
        self.background = MyImage( "images/app_panel_background.png", 0,0, False, width, height)

        self.Bind(wx.EVT_ERASE_BACKGROUND, self.Chillout)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        if wx.Platform == "__WXMSW__":  # this is needed on windows for proper painting after resize
            self.Bind(wx.EVT_SIZE, self.OnSize)
        

    #----------------------------------------------------------------------

    def OnPaint(self, event=None):
        if DEBUG: print "\tOnPaint --> AppPanel"; sys.stdout.flush()
        if not event == None and event.GetEventType() == wx.EVT_PAINT:
            dc = wx.PaintDC(self)
        else:
            dc = wx.ClientDC(self)
        #self.PrepareDC(dc)
        self.Redraw(dc)

   
    def Redraw(self, dc=None):
        if DEBUG: print "\tRedraw --> AppPanel"; sys.stdout.flush()
        if dc == None:
            if "__WXMAC__" in wx.PlatformInfo:
                self.Refresh()
                return
            dc = wx.ClientDC(self)
            #self.PrepareDC(dc)

        # first draw the bar and the background
        dc.DrawBitmap( self.background.bitmap, self.background.GetX(), self.background.GetY(), self.background.IsTransparent())
        dc.DrawBitmap( self.bar.bitmap, self.bar.GetX(), self.bar.GetY(), self.bar.IsTransparent() )
        del dc  # for mac os to stop complaining about nested wxDCs

        # draw all the app buttons in their respective locations
        for name, button in self.appButtons.iteritems():
            button.OnPaint()
        

    def Chillout(self, event):
        """ For intercepting the EVT_ERASE_BACKGROUND. Do nothing here on purpose
            in order to reduce the flicker during drawing"""
        pass


    def OnSize(self, event=None):
        if DEBUG: print "\tOnSize --> AppPanel"; sys.stdout.flush()
        # get some measures that will help us determine the size of the panel
        parentSize = self.parent.GetClientSize()
        borderWidth = self.parent.GetDisplayCanvas().GetBorderWidth()
        arrowWidth = self.parent.GetArrowWidth()
        infoPanelWidth = self.parent.GetInfoPanel().GetSize().width#GetInfoPanelWidth()

        # determine the new size
        newWidth = parentSize.width - infoPanelWidth - 2*arrowWidth - 2*borderWidth - 20
        newHeight = parentSize.height - 1.5*borderWidth
	if newHeight < 1:
	   newHeight = 1
	if newWidth < 1:
	   newWidth = 1
        self.SetSize( (newWidth, newHeight) )

        #resize the background image
        self.background.Rescale(newWidth, newHeight)
        self.Redraw()


    #----------------------------------------------------------------------

    
    def ScrollLeft(self):
        if not self.IsButtonVisible( self.first ):
            self.ScrollWindow( 30, 0 )
            self.Update()
                

    def ScrollRight(self):
        if not self.IsButtonVisible( self.last ):
            self.ScrollWindow( -30, 0 )
            self.Update()


    def IsButtonVisible(self, button):
        # Used in scrolling... checks if a button is on the screen.
        # If not, return false so that we have to scroll
        x = button.GetPosition().x
        width = button.GetSize().width

        if (x > 0) and ((x+width) < self.GetSize().width):
            return True
        else:
            return False
            

    #----------------------------------------------------------------------


    def AddRemoteButton(self, name, (x,y), sageGate):
        self.appNames.append(name)
        
        # create the buttons and check if images are available for them
        if os.path.isfile( ConvertPath("images/remote_up.jpg")):
            bitmapName = "images/remote"
        else:
            bitmapName = "images/default"
        self.appButtons[ name ] = RemoteAppButton( self, bitmapName+"_up.jpg", bitmapName+"_down.jpg", bitmapName+"_over.jpg", (x,y), (self.GetButtonWidth(),self.GetButtonWidth()), sageGate)
        self.buttonOffset = self.buttonOffset + 1

        # this is the total area taken by the buttons (so that they are all displayed at once)
        self.scrollWidth = self.GetButtonWidth() * 2 * len(self.appButtons)
    
        # remember which is the first and the last button... used for scrolling
        if self.first == None:
            self.first = self.appButtons[ name ]
            self.last = self.appButtons[ name ]  #the first is also the last
        else:
            self.last = self.appButtons[ name ]


    def AddButton(self, app, (x,y)):
        self.appNames.append(app.GetName())
        
        # create the buttons and check if images are available for them
        if os.path.isfile( ConvertPath("images/" + string.lower(app.GetName()) + "_up.jpg")):
            bitmapName = "images/" + string.lower(app.GetName())
        else:
            bitmapName = "images/default"
        self.appButtons[ app.GetName() ] = AppButton( self, bitmapName+"_up.jpg", bitmapName+"_down.jpg", bitmapName+"_over.jpg", (x,y), (self.GetButtonWidth(),self.GetButtonWidth()), app)
        self.buttonOffset = self.buttonOffset + 1

        # this is the total area taken by the buttons (so that they are all displayed at once)
        self.scrollWidth = self.GetButtonWidth() * 2 * len(self.appButtons)
    
        # remember which is the first and the last button... used for scrolling
        if self.first == None:
            self.first = self.appButtons[ app.GetName() ]
        else:
            self.last = self.appButtons[ app.GetName() ]


    def RepositionAppButtons(self):
        # repositions all the app buttons and redraws them
        # the app buttons then reposition all the instance buttons and redraw them
        buttonOffset = round( self.GetSize().width / float( len(self.appNames)*2+1 ) )
        buttonNum = 1
        
        for k, btn in self.appButtons.iteritems():
            btn.MoveXY(buttonNum*buttonOffset, 10)
            buttonNum = buttonNum + 2


    def GetButtonWidth(self):
        return self._appButtonWidth
    
    def GetAppInfoPanel(self):
        return self.parent


    #----------------------------------------------------------------------

    # fills the remote application button with the correct menu data for each app launcher
    def AddLauncherMenu(self, name, launcherHash):
        self.appButtons[ name ].AddLauncherMenu(launcherHash)    

    # once the 40006 message arrives, we add a menu to the button
    def AddConfigMenu(self, appInitial):
        self.appButtons[ appInitial.GetName() ].AddConfigMenu(appInitial)
    
    # once the 40001 message arrives and the app doesnt exists yet, create an instance button for it
    def AddInstanceButton(self, sageApp):
        if sageApp.getName() in self.appButtons:
            self.appButtons[ sageApp.getName() ].AddInstanceButton(sageApp)

    # once the 40003 message arrives and we close the app, remove the instance button for it
    def RemoveInstanceButton(self, sageApp):
        if sageApp.getName() in self.appButtons:
            self.appButtons[ sageApp.getName() ].RemoveInstanceButton(sageApp.getId())
            self.GetAppInfoPanel().GetInfoPanel().OnPerfPanelRemoved(sageApp.getId())




        
############################################################################
#
#  CLASS: InfoPanel
#  
#  DESCRIPTION: It describes the canvas that holds all the buttons for
#               starting and stopping SAGE apps from within the SAGE UI.
#               It also draws all the graphics that is used instead of the
#               regular wx controls. It handles button clicks and other app
#               specific actions. It does all the drawing with double buffering.
#
#  DATE:        April, 2005
#
############################################################################

class InfoPanel(wx.Panel):

    def __init__(self, parent, width, height, x, y):
        wx.Panel.__init__(self, parent, -1, (x,y), (width,height), wx.NO_BORDER | wx.NO_FULL_REPAINT_ON_RESIZE)
        self.SetBackgroundColour(appPanelColor)
        self.page = parent.parent
        self.parent = parent
        self.SetVirtualSize( (width, height) )
        self.visible = True
        self._minWidth = 250
        self.width = 0 #initial width  
        self.displayCanvas = self.page.GetDisplayCanvas()
        self.dataPanels = []
        
        self.background = MyImage( "images/app_panel_background.png", 0,0, False, width, height)
        
        if "__WXMSW__" in wx.PlatformInfo:  # this is needed on windows for proper painting after resize
            self.Bind(wx.EVT_SIZE, self.OnSize)
        elif "__WXGTK__" in wx.PlatformInfo:
            self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.Chillout)
        
    #----------------------------------------------------------------------

    def Chillout(self, event):
        """ For intercepting the EVT_ERASE_BACKGROUND. Do nothing here on purpose
            in order to reduce the flicker during drawing"""
        pass


    def OnPaint(self, event=None):
        if not self.visible: 
            if event.GetEventType() == wx.EVT_PAINT:
                dc=wx.PaintDC(self) 
            else:
                dc=wx.ClientDC(self)
            return
        if DEBUG: print "\tOnPaint --> InfoPanel"; sys.stdout.flush()
        if not event == None and event.GetEventType() == wx.EVT_PAINT:
            dc = wx.PaintDC(self)
        else:
            dc = wx.ClientDC(self)
        #dc = wx.PaintDC(self)
        #self.PrepareDC(dc)
      	self.Redraw(dc)

   
    def Redraw(self, dc=None):
        if not self.visible: return
        if DEBUG: print "\tRedraw --> InfoPanel"; sys.stdout.flush()
        if dc == None:
            if "__WXMAC__" in wx.PlatformInfo:
                self.Refresh()
                return
            dc = wx.ClientDC(self)
            #self.PrepareDC(dc)
        
        if "__WXMSW__" in wx.PlatformInfo:
            self.Refresh()
            self.Update()

        dc.DrawBitmap( self.background.bitmap, self.background.GetX(), self.background.GetY(), self.background.IsTransparent())
        dc.SetBrush(wx.TRANSPARENT_BRUSH)
        dc.SetPen(wx.BLACK_PEN)
        dc.DrawRectangle(2, 2, self.GetClientSize()[0]-2, self.GetClientSize()[1]-2)
        dc.SetPen(wx.Pen(wx.Colour(153, 153, 153)))
        dc.DrawRectangle(0, 0, self.GetClientSize()[0]-2, self.GetClientSize()[1]-2)
        del dc
        

        
    def OnSize(self, event=None):
        #if not self.visible: return
        if DEBUG: print "\tOnSize --> InfoPanel"; sys.stdout.flush()
        # get some measures that will help us determine the size of the panel
        parentSize = self.GetAppInfoPanel().GetClientSize()
        borderWidth = self.GetAppInfoPanel().GetDisplayCanvas().GetBorderWidth()
        arrowWidth = self.GetAppInfoPanel().GetArrowWidth()

        # the new size depends on if there is a dataPanel in there or not
        # if not, the whole InfoPanel will be of default size
        if not self.visible or self.GetDataPanelWidth() == 0:   #not visible, setting width to 0
            newWidth = 0
            self.Hide()
        else:          #dataPanel, setting width to dataPanelWidth
            self.Show()
            newWidth = self.GetDataPanelWidth() + 7  #+5 added to allow for the border
        newHeight = parentSize.height - 1.5*borderWidth

        # apply the changes 
        self.SetSize( (newWidth, newHeight) )
        self.SetVirtualSize( (self.GetVirtualSize().width, self.GetVirtualSize().height) )
        self.MoveXY( parentSize.width - borderWidth - newWidth, self.GetPosition().y)

        # hide the little scrolly thingy button from the AppInfoPanel
        if newHeight < 2:
            self.GetAppInfoPanel().hideInfoPanelButton.Hide()
        else:
            self.GetAppInfoPanel().hideInfoPanelButton.Show()

        #resize the background image
        self.background.Rescale(newWidth, newHeight)
        self.Redraw()
        

    #----------------------------------------------------------------------
    

    def Minimize(self):
        self.visible = False
        self.Hide()

    def Maximize(self):
        self.visible = True
        self.Show()

    def GetCurrentMinSize(self):
        if self.GetDataPanelWidth() == 0:  #no dataPanel, setting width to default
            newWidth = self.GetMinimumWidth()
        else:          #dataPanel, setting width to dataPanelWidth
            newWidth = self.GetDataPanelWidth()
        return newWidth

    def GetAppInfoPanel(self):
        return self.page.GetAppInfoPanel()
    
    def GetMinimumWidth(self):
        return self._minWidth

    def SetMinimumWidth(self, newMinWidth):
        self._minWidth = newMinWidth

        
    #----------------------------------------------------------------------


    # this shows all the data that's relevant to the app
    def ShowData(self, appName, windowId):
	if self.visible:
	    self.ShowPerformanceData(appName, windowId)
        

        # shows the performance data panel in this InfoPanel
    def ShowPerformanceData(self, appName, windowId):
        if self.displayCanvas.GetGraphManager().GetSparklineGraph(windowId) == -1: #does the panel exist yet?
            self.displayCanvas.GetGraphManager().addGraph(appName, windowId, self)
            dataPanel = self.displayCanvas.GetGraphManager().GetSparklineGraph(windowId)
            self.dataPanels.append(dataPanel)  # add it to the list of panels
	    if len(self.dataPanels) < 2:
		self.Freeze()
		self.GetAppInfoPanel().OnSize()  #refresh/resize the panels
		self.Thaw()
        else:   #panel already exists so just show it on top of the other ones
            dataPanel = self.displayCanvas.GetGraphManager().GetSparklineGraph(windowId)
            self.Freeze()
	    for p in self.dataPanels:  #hide all the other panels but show the one that's selected
                try:
                    p.Hide()
                except Exception:  # an exception will occur if there are panels in the list that dont exist anymore
                    del p  #this will delete it from the list as well
            dataPanel.Show()
	    self.Thaw()
            #if not "__WXGTK__" in wx.PlatformInfo:  #extra refresh needed on Mac and Windows
            #    self.GetAppInfoPanel().OnSize()

            
            # returns the width of the dataPanel, if none exist it returns a 0
    def GetDataPanelWidth(self):
        try:
            width = self.dataPanels[0].GetSize().width
        except Exception:
            del self.dataPanels
            self.dataPanels = []
            return 0
        else:
            return width


        # resizes the parent panel to a default size if there are no more perf panels in here
    def OnPerfPanelRemoved(self, windowId):
        dataPanel = self.displayCanvas.GetGraphManager().GetSparklineGraph(windowId)

        try:
            self.dataPanels.remove(dataPanel)
        except Exception:
            pass
        else:
            if len(self.dataPanels) < 1:
                self.GetAppInfoPanel().OnSize()  #resize and refresh the parent panel



