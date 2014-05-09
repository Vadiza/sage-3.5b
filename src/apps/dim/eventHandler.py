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




from globals import *
from math import floor, sqrt
from xml.dom.minidom import Document
import xml.parsers.expat
import copy
from sageGate import getGate
# from sageData import SageData

class EventHandler:
    """ The base class for any objects wanting to receive device events.
        Make sure the setSize and setXY are called since it's used
        in deciding whether an event should be delivered to it or not.
        Also, make sure you update the z value (if applicable)

        Example of registering for events:
        ----------------------------------------
        self.registerForEvent(EVT_CLICK, self.onClick)
        def onClick(self, event):
            # do something with the click event...
    """

    def __init__(self, app=None):
        self.__evtMgr = getEvtMgr()  # for registering for events
        self._captured = False  # this is used by the eventManager to NOT forward events if widget is already captured... captured flag is controlled by the widgets themselves as needed...
        self._added = False    # true if the widget has been added or its in the add queue
        self.__selected = False
        self._allowSelection = False
        
        # None if we are not tied to an app
        self.app = app

        # default the parent bounds to the displayBounds
        self._parentBounds = Bounds()
        d = getSageData().getDisplayInfo()
        self._parentBounds.setAll(0, d.sageW, d.sageH, 0)
        
        # based on this we determine whether this evtHandler should get a certain event
        # using size and pos guidelines/restrictions we calculate bounds
        self.__bounds = Bounds()   # the actual size of the widget in SAGE coords
        self.size = ObjSize()      # size description as set by the user
        self.pos = ObjPos()        # pos description as set by the user
        self._scale = 1.0    # adjustment for different display sizes
        self._canScale = True   # some things aren't meant to scale (like app borders)
        self._mult = 1.0  # used for resizing widgets when mouse gets close
        self._roundEvt = False  # used in Fitts' law test... treat target as round

        # can the object be deliberately moved around (like App object)?
        self.__allowsDrag = False

        # can other objects be dropped on this object?
        self.__onDropCallback = None    # callback to call, set via onDrop(callback) 

        # slide in/out of the sides
        self.__allowSlide = False
        self.__slideHidden = False
        self.__slideNum = 0   # number of objects keeping the widget shown

        # params to be filled in from the xml or subclasses
        self.displayId = 0         
        self.__isGlobal = True   # for the whole display as opposed to 1 per tile
        self.__drawOrder = INTER_DRAW   
        self.__delayDraw = False

        # anything you want it to be
        self.__userData = None

        # >-1 is for app-created widgets, <-1 for dim-created ones
        self.__widgetId = getOverlayMgr().getNewId() 

        # parent child relationship
        self.__parentId = -1  # which parent is this widget attached to
        self.__parentOrder = -1  # which position in the parent this widget is in
        self.__children = {}  # key=parentOrder, value=child overlay object
        self.parent = None  # the parent object

        # a list of widget events that this widget wants
        self.widgetEvents = []  

        # all xml data that wasn't parsed by this class 
        # keyed by element name, value=XMLElement object
        # (if there is more than one element of the same name, they are in a list)
        self.xmlElements = {}  
        
        self.z = TOP_Z   # always on top (for now, may change based on the app or manually)
        self._zOffset = 0  # how much to add/subtract from the z value of the app
        self._eventTransparent = False # transparent objects let events through (e.g. sizers)
        self._tempZOffset = 0.0  # a hack to get Thumbnails mouseOverScale working

        # label
        self.__label = ""
        self.__labelAlignment = -1   # means widget resizes around label
        self._ftSize = NORMAL
        self.__relativeFontSize = 1.0
        self.__fontColor = None
        self.__backgroundColor = None
        self.__alpha = None

        # tooltip
        self.__tooltip = ""
        self.__ttSize = NORMAL
        self.__relativeTooltipSize = 1.0
        
        # key eventId, value=callback
        self.__callbacks = {}  

        # if the evtHandler is tied to the app, listen for changes in that app
        if app:
            self._parentBounds.setAll(self.app.getLeft(), self.app.getRight(),
                                       self.app.getTop(), self.app.getBottom())

            self.displayId = app.getDisplayId()
            self._zOffset = DIM_WIDGET_Z
            self.z = app.getZvalue() + self._zOffset
            self.registerForEvent(EVT_APP_INFO, self._onAppChanged)
            self.registerForEvent(EVT_Z_CHANGE, self._onZChanged)



    # -------    MUST SET POS AND SIZE (or set label which 
    #            will automatically resize the widget)   -----------------    


    def setSize(self, w, h):
        """ either relative or absolute (int or float)
            if relative, you must supply app to the constructor
        """
        
        # width
        self.size.w = w
        if isinstance(w, int):
            self.size.wType = ABSOLUTE
        else:
            self.size.wType = NORMALIZED

        # height
        self.size.h = h            
        if isinstance(h, int):
            self.size.hType = ABSOLUTE
        else:
            self.size.hType = NORMALIZED

        # since we are setting the size by hand, set the default label alignment
        if self.__labelAlignment == -1:
            self.__labelAlignment = LEFT

        # resize the draw object
        if self.isInitialized():
            self.sendOverlayMessage(SET_SIZE, self.size.w, self.size.h)

    
    def align(self, xAlignment, yAlignment, xMargin=0, yMargin=0):
        """ supply as alignment flags: LEFT, RIGHT_OUTSIDE, CENTER ..."""
        self.pos.xType = ALIGNED
        self.pos.xMargin = xMargin     
        self.pos.x = xAlignment   
        self.pos.yType = ALIGNED
        self.pos.yMargin = yMargin     
        self.pos.y = yAlignment   

        
    def alignX(self, xAlignment, xMargin=0):
        """ supply as alignment flags: LEFT, RIGHT_OUTSIDE, CENTER ..."""
        self.pos.xType = ALIGNED
        self.pos.xMargin = xMargin     
        self.pos.x = xAlignment   
        

    def alignY(self, yAlignment, yMargin=0):
        """ supply as alignment flags: TOP, BOTTOM_OUTSIDE, CENTER ..."""
        self.pos.yType = ALIGNED
        self.pos.yMargin = yMargin     
        self.pos.y = yAlignment   


    def alignLabel(self, alignment):
        """ supply as alignment flags: LEFT, RIGHT, CENTER (default) """
        self.__labelAlignment = alignment
        
   
    def setPos(self, x, y):
        """ 
           supply x and y as:
           normalized (float) -OR- 
           absolute (int)
        """
        if self.pos.x != x or self.pos.y != y:
            self.setX(x)
            self.setY(y)

            # move the draw object
            if self.isInitialized():
                self.sendOverlayMessage(SET_POS, self.pos.x, self.pos.y)


    def setY(self, y):
        """ 
           supply y as:
           normalized (float) -OR- 
           absolute (int)
        """
        if isinstance(y, float):
            self.pos.yType = NORMALIZED
        else:             
            self.pos.yType = ABSOLUTE
        self.pos.y = y

        
    def setX(self, x):
        """ 
           supply x as:
           normalized (float) -OR- 
           absolute (int)
        """
        if isinstance(x, float):
            self.pos.xType = NORMALIZED
        else:             
            self.pos.xType = ABSOLUTE
        self.pos.x = x   


    def setUserData(self, d):
        self.__userData = d
        
    def getUserData(self):
        return self.__userData

    
    def setDrawOrder(self, do):
        self.__drawOrder = do

        
    def drawLast(self, dd):
        self.__delayDraw = dd


    def _getScale(self):
        """ adjustment for different display sizes"""
        return self._scale

    def _setScaleMultiplier(self, m):
        """ dynamic resizing of widgets when mouse gets closer """
        self._mult = m
        if self.isInitialized():
            self.sendOverlayMessage(TEMP_SCALE, self._scale*m)
            self._refresh()

    def _getScaleMultiplier(self):
        """ dynamic resizing of widgets when mouse gets closer """
        return self._mult


    def __fitAroundLabel(self):
        """ resizes widget around the label... unless we are using alignment flag
            in which case the user must specify the size of the widget """
        if self.__labelAlignment == -1:
            fs = int(self._ftSize * float(self._scale * self._mult))
            if fs < MIN_FONT_SIZE:
                fs = MIN_FONT_SIZE
                
            (w,h) = getTextSize(self.__label, fs)
            wPadding = int(fs*0.8);
            hPadding = int(fs*0.4);
            return w+wPadding, h+hPadding



    def allowSlide(self, doSlide):
        self.__allowSlide = doSlide

    def _isSlideHidden(self):
        return self.__slideHidden

    def slideHide(self):
        if self.__allowSlide:
            self.__slideNum -= 1
            if self.__slideNum < 0: self.__slideNum = 0
            if self.__slideNum == 0:
                self.__slideHidden = True
                self.sendOverlayMessage(SLIDE_DO_HIDE, 1)
                self._refresh()
            
    def slideShow(self):
        if self.__allowSlide:
            self.__slideNum += 1
            if self.__slideHidden:
                self.__slideHidden = False
                self.sendOverlayMessage(SLIDE_DO_HIDE, 0)
                self._refresh()
                


    # -------    WIDGET PARAMS TO XML   --------------    
    
    def _dataToXml(self, overlayType):
        """ creates xml string from the parameters for sending
            to sage and display nodes 
        """

        # assign the scale 
        if self._canScale:
            self._scale = getGlobalScale()
            

        # calculate all the bounds
        self._refresh()


        # figure out the zOffset and the drawOrder from it
        if self._isAppWidget():
            self._zOffset = APP_WIDGET_Z
            zType = APP_Z
        elif self.app:
            if self._zOffset == 0:
                self._zOffset = DIM_WIDGET_Z
            zType = APP_Z
        elif self.z > BOTTOM_Z-1000:   # in case the widget is in the back...
            self.__drawOrder = PRE_DRAW
            self._zOffset = (self.z - BOTTOM_Z)
            zType = MAX_Z
        else:
            #self._zOffset = 0
            zType = MIN_Z
            
        
        # Create the minidom document
        self.doc = Document()

        # Create the <widget> base element
        root = self.doc.createElement("widget")
        root.setAttribute("widgetType", overlayType)
        root.setAttribute("widgetID", str(self.__widgetId))
        #if self.app:                        # XML FIELD NOT USED ANYMORE!!!!
        #    root.setAttribute("winID", str(self.app.getId()))
        #else:
        #    root.setAttribute("winID", str(-1))
        root.setAttribute("isGlobal", str(int(self.__isGlobal)))
        root.setAttribute("delayDraw", str(int(self.__delayDraw)))
        root.setAttribute("isVisible", str(int(self.visible)))
        root.setAttribute("isSlideHidden", str(int(self.__slideHidden)))
        root.setAttribute("drawOrder", str(self.__drawOrder))
        root.setAttribute("displayID", str(self.displayId))
        root.setAttribute("parentID", str(self.__parentId))
        root.setAttribute("parentOrder", str(self.__parentOrder))
        root.setAttribute("displayScale", str(getGlobalScale()))
        
        common = self.doc.createElement("common")
        
        # width
        w = self.doc.createElement("w")
        w.setAttribute("type", str(self.size.wType))
        wText = self.doc.createTextNode(str(self.size.w))
        w.appendChild(wText)
        common.appendChild(w)

        # height
        h = self.doc.createElement("h")
        h.setAttribute("type", str(self.size.hType))
        hText = self.doc.createTextNode(str(self.size.h))
        h.appendChild(hText)
        common.appendChild(h)

        # x
        x = self.doc.createElement("x")
        x.setAttribute("type", str(self.pos.xType))
        if self.pos.xMargin != 0:
            x.setAttribute("margin", str(self.pos.xMargin))
        x.appendChild(self.doc.createTextNode(str(self.pos.x)))
        common.appendChild(x)

        # y
        y = self.doc.createElement("y")
        y.setAttribute("type", str(self.pos.yType))
        if self.pos.yMargin != 0:
            y.setAttribute("margin", str(self.pos.yMargin))
        y.appendChild(self.doc.createTextNode(str(self.pos.y)))
        common.appendChild(y)

        # z
        z = self.doc.createElement("z")
        z.setAttribute("offset", str(self._zOffset))
        z.appendChild(self.doc.createTextNode(str(zType)))
        common.appendChild(z)

        # scale (only if the widget is allowed to scale)
        if self._canScale:
            s = self.doc.createElement("scale")
            s.appendChild(self.doc.createTextNode(str(self._scale)))
            common.appendChild(s)


        # if any of the above are relative or aligned, include parentBounds
        # if not tied to the app...
        if not self.app and (self.pos.x != ABSOLUTE or 
                             self.pos.y != ABSOLUTE or 
                             self.size.w != ABSOLUTE or 
                             self.size.h != ABSOLUTE):

            # the parentBounds are the actual wall display bounds
            pb = self._parentBounds
            b = self.doc.createElement("parentBounds")
            b.setAttribute("x", str(pb.left))
            b.setAttribute("y", str(pb.bottom))
            b.setAttribute("w", str(pb.getWidth()))
            b.setAttribute("h", str(pb.getHeight()))
            common.appendChild(b)
            

        root.appendChild(common)
        self.doc.appendChild(root)

        # add specific info 
        s = self.doc.createElement("specific")
        
        # add label if present
        if self.__label:
            l = self.doc.createElement("label")
            l.setAttribute("relativeFontSize", str(self.__relativeFontSize))
            l.setAttribute("fontSize", str(self._ftSize))

            if self.__labelAlignment != -1:
                l.setAttribute("labelAlignment", str(self.__labelAlignment))
                               
            l.appendChild(self.doc.createTextNode(self.__label))
            s.appendChild(l)

        # add label if present
        if self.__tooltip:
            tt = self.doc.createElement("tooltip")
            tt.setAttribute("relativeTooltipSize", str(self.__relativeTooltipSize))
            tt.setAttribute("fontSize", str(self.__ttSize))
            tt.appendChild(self.doc.createTextNode(self.__tooltip))
            s.appendChild(tt)
        
        root.appendChild(s)
        self._specificToXml(s)


        # add graphics info
        g = self.doc.createElement("graphics")

        # add fontColor
        if self.__fontColor:
            fc = self.doc.createElement("fontColor")
            fc.appendChild(self.doc.createTextNode(str(self.__fontColor.toInt())))
            g.appendChild(fc)

        # add backgroundColor
        if self.__backgroundColor:
            bc = self.doc.createElement("backgroundColor")
            bc.appendChild(self.doc.createTextNode(str(self.__backgroundColor.toInt())))
            g.appendChild(bc)

        # add alpha
        if self.__alpha:
            al = self.doc.createElement("alpha")
            al.appendChild(self.doc.createTextNode(str(self.__alpha)))
            g.appendChild(al)

        root.appendChild(g)
        self._graphicsToXml(g)

        xml = self.doc.toxml()
        return xml

    
    def _specificToXml(self, parent):
        """ this is the overlay specific stuff to be embedded in the xml doc
            You may override this method and append your 
            specific stuff to *parent* 
        """
        pass
        

    def _graphicsToXml(self, parent):
        """ this is the overlay specific stuff to be embedded in the xml doc
            You may override this method and append your 
            specific stuff to *parent* 
        """
        pass

    

    # -------    XML TO WIDGET PARAMS   --------------    

    def _xmlToData(self, event):
        self.__widgetId = event.widgetId
        self.lastTag = ""
        
        def __onElement(name,attrs):
            self.lastTag = name
            if name == "widget":
                #if "winID" in attrs:     self.appId = int(attrs["winID"])  # XML FIELD NOT USED ANYMORE
                if "isGlobal" in attrs:  self.__isGlobal = bool(int(attrs["isGlobal"]))
                if "delayDraw" in attrs:  self.__delayDraw = bool(int(attrs["delayDraw"]))
                if "isVisible" in attrs:  self.visible = bool(int(attrs["isVisible"]))
                if "displayId" in attrs: self.displayId = int(attrs["displayId"])
                if "drawOrder" in attrs: self.__drawOrder = int(attrs["drawOrder"])
                if "parentID" in attrs:   self.__parentId = int(attrs["parentID"])
                if "parentOrder" in attrs: self.__parentOrder = int(attrs["parentOrder"])
            elif name == "w":  self.size.wType = int(attrs["type"])
            elif name == "h":  self.size.hType = int(attrs["type"])
            elif name == "x":  
                self.pos.xType = int(attrs["type"])
                if "margin" in attrs:  self.pos.xMargin = int(attrs["margin"])
            elif name == "y":  
                self.pos.yType = int(attrs["type"])
                if "margin" in attrs:  self.pos.yMargin = int(attrs["margin"])
            elif name == "z":
                if "offset" in attrs:  self._zOffset = float(attrs["offset"])
            elif name == "label":
                if "relativeFontSize" in attrs: self.__relativeFontSize = float(attrs["relativeFontSize"])
                if "fontSize" in attrs: self._ftSize = int(attrs["fontSize"])
                if "labelAlignment" in attrs: self.__labelAlignment = int(attrs["labelAlignment"])
            elif name == "tooltip":
                if "relativeTooltipSize" in attrs: self.__relativeTooltipSize = float(attrs["relativeTooltipSize"])
                if "fontSize" in attrs: self.__ttSize = int(attrs["fontSize"])
            elif name != "common" or name != "graphics":
                if name not in self.xmlElements:
                    self.xmlElements[name] = []
                el = XMLElement(name)  # store specific tags for later use
                el.attrs = attrs
                self.xmlElements[name].append(el)

        def __onValue(data):
            if self.lastTag == "w":
                if self.size.wType == NORMALIZED: self.size.w = float(data)
                else:                             self.size.w = int(data)
            elif self.lastTag == "h":
                if self.size.hType == NORMALIZED: self.size.h = float(data)
                else:                             self.size.h = int(data)
            elif self.lastTag == "x":
                if self.pos.xType == NORMALIZED:  self.pos.x = float(data)
                else:                             self.pos.x = int(data)
            elif self.lastTag == "y":
                if self.pos.yType == NORMALIZED:  self.pos.y = float(data)
                else:                             self.pos.y = int(data)
            elif self.lastTag == "z":
                zType = int(data)
                if zType == 1:  # tied to APP_Z
                    self.z = self.app.getZvalue() + self._zOffset
                elif zType == 0:
                    self.z = TOP_Z
                elif zType == 2:
                    self.z = BOTTOM_Z
            elif self.lastTag == "scale":
                self._scale = float(data)
            elif self.lastTag == "event":
                self.widgetEvents.append(int(data))
            elif self.lastTag == "label":
                self.__label = data
            elif self.lastTag == "tooltip":
                self.__tooltip = data
                
            elif self.lastTag != "common" or self.lastTag != "graphics": 
                if self.lastTag in self.xmlElements:   # store specific tags for later use
                    el = self.xmlElements[self.lastTag].pop()  # remove it from the list
                    el.value = data   # set its data
                    self.xmlElements[self.lastTag].append(el)  # add it again
                    
        

        # parse the incoming xmlString
        p = xml.parsers.expat.ParserCreate()
        p.StartElementHandler = __onElement
        p.CharacterDataHandler = __onValue
        p.Parse(event.xml, True)

        # allow the widget object to use the specific info
        self._onSpecificInfo()

        self._updateBounds()



    def _onSpecificInfo(self):
        """ specific info was parsed... use it here. 
            To use it, override this method and access self.xmlElements where
            all the specific xml tags are stored
        """
        pass


    def _addGraphicsElement(self, parent, name, value):
        if value != "":
            el = self.doc.createElement(name)
            el.setAttribute("format", str(FILE_FMT))
            el.appendChild(self.doc.createTextNode(value))
            parent.appendChild(el)


    def isInitialized(self):
        """ initialized means that it's shown on the display and in use """
        if hasattr(self, "overlayId"):
            return self.overlayId > -1
        else:
            return False


    def isAdded(self):
        """ true if the widget has been added or its in the add queue """
        return self._added


    # -------    WIDGET PARAMETERS   -----------------    
    
    def getBounds(self):
        """ in sage coords """
        return self.__bounds


    def getWidth(self):
        """ in sage coords """
        return self.__bounds.getWidth()


    def getHeight(self):
        """ in sage coords """
        return self.__bounds.getHeight()


    def getX(self):
        """ in sage coords """
        return self.__bounds.getX()


    def _setX(self, x):
        self.__bounds.setX(x)


    def getY(self):
        """ in sage coords """
        return self.__bounds.getY()


    def _setY(self, y):
        self.__bounds.setY(y)


    def setFontColor(self, r, g, b):
        """ in 0-255 range """
        self.__fontColor = Color(r,g,b)
    
        
    def setTransparency(self, a):
        """ in 0-255 range """
        self.__alpha = a


    def setBackgroundColor(self, r, g, b):
        """ in 0-255 range """
        self.__backgroundColor = Color(r,g,b)


    def getLabel(self):
        return self.__label
    

    def setLabel(self, label, fontSize=NORMAL):
        self.__label = label
        if fontSize != NORMAL:  # don't change the font size if not specified
            self._ftSize = fontSize

        if self._isAppWidget():
            self.__relativeFontSize = float(self._ftSize) / float(self.app.getHeight())
        self.__fitAroundLabel()

        # if this widget has been initialized, just send a message to change the
        # label to the display node
        if self.isInitialized():
            self.sendOverlayMessage(SET_LABEL, self._ftSize, self.__label)
            self._refresh()


    def setTooltip(self, tooltip, fontSize=NORMAL):
        self.__tooltip = tooltip
        if fontSize != NORMAL:  # don't change the font size if not specified
            self.__ttSize = fontSize

        if self._isAppWidget():
            self.__relativeTooltipSize = float(self.__ttSize) / float(self.app.getHeight())


    def getTooltip(self):
        return self.__tooltip


    def select(self, doSelect):
        self.__selected = doSelect
        if hasattr(self, "sendOverlayMessage"):
            self.sendOverlayMessage(SELECT_OVERLAY, int(doSelect))

    def isSelected(self):
        return self.__selected
    

    def toggleSelect(self):
        self.select(not self.__selected)

            
    def _isAppWidget(self):
        """ returns true if this widget was created by the app """
        return self.__widgetId > -1


    def getWindowId(self):
        if self.app:
            return self.app.getId()
        else:
            return -1


    def getWidgetId(self):
        return self.__widgetId


    def getFinalHeight(self):
        if self.size.hType == ABSOLUTE:
            return int(self.size.h * self._mult * getGlobalScale())
        else:  # this should raise exception in the caller...
            print "Returning None for height since we tried to calculate NORMALIZED final size... only ABSOLUTE works"
            return None

        
    def getFinalWidth(self):
        if self.size.wType == ABSOLUTE:
            return int(self.size.w * self._mult * getGlobalScale())
        else:  # this should raise exception in the caller...
            print "Returning None for width since we tried to calculate NORMALIZED final size... only ABSOLUTE works"
            return None
        
    
    
    def _updateBounds(self):
        """ this updates bounds according to self.size and self.pos and parent position """

        # less typing
        pb = self._parentBounds

        # update children first
        self.__updateChildrenBounds()

        # update the font size first
        if self.__label and self._isAppWidget():
            self._ftSize = int(floor(pb.getHeight() * self.__relativeFontSize))

        # now figure out our size (either fitting around children or
        # based on the self.size restrictions set up at creation)
        if len(self.__children) > 0:
            w,h = self._fitAroundChildren()
            self.__bounds.setSize(w, h)
            
        elif self.__label and self.__labelAlignment == -1:
            w,h = self.__fitAroundLabel()
            self.__bounds.setSize(w,h)

        else:
            if self.size.wType == NORMALIZED:
                w = int(floor(pb.getWidth() * self.size.w) * self._mult) # * self._scale
            else:
                w = int(self.size.w * self._scale * self._mult)
            if self.size.hType == NORMALIZED:
                h = int(floor(pb.getHeight() * self.size.h) * self._mult)
            else:
                h = int(self.size.h * self._scale * self._mult)
            self.__bounds.setSize(w,h)
                            

        self._updateX(pb)
        self._updateY(pb)

        # now that we have our size, position the children with respect to us
        self._positionChildren()


    
    # calculate x position based on the parent bounds (pb)
    def _updateX(self, pb):
        w,h = self.getWidth(), self.getHeight()
        slideGap = -1

        if self.pos.xType == ALIGNED:
            xa = self.pos.x  # less typing

            if xa == CENTER:
                x = pb.left + int(floor(pb.getWidth()/2.0)) - int(floor(w/2.0))
            elif xa == LEFT_OUTSIDE:
                x = pb.left - w - self.pos.xMargin
            elif xa == LEFT:
                if self.__slideHidden:
                    x = pb.left - w + slideGap
                else:
                    x = pb.left + self.pos.xMargin
            elif xa == RIGHT:
                if self.__slideHidden:
                    x = pb.right - slideGap
                else:
                    x = pb.right - w - self.pos.xMargin
            elif xa == RIGHT_OUTSIDE:
                x = pb.right + self.pos.xMargin
            else:
                x = pb.left

        elif self.pos.xType == NORMALIZED:
            x = int(floor(pb.getWidth() * self.pos.x)) + pb.left

        else:
            x = self.pos.x
            
        self.__bounds.setX(x)

    
    # calculate y position based on the parent bounds (pb)
    def _updateY(self, pb):
        w,h = self.getWidth(), self.getHeight()
        slideGap = -1
        
        if self.pos.yType == ALIGNED:
            ya = self.pos.y  # less typing

            if ya == CENTER:
                y = pb.bottom + int(floor(pb.getHeight()/2.0)) - int(floor(h/2.0))
            elif ya == BOTTOM_OUTSIDE:
                y = pb.bottom - h - self.pos.yMargin
            elif ya == BOTTOM:
                if self.__slideHidden:
                    y = pb.bottom - h + slideGap
                else:
                    y = pb.bottom + self.pos.yMargin
            elif ya == TOP:
                if self.__slideHidden:
                    y = pb.top - slideGap
                else:
                    y = pb.top - h - self.pos.yMargin
            elif ya == TOP_OUTSIDE:
                y = pb.top + self.pos.yMargin
            else:
                y = pb.bottom

        elif self.pos.yType == NORMALIZED:
            y = int(floor(pb.getHeight() * self.pos.y)) + pb.bottom
        
        else:
            y = self.pos.y

        self.__bounds.setY(y)


    # -------    FOR  EVENTS   -----------------    


    def doesOverlap(self, l, r, t, b):
        ll,rr,tt,bb = self.__bounds.getAll()
        if rr < l or ll > r or tt < b or bb > t:
            return False
        else:
            return True
        

    def isIn(self, x, y):
        if self._roundEvt:
            b = self.__bounds
            cx = b.getX() + b.getWidth() / 2.0
            cy = b.getY() + b.getHeight() / 2.0
            d = int(sqrt( (x-cx)*(x-cx) + (y-cy)*(y-cy) ))
            return d <= int(b.getWidth()/2.0)
        else:
            return self.__bounds.isIn(x,y)


    def _onAppChanged(self, event):
        """ you can override this but make sure you call it first in your new method """
        if self.app == event.app:
            self.displayId = self.app.getDisplayId()

            # if this widget is associated with a parent, it will be updated
            # by the parent so don't do it here
            if not self._hasParent():
                self._parentBounds.setAll(self.app.getLeft(), self.app.getRight(),
                                       self.app.getTop(), self.app.getBottom())
                self._refresh()
                        

            
    def _onZChanged(self, event):
        """ you can override this but make sure you call it first in your new method """
        if not self._hasParent():
            self.z = self.app.getZvalue() + self._zOffset
            self._updateChildrenZ()


    def _sendAllRemoteSync(self):
        if self.app.isShared:
            for h in self.app.sharedHosts:
                rgate, rdata = getGate(h)
                remote_apps = rgate.getAppStatus()
                local_apps = self.sageGate.getAppStatus()
                local_params = local_apps[str(self.app.appId)][1].split()
                directory, local_filename = os.path.split( local_params[-1] )
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
                    if v.appName == self.app.appName:
                        if v.appId == winner:
                            print ("    Remote app to control>", v.sailID)
                            rgate.sendAppEvent(APP_SYNC, rid)

    def _sendOneRemoteSync(self,h):
        if self.app.isShared:
            rgate, rdata = getGate(h)
            remote_apps = rgate.getAppStatus()
            local_apps = self.sageGate.getAppStatus()
            local_params = local_apps[str(self.app.appId)][1].split()
            directory, local_filename = os.path.split( local_params[-1] )
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
                if v.appName == self.app.appName:
                    if v.appId == winner:
                        print ("    Remote app to control>", v.sailID)
                        rgate.sendAppEvent(APP_SYNC, rid)


    def _sendAppWidgetEvent(self, widgetEventId, *data):
        if self.app.isShared:
            for h in self.app.sharedHosts:
                rgate, rdata = getGate(h)
                remote_apps = rgate.getAppStatus()
                local_apps = self.sageGate.getAppStatus()
                local_params = local_apps[str(self.app.appId)][1].split()
                directory, local_filename = os.path.split( local_params[-1] )
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
                    if v.appName == self.app.appName:
                        if v.appId == winner:
                            print ("    Remote app to control>", v.sailID)
                            rgate.sendAppEvent(EVT_WIDGET, v.sailID, widgetEventId, self.getWidgetId(), *data)
                            #rgate.sendAppEvent(APP_QUIT, v.sailID)
                            #rgate.resizeWindow(v.windowId, int(rl), int(rr), int(rb), int(rt))
        # send the event anyway to the local application
        self.sageGate.sendAppEvent(EVT_WIDGET, self.app.getSailID(), 
                                    widgetEventId, self.getWidgetId(), *data)



    def registerForEvent(self, eventId, callback):
        self.__callbacks[eventId] = callback
        self.__evtMgr.register(eventId, callback)


    def _getCallback(self, eventId):
        if eventId in self.__callbacks:
            return self.__callbacks[eventId]
        else:
            return None


    def _destroy(self):
        """ may be overridden but you need to call this base class' method """
        self.__evtMgr.unregister(self.__callbacks, self)
        self.__callbacks.clear()
        
        # destroy the children as well
        for child in self.__children.values():
            child._destroy()
        self.__children.clear()

        # if the widget is a child to some parent, remove
        # it from that parent as well
        parentObj = getOverlayMgr().getParentObj(self)
        if parentObj: parentObj._removeChild(self)
        

    # can this object be dragged onto another object?
    def allowDrag(self, allowDrag):
        self.__allowsDrag = allowDrag

    def _doesAllowDrag(self):
        return self.__allowsDrag


    # can the object accept other objects being dropped onto it?
    def onDrop(self, callback):
        """ the callback must accept DropEvent object """
        self.__onDropCallback = callback
        self.registerForEvent(EVT_DROP, self.__onDropCallback)

        
    def _doesAllowDrop(self):
        return self.__onDropCallback != None
    

    # -------    PARENT-CHILD RELATIONSHIP   -----------------    
        

    def _getParentId(self):
        return self.__parentId


    def _setParentId(self, id):
        self.__parentId = id


    def _hasParent(self):
        return self.__parentId != -1

    
    def _getParentObj(self):
        return getOverlayMgr().getParentObj(self)


    def _getParentOrder(self):
        """ position of the overlay in the parent (0,1,2....)
            numbers increase left->right or top->down
            - makes most sense for sizers, panels
        """
        return self.__parentOrder


    def _setParentOrder(self, pos):
        self.__parentOrder = pos


    def _refresh(self):
        parentObj = getOverlayMgr().getParentObj(self)
        if parentObj:
            parentObj._refresh()  # parent will update us
        else:
            self._updateBounds()  # we are the top parent so we will update
            self._updateChildrenZ()

            # show the bounds that DIM thinks are correct (for debugging)
            if DEBUG:
                self._showDimBounds()


    def _showDimBounds(self):            
        if self.isInitialized():
            self.sendOverlayMessage(SHOW_DIM_BOUNDS, str(self.__bounds))

        for child in self.__children.values():
            child._showDimBounds()


    def _getChildren(self):
        return self.__children


    def _getNumChildren(self):
        return len(self.__children)


    def _getFirstChild(self):
        minKey = min(self.__children.keys())
        return self.__children[minKey]


    def _getLastChild(self):
        maxKey = max(self.__children.keys())
        return self.__children[maxKey]
    

    def _getNewParentOrder(self):
        parentOrder = 0
        if len(self.__children) > 0:
            parentOrder = max(self.__children)+1
        return parentOrder


  ##   def replaceChild(self, inChild, outChild):
