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
# Author: Allan Spale
#         Ratko Jagodic
#
############################################################################


import wx
import time
import wx.lib.newevent
import thread

# (AKS 2005-04-14) Modified a single line of wx.lib.plot to remove
# an obstructed call if the platform was Mac
#import plot 
from wx.lib import plot

import string
import random
import sys
import time
import os
from globals import *
from Mywx import *

# This creates a new Event class and a EVT binder function
(UpdateDataEvent, EVT_UPDATE_GRAPH) = wx.lib.newevent.NewEvent()







############################################################################

# it gets the data from the SAGE messages (sageUIdataInfo).
# this is the actual graph (lines and data)
# 

class PerformanceGraph:  
    def __init__( self, appName, stDataTitle, iArraySize, iPerfUpdateInterval ):  

        # CLASS CONSTANTS
        # self.__fMin, self.__fMax, self.__fAvg, self.__fCurrent
        self.I_MINIMUM = 0
        self.I_MAXIMUM = 1
        self.I_AVERAGE = 2
        self.I_CURRENT = 3

        # instance variables
        self.__fAvg = 0.0
        self.__fMax = 0.0
        self.__fMin = 0.0
        self.__fCurrent = 0.0
        self.__iArraySize = iArraySize
        self.__stDataTitle = stDataTitle
        self.__stLabel = ""
        self.__stAppName = appName

        # (AKS 2005-05-03) Performance update interval
        self.__iPerformanceUpdateInterval = iPerfUpdateInterval


        # create panel
        #self.__wxpGraphPanel = wx.Panel( self.__wxContainer, id=wx.ID_ANY,
        #                                 pos=(-1,-1), size=(-1,-1) )

        # setup data structure
        self.__naData = [ [0]*2 for i in range(self.__iArraySize) ]


        # (AKS 2004-10-26) Because data is in reverse order (most recent value
        # is index 0), it is necessary to reverse the x values in the array
        iTimeValue = -(self.__iArraySize-1) * self.__iPerformanceUpdateInterval
        for index in range( 0, self.__iArraySize ):
            # print self.__iArraySize - index - 1
            #RJ self.__naData[ self.__iArraySize - index - 1, 0 ] = iTimeValue
            self.__naData[ self.__iArraySize - index - 1][ 0 ] = iTimeValue
            
            iTimeValue = iTimeValue + self.__iPerformanceUpdateInterval
            
            #self.__naData[ index - self.__iArraySize, 0 ] = index


        # create graph
        self.__lines = plot.PolyLine(self.__naData, colour='red', width=3)

        self.__graph = plot.PlotGraphics( [self.__lines],"", "Time (s)",
                                            self.__stDataTitle)

#------------------------------------------------------------------------------        
    
    def update( self, naNewData ):
        # Because naData is sent as a reference, it is necessary to copy the
        # data from naNewData to naData to preserve these references

        # (AKS 2005-05-09)
        self.__fCurrent = float( naNewData[ 0 ] )
        
        fSum = 0.0

        # (AKS 2005-02-08) Max and min for a range
        self.__fAvg = 0.0
        self.__fMax = 0.0
        self.__fMin = float( sys.maxint )
      
        for index in range( 0, self.__iArraySize ):
            # (AKS 2005-05-09)
            item = float( naNewData[ index ] )

            #RJ self.__naData[ index, 1 ] = item
            self.__naData[ index ][ 1 ] = item
            fSum = fSum + item

            # Set maximum value
            if ( item > self.__fMax ):
                self.__fMax = item

            # Set minimum value
            elif ( item < self.__fMin ):
                self.__fMin = item

        # Calculate average
        #self.__fAvg = fSum / self.__iArraySize

        # (AKS 2005-05-09)
        self.__fAvg = float( fSum / self.__iArraySize )
        

        # (AKS 2005-03-08) Update graph
        self.__lines = plot.PolyLine(self.__naData, colour='red', width=3)

        self.__graph = plot.PlotGraphics( [self.__lines],"", "Time (s)",
                                          self.__stDataTitle)
      

#------------------------------------------------------------------------------

    def setGraphType( self, stType ):
        if ( stType == 'frame rate' ):
            self.__stLabel = " fps"
        elif ( stType == 'bandwidth' ):
            self.__stLabel = " Mbps"
    
#------------------------------------------------------------------------------        
    
    def getGraph( self ):
        return self.__graph
    
#------------------------------------------------------------------------------

    def getStatistics( self ):
        return ( self.__fMin, self.__fMax, self.__fAvg, self.__fCurrent )

#------------------------------------------------------------------------------

    def getCurrentValue( self ):
        return self.__fCurrent

#------------------------------------------------------------------------------

    def getPolyLine( self ):
        return self.__lines








   
############################################################################

# (AKS 2005-04-26) Acts like PerformanceGraph (used with SimpleGraph) but supports
# multiple plots of the same metric (i.e. bandwidth, frame rate) although this is
# more of a usage rule rather than a code-enforced rule.  This class will contain
# an array of PerformanceGraph instances across applications.

