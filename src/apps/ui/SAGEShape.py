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
#         Andrew Johnson
#
############################################################################



import wx.lib.ogl as ogl
from wx.lib.ogl._oglmisc import *     #needed for the MyShape class (MakeControlPoints)
import wx
from globals import *
from Mywx import MyImage


############################################################################
#
#  CLASS:       MyShape
#
#  DESCRIPTION: Extends the original ogl.Shape class so that the resizing
#               control points behave as we want them. There are only 4 control
#               points on the corners and they resize the shape while preserving
#               the aspect ratio (optional). The control points have also been
#               overridden to MyControlPoint.
#
#  DATE:        October 11, 2004
#
############################################################################

class MyShape(ogl.Shape):
    def __init__(self, canvas = None):
      ogl.Shape.__init__(self, canvas)
      self._shadowOffsetX = 6
      self._shadowOffsetY = 6
      self._shadowBrush = wx.Brush(shapeShadowColor)
      self._timer = MouseCaptureTimer()
      self._timer.SetCallback(self.ReleaseCapture)

    
    # Overriden method for creating the resizing points 
    # (corner ones behave the same as the ones on the side)

    def MakeControlPoints(self):
        """Make a list of control points (draggable handles) appropriate to
        the shape.
        """

        maxX, maxY = self.GetBoundingBoxMax()
        minX, minY = self.GetBoundingBoxMin()

        widthMin = minX - CONTROL_POINT_SIZE   #+ CONTROL_POINT_SIZE + 2
        heightMin = minY - CONTROL_POINT_SIZE  #+ CONTROL_POINT_SIZE + 2

        # Offsets from main object
        top = -heightMin / 2.0
        bottom = heightMin / 2.0 #+ (maxY - minY)
        left = -widthMin / 2.0
        right = widthMin / 2.0 #+ (maxX - minX)


        # TOP LEFT
        control = MyControlPoint(self._canvas, self, CONTROL_POINT_SIZE, left, top, CONTROL_POINT_DIAGONAL, "NW")
        self._canvas.AddShape(control)
        self._controlPoints.append(control)

        # TOP RIGHT
        control = MyControlPoint(self._canvas, self, CONTROL_POINT_SIZE, right, top, CONTROL_POINT_DIAGONAL, "NE")
        self._canvas.AddShape(control)
        self._controlPoints.append(control)

        # BOTTOM RIGHT
        control = MyControlPoint(self._canvas, self, CONTROL_POINT_SIZE, right, bottom, CONTROL_POINT_DIAGONAL, "SE")
        self._canvas.AddShape(control)
        self._controlPoints.append(control)

        # BOTTOM LEFT
        control = MyControlPoint(self._canvas, self, CONTROL_POINT_SIZE, left, bottom, CONTROL_POINT_DIAGONAL, "SW")
        self._canvas.AddShape(control)
        self._controlPoints.append(control)



    ### RJ 2005-02-02
    ### overridden to have the shapes resizable from corners only

    def ResetControlPoints(self):
        """Reset the positions of the control points (for instance when the
        shape's shape has changed).
        """
        self.ResetMandatoryControlPoints()

        if len(self._controlPoints) == 0:
            return

        maxX, maxY = self.GetBoundingBoxMax()
        minX, minY = self.GetBoundingBoxMin()

        widthMin = minX - CONTROL_POINT_SIZE  #+ CONTROL_POINT_SIZE + 2
        heightMin = minY - CONTROL_POINT_SIZE  #+ CONTROL_POINT_SIZE + 2

        # Offsets from main object
        top = -heightMin / 2.0
        bottom = heightMin / 2.0 #+ (maxY - minY)
        left = -widthMin / 2.0
        right = widthMin / 2.0 #+ (maxX - minX)


        self._controlPoints[0]._xoffset = left
        self._controlPoints[0]._yoffset = top

        self._controlPoints[1]._xoffset = right
        self._controlPoints[1]._yoffset = top

        self._controlPoints[2]._xoffset = right
        self._controlPoints[2]._yoffset = bottom

        self._controlPoints[3]._xoffset = left
        self._controlPoints[3]._yoffset = bottom
        

#----------------------------------------------------------------------


    ### RJ 2005-02-07
    ### overridden to have the shapes resizable from corners only
    # Control points ('handles') redirect control to the actual shape, to
    # make it easier to override sizing behaviour.

    def OnSizingDragLeft(self, pt, draw, x, y, keys = 0, attachment = 0):
        bound_x, bound_y = self.GetBoundingBoxMin()

        dc = wx.ClientDC(self.GetCanvas())
        self.GetCanvas().PrepareDC(dc)

        dc.SetLogicalFunction(OGLRBLF)

        dottedPen =  wx.Pen(wx.Colour(1, 0, 1), 2, wx.SOLID)
        dc.SetPen(dottedPen)
        dc.SetBrush(wx.TRANSPARENT_BRUSH)

        # Don't maintain the same centre point
        newX1 = min(pt._controlPointDragStartX, x)
        newY1 = min(pt._controlPointDragStartY, y)
        newX2 = max(pt._controlPointDragStartX, x)
        newY2 = max(pt._controlPointDragStartY, y)

        if self.GetMaintainAspectRatio():
            newH = (newX2 - newX1) * (float(pt._controlPointDragStartHeight) / pt._controlPointDragStartWidth)
            if self.GetY() > pt._controlPointDragStartY:
                newY2 = newY1 + newH
            else:
                newY1 = newY2 - newH

        newWidth = float(newX2 - newX1)
        newHeight = float(newY2 - newY1)

        pt._controlPointDragPosX = newX1 + newWidth / 2.0
        pt._controlPointDragPosY = newY1 + newHeight / 2.0
        if self.GetFixedWidth():
            newWidth = bound_x

        if self.GetFixedHeight():
            newHeight = bound_y

        pt._controlPointDragEndWidth = newWidth
        pt._controlPointDragEndHeight = newHeight
        self.GetEventHandler().OnDrawOutline(dc, pt._controlPointDragPosX, pt._controlPointDragPosY, newWidth, newHeight)


