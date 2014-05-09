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





from overlay import Overlay
from eventHandler import EventHandler
from globals import *
from button import Button
from time import time

def makeNew(app=None):
    """ this is how we instantiate the device object from
        the main program that loads this plugin """
    return Menu(app)


TYPE = WIDGET_PLUGIN


# states
SHOWN = 0
HIDDEN = 1

# messages
SHOW_MENU = 10
HIDE_MENU = 11
SELECT_MENU_ITEM = 12


class Menu(Button):
    
    def __init__(self, app=None):
        """ If you pass in None for app, the widget will reside on the top layer
            above all apps but below pointers.
        """
        Button.__init__(self, app)
        self.overlayType = OVERLAY_MENU  #override what Button.__init__ set

        self.__shownBounds = Bounds()
        self.__shownZ = TOP_Z
        self.__hiddenZ = self.z
        self.__lastClickTime = 0
        self.__selection = -1  # item ID currently selected
        self.__menuItems = []
        self.__state = HIDDEN
        self.__callbacks = {}  # what to call when a menu item is clicked

        self.doPlaySound(False)  # because menu has its own sounds
        
        self.registerForEvent(EVT_DRAG, self._onMove)

       
    # -------    WIDGET PARAMETERS   -----------------    

    def addMenuItem(self, txt, callback=None, data=None):
        """ callback must accept on parameter which is the menu item 
            object that got clicked
        """
        id = len(self.__menuItems)
        mi = TextMenuItem(txt, id, callback)
        mi._setUserData(data)
        self.__menuItems.append(mi)
        return id


    def _showMenu(self, x, y):
        self.__updateMenuBounds(x, y)

        # the menu itself is always shown on top
        self.__hiddenZ = self.z
        self.z = TOP_Z

        # send the bounds of the menu to the display nodes
        self.sendOverlayMessage(SHOW_MENU, 
                                self.__shownBounds.left, 
                                self.__shownBounds.right, 
                                self.__shownBounds.top, 
                                self.__shownBounds.bottom)
        self.__state = SHOWN

        playSound(MENU_OPEN)


    def _hideMenu(self):
        self.z = self.__hiddenZ
        self.sendOverlayMessage(HIDE_MENU)
        self.__state = HIDDEN
        self.__selection = -1

        playSound(MENU_CLOSE)
        

    # -------    GENERIC  EVENTS   -----------------    

    def _onLeftWindow(self, event):
        Button._onLeftWindow(self, event)
        if self.__state == SHOWN:
            event.device.toEvtHandler = self

    def _onEnteredWindow(self, event):
        Button._onEnteredWindow(self, event)


    # -------    DEVICE EVENTS   -----------------
    
    def _onMove(self, event):
        """ need this to receive LEFT_WINDOW and ENTERED_WINDOW events """
        if self.__state == SHOWN:
            #event.device.pointer.resetPointer()
            event.device.toEvtHandler = self
            
            for item in self.__menuItems:
                if item._isIn(event.x, event.y):
                    if self.__selection != item.getItemId():
                        self.__selection = item.getItemId()
                        self.sendOverlayMessage(SELECT_MENU_ITEM, self.__selection)
                        playSound(MENU_ITEM_HIGHLIGHT)
                    break
                        
            else:   # deselect any selected items
                if self.__selection != -1:
                    self.__selection = -1
                    self.sendOverlayMessage(SELECT_MENU_ITEM, self.__selection)

 
    def _onClick(self, event):
        def processClick(x,y):   # find whether an item was clicked and hide the menu
            for item in self.__menuItems:
                if item._isIn(x, y):
                    item._onClick()
                    if self._isAppWidget() and MENU_ITEM_CLICKED in self.widgetEvents:
                        self._sendAppWidgetEvent(MENU_ITEM_CLICKED, str(item.getItemId()))
                    playSound(CLICK)
                    break
            self._hideMenu()

        # figure out if it was a quick click or a drag motion
        timeDiff = time() - self.__lastClickTime
        self.__lastClickTime = time()
        shortClick = (not event.isDown) and (timeDiff < 0.75)   

        if self.__state == SHOWN:
            if event.isDown or not shortClick: 
                processClick(event.x, event.y)
            
        elif event.isDown:
            self._showMenu(event.x, event.y)
            event.device.toEvtHandler = self  # make all the events go here


    def isIn(self, x, y):
        """ overridden to detect hits when a menu is shown """
        if self.__state == SHOWN:
            return self.__shownBounds.isIn(x,y)
        else:
            return EventHandler.isIn(self,x,y)


    # -------     XML STUFF     -----------------    

    def _specificToXml(self, parent):
        """ add the label parameter """
        menu = self.doc.createElement("menu")

        for item in self.__menuItems:
            mi = self.doc.createElement("menuItem")
            mi.setAttribute("id", str(item.getItemId()))
            mi.appendChild(self.doc.createTextNode(item.getLabel()))
            menu.appendChild(mi)
            
        parent.appendChild(menu)
     

    # fill the menu info if it was requested by the app
    def _onSpecificInfo(self):
        if "menu" in self.xmlElements:
            for mi in self.xmlElements["menuItem"]:
                newItem = TextMenuItem(mi.value, int(mi.attrs["id"]), None)
                self.__menuItems.append(newItem)



    # -------     MISC STUFF     -----------------    

    def __updateMenuBounds(self, x, y):
        # first set the font size for each menu item
        for item in self.__menuItems:
            item._setFontSize(int(self._ftSize *
                                 self._getScale() * 
                                 self._getScaleMultiplier()))  # same as parent for now
            

        # get the maximum width and total height of all menu items
        width = 0
        height = 0
        for item in self.__menuItems:
            if item._getMinWidth() >= width:
                width = item._getMinWidth()
            if item._getMinHeight() >= height:
                height = item._getMinHeight()

        # figure out where to position the menu so that it's visible
        # consider mullions in the future
        b = self.__shownBounds   # less typing
        b.left = x
        b.bottom = y - height*len(self.__menuItems)
        b.top = y
        b.right = x + width

        # move it to the center of the mouse click
        centerX = b.left - int(b.getWidth() / 2.0)
        centerY = b.bottom + int(b.getHeight() / 2.0)
        b.setPos(centerX, centerY)

        # check that all of the menu is visible... if not move it
        # when checking assume x,y are always in bounds because they came from
        # a click
        d = getSageData().getDisplayInfo()

        if b.bottom < 0:
            b.setY(0)
        elif b.top > d.sageH:
            b.setY(d.sageH - b.getHeight())

        if b.right > d.sageW:
            b.setX(d.sageW-width)
        elif b.left < 0:
            b.setX(0)


        # make all of them equal width and set the positions
        for item in self.__menuItems:
            item._setPos(b)