class MultiPerformanceGraph:
    # NEED TO CHANGE listPerformanceGraphs to hashPerformanceGraphs to allow
    # for easy addition and deletion as apps come and go.
    def __init__( self, hashPerformanceGraphs, stDataTitle ):  

        # CLASS CONSTANTS
        # self.__fMin, self.__fMax, self.__fAvg, self.__fCurrent
        self.I_MINIMUM = 0
        self.I_MAXIMUM = 1
        self.I_AVERAGE = 2
        self.I_CURRENT = 3

        # instance variables
        self.__fAvg = 0.0
        self.__fMax = 0.0
        self.__fMin = 0.0
        self.__fCurrent = 0.0
        self.__stDataTitle = stDataTitle
        self.__stLabel = ""

        self.__listLines = []
        self.__hashPerformanceGraphs = {}

        for windowId in hashPerformanceGraphs:
            graph = hashPerformanceGraphs[ windowId ]
            self.__hashPerformanceGraphs[ windowId ] = graph
            self.__listLines.append( graph.getPolyLine() )

        # create graph
        self.__graph = plot.PlotGraphics( self.__listLines, "", "Time (s)",
                                            self.__stDataTitle)

#------------------------------------------------------------------------------        
    
    def update( self ):
        if ( len( self.__hashPerformanceGraphs ) > 0 ):
            # Because naData is sent as a reference, it is necessary to copy the
            # data from naNewData to naData to preserve these references

            # Get current data
            #print ">>> UPDATE WITH: ", naNewData
            #print "new data size = ", len( naNewData )
            #print "plottable data size = ", self.__naData.shape
            self.__fCurrent = naNewData[ 0 ]

            fSum = 0.0

            # (AKS 2005-02-08) Max and min for a range
            self.__fAvg = 0.0
            self.__fMax = 0.0
            self.__fMin = float( sys.maxint )

            for graph in self.__hashPerformanceGraphs.values():
                fMin, fMax, fAvg, fCurrent = graph.getStatistics()
                
                #print index
                fSum = fSum + fCurrent

                # Set maximum value
                if ( fMax > self.__fMax ):
                    self.__fMax = fMax

                    # Set minimum value
                elif ( fMin < self.__fMin ):
                    self.__fMin = fMin
                    
            # end loop


            # Calculate average
            self.__fAvg = fSum / self.__iArraySize


            # (AKS 2005-03-08) Update graph
            del self.__lines[ 0:len( self.__lines ) ]
        
            for windowId in self.__hashPerformanceGraphs:
                graph = self.__hashPerformanceGraphs[ windowId ]
                self.__listLines.append( graph.getPolyLine() )

                self.__graph = plot.PlotGraphics( self.__listLines,"", "Time",
                                                  self.__stDataTitle)
        # end

        else:
            self.__fAvg = 0.0
            self.__fMax = 0.0
            self.__fMin = 0.0
            self.__fCurrent = 0.0

        # end if

#------------------------------------------------------------------------------

    # (AKS 2005-04-29) Need this function because apps come and go.
    def addPerformanceGraph( self, windowId, pgGraph ):
        self.__hashPerformanceGraphs[ windowId ] = pgGraph

#------------------------------------------------------------------------------

    def removePerformanceGraph( self, windowId ):
        del self.__hashPerformanceGraphs[ windowId ]

#------------------------------------------------------------------------------

    def setGraphType( self, stType ):
        if ( stType == 'frame rate' ):
            self.__stLabel = " fps"
        elif ( stType == 'bandwidth' ):
            self.__stLabel = " Mbps"
    
#------------------------------------------------------------------------------        
    
    def getGraph( self ):
        return self.__graph
    
#------------------------------------------------------------------------------

    def getStatistics( self ):
        return ( self.__fCurrent, self.__fMin, self.__fMax, self.__fAvg )

#------------------------------------------------------------------------------









############################################################################

# the main panel that shows the totals for an application for all the
# different metrics.
# one of these exists for every application