#----------------------------------------------------------------------


    ### RJ 2005-02-07
    ### overridden to have the shapes resizable from corners only

    def OnSizingBeginDragLeft(self, pt, x, y, keys = 0, attachment = 0):
        self._canvas.CaptureMouse()
        self._timer.Start(5000, True)   # automatically release the mouse after 5 secs
        

        dc = wx.ClientDC(self.GetCanvas())
        self.GetCanvas().PrepareDC(dc)

        dc.SetLogicalFunction(OGLRBLF)

        bound_x, bound_y = self.GetBoundingBoxMin()
        self.GetEventHandler().OnBeginSize(bound_x, bound_y)

        # Choose the 'opposite corner' of the object as the stationary
        # point in case this is non-centring resizing.
        if pt.GetX() < self.GetX():
            pt._controlPointDragStartX = self.GetX() + bound_x / 2.0
        else:
            pt._controlPointDragStartX = self.GetX() - bound_x / 2.0

        if pt.GetY() < self.GetY():
            pt._controlPointDragStartY = self.GetY() + bound_y / 2.0
        else:
            pt._controlPointDragStartY = self.GetY() - bound_y / 2.0

        # We may require the old width and height
        pt._controlPointDragStartWidth = bound_x
        pt._controlPointDragStartHeight = bound_y

        dottedPen = wx.Pen(wx.Colour(1, 0, 1), 2, wx.SOLID)
        dc.SetPen(dottedPen)
        dc.SetBrush(wx.TRANSPARENT_BRUSH)

        # Don't maintain the same centre point
        newX1 = min(pt._controlPointDragStartX, x)
        newY1 = min(pt._controlPointDragStartY, y)
        newX2 = max(pt._controlPointDragStartX, x)
        newY2 = max(pt._controlPointDragStartY, y)

        if self.GetMaintainAspectRatio():
            newH = (newX2 - newX1) * (float(pt._controlPointDragStartHeight) / pt._controlPointDragStartWidth)
            if pt.GetY() > pt._controlPointDragStartY:
                newY2 = newY1 + newH
            else:
                newY1 = newY2 - newH

        newWidth = float(newX2 - newX1)
        newHeight = float(newY2 - newY1)

        pt._controlPointDragPosX = newX1 + newWidth / 2.0
        pt._controlPointDragPosY = newY1 + newHeight / 2.0
        if self.GetFixedWidth():
            newWidth = bound_x

        if self.GetFixedHeight():
            newHeight = bound_y

        pt._controlPointDragEndWidth = newWidth
        pt._controlPointDragEndHeight = newHeight
        self.GetEventHandler().OnDrawOutline(dc, pt._controlPointDragPosX, pt._controlPointDragPosY, newWidth, newHeight)


#----------------------------------------------------------------------


    ### RJ 2005-02-07
    ### overridden to have the shapes resizable from corners only

    def OnSizingEndDragLeft(self, pt, x, y, keys = 0, attachment = 0):
        dc = wx.ClientDC(self.GetCanvas())
        self.GetCanvas().PrepareDC(dc)

        if self._canvas.HasCapture():
            self._canvas.ReleaseMouse()
        dc.SetLogicalFunction(wx.COPY)
        self.Recompute()
        self.ResetControlPoints()

        self.Erase(dc)

        self.SetSize(pt._controlPointDragEndWidth, pt._controlPointDragEndHeight)
        self.Move(dc, pt._controlPointDragPosX, pt._controlPointDragPosY)

        # Recursively redraw links if we have a composite
        if len(self.GetChildren()):
            self.DrawLinks(dc, -1, True)

        width, height = self.GetBoundingBoxMax()
        self.GetEventHandler().OnEndSize(width, height)

        if not self._canvas.GetQuickEditMode() and pt._eraseObject:
            self._canvas.Redraw(dc)
        

    def ReleaseCapture(self):
        """ called with a timer to release the capture after a certain time """
        if self._canvas.HasCapture():
            self._canvas.ReleaseMouse()


class MouseCaptureTimer(wx.Timer):
    def SetCallback(self, cb):
        self.callback = cb
    def Notify(self):
        self.callback()





class MyControlPoint(ogl.ControlPoint):
    def __init__(self, theCanvas, object, size, the_xoffset, the_yoffset, the_type, location):
        ogl.ControlPoint.__init__(self, theCanvas, object, size, the_xoffset, the_yoffset, the_type)
        self.location = location
        self.SetPen(wx.TRANSPARENT_PEN)
        self.SetBrush(wx.TRANSPARENT_BRUSH)


