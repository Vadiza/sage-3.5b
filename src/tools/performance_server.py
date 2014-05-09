#!/usr/bin/env python

import Numeric, random
import socket, sys, os
from wx.lib.plot import *

ARRAY_SIZE=100

class LineFrame(wx.Dialog):
    def _createObjects(self):
        print "create ", self.numgraphs, "graphs"
        self.current = {}
        self.datasets = {}
        for i in range(self.numgraphs):
            self.current[i] = 0
            self.datasets[i] = Numeric.arange(ARRAY_SIZE*2)/float(ARRAY_SIZE*2)
            self.datasets[i].shape = (ARRAY_SIZE, 2)
            self.datasets[i][:,1] = 0
        
    def _addPoint(self,values):
        x = float(values[0])
        for j in range(self.numgraphs):
            ar = [x, float(values[j+1])]
            if self.current[j] < ARRAY_SIZE:
                for i in range(self.current[j],ARRAY_SIZE):
                    self.datasets[j][i] = ar
                self.current[j] = self.current[j] + 1
            else:
                for i in range(1,ARRAY_SIZE):
                    self.datasets[j][i-1] = self.datasets[j][i]
                self.datasets[j][ARRAY_SIZE-1] = ar
        self.client.Draw(self._drawObjects())
        
    def _drawObjects(self):
        # ARRAY_SIZE points, plotted as red line
        lines = []
        for i in range(self.numgraphs):
            lines.append( PolyLine(self.datasets[i], legend=self.ytitle[i], width=3,
                                   colour=self.colors[i%6]) )
        return PlotGraphics(lines, self.title, self.xtitle, "")

    def __init__(self, parent, id, title):
        wx.Dialog.__init__(self, parent, id, title,
                          wx.DefaultPosition, (600, 400),style=wx.DEFAULT_FRAME_STYLE)
        self.frame = parent

        # Network connection
        self.conn = None
        
        # self.timer = wx.Timer(self)
        # self.timer.Start(10)
        # self.Bind(wx.EVT_TIMER, self.OnTimer)
        self.Bind(wx.EVT_IDLE, self.OnTimer)
        
        self.client = PlotCanvas(self)
        # Create mouse event for showing cursor coords in status bar
        self.client.Bind(wx.EVT_LEFT_DOWN, self.OnMouseLeftDown)
        self.Show(True)

        self.colors = ['red', 'blue', 'black', 'green', 'yellow', 'cyan', 'magenta' ]
        
        # settings
        self.title  = title
        self.xtitle = "X Title"
        self.ytitle = {}
        self.ytitle[0] = "Y Title"
        self.numgraphs = 1
        self.client.SetEnableLegend(0)
        self.client.SetEnableGrid(1)

        # draw
        self.resetDefaults()

    def SetClient(self, conn):
        self.conn = conn

    def SetNumGraphs(self, n):
        self.numgraphs = n
        self._createObjects()
    def SetTitle(self, t):
        self.title = t
    def SetXtitle(self, t):
        self.xtitle = t
    def SetYtitle(self, ind, t):
        self.ytitle[ind] = t
        self.numgraphs = ind+1

    def OnTimer(self, event):
        if self.conn:
            data = 1
            it = 0
            while data and (it<5):
                try:
                    msglen = 12*(self.numgraphs+1) + self.numgraphs
                    data = self.conn.recv(msglen)
                    if data:
                        values = data.split()
                        self._addPoint(values)
                        it = it + 1
                    else:
                        return
                except:
                    return
        self.client.Draw(self._drawObjects())
        #event.RequestMore(True)
        
    def OnMouseLeftDown(self,event):
        s= "Left Mouse Down at Point: (%.4f, %.4f)" % self.client.GetXY(event)
        self.frame.SetStatusText(s)
        event.Skip()

    def OnFilePageSetup(self, event):
        self.client.PageSetup()
        
    def OnFilePrintPreview(self, event):
        self.client.PrintPreview()
        
    def OnFilePrint(self, event):
        self.client.Printout()
        
    def OnSaveFile(self, event):
        self.client.SaveFile()

    def OnFileExit(self, event):
        self.Close()

    def OnPlotRedraw(self,event):
        self.client.Redraw()

    def OnPlotClear(self,event):
        self.client.Clear()
        
    def OnPlotScale(self, event):
        if self.client.last_draw != None:
            graphics, xAxis, yAxis= self.client.last_draw
            self.client.Draw(graphics,(1,3.05),(0,1))

    def OnEnableZoom(self, event):
        self.client.SetEnableZoom(event.IsChecked())
        
    def OnEnableGrid(self, event):
        self.client.SetEnableGrid(event.IsChecked())
        
    def OnEnableLegend(self, event):
        self.client.SetEnableLegend(event.IsChecked())

    def OnScrUp(self, event):
        self.client.ScrollUp(1)
        
    def OnScrRt(self,event):
        self.client.ScrollRight(2)

    def OnReset(self,event):
        self.client.Reset()

    def OnHelpAbout(self, event):
        from wx.lib.dialogs import ScrolledMessageDialog
        about = ScrolledMessageDialog(self, __doc__, "About...")
        about.ShowModal()

    def resetDefaults(self):
        """Just to reset the fonts back to the PlotCanvas defaults"""
        self.client.SetFont(wx.Font(12,wx.SWISS,wx.NORMAL,wx.NORMAL))
        self.client.SetFontSizeAxis(12)
        self.client.SetFontSizeLegend(12)
        self.client.SetXSpec('auto')
        self.client.SetYSpec('auto')
        