class SparklineGraphs( wx.Panel ):
    def __init__( self, wxfParentFrame, list_pgGraph, list_stWindowTitle,
                  list_stGraphTitle, list_stYAxisTitle, stSingleWindowTitle, id, frame=None ):

        ### Setup element sizes
        btnSize = ( 89, 24 )

        ### initial spacing and positioning constants
        xOrigin = 5
        yOrigin = 5
        titleY = yOrigin
        titleX = xOrigin
        titleHeight = 25
        renderX = xOrigin
        renderY = titleY + titleHeight
        renderHeight = 40 - titleHeight
        btnX = xOrigin
        btnY = renderY + renderHeight
        btnHeight = btnSize[1]
        btnWidth = btnSize[0]
        labelXOffset = 15         #from the end of the buttons (or app label) to the right
        labelX = btnX + btnWidth + labelXOffset
        labelY = 0 #yOrigin
        labelWidth = 60
        labelHeight = titleHeight
        perfLabelX = labelX
        numArea1X = btnX + btnWidth
        numArea1Y = btnY
        numAreaWidth = 4*labelWidth + labelXOffset + 10 #numArea1X + 4*labelWidth
        numAreaHeight = 2*btnHeight
        numArea2X = numArea1X
        numArea2Y = numArea1Y + numAreaHeight + renderHeight

        # panel size
        width = numArea1X + numAreaWidth + xOrigin #415
        height = numArea2Y + numAreaHeight
        self._buffer = wx.EmptyBitmap(width, height)  # for double buffering

        self.frame = frame
        wx.Panel.__init__(self, wxfParentFrame, wx.ID_ANY, (3,3), (width,height))
        self.Bind(wx.EVT_CLOSE, self.shutdown)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.SetBackgroundColour(appPanelColor)
        # since SetBackgroundColour doesnt work always, manually draw the background
        #self.background = MyImage( "images/app_info_background.png", 0,0, False, width, height)
        
        # Font setup
        dc = wx.ClientDC(self)   # this makes font the same size on all platforms
        ppp = 72.0 / dc.GetPPI().height
        del dc
        self.boldFont = BoldFont()
        self.normalFont = StandardFont()

        # Order of items in the list must adhere to the following constants
        # INTERNAL CLASS CONSTANTS
        self.I_RENDER_BW = 0
        self.I_RENDER_FPS = 1
        self.I_RENDER_NODES = 2
        self.I_RENDER_STREAMS = 3        
        self.I_DISPLAY_BW = 4
        self.I_DISPLAY_FPS = 5
        self.I_DISPLAY_NODES = 6
        self.I_DISPLAY_STREAMS = 7        
        
        # Initialize instance variables
        self.__windowId = id   # (AKS 2005-03-10) Added for shutdown callback
        self.__list_stGraphTitle = list_stGraphTitle
        self.__list_stWindowTitle = list_stWindowTitle

        # FIX: delete the metrics we are not using this time... just use the bw and fps for rendering and display
        self.__list_pgGraph = list_pgGraph
        #self.__

        self.__list_stYAxisTitle = list_stYAxisTitle
        self.__stAppName = stSingleWindowTitle

        ### Header Row
        self.__colLabels = []
        self.__colLabels.append((self.__stAppName + " " + str(self.__windowId), xOrigin, 0, btnWidth, titleHeight))
        self.__colLabels.append(("Current", labelX, labelY, labelWidth, labelHeight))
        labelX = labelX + labelWidth
        self.__colLabels.append(("Min", labelX, labelY, labelWidth, labelHeight))
        labelX = labelX + labelWidth
        self.__colLabels.append(("Ave", labelX, labelY, labelWidth, labelHeight))
        labelX = labelX + labelWidth
        self.__colLabels.append(("Max", labelX, labelY, labelWidth, labelHeight))
        
        ### backgrounds for the two areas with the actual numbers
        self.__numAreaBg = []
        self.__numAreaBg.append(MyImage( "images/stats_green.jpg",
                                         numArea1X, numArea1Y, False,
                                         numAreaWidth, numAreaHeight))
        self.__numAreaBg.append(MyImage( "images/stats_green.jpg",
                                         numArea2X, numArea2Y, False,
                                         numAreaWidth, numAreaHeight))
        

        self.__divLabels = []
        self.__divLabels.append(("RENDER", renderX+185, renderY-2, btnWidth, renderHeight))
        self.__divLabels.append(("DISPLAY", renderX+185, btnY + btnHeight*2-2, btnWidth, renderHeight))
        
        # graphs and performance data
        self.__graphButtons = {}  #stores the graph buttons keyed by data type
        self.__hash_sgSimpleGraph = {}
        self.__perfNumLabels = {}  # stores the top-left corners of the first label and the size of each label

        # buttons for showing the graphs for each metric
        #self.__btnList = {0:"BW", 1:"FPS", 2:"Nodes", 3:"Streams", 4:"BW", 5:"FPS", 6:"Nodes", 7:"Streams"} 
        self.__btnList = {0:"BW", 1:"FPS", 4:"BW", 5:"FPS"} 
        

        # load the buttons and create the labels that will hold all the performance data
        for row in [0,1,4,5]: #range( 0, len( self.__btnList ) ):
            # Space between each row
            if row == 4:
                btnY = btnY + renderHeight
            if row == 0:
                pass  #we already set the first one
            else:
                btnY = btnY + btnHeight

            btnId =  row ##wx.ID_ANY  ## RJ 05-08-2005 - id by row
            btn = PerfToggleButton(self, btnId,
                                     self.__createGraph,
                                     self.removeGraph,
                                     "images/stats_"+self.__btnList[btnId]+"_up.jpg",
                                     "images/stats_"+self.__btnList[btnId]+"_down.jpg",
                                     "images/stats_"+self.__btnList[btnId]+"_over.jpg",
                                     ( btnX, btnY ), btnSize) 
            self.__graphButtons[btnId] = btn
            
            self.__perfNumLabels[row] = (perfLabelX, btnY, labelWidth, btnHeight)
        # end of loop

        self.__redraw()


    def Redraw(self):
        self.__redraw()