# these are the buttons for maximizing, minimizing and closing shapes
# they are not buttons per se but just images drawn on top of shapes
# the clicks are handled manually (by comparing mouse coordinates) through
# the OnLeftClick event handler for the shape
class ShapeButton(MyImage):
    def __init__(self, shape, handler, bitmap, x=0, y=0, rotate=False):
        MyImage.__init__(self, bitmap, x,y, False, 20, 20)
        self.shape = shape
        self.handler = handler
        self._visible = True
        self.rotate = rotate

        if rotate:
            self.originalImage = self.image  # preserve the orientation of the original image
            

    def SetOrientation(self, degrees):
        if degrees > 0:
            timesToRotate = int(degrees/90)
            self.image = self.originalImage
            for i in range(timesToRotate):
                self.image = self.image.Rotate90(False) 
        else:
            self.image = self.originalImage
        self.bitmap = wx.BitmapFromImage(self.image)
        
        
    def OnPaint(self, dc):
        if self.GetVisible():
            dc.DrawBitmap(self.bitmap, self.GetX(), self.GetY(), self.transparent)

    def SetVisible(self, visible):
        self._visible = visible

    def GetVisible(self):
        return self._visible

    def Move(self, newX, newY):
        self.SetX(newX)
        self.SetY(newY)

        # test if a click on the shape landed on this button (used instead of an event handler)
    def HitTest(self, x, y):
        btnWidth = self.bitmap.GetWidth()
        btnHeight = self.bitmap.GetHeight()
        
        if ((x >= self.GetX()) and (x <= self.GetX()+btnWidth) and
            (y >= self.GetY()) and (y <= self.GetY()+btnHeight)):
            self.handler()  # call the correct handler because we hit the button
            return True
        else:
            return False    # it was a miss
    



    
############################################################################
#
#  CLASS:       MyRectangleShape
#
#  DESCRIPTION: Extends the original ogl.RectangleShape class. The original
#               RectangleShape class extends ogl.Shape but we overrode ogl.Shape
#               so we had to make MyRectangleShape extend MyShape. However, 
#               since this is still a rectangle, we also have to extend from
#               ogl.RectangleShape. The only method we override is the
#               constructor since this is where we specify which Shape class
#               are we based on. Hope this makes sense.
#
#  DATE:        October 12, 2004
#
############################################################################