class MatrixCanvas(wx.Window):
    def __init__(self, parent, id = -1, pos=wx.DefaultPosition,
            size=wx.DefaultSize, style= wx.DEFAULT_FRAME_STYLE, name= ""):
        wx.Window.__init__(self, parent, id, pos, size, style, name)
        self.border = (1,1)

        self.SetBackgroundColour("black")
        
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        
        self.parent= parent

        self.last_draw = None

        # now on Mac, this seems to be a problem
        #self.OnSize(None) # sets the initial size based on client size

        
    def Draw(self, graphics):
        # allows using floats for certain functions 
        dc = wx.BufferedDC(wx.ClientDC(self), self._Buffer)
            
        dc.BeginDrawing()
        dc.Clear()

        Size  = self.GetClientSize()
        width, height, colors =  graphics
        stepw = (Size[0] - 60) / width
        steph = (Size[1] - 60) / height
        dc.SetPen(wx.Pen(wx.BLUE,1))
        k = 0
        for i in range(width):
            for j in range(height):
                dc.SetBrush(wx.Brush(colors[k]))
		#if os.uname()[0] == 'Darwin':
		#    dc.DrawRectangle( (30+i*stepw,30+j*steph), (stepw,steph))
		#else:
                dc.DrawRectangle( 30+i*stepw,30+j*steph, stepw,steph)
                k = k + 1
        self.last_draw = graphics

        dc.EndDrawing()
        
    def Redraw(self, dc= None):
        """Redraw the existing plot."""
        if self.last_draw:
            self.Draw(last_draw)

    def Clear(self):
        """Erase the window."""
        dc = wx.BufferedDC(wx.ClientDC(self), self._Buffer)
        dc.Clear()
        self.last_draw = None

    def OnPaint(self, event):
        # All that is needed here is to draw the buffer to screen
        dc = wx.BufferedPaintDC(self, self._Buffer)        
        
    def OnSize(self,event):
        # The Buffer init is done here, to make sure the buffer is always
        # the same size as the Window
        Size = self.GetClientSize()
        self._Buffer = wx.EmptyBitmap(Size[0],Size[1])
        if self.last_draw is None:
            self.Clear()
        else:
            self.Draw(self.last_draw)

        
class MatrixFrame(wx.Dialog):
    def _createObjects(self):
        print "create matrix"
        self.data = Numeric.arange(self.w*self.h)/3.0
        self.data.shape = (self.w,self.h)
        self.data[:] = 0.0

    def _drawObjects(self):        
        ar = []
        for i in range(self.w):
            for j in range(self.h):
                val = self.data[i][j]
                col = wx.Color( int(val*255.),
                                int(val*255.),
                                int(val*255.))
                ar.append( col )
        return (self.w, self.h, ar)

    def _addPoint(self,values):
        x   = int(values[0])
        y   = int(values[1])
        val = float(values[2])
        self.data[x][y] = val
        self.client.Draw(self._drawObjects())
        
    def __init__(self, parent, id, title, xs, ys):
        wx.Dialog.__init__(self, parent, id, title,
                          wx.DefaultPosition, (500, 400),style=wx.DEFAULT_FRAME_STYLE)
        self.frame = parent

        # Network connection
        self.conn = None

        # Drawing object
        self.client = MatrixCanvas(self)        
        
        self.Bind(wx.EVT_IDLE, self.OnTimer)
        self.Show(True)

        self.w = xs
        self.h = ys
        print "Matrix size", xs, ys

        self._createObjects()
        
        # settings
        self.title  = title
        self.xtitle = "X Title"
        self.ytitle = "Y Title"

    def SetClient(self, conn):
        self.conn = conn
    def SetTitle(self, t):
        self.title = t
    def SetXtitle(self, t):
        self.xtitle = t
    def SetYtitle(self, t):
        self.ytitle = t

    def OnTimer(self, event):
        if self.conn:
            data = 1
            it = 0
            while data and (it<5):
                try:
                    msglen = 6 + 1 + 6 + 1 + 12  # x y value
                    data = self.conn.recv(msglen)
                    if data:
                        values = data.split()
                        self._addPoint(values)
                        it = it + 1
                    else:
                        return
                except:
                    return
        self.client.Draw(self._drawObjects())
        #event.RequestMore(True)