#------------------------------------------------------------------------------
                
    def registerShutdownCallback( self, function ):
        self.__shutdownCallback = function        
        
#------------------------------------------------------------------------------    

    def shutdown( self, evt=None ):
        self.__shutdownCallback( self.__windowId )

#------------------------------------------------------------------------------

    # (AKS 2005-03-17)  This function will be called if the user closed a "higher-
    # level" window that sends a "close event" down the window hierarchy.  In this
    # case, the higher-level window will be able to track what IDs to remove from
    # its data structures and will not need to receive any "callback" function
    # to indicate this information.

    def manualShutdown( self ):
        # shutdown all the associated SingleGraphs
        for iKey in self.__hash_sgSimpleGraph:
            self.__hash_sgSimpleGraph[ iKey ].manualShutdown()
        
        # (AKS 2005-03-17) Clear all keys after graphs are shutdown
        self.__hash_sgSimpleGraph.clear()
        self.Destroy()       # destroy panel


#------------------------------------------------------------------------------

    def getGraphList( self ):
        return self.__list_pgGraph

#------------------------------------------------------------------------------

    def removeGraph( self, iGraphID ):
        self.__hash_sgSimpleGraph[ iGraphID ].manualShutdown()
        del self.__hash_sgSimpleGraph[ iGraphID ]
        
        # Untoggle button
        self.__graphButtons[ iGraphID ].SetValue( False )

#------------------------------------------------------------------------------

    # Call in case no function was registered
    def __ignoreFunction( self ):
        pass

#------------------------------------------------------------------------------                

    def OnPaint(self, event):
	# for some reason double buffered panel doesnt show up right on macs so just 
	# do the normal drawing... single buffering
	if "__WXMAC__" in wx.PlatformInfo:
	    dc = wx.PaintDC(self)
	    self.__redraw(dc=dc)
	else:
	    dc = wx.BufferedPaintDC(self, self._buffer)
	

    def __redraw( self, evt=None, dc=None ):
        if not self.IsShown():  #only redraw the panel if it's shown
            return
	if "__WXMAC__" in wx.PlatformInfo:
	    if dc == None: dc = wx.ClientDC(self)
	else:
	    dc = wx.BufferedDC(wx.ClientDC(self), self._buffer)
	dc.SetBackground( wx.Brush(self.GetBackgroundColour()))
        dc.SetTextForeground(wx.WHITE)
	dc.Clear()

        # draw the column labels
        dc.SetFont(self.normalFont)
        i = 0
        for col in self.__colLabels:
            if i==0:
                dc.SetFont(self.boldFont)
                dc.DrawLabel(col[0], wx.Rect(col[1], col[2], col[3], col[4]), wx.ALIGN_TOP | wx.ALIGN_LEFT)
                dc.SetFont(self.normalFont)
            else: dc.DrawLabel(col[0], wx.Rect(col[1], col[2], col[3], col[4]), wx.ALIGN_TOP | wx.ALIGN_RIGHT)
            i=1

        # draw the "render" and "display" labels
        dc.SetTextForeground(wx.Colour(102, 204, 153))
        for div in self.__divLabels:
            dc.DrawLabel(div[0], wx.Rect(div[1], div[2], div[3], div[4]), wx.ALIGN_LEFT)
        dc.SetTextForeground(wx.WHITE)
                        
        # draw the number area backgrounds
        for im in self.__numAreaBg:
            dc.DrawBitmap( im.GetBitmap(), im.GetX(), im.GetY(), im.IsTransparent())

        # update the SparklineGraph panel
        for iIndex in [0,1,4,5]: #range( 0, 4): #len( self.__list_pgGraph ) ):
	    pgGraph = self.__list_pgGraph[ iIndex ]
            (x,y, w,h) = self.__perfNumLabels[iIndex]  # get the location and size of the first label in each row

            # Get statistics               
            tupleStats = pgGraph.getStatistics()

            # draw the labels for each metric
            for index in (pgGraph.I_CURRENT, pgGraph.I_MINIMUM, pgGraph.I_AVERAGE, pgGraph.I_MAXIMUM):
                if (iIndex == self.I_RENDER_BW) or (iIndex == self.I_RENDER_FPS) or (iIndex == self.I_DISPLAY_BW) or (iIndex == self.I_DISPLAY_FPS):
                    stFormatString = "%5.1f" % tupleStats[ index ]
                else:
                    stFormatString = "%5d" % tupleStats[ index ]
                dc.DrawLabel(stFormatString, wx.Rect(x,y,w,h), wx.ALIGN_CENTRE | wx.ALIGN_RIGHT)
                x+=w  # move the next label to the right by the width of the label

	# redraw the buttons on mac
        if "__WXMAC__" in wx.PlatformInfo:
            del dc
            for btn in self.__graphButtons.itervalues():
                btn.OnPaint()
        
        # now update all the SingleGraphs
        for k, singleGraph in self.__hash_sgSimpleGraph.iteritems():
            singleGraph.Redraw()
                



    ### RJ 05-08-2005 - this function replaces the 8 below
    def __OnToggleButton(self, event):
        button = event.GetEventObject()
        if button.GetValue():   #RJ 05-08-2005
            self.__createGraph( button.GetId() )
        else:
            self.removeGraph( button.GetId() )


        