class MyRectangleShape(ogl.RectangleShape, MyShape):
    
    #overridden constructor so that it uses MyShape class and not ogl.Shape
    def __init__(self, w, h, app, sageGate, canvas=None, title="untitled"):
        MyShape.__init__(self, canvas)
        self.app = app
        self.initialWidth = w
        self.initialHeight = h
        self._width = w
        self._height = h
        self._cornerRadius = 0.0
        self.SetDefaultRegionSize()
        self.Z = 0
        self.name = ""
        self.title = title
        self.borderColor = wx.Colour(255,255,255)
        self.titleColor = wx.Colour(255,255,255)
        self.sageGate = sageGate
        self.displayCanvas = canvas
        self.oldWidth = 0
        self.oldHeight = 0
        self.oldX = 0
        self.oldY = 0
        self.maximized = False
        self.minimized = False
        self.minimizedPosition = -1

        self.oldLeft = 0
        self.oldRight = 0
        self.oldTop = 0
        self.oldBottom = 0

        # control buttons in the top right corner
        self.buttons = []
        self.buttonsVisible = True
        self.buttons.append( ShapeButton(self, self.Close, "images/close_shape.gif", self._xpos, self._ypos))
        self.buttons.append( ShapeButton(self, self.Maximize, "images/maximize_shape.gif", self._xpos, self._ypos))
        self.buttons.append( ShapeButton(self, self.Rotate, "images/rotate_shape.gif", self._xpos, self._ypos, rotate=True))

        self.SetOrientation(self.app.getOrientation())


        # go through all the buttons and check if we clicked on any of the shape's buttons
        # if we did, return True, otherwise False
    def ButtonsClicked(self, x,y):
        if self.buttons[0].GetVisible():  #if buttons are visible at all, check for clicks
            for button in self.buttons:
                if button.HitTest(x,y):  # HitTest will also call the right event handler if the button was indeed hit
                    return True
        return False
    

    def Close(self):
        self.sageGate.shutdownApp(self.GetId())


    def Rotate(self):
        self.sageGate.rotateWindow(self.GetId(), 90)


    def Maximize(self):
        if not self.GetMaximized():
            dispUIBounds = self.displayCanvas.GetUIDisplayBounds(self.app.getDisplayId())
            width, height = self.GetBoundingBoxMin()

            # old values are already preserved if the app was minimized
            if not self.GetMinimized():  
                self.PreserveOldParams()

            # now set the new parameters (maximized)
            # place the maximized shape in the center of the screen
            self.SetX(round(dispUIBounds.left + dispUIBounds.getWidth() / 2))
            self.SetY(round(dispUIBounds.top + dispUIBounds.getHeight() / 2))

            # figure out by how much to multiply the size of the shape in order
            # to fill the tiled display and not go over all while preserving the aspect ratio (if needed)
            if self.displayCanvas.GetMaintainAspectRatio():
                widthRatio = dispUIBounds.getWidth() / float(width)
                heightRatio = dispUIBounds.getHeight() / float(height)
                if widthRatio > heightRatio:
                    maximizeRatio = heightRatio
                else:
                    maximizeRatio = widthRatio
                self.SetSize( round(width*maximizeRatio), round(height*maximizeRatio))
            else:  #do not maintain aspect ratio
                self.SetSize( dispUIBounds.getWidth(), dispUIBounds.getHeight() )

            self.SetMaximized(True)
            
        else:   #restore
            #pull out the old parameters and resize/move the shape
            self.RestoreOldParams()
            self.SetMaximized(False)

        self.SetMinimized(False) # an app can't be maximized and minimized at the same time
        self.displayCanvas.FreeMinimizedPosition(self.minimizedPosition, self.app)
        self.UpdateSAGEParams()

        
            
    def Minimize(self):
        if self.GetMinimized(): #restore
            #pull out the old parameters and resize/move the shape
            self.RestoreOldParams()
            self.SetMinimized(False)
            self.displayCanvas.FreeMinimizedPosition(self.minimizedPosition, self.app)
        else:  #minimize
            dispUIBounds = self.displayCanvas.GetUIDisplayBounds(self.app.getDisplayId())
            minimizedWidth = dispUIBounds.getWidth()/10  #height of the minimized apps
            
            width, height = self.GetBoundingBoxMin()
            self.SetMinimized(True)
            if not self.GetMaximized():
                self.PreserveOldParams()
        
            aspectRatio = float( height / float(width) )
            newHeight = aspectRatio * minimizedWidth
            self.SetSize( minimizedWidth, newHeight )
            self.minimizedPosition = self.displayCanvas.GetNextMinimizedPosition(self.app)
            self.SetX( dispUIBounds.left + minimizedWidth/2 + minimizedWidth*self.minimizedPosition)
            self.SetY( dispUIBounds.bottom - newHeight/2 )

        self.SetMaximized(False)  # an app can't be maximized and minimized at the same time
        self.UpdateSAGEParams()

                    
    def HideButtons(self):
        for button in self.buttons:
            button.SetVisible(False)
        self.buttonsVisible = False

    def ShowButtons(self):
        for button in self.buttons:
            button.SetVisible(True)
        self.buttonsVisible = True

                        
        # overridden so that we can display buttons on the shape (min, max, close...)
    def OnDrawContents(self, dc):
        self.SetFont(StandardFont())
        btnWidth = self.buttons[0].GetWidth()
        if btnWidth+20 < self.GetWidth():
            self.ShowButtons()
        else:
            self.HideButtons()

        # dont draw anything if buttons aren't visible (there's not enough space for them)
        if not self.buttonsVisible:
            return
        
        i = 0
        for i in range(len(self.buttons)):
            self.buttons[i].Move( self.GetRight() - btnWidth*(i+1)-1, self.GetTop()+1 )
            self.buttons[i].OnPaint(dc)
            i = i+1
        ogl.Shape.OnDrawContents(self, dc)



        # convert all the boundries and send a message to SAGE about the new size and/or position
        # of the app
    def UpdateSAGEParams(self):
        left = self.displayCanvas.ToSAGECoordsX( self.GetLeft(), self.app.getDisplayId() )
        right = self.displayCanvas.ToSAGECoordsX( self.GetRight(), self.app.getDisplayId() )
        top = self.displayCanvas.ToSAGECoordsY( self.GetTop(), self.app.getDisplayId() )
        bottom = self.displayCanvas.ToSAGECoordsY( self.GetBottom(), self.app.getDisplayId() )
        self.sageGate.resizeWindow( self.GetId(), left, right, bottom, top )
        

#----------------------------------------------------------------------


    # these methods have been overridden in order to make the center of the
    # shape appear to be in the top left corner and not in the center
    # NOTE!!!!
    # if you are going to use these methods at the same time when you are changing
    # the size of the shape, make sure you CHANGE THE SIZE OF THE SHAPE FIRST and
    # then use these functions since they rely on the size
    def GetMyX(self):
        width, height = self.GetBoundingBoxMin()
        return ogl.Shape.GetX(self) - width/2
      
    def GetMyY(self):
        width, height = self.GetBoundingBoxMin()
        return ogl.Shape.GetY(self) - height/2

    def SetMyY(self, newY):
        width, height = self.GetBoundingBoxMin()
        self.SetY( newY + height/2 )

    def SetMyX(self, newX):
        width, height = self.GetBoundingBoxMin()
        self.SetX( newX + width/2 )

