/*****************************************************************************************
 * VNCViewer for SAGE
 * Copyright (C) 2004 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about VNCViewer for SAGE to www.evl.uic.edu/cavern/forum
 *****************************************************************************************/

#include "sail.h"
#include "sgVNCViewer.h"


static int ok = 0;


static int
NextPower(int val)
{
  int res;
  float tmp;

  res = 0;
  tmp = (float) val;
  while (tmp > 1.0)
  {
    tmp /= 2.0;
    res ++;
  }
  res = 1 << res;
  return res;
}


sgVNCViewer::sgVNCViewer(char *host, int display, char *passwd, int& _sw, int& _sh)
{
  char dest[256];
  VNC = new VNCViewer;

#if defined(WIN32)
  InitWinsock();
#endif

  hostname = strdup( host );
  password = strdup( passwd );
  displaynumber = display;
  sprintf(dest, "%s:%d", hostname, displaynumber);

  int argc = 2;
  char *argv[2];
  argv[0] = strdup ( "viewer" );   // program name
  argv[1] = strdup ( dest );       // display name

  // Connection to the server
  VNC->GetArgsAndResources(argc, argv);

  if (VNC->ConnectToRFBServer())
    SAGE_PRINTLOG("VNC> ConnectToRFBServer");
  else {
    SAGE_PRINTLOG("VNC> Connection to server failed");
    exit(0);
  }

  if (VNC->InitialiseRFBConnection(password))
    SAGE_PRINTLOG("VNC> InitialiseRFBConnection");
  else {
    SAGE_PRINTLOG("VNC> Wrong password");
    exit(0);
  }

  VNC->SetVisualAndCmap();
  VNC->SetFormatAndEncodings();
  SAGE_PRINTLOG("VNC> Framebuffer width %d , height %d",
                VNC->si.framebufferWidth, VNC->si.framebufferHeight);
  // fill the return values
  _sw = VNC->si.framebufferWidth;
  _sh = VNC->si.framebufferHeight;

  // Building the visualization
  ox = 0;
  oy = 0;
  // store width and height in the current object
  sw = _sw;
  sh = _sh;

  screen_back = (uchar* )malloc(sw * sh * 4);
  memset(screen_back, 0, sw * sh * 4);
  screen_front = (uchar* )malloc(sw * sh * 4);
  memset(screen_front, 0, sw * sh * 4);

  // First update from VNC server
  VNC->SendFramebufferUpdateRequest(ox, oy, sw, sh, False);

  focus = 0;    // Tracker inside, i.e. the mouse
  bstate = 0;   // button state

  // No update yet
  ok = 0;

  //Step();
}

sgVNCViewer::~sgVNCViewer()
{
  delete VNC;

}



void sgVNCViewer::SendKey(unsigned long key, int down)
{
  //SAGE_PRINTLOG("VNC> sending key %d (%d)", key, down);

  VNC->SendKeyEvent(key, down);
}

void sgVNCViewer::SendMouse(int x, int y, int button)
{
  int mask;

  switch (button)
  {
  case MK_LEFT:
    mask = 1;
    break;
  case MK_RIGHT:
    mask = 2;
    break;
  case MK_MIDDLE:
    mask = 4;
    break;
  default:
    mask = 0;
    break;
  }
  //SAGE_PRINTLOG("VNC> sending mouse %d", mask);

  VNC->SendPointerEvent(x, y, mask);
}


uchar*
sgVNCViewer::Data()
{
  return screen_front;
}

bool
sgVNCViewer::Step()
{
  if (VNC->HandleRFBServerMessage(this))
  {
    // Copy into main memory
    memcpy(screen_front, screen_back, sw*sh*4);

    VNC->SendFramebufferUpdateRequest(ox, oy, sw, sh, True);

    return true;
  }
  return false;

}

void
sgVNCViewer::CopyDataToScreen(char *buf, int x, int y, int width, int height)
{
  x -= ox;
  y -= oy;


#if defined(WIN32)
  if (VNC->rawDelay != 0) Sleep(VNC->rawDelay);
#else
  if (VNC->rawDelay != 0) usleep(VNC->rawDelay*1000);
#endif

  if (!VNC->useBGR233)
  {
    int h;
    uchar *src, *scr;
    scr = &screen_back[y*sw*4+x*4];
    src = (uchar*) buf;
    for (h = 0; h < height; h++)
    {
      memcpy(scr, src, width*4);
      src += width*4;
      scr += sw*4;
    }
  }
}

void
sgVNCViewer::FillToScreen(CARD32 pix, int x, int y, int width, int height)
{
  x -= ox;
  y -= oy;

#if defined(WIN32)
  if (VNC->rawDelay != 0) Sleep(VNC->rawDelay);
#else
  if (VNC->rawDelay != 0) usleep(VNC->rawDelay*1000);
#endif
  if (!VNC->useBGR233)
  {
    int h,i;
    uchar *src, *scr;

    scr = &screen_back[y*sw*4+x*4];
    src = (uchar*) pix;
    for (h = 0; h < height; h++)
    {
      //memcpy(scr, src, width*4);
      for (i = 0; i<width;i++)
        memcpy(&scr[i*4], &pix, 4);;

      src += width*4;
      scr += sw*4;
    }
  }
}

void
sgVNCViewer::RectDataToScreen(int x, int y, int w, int h, int orix, int oriy)
{

  x -= ox;
  y -= oy;
  orix -= ox;
  oriy -= oy;

  uchar *scr = &screen_back[y*sw*4+x*4];
  uchar *src = &screen_front[oriy*sw*4+orix*4];
  for (int _h = 0; _h < h; _h++)
  {
    memcpy(scr, src, w*4);
    src += sw*4;
    scr += sw*4;
  }
}