#------------------------------------------------------------------------------

        
    def __createGraph( self, iGraphCode ):
        stRevisedWindowTitle = self.__stAppName + ": " + self.__list_stWindowTitle[ iGraphCode ]

        #if self.frame = None
        self.__hash_sgSimpleGraph[ iGraphCode ] = SingleGraph(
            self.frame, self.__list_pgGraph[ iGraphCode ], stRevisedWindowTitle,
            self.__list_stGraphTitle[ iGraphCode ],
            self.__list_stYAxisTitle[ iGraphCode ], iGraphCode )

        # registered call
        #print "........creating graph and registering shutdown callback for graph #", iGraphCode
        self.__hash_sgSimpleGraph[ iGraphCode ].registerShutdownCallback( self.removeGraph )
            
#------------------------------------------------------------------------------

    # (AKS 2005-05-03) Send a list of PerformanceGraphs related to a single app.
    # Then add each PerformanceGraph to the appropriate MultiPerformanceGraph
    # (one MultiPerformanceGraph for each performance metric).
    # CAUTION: Ordering of performance metrics is assumed to be identical
    # to the related graphs in listPerformanceGraphs

    def addAppPerformanceGraphList( self, windowId, listPerformanceGraphs,
                                    iPerformanceMetricCount ):

        # CAUTION: Only works for MultiPerformanceGraph instances
        for iIndex in range( 0, iPerformanceMetricCount ):
            self.__list_pgGraph[ iIndex ].addPerformanceGraph(
                windowId, listPerformanceGraphs[ index ] )
                
        # end loop        

#------------------------------------------------------------------------------

    # (AKS 2005-05-03) Remove a PerformanceGraph with the given windowId from each
    # MultiPerformanceGraph.
    # CAUTION: Ordering of performance metrics is assumed to be identical
    # to the related graphs in listPerformanceGraphs

    def reomveAppPerformanceGraphList( self, windowId, iPerformanceMetricCount ):
        print "Removing from totals sparkline APP ID #", windowId

        for iIndex in range( 0, iPerformanceMetricCount ):
            self.__list_pgGraph[ iIndex ].removePerformanceGraph( windowId )

        # end loop

#------------------------------------------------------------------------------

    # (AKS 2005-05-04) ONLY CALL WHEN USING MultiPerformanceGraphs in this class
    # so that totals can be calculated.

    def updateMultiPerformanceGraphs( self, iPerformanceMetricCount ):
        print "Updating totals sparkline..."        
        for iIndex in range( 0, iPerformanceMetricCount ):
            self.__list_pgGraph[ iIndex ].update()
        






############################################################################
# it displays only one metric as a graph in a frame
# it runs in a separate thread spawned by the SparklineGraph

class SingleGraph( wx.Frame ):
    def __init__( self, wxfParentFrame, pgGraph, stWindowTitle, stGraphTitle, stYAxisTitle, iGraphCode ):

        # Font setup
        self.normalFont = StandardFont()#wx.Font( 11, wx.DEFAULT, wx.NORMAL, wx.NORMAL )

        # Initialize instance variables
        self.__stGraphTitle = stGraphTitle
        self.__stWindowTitle = stWindowTitle
        self.__pgGraph = pgGraph
        self.__stYAxisTitle = stYAxisTitle
        self.__iGraphCode = iGraphCode

        # (AKS 2005-05-05) Pause playback flag
        self.__bPausePlayback = False

        # Setup window                
        wx.Frame.__init__(self, wxfParentFrame, wx.ID_ANY, stWindowTitle, wx.DefaultPosition, size=( 400, 300 ))
        wx.EVT_CLOSE(self, self.shutdown)
        
        self.CenterOnParent()

        # (AKS 2005-04-26) Use status bar instead
        self.CreateStatusBar()

        # (AKS 2005-05-05) Create menu bar to pause
        self.SetMenuBar( self.CreateMenu() )

        # Setup graph
        self.__wxpcPlotCanvas = plot.PlotCanvas( self )#, wx.ID_ANY, pos=( 0, 0 ), size=( 400,300 ) )
	self.__wxpcPlotCanvas.SetSize((400,300))
        # this is a workaround for what appears to be a bug in wx.lib.plot
        # On macs OnSize is not getting called therefore the buffer is not getting
        # created anywhere so we forcefully call it here
        #if wx.Platform == "__WXMAC__":
	#    print "calling on size..."
	#self.__wxpcPlotCanvas.OnSize(None)
            
        #self.__wxpcPlotCanvas.OnSize(wx.EVT_SIZE)
        self.__wxpcPlotCanvas.SetEnableZoom( True )
        self.__wxpcPlotCanvas.SetEnableGrid( True )

        self.Bind( EVT_UPDATE_GRAPH, self.__redraw )
        self.__redraw()
        self.Show( True )


    def Redraw(self):
        self.__redraw()