class MyApp(wx.App):

    def OnIdle(self, event):
        try:
            conn, addr = self.s.accept()
            conn.setblocking(1)
            print 'Connected by', addr
            type = conn.recv(64)
            type = type.strip()
            print "\t type[", type, "]"
            if type == "lines":
                title = conn.recv(64)
                title = title.strip()
                print "\t title[", title, "]"
                xtitle = conn.recv(64)
                xtitle = xtitle.strip()
                print "\t xtitle[", xtitle, "]"
                numgraphs = int( conn.recv(6) )
                print "\t numgraphs", numgraphs
                ytitle = {}
                for i in range(numgraphs):
                    ytitle[i] = conn.recv(64)
                    ytitle[i] = ytitle[i].strip()
                    print "\t\t graph", i, " name[", ytitle[i], "]"
                frame = LineFrame(self.myframe, -1, title)
                frame.SetNumGraphs(numgraphs)
                frame.SetXtitle(xtitle)
                for i in range(numgraphs):
                    frame.SetYtitle(i, ytitle[i])
                frame.SetClient(conn)
                conn.send("1")
                conn.setblocking(0)
            if type == "matrix":
                title = conn.recv(64)
                title = title.strip()
                print "\t title[", title, "]"
                xtitle = conn.recv(64)
                xtitle = xtitle.strip()
                print "\t xtitle[", xtitle, "]"
                sizes = conn.recv(13)
                sizes = sizes.strip()
                xs,ys = sizes.split()
                xs = int(xs)
                ys = int(ys)
                numgraphs = 2
                print "\t numgraphs", numgraphs
                ytitle = {}
                for i in range(numgraphs):
                    ytitle[i] = conn.recv(64)
                    ytitle[i] = ytitle[i].strip()
                    print "\t\t graph", i, " name[", ytitle[i], "]"
                frame = MatrixFrame(self.myframe, -1, title, xs, ys)
                #frame.SetXtitle(xtitle)
                #for i in range(numgraphs):
                #    frame.SetYtitle(i, ytitle[i])
                frame.SetClient(conn)
                conn.send("1")
                conn.setblocking(0)
        except:
            pass
        event.RequestMore(True)

    def OnInit(self):
        wx.InitAllImageHandlers()
        self.myframe= wx.Frame(None, -1, "MainFrame")

        self.Bind(wx.EVT_IDLE, self.OnIdle)
        
        # Now Create the menu bar and items
        self.myframe.mainmenu = wx.MenuBar()

        menu = wx.Menu()
        menu.Append(200, 'Page Setup...', 'Setup the printer page')        
        menu.Append(201, 'Print Preview...', 'Show the current plot on page')
        menu.Append(202, 'Print...', 'Print the current plot')
        menu.Append(203, 'Save Plot...', 'Save current plot')
        menu.Append(205, 'E&xit', 'Enough of this already!')
        self.myframe.mainmenu.Append(menu, '&File')

        menu = wx.Menu()

        menu.Append(211, '&Redraw', 'Redraw plots')
        menu.Append(212, '&Clear', 'Clear canvas')
        menu.Append(213, '&Scale', 'Scale canvas')
        menu.Append(214, 'Enable &Zoom', 'Enable Mouse Zoom', kind=wx.ITEM_CHECK)
        menu.Append(215, 'Enable &Grid', 'Turn on Grid', kind=wx.ITEM_CHECK)
        menu.Append(220, 'Enable &Legend', 'Turn on Legend', kind=wx.ITEM_CHECK)
        menu.Append(225, 'Scroll Up 1', 'Move View Up 1 Unit')
        menu.Append(230, 'Scroll Rt 2', 'Move View Right 2 Units')
        menu.Append(235, '&Plot Reset', 'Reset to original plot')

        self.myframe.mainmenu.Append(menu, '&Plot')

        menu = wx.Menu()
        menu.Append(300, '&About', 'About this thing...')
        self.myframe.mainmenu.Append(menu, '&Help')

        self.myframe.SetMenuBar(self.myframe.mainmenu)

        # A status bar to tell people what's happening
        self.myframe.CreateStatusBar(1)
        self.myframe.Show(True)

        HOST = ''                 # Symbolic name meaning the local host
        PORT = 50008              # Arbitrary non-privileged port
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.bind((HOST, PORT))
        self.s.listen(5)
        self.s.setblocking(0)
                    
        self.SetTopWindow(self.myframe)
        
        return True

            
app = MyApp(0)
app.MainLoop()

