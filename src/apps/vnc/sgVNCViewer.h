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


#ifndef _AURA_H_sgVNCViewer
#define _AURA_H_sgVNCViewer

#include "vncviewer.h"


#define CMD_UPDATE 0xDE50001

#if defined(WIN32)
void InitWinsock();
#endif

// [M]ouse buttons
#define MK_NOTHING 0x0000
#define MK_LEFT    0x0001
#define MK_RIGHT   0x0002
#define MK_MIDDLE  0x0003


class sgVNCViewer
{
protected:
  int ox, oy;     // origin of the tile
  int sw, sh;     // size of the tile

  char *hostname;
  char *password;
  int displaynumber;
  int focus;
  int bstate;

  VNCViewer *VNC;
  uchar *screen_back;
  uchar *screen_front;

public:
  sgVNCViewer(char *host, int display, char *passwd, int& _sw, int& _sh);
  ~sgVNCViewer();

  virtual bool Step();
  uchar *Data();

  void CopyDataToScreen(char *buf, int x, int y, int width, int height);
  void RectDataToScreen(int x, int y, int w, int h, int ox, int oy);
  void FillToScreen(CARD32 pix, int x, int y, int width, int height);
  void SendKey(unsigned long key, int down);
  void SendMouse(int x, int y, int button);

  int CursorX() { return VNC->cursorx;};
  int CursorY() { return VNC->cursory;};
};

#endif