#------------------------------------------------------------------------------    

    def shutdown( self, evt=None ):
        # registered callback function to notify any listeners that graph
        # was destroyed
        self.__shutdownCallback( self.__iGraphCode )  
        
#------------------------------------------------------------------------------

    # (AKS 2005-03-17)  This function will be called if the user closed a "higher-
    # level" window that sends a "close event" down the window hierarchy.  In this
    # case, the higher-level window will be able to track what IDs to remove from
    # its data structures and will not need to receive any "callback" function
    # to indicate this information.
    
    def manualShutdown( self ):
        self.Destroy()
        
#------------------------------------------------------------------------------
    
    def registerShutdownCallback( self, function ):
        self.__shutdownCallback = function
        
#------------------------------------------------------------------------------

    def __redraw( self, evt=None ):
        #print "...redraw..."
        tupleStats = self.__pgGraph.getStatistics()

        if ( self.__bPausePlayback == False ):
        
            stStatusText = "Minimum: %4.2f   Average: %4.2f   Maximum: %4.2f   Current: %4.2f" % (
                tupleStats[ self.__pgGraph.I_MINIMUM ],
                tupleStats[ self.__pgGraph.I_AVERAGE ],
                tupleStats[ self.__pgGraph.I_MAXIMUM ],
                tupleStats[ self.__pgGraph.I_CURRENT ] )

            #self.__wxsInfoLine.SetLabel( stStatusText )

            fNewMax = tupleStats[ self.__pgGraph.I_MAXIMUM ] * 1.2
            # graphics are already stored in PerformanceGraph
	    self.__wxpcPlotCanvas.Draw( self.__pgGraph.getGraph(), yAxis=(0, fNewMax) ) #, xAxis="Time",
                                        #yAxis=self.__stYAxisTitle )
        else:
            stStatusText = "|| DRAWING PAUSED || Minimum: %4.2f   Average: %4.2f   Maximum: %4.2f   Current: %4.2f" % (
                tupleStats[ self.__pgGraph.I_MINIMUM ],
                tupleStats[ self.__pgGraph.I_AVERAGE ],
                tupleStats[ self.__pgGraph.I_MAXIMUM ],
                tupleStats[ self.__pgGraph.I_CURRENT ] )

        # end if

        # (AKS 2005-04-26) Use status bar instead
        self.SetStatusText( stStatusText )

#------------------------------------------------------------------------------

    def CreateMenu(self):
        menuBar = wx.MenuBar()

        # Playback
        menuFile = wx.Menu()
        self.miPlaybackPause = menuFile.Append(-1, "&Pause\tAlt-P", "", wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.OnPause, self.miPlaybackPause)
        menuBar.Append(menuFile, "&Playback")

        return menuBar

#------------------------------------------------------------------------------
    
    def OnPause( self, evt ):
        if ( self.miPlaybackPause.IsChecked() == True ):
            self.__bPausePlayback = True
        else:
            self.__bPausePlayback = False








    
###########################################################################

# it encapsulates all the performance data monitoring for all the apps
# it creates SparklineGraphs
# created only once during the execution of the program

class GraphManager:
    def __init__( self, wxfParentFrame, sageAppState ):

        # (AKS 2005-04-29) Put MultiPerformanceGraph instances in GraphManager.
        # There is one MultiPerformanceGraph for each metric listed in
        # self.__list_stGraphTitles.  Multiple applications will post their
        # PerformanceGraph PolyLine data into MultiPerformanceGraph.
        

        # UTILITY FUNCTIONS FOR GUI BUILDING...
        normalFont = StandardFont()#wx.Font( 11, wx.DEFAULT, wx.NORMAL, wx.NORMAL )

        self.__gmSageAppState = sageAppState
        self.__wxfParentFrame = wxfParentFrame
        self.__hashAppGraph = {}
        self.__hashAppGraphUpdateFlag = {}
        #self.__bCloseSparklines = False
        self.__updateList = []   #all the containers that need updating
        
        # (AKS 2005-05-03) Use the variable to store the performance update interval
        self.__iPerformanceUpdateInterval = 2

        self.__hashCallback = {}

        
        self.__list_stGraphTitles = [ "Rendering Bandwidth (Mbps)",
            "Rendering Frame Rate (Frames/Sec)", "Rendering Nodes", "Rendering Streams",
            "Display Bandwidth (Mbps)", "Display Frame Rate (Frames/Sec)", "Display Nodes",
            "Display Streams" ]

        self.__list_stYAxisTitles = [ "Mbps", "fps", "Nodes", "Streams",
                                      "Mbps", "fps", "Nodes", "Streams" ]

        self.__list_stOverallGraphTitles = [ "Total Rendering Bandwidth",
            "Rendering: Average Frames/Sec", "Total Rendering Streams",
            "Total Rendering Bandwidth", "Rendering: Average Frames/Sec",
            "Total Rendering Streams" ]

        self.__list_stOverallYAxisTitles = [ "Mbps", "fps", "Streams",
                                             "Mbps", "fps", "Streams" ]        


        # (AKS 2005-05-03) Create totals launcher for multiplot graphs
        self.__bTotalsLauncherExists = False