#----------------------------------------------------------------------


    # for returning the boundries of the shape
    def GetLeft(self):
        return self.GetMyX()

    def GetRight(self):
        width, height = self.GetBoundingBoxMin()
        return self.GetMyX() + width

    def GetTop(self):
        return self.GetMyY()

    def GetBottom(self):
        width, height = self.GetBoundingBoxMin()
        return self.GetMyY() + height

    def GetWidth(self):
        return self.GetBoundingBoxMin()[0]

    def GetHeight(self):
        return self.GetBoundingBoxMin()[1]
    

    #----------------------------------------------------------------------


    def SetOrientation(self, degrees):
        for b in self.buttons:
            if b.rotate: b.SetOrientation(degrees)
    

    #for maximizing/minimizing
    def SetOldWidth(self, w):
        self.oldWidth = w

    def SetOldHeight(self, h):
        self.oldHeight = h

    def GetOldWidth(self):
        return self.oldWidth

    def GetOldHeight(self):
        return self.oldHeight


    def SetOldX(self, x):
        self.oldX = x

    def SetOldY(self, y):
        self.oldY = y

    def GetOldX(self):
        return self.oldX

    def GetOldY(self):
        return self.oldY


    def PreserveOldParams(self):
        width, height = self.GetBoundingBoxMin()
        self.SetOldX(self.GetLeft() + width/2)
        self.SetOldY(self.GetTop() + height/2)
        self.SetOldWidth(width)
        self.SetOldHeight(height)

    def RestoreOldParams(self):
        self.SetSize(self.GetOldWidth(), self.GetOldHeight())
        self.SetX(self.GetOldX())
        self.SetY(self.GetOldY())


    def GetMaximized(self):
        return self.maximized

    def SetMaximized(self, max):
        self.maximized = max

    def GetMinimized(self):
        return self.minimized

    def SetMinimized(self, min):
        self.minimized = min
      

#----------------------------------------------------------------------

    # z-ordering
    def SetZ(self, newZ):
        self.Z = newZ

    def GetZ(self):
        return self.Z


    # sets the title of the app
    def SetTitle(self, title):
        self.title = title
        self.ClearText()
        self.AddText(self.name + " " + str(self.GetId()))
        self.AddText(title)

    def GetTitle(self):
        return self.title
    

    # sets the name of the app 
    def SetName(self, name):
        self.name = name
        #self.ClearText()
        #self.AddText(name)

    def GetName(self):
        return self.name


    # highlights the app when a mouse goes over the app instance button
    def Highlight(self, doHightlight):
        if doHightlight:
            self.SetBrush(wx.Brush(shapeHighlightColor))#wx.MEDIUM_GREY_BRUSH)
        else:
            if not self.Selected():
                self.SetBrush(wx.Brush(shapeColor))
            else:
                self.SetBrush(wx.Brush(shapeSelectedColor))#wx.GREY_BRUSH)
        self.GetCanvas().Redraw()


    # applies the new size and position to the shape (after screen resize)
    def Recalculate(self, left, right, top, bottom):
        self.SetSize( (right - left), (bottom - top) )
        self.SetMyX(left)
        self.SetMyY(top)

        self.ResetControlPoints()

    # changes the border of the shape if it's selected
    def Select(self, doSelect, dc=None):
        if doSelect:
            self.SetTextColour(shapeSelectedTextColor)
            self.SetPen(wx.Pen(shapeSelectedBorderColor, 2))#.SetWidth(2)
            self.SetBrush( wx.Brush(shapeSelectedColor) ) #wx.GREY_BRUSH )
        else:
            self.SetTextColour(shapeTextColor)
            self.SetPen(wx.Pen(shapeBorderColor,1))#wx.Colour(0,0,0), 1))#self.SetPen().SetWidth(1)
            self.SetBrush( wx.Brush(shapeColor) ) #wx.GREY_BRUSH )
        ogl.RectangleShape.Select(self, doSelect, dc)

            
#----------------------------------------------------------------------


    ### this function figures out the right Z order of the windows and sends it to SAGE
    def BringToFront(self):
        if self.GetCanvas().GetTopShape() == self.GetId():
            return  #the shape is already on top
        
        # now send a message to SAGE
        self.sageGate.bringToFront(self.GetId())
                

#----------------------------------------------------------------------


    # if we try to resize the shape beyond the edges of the display, this will
    # size the shape down so that it fits inside the display
    def SizeDown(self, pt):
      
        displayCanvas = self.GetCanvas()
        myWidth, myHeight = self.GetBoundingBoxMin()  #get the size without the shadow
        newWidth = myWidth
        newHeight = myHeight
        ratio = myWidth/myHeight  #remember the aspect ratio
        minSize = 30    # the min size of the window on the screen

            
        # figure out if we resized the shape too much so that it's out of bounds
        # of the tiled display
        if self.GetBottom() > displayCanvas.GetBottom():
            newHeight = displayCanvas.GetBottom() - self.GetTop()
            newWidth = ratio * newHeight
            if pt.location == "SE":
                self.SetX(self.GetLeft() + newWidth/2)
            else:
                self.SetX(self.GetRight() - newWidth/2)
            self.SetY(self.GetTop() + newHeight/2)
            self.SetSize(newWidth, newHeight)
        if self.GetTop() < displayCanvas.GetTop():
            width, height = self.GetBoundingBoxMin()
            newHeight = self.GetBottom() - displayCanvas.GetTop() 
            newWidth = ratio * newHeight
            if pt.location == "NE":
                self.SetX(self.GetLeft() + newWidth/2)
            else:
                self.SetX(self.GetRight() - newWidth/2)
            self.SetY(self.GetBottom() - newHeight/2)
            self.SetSize(newWidth, newHeight)
        if self.GetLeft() < displayCanvas.GetLeft():
            width, height = self.GetBoundingBoxMin()
            newWidth = self.GetRight() - displayCanvas.GetLeft()
            newHeight = newWidth / ratio
            self.SetX(self.GetRight() - newWidth/2)
            if pt.location == "SW":
                self.SetY(self.GetTop() + newHeight/2)
            else:
                self.SetY(self.GetBottom() - newHeight/2)
            self.SetSize(newWidth, newHeight)
        if self.GetRight() > displayCanvas.GetRight():
            width, height = self.GetBoundingBoxMin()
            newWidth = displayCanvas.GetRight() - self.GetLeft()
            newHeight = newWidth / ratio
            self.SetX(self.GetLeft() + newWidth/2)
            if pt.location == "SE":
                self.SetY(self.GetTop() + newHeight/2)
            else:
                self.SetY(self.GetBottom() - newHeight/2)
            self.SetSize(newWidth, newHeight)


        # dont allow the windows to be smaller than "minSize" pixels
        if newWidth < minSize or newHeight < minSize:
            widthRatio = minSize / newWidth
            heightRatio = minSize / newHeight
            if widthRatio > heightRatio:
                resizeRatio = widthRatio
            else:
                resizeRatio = heightRatio
            self.SetSize( round(newWidth * resizeRatio), round(newHeight * resizeRatio) )


        # set the final size of the app window
        #self.SetSize( int(newWidth), int(newHeight) )
        self.SetX( round(self.GetX()) )
        self.SetY( round(self.GetY()) )


        # since we manually changed the parameters of the shape,
        # we manually have to redraw the screen
        self.ResetControlPoints() 
        displayCanvas.Redraw()