##         """ replace outChild in a parent object with inChild
##             - outChild gets deleted
##         """
##         inChild._setParentOrder(outChild._getParentOrder())
##         inChild._setParentId(outChild._getParentId())
        
##         self.__children[inChild._getParentOrder()] = inChild
##         self._removeChild(outChild) # this will do a refresh

##         # if we are adding a child AFTER this parent has been added to SAGE,
##         # add the child directly
##         if self._added: 
##             getOverlayMgr().addWidget(inChild)
        

    def addChild(self, child):
        """ attach the widget to the parent """

        # if the child has already been added, do not add it again
        if child in self.__children.values():
            return

        child.parent = self

        # create a new parentOrder if this is not an app-created widget
        # otherwise the parentOrder has already been assigned
        if not child._isAppWidget():
            child._setParentOrder(self._getNewParentOrder())
            child._setParentId(self.getWidgetId())

        self.__children[child._getParentOrder()] = child
        self._refresh()

        # if we are adding a child AFTER this parent has been added to SAGE,
        # add the child directly
        if self._added: 
            getOverlayMgr().addWidget(child)


    def _addChildren(self):
        """ the parent (this widget) will add all its children to SAGE """
        addWidget = getOverlayMgr().addWidget
        for child in self.__children.values():
            addWidget(child)


    def _removeChild(self, child):
        """ removes the child from the local list of children 
            This is called automatically when a widget is deleted
        """
        if child in self.__children.values():
            del self.__children[child._getParentOrder()]
            self._refresh()


    def _isParentVisible(self):
        """ check if any of the ancestors are invisible """
        parent = getOverlayMgr().getOverlay(self.__parentId)
        if not self.visible:
            return False
        elif parent:
            return parent._isParentVisible()
        else:
            return True
        

    def _showChildren(self):
        """ recursively show all the children """
        self.visible = True
        for child in self.__children.values():
            child._showChildren()


    def _hideChildren(self):
        """ recursively hide all the children """
        self.visible = False
        for child in self.__children.values():
            child._hideChildren()
    

    def __updateChildrenBounds(self):
        # sort the children based on their order in the sizer, panel
        sortedChildren = self.__children.keys()
        sortedChildren.sort()

        # update the size of all children (set their parent bounds first)
        for childId in sortedChildren:
            child = self.__children[childId]
            child._parentBounds = copy.copy(self._parentBounds)
            child._updateBounds()


    def _updateChildrenZ(self):
        for child in self.__children.values():
            child.z = self.z + Z_CHILD_DIFF + child._tempZOffset
            child._updateChildrenZ()

        
    def _positionChildren(self):
        """ override this method if you want to position children 
            based on parent's position (a sizer or panel for example)
        """
        pass



    def _fitAroundChildren(self):
        """ override this method if you want to resize the parent 
            based on children's size
        """
        pass


    