#------------------------------------------------------------------------------    

    # Responds to 40002...remember, all data comes at once
    def update( self, windowId ):
        # Send the update message
        if windowId in self.__hashAppGraph:

            # Get graphs from Sparklines
            sgAppSparklines = self.__hashAppGraph[ windowId ]
            list_pgGraphs = sgAppSparklines.getGraphList()

            #print "Get render bandwidth..."
            list_pgGraphs[ sgAppSparklines.I_RENDER_BW ].update(
                self.__gmSageAppState.getRenderItem( 'bandWidth', windowId, 30 ) )

            list_pgGraphs[ sgAppSparklines.I_RENDER_FPS ].update(
                self.__gmSageAppState.getRenderItem( 'frameRate', windowId, 30 ) )

            list_pgGraphs[ sgAppSparklines.I_RENDER_NODES ].update(
                self.__gmSageAppState.getRenderItem( 'nodes', windowId, 30 ) )

            list_pgGraphs[ sgAppSparklines.I_RENDER_STREAMS ].update(
                self.__gmSageAppState.getRenderItem( 'cpu', windowId, 30 ) )
            

            #print "Get display bandwidth..."        
            list_pgGraphs[ sgAppSparklines.I_DISPLAY_BW ].update(
                self.__gmSageAppState.getDisplayItem( 'bandWidth', windowId, 30 ) )

            list_pgGraphs[ sgAppSparklines.I_DISPLAY_FPS ].update(
                self.__gmSageAppState.getDisplayItem( 'frameRate', windowId, 30 ) )

            list_pgGraphs[ sgAppSparklines.I_DISPLAY_NODES ].update(
                self.__gmSageAppState.getDisplayItem( 'nodes', windowId, 30 ) )

            list_pgGraphs[ sgAppSparklines.I_DISPLAY_STREAMS ].update(
                self.__gmSageAppState.getDisplayItem( 'cpu', windowId, 30 ) )
            

            self.__hashAppGraphUpdateFlag[ windowId ] = True
            
            
        

        # send an update to all the windows
        for win in self.__updateList:    
            try:
                win.GetSize()
            except Exception:
                #print "exception occured... deleting the window"
                del win
            else:
                win.Redraw()  # call redraw on SparklineGraph
        

        # (AKS 2005-05-04) ALWAYS EXECUTE...Call update function for totals Sparkline;
        # instances of PerformanceGraphs in MultiPerformanceGRaphs in Sparkline
        # have already updated since they share the same reference address
        # (hopefully).  Now, just call a special update that will get the
        # totals Sparkline to calculate totals based on these updates.
        
        if ( self.__bTotalsLauncherExists == True ):
            self.__sgTotalsLauncher.updateMultiPerformanceGraphs( self.__iPerformanceMetricCount )

             
#------------------------------------------------------------------------------
    
    # Responds to 40000, 40001...OR JUST RIGHT-CLICK
    def addGraph( self, stAppName, windowId, parent=None ):  #RJ added "parent=None"
        if parent == None:
            parent = self.__wxfParentFrame
        list_pgGraphs = []

        for stGraphTitle in self.__list_stGraphTitles:
            list_pgGraphs.append( PerformanceGraph( 
                stAppName, stGraphTitle, 30, self.__iPerformanceUpdateInterval ) )

        
        # Create an application Sparkline graph launcher
        # (AKS 2005-05-03) Only create the SparklineGraph if it has not already been created

        if ( windowId not in self.__hashAppGraph ):
            
            slTotalsGraphLauncher = SparklineGraphs( parent,   #RJ - put parent instead of __wxf...
                list_pgGraphs, self.__list_stGraphTitles,
                self.__list_stGraphTitles, self.__list_stYAxisTitles, stAppName, windowId, self.__wxfParentFrame )
            

            # (AKS 2005-03-10) Register callback to destroy graph when the sparklines
            # window is closed.
            slTotalsGraphLauncher.registerShutdownCallback( self.removeGraph )

            # Update the hashes
            self.__hashAppGraph[ windowId ] = slTotalsGraphLauncher
            self.__hashAppGraphUpdateFlag[ windowId ] = False
            self.AddToUpdateList(slTotalsGraphLauncher)

            # (AKS 2005-05-04) Add newly created PerformanceGraphs to totals Sparkline
            if ( self.__bTotalsLauncherExists == True ):
                self.__sgTotalsLauncher.addAppPerformanceGraphList( windowId, list_pgGraphs,
                    self.__iPerformanceMetricCount )                                   
                
        # end...else do nothing


    # RJ 05-08-2005
    def GetSparklineGraph(self, windowId):
        if not windowId in self.__hashAppGraph:
            return -1
        else:
            return self.__hashAppGraph[ windowId ]


    def AddToUpdateList(self, win):
        if not win in self.__updateList:
            self.__updateList.append(win)

    def RemoveFromUpdateList(self, win):
        if win in self.__updateList:
            self.__updateList.remove(win)
        
    