#----------------------------------------------------------------------


    # if we tried to move the app window beyond the edges of the tiled display
    # this would snap it to the edge

    def SnapToEdges(self, oldx, oldy):
        width, height = self.GetBoundingBoxMin()
        displayCanvas = self.GetCanvas()
        
        # RJ
        # october 15, 2004
        # modified Gideon's code to "snap" windows to the edges if they go over
	if self.GetTop() < displayCanvas.GetTop():
            self.SetMyY(displayCanvas.GetTop())       
            self.SetMyX(self.GetMyX())
        if self.GetLeft() < displayCanvas.GetLeft():
            self.SetMyX(displayCanvas.GetLeft())       
            self.SetMyY(self.GetMyY())
        if self.GetRight() > displayCanvas.GetRight():
            self.SetMyX(displayCanvas.GetRight() - width)   
            self.SetMyY(self.GetMyY())
        if self.GetBottom() > displayCanvas.GetBottom():
            self.SetMyY(displayCanvas.GetBottom() - height)        
            self.SetMyX(self.GetMyX())


        # redraw the screen
        self.ResetControlPoints() 
        displayCanvas.Redraw()
      




############################################################################
#
#  CLASS:       MyEvtHandler
#
#  DESCRIPTION: Extends the original ogl.ShapeEvtHandler class. 
#               This is the main event handler for all the actions performed
#               on the shapes themselves. It handles resizing, moving, click
#               and double click. It also sends corresponding messages to SAGE
#               about the actions performed.
#
#  DATE:        October 2004
#
############################################################################

class MyEvtHandler(ogl.ShapeEvtHandler):

    def __init__(self, log, statBarFrame, sageGate, displayCanvas):
        ogl.ShapeEvtHandler.__init__(self)
        self.log = log
        self.statbarFrame = statBarFrame
        self.sageGate = sageGate #we need access to the SAGE messenger
        self.displayCanvas = displayCanvas
        self.usersData = getUsersData()
        
#----------------------------------------------------------------------

    def UpdateStatusBar(self, shape):
        x, y = shape.GetX(), shape.GetY()
        width, height = shape.GetBoundingBoxMax()
        myID = shape.GetId() # always 0
        #self.statbarFrame.SetStatusText("ID: %d Pos: (%d, %d)  Size: (%d, %d)" %
        #                                (myID, x, y, width, height))

        
#----------------------------------------------------------------------

    def OnLeftClick(self, x, y, keys=0, attachment=0):
        shape = self.GetShape()
        canvas = shape.GetCanvas()
        dc = wx.ClientDC(canvas)
        canvas.PrepareDC(dc)


        # here we check if the user actually clicked on any of the shape buttons (min, max, close...)
        # and if so, don't continue with this event handler
        if shape.ButtonsClicked(x,y):
            return
                
        # we clicked on the shape so bring it to front (above all other windows) and send a message to SAGE
        shape.BringToFront()
        
        if shape.Selected():
            return
        else:
            redraw = False
            shapeList = canvas.GetDiagram().GetShapeList()
            toUnselect = []

            for s in shapeList:
                if s.Selected():
                    # If we unselect it now then some of the objects in
                    # shapeList will become invalid (the control points are
                    # shapes too!) and bad things will happen...
                    toUnselect.append(s)

            shape.Select(True, dc)

            if toUnselect:
                for s in toUnselect:
                    s.Select(False, dc)
                    #s.GetPen().SetWidth(1)
                #canvas.Redraw(dc)

        del dc
                    
        self.UpdateStatusBar(shape)

        # bring up the performance data
        canvas.GetParent().GetAppInfoPanel().GetInfoPanel().ShowData(shape.GetName(), shape.GetId())
        
        # since we manually changed the parameters of the shape,
        # we manually have to redraw the screen
        #canvas.Redraw(dc)#Refresh()
        
        
        