class TextMenuItem:
    """ bounds are relative to the menu """

    def __init__(self, label, itemId, callback):
        self.__label = label
        self.__itemId = itemId
        self.__data = None
        self.__callback = callback
        self._setFontSize(10)  # default
    
    def _setUserData(self, data):
        self.__data = data
    
    def getUserData(self):
        return self.__data

    def getItemId(self):
        return self.__itemId

    def getLabel(self):
        return self.__label

    def _getMinHeight(self):
        """ based on the label point size """
        return self.__minHeight + int(self.__minHeight*0.2) #+ 10

    def _getMinWidth(self):
        """ based on the label point size """
        return self.__minWidth + int(self.__minWidth*0.2) #+ 10

    def _isIn(self, x, y):
        """ returns True if the (x,y) is in bounds, False otherwise """
        if self.x <= x and (self.x+self.w) >= x and \
                self.y <= y and (self.y+self.h) >= y:
            return True
        else:
            return False
    
    def _setPos(self, bounds):
        self.w = bounds.getWidth()
        self.x = bounds.left
        self.y = bounds.top - self.__itemId * self._getMinHeight() - self._getMinHeight()
        
        
    def _onClick(self):
        """ what to call when a menu item is selected """
        if self.__callback:
            self.__callback(self)
            
    def _setFontSize(self, ftSize):
        self.__ftSize = ftSize        
        (self.__minWidth, self.__minHeight) = getTextSize(self.__label, self.__ftSize)
        

        yOffset = self.__itemId * self._getMinHeight()

        self.x = 0
        self.y = yOffset
        self.w = self._getMinWidth()
        self.h = self._getMinHeight()