#------------------------------------------------------------------------------

    # Responds to 40003...or nothing
    def removeGraph( self, windowId ):
        if windowId in self.__hashAppGraph:
            
            # (AKS 2005-05-04) Remove performance data related to departing application
            # from totals Sparkline
            if ( self.__bTotalsLauncherExists == True ):
                self.__sgTotalsLauncher.reomveAppPerformanceGraphList( windowId,
                    self.__iPerformanceMetricCount )
                
            
            sgAppSparklines = self.__hashAppGraph[ windowId ]
            self.RemoveFromUpdateList(sgAppSparklines)
            sgAppSparklines.manualShutdown()
            
            del self.__hashAppGraph[ windowId ]     # remove app id
            
        # else, do nothing          
            
#------------------------------------------------------------------------------

    def showTotalsGraph( self ):
        self.__bCloseSparklines = False

        list_pgGraphs = []

        # Create totals graphs
        for iIndex in range( 0,len( self.__list_stOverallGraphTitles ) ):
            list_pgGraphs.append( PerformanceGraph( "SAGE Totals",
                self.__list_stOverallGraphTitles[ iIndex ], 30 ) )

        # (AKS 2005-03-10) Need "bogus app id" for totals graph; it will be 88888888
        self.__slTotalsGraphLauncher = SparklineGraphs(
            self.__wxfParentFrame, list_pgGraphs, self.__list_stOverallGraphTitles,
            self.__list_stOverallGraphTitles, self.__list_stOverallYAxisTitles, 88888888 )

        self.__slTotalsGraphLauncher.registerCallback( self.onCloseTotalsGraph )

#------------------------------------------------------------------------------
    
    def onCloseTotalsGraph( self ):
        self.__slTotalsGraphLauncher = None
    
#------------------------------------------------------------------------------
        
    def shutdown( self ):
        # For each graph, shut down
        keys = self.__hashAppGraph.keys()
        for key in keys:
            self.__hashAppGraph[ key ].shutdown()

        # Makes sure to only call shutdown if the window still exists
        if ( self.__bTotalsLauncherExists == True ):
            self.__bTotalsLauncherExists = False
            self.__slTotalsGraphLauncher.shutdown()


#------------------------------------------------------------------------------
    # (AKS 2005-05-03) Get the performance data update interval
    def getPerformanceUpdateInterval( self ):
        return self.__iPerformanceUpdateInterval

#------------------------------------------------------------------------------
    # (AKS 2005-05-03) Create the totals performance graph

    def createTotalsLauncher( self ):
        # Create list of MultiPerformanceGraphs...one for each performance metric
        list_mpgMultiPerfGraphs = []
        
        for index in range( 0, len( self.__list_stGraphTitles ) ):
            list_mpgMultiPerfGraphs.append( MultiPerformanceGraph(
                {}, self.__list_stGraphTitles ) )

        # end loop
        
        # Create the "totals" SparklineGraphs object...need bogus app id of -1
        # CAREFUL: DIFFERENT TYPE...SENDING LIST OF MultiPerformanceGraph instances
        # and not PerformanceGraph instances!!!
        
        self.__sgTotalsLauncher = SparklineGraphs( self.__wxfParentFrame,
            list_mpgMultiPerfGraphs, self.__list_stGraphTitles,
            self.__list_stGraphTitles, self.__list_stYAxisTitles,
            "SAGE Application Totals", -1 )

        self.__iPerformanceMetricCount = len( self.__list_stGraphTitles )
        
        # Populate the MultiPerformanceGraphs from existing app SparklineGraphs
        for windowId in self.__hashAppGraph:
            sgAppLauncher = self.__hashAppGraph[ windowId ]
            list_pgGraphs = sgAppLauncher.getGraphList()

            self.__sgTotalsLauncher.addPerformanceGraphList( list_pgGraphs,
                self.__iPerformanceMetricCount )

        # end loop
        self.__bTotalsLauncherExists = True

#------------------------------------------------------------------------------
    # (AKS 2005-05-04) Shutdown the totals performance graph; since this
    # exists independent of the SparklineGraph objects, the totals launcher
    # can be destroyed independently and created independently at any time
    
    def shutdownTotalsLauncher( self ):
        if ( ( self.__sgTotalsGraphLauncher != None ) and
             ( self.__sgTotalsGraphLauncher.__class__.__name__ ==
                 SparklineGraphs.__name__ ) ):
            self.__sgTotalsGraphLauncher.shutdown()


    def getSparkline( self, windowId ):
        if ( windowId in self.__hashAppGraph ):
            return self.__hashAppGraph[ windowId ]