#----------------------------------------------------------------------

    def OnEndDragLeft(self, x, y, keys=0, attachment=0):
        shape = self.GetShape()

        # test if the user dropped the application window onto another tab
        # if so, just send a message to SAGE to stream that app to that new display
        if shape.GetCanvas().GetParent().GetNotebook().DropTest(shape.GetId(), (x,y)):
            if shape.GetCanvas().HasCapture():
                shape.GetCanvas().ReleaseMouse()
            return 

        #if not shape.Selected():
        self.OnLeftClick(x, y, keys, attachment)


        oldx, oldy = shape.GetX(), shape.GetY()
        ogl.ShapeEvtHandler.OnEndDragLeft(self, x, y, keys, attachment)


        # check for move (note: NOT resize) out of bounds
        # i.e. shape partially outside of "tiled" region
        #shape.SnapToEdges(oldx, oldy)


        # figure out the distance moved and send a message to SAGE
        self.sageGate.moveWindow(shape.GetId(),
                                 self.displayCanvas.ToSAGEDistX( oldx, shape.GetX(), shape.app.getDisplayId() ),
                                 self.displayCanvas.ToSAGEDistY( oldy, shape.GetY(), shape.app.getDisplayId() ))

        self.UpdateStatusBar(shape)
        

#----------------------------------------------------------------------


    def OnSizingEndDragLeft(self, pt, x, y, keys, attch):
      shape = self.GetShape()
      
      # call the overridden method for handling the sizing
      shape.OnSizingEndDragLeft(pt, x, y, keys, attch)

      # figure out if we resized the shape beyond the edge of our UI tiled display
      # if so, resize it down to the maximum allowable size
      #shape.SizeDown(pt)

      # convert the new coords and send a message to SAGE
      left = self.displayCanvas.ToSAGECoordsX( shape.GetLeft(), shape.app.getDisplayId() )
      right = self.displayCanvas.ToSAGECoordsX( shape.GetRight(), shape.app.getDisplayId() )
      top = self.displayCanvas.ToSAGECoordsY( shape.GetTop(), shape.app.getDisplayId() )
      bottom = self.displayCanvas.ToSAGECoordsY( shape.GetBottom(), shape.app.getDisplayId() )
      self.sageGate.resizeWindow( shape.GetId(), left, right, bottom, top )
      
      # if we were maximized and we tried to resize a shape, reset the maximized state
      shape.SetMaximized(False)
      self.UpdateStatusBar(shape)

#----------------------------------------------------------------------

    # (AKS 2005-01-28)
    # Use this for displaying performance graphs
    def OnRightClick(self, x, y, keys=0, attachment=0):
        shape = self.GetShape()
        menu = wx.Menu()
    
        machineMenu = wx.Menu()        
        machines = self.usersData.GetMachinesStatus()
        c = 30
        for mId, m in machines.iteritems():
            if m.IsAlive(): txt=m.GetName()+"  ( + )"
            else: txt=m.GetName()+"  ( - )"
            mi = machineMenu.Append(c, txt)
            mi.SetHelp(mId)   #set the help to the machineId... we will use this later to get the right SAGEMachine
            machineMenu.Bind(wx.EVT_MENU, self.OnStreamMenu, id=c)
            c+=1
            
        menu.Append(1, "Edit Properties")        
        menu.AppendMenu(3, "Stream To:", machineMenu)
        menu.Bind(wx.EVT_MENU, self.OnContextMenu)
        self.displayCanvas.PopupMenu(menu)
        #self.rightClickFunctionCallback( shape.GetName(), shape.GetId() )


    def OnStreamMenu(self, event):
        mi = event.GetEventObject().FindItemById(event.GetId())
        machine = self.usersData.GetMachine(mi.GetHelp())  # help string actually contains the machineId
        self.sageGate.streamApp(self.GetShape().GetId(), machine.GetSystemIP(), machine.GetSystemPort())
        

    def OnContextMenu(self, event):
        shape = self.GetShape()
        eventId = event.GetId()
        
        if eventId == 1:  #this must be the "Edit Properties"
            self._ShowPropertiesDialog(shape)
        elif eventId >= 30 and eventId < 40:
            mi = event.GetEventObject().FindItemById(event.GetId())
            machine = self.usersData.GetMachine(mi.GetHelp())  # help string actually contains the machineId
            self.sageGate.streamApp(self.GetShape().GetId(), machine.GetSystemIP(), machine.GetSystemPort())

            
    def _ShowPropertiesDialog(self, shape):
        dlg = wx.Dialog(None, -1, "Edit Properties", style=wx.CLOSE_BOX | wx.RESIZE_BORDER)
        okBtn = wx.Button(dlg, wx.ID_OK, "OK")
        cancelBtn = wx.Button(dlg, wx.ID_CANCEL, "Cancel")
        
        line1 = "Enter a new title for "+str(shape.GetName()+" "+str(shape.GetId()))
        text1 = wx.StaticText(dlg, -1, line1, style = wx.ALIGN_LEFT)
        titleEntry = wx.TextCtrl(dlg, -1, shape.GetTitle())
        titleColorBtn = wx.Button(dlg, -1, "Title Color")
        self.titleColorText = wx.StaticText(dlg, -1, "unchanged", style = wx.ALIGN_CENTER)
        titleColorBtn.Bind(wx.EVT_BUTTON, self._ChangeTitleColor) #(shape.titleColor, "title", titleColorText))
        borderColorBtn = wx.Button(dlg, -1, "Border Color")
        self.borderColorText = wx.StaticText(dlg, -1, "unchanged", style = wx.ALIGN_CENTER)
        borderColorBtn.Bind(wx.EVT_BUTTON, self._ChangeBorderColor) #(shape.borderColor, "border", borderColorText))
        self.sliderText = "Maximum frame rate: "
        self.frameRateText = wx.StaticText(dlg, -1, self.sliderText + "UNCHANGED", style = wx.ALIGN_CENTER)
        self.frameRateSlider = wx.Slider(dlg, -1, 0, 0, 60, style = wx.SL_LABELS | wx.SL_HORIZONTAL)
        self.frameRateSlider.Bind(wx.EVT_SCROLL, self._ProcessSliderEvent)

        hSizer1 = wx.BoxSizer(wx.HORIZONTAL)
        hSizer1.Add(titleColorBtn, 0, wx.ALIGN_LEFT | wx.LEFT,  border=0)
        hSizer1.Add(self.titleColorText, 0, wx.ALIGN_LEFT | wx.LEFT, border=20)
        hSizer2 = wx.BoxSizer(wx.HORIZONTAL)
        hSizer2.Add(borderColorBtn, 0, wx.ALIGN_LEFT | wx.LEFT, border=0)
        hSizer2.Add(self.borderColorText, 0, wx.ALIGN_LEFT | wx.LEFT, border=10)
        hSizer3 = wx.BoxSizer(wx.HORIZONTAL)
        hSizer3.Add(cancelBtn, 0, wx.ALIGN_CENTER | wx.RIGHT | wx.LEFT, border=10)
        hSizer3.Add(okBtn, 0, wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT, border=10)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.AddSpacer((20,20))
        sizer.Add(text1, 0, wx.ALIGN_LEFT | wx.LEFT, border=15)
        sizer.Add(titleEntry, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT | wx.EXPAND, border=15)
        sizer.AddSpacer((20,20))
        sizer.Add(hSizer1, 0, wx.ALIGN_LEFT | wx.LEFT, border=15)
        sizer.AddSpacer((10,10))
        sizer.Add(hSizer2, 0, wx.ALIGN_LEFT | wx.LEFT, border=15)
        sizer.AddSpacer((25,25))
        sizer.Add(self.frameRateText, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT, border=15)
        sizer.Add(self.frameRateSlider, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT | wx.EXPAND, border=15)
        sizer.AddSpacer((30,30))
        sizer.Add(hSizer3, 0, wx.ALIGN_CENTER, border=15)
        sizer.AddSpacer((10,10))
        

        dlg.SetSizer(sizer)
        dlg.Fit()
        if dlg.ShowModal() == wx.ID_OK:
            shape.SetTitle(titleEntry.GetValue())
            retValTitleColor = (-1,-1,-1)
            retValBorderColor = (-1,-1,-1)
            if not self.titleColorText.GetLabel() == "unchanged":
                retValTitleColor = (shape.titleColor.Red(),shape.titleColor.Green(),shape.titleColor.Blue())
            if not self.borderColorText.GetLabel() == "unchanged":
                retValBorderColor = (shape.borderColor.Red(),shape.borderColor.Green(),shape.borderColor.Blue())
        
            self.sageGate.changeAppProperties(shape.GetId(), shape.GetTitle(), retValTitleColor, retValBorderColor)

            if not self.frameRateSlider.GetValue() == 0:
                self.sageGate.changeAppFrameRate(shape.GetId(), self.frameRateSlider.GetValue())
            self.displayCanvas.Redraw()

        dlg.Destroy()
        

    def _ChangeTitleColor(self, event):
        shape = self.GetShape()
        newColor = ShowColorDialog(shape.titleColor)
        if not newColor == (-1,-1,-1):
            shape.titleColor = newColor
            self.titleColorText.SetLabel( str(newColor) )

        
    def _ChangeBorderColor(self, event):
        shape = self.GetShape()
        newColor = ShowColorDialog(shape.borderColor)
        if not newColor == (-1,-1,-1):
            shape.borderColor = newColor
            self.borderColorText.SetLabel( str(newColor) )

    def _ProcessSliderEvent(self, event):
        if self.frameRateSlider.GetValue() == 0:
            self.frameRateText.SetLabel(self.sliderText + str("UNCHANGED"))
        else:
            if not self.frameRateText.GetLabel() == self.sliderText:
                self.frameRateText.SetLabel(self.sliderText)

        
#----------------------------------------------------------------------


    def OnMovePost(self, dc, x, y, oldX, oldY, display):
        ogl.ShapeEvtHandler.OnMovePost(self, dc, x, y, oldX, oldY, display)
        self.UpdateStatusBar(self.GetShape())
        

#----------------------------------------------------------------------

   # (AKS 2005-01-28) Added so that GraphManager gets called when someone
   # right clicks on the SAGE window
    #def RegisterRightClickFunction( self, function ):
    #    self.rightClickFunctionCallback = function



