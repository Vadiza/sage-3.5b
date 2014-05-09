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

#ifndef AURA_VNCVIEW_H
#define AURA_VNCVIEW_H

#define False 0
#define True 1
typedef char *String;

typedef unsigned char uchar;
typedef int Bool;

#if defined(WIN32)
//#include <Winsock2.h>
#else

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdarg.h>


#include "rfb.h"

#define MAX_ENCODINGS 10

#define FLASH_PORT_OFFSET 5400
#define SERVER_PORT_OFFSET 5900

// note that the CoRRE encoding uses this buffer and assumes it is big enough
// to hold 255 * 255 * 32 bits -> 260100 bytes.  640*480 = 307200 bytes
// also hextile assumes it is big enough to hold 16 * 16 * 32 bits
#define BUFFER_SIZE (640*480)

// socket buffer
#define BUF_SIZE 8192

class sgVNCViewer;

class VNCViewer
{
public:
  Bool shareDesktop;
  Bool viewOnly;
  Bool fullScreen;

  String encodingsString;

  Bool useBGR233;
  int nColours;
  Bool useSharedColours;
  Bool forceOwnCmap;
  Bool forceTrueColour;
  int requestedDepth;

  Bool useShm;

  int wmDecorationWidth;
  int wmDecorationHeight;

  char *passwordFile;
  Bool passwordDialog;

  int rawDelay;
  int copyRectDelay;

  Bool debug;

  int popupButtonCount;

  int bumpScrollTime;
  int bumpScrollPixels;

  int compressLevel;
  int qualityLevel;
  Bool useRemoteCursor;

  char vncServerHost[256];
  int  vncServerPort;

  int  rfbsock;
  Bool canUseCoRRE;
  Bool canUseHextile;
  char *desktopName;
  rfbPixelFormat myFormat;
  rfbServerInitMsg si;
  char *serverCutText;
  Bool newServerCutText;

  // cursor stuff
  CARD8 *rcSource;

  // socket buffering
  char buffer[BUFFER_SIZE];
  int buffered;
  char buf[BUF_SIZE];
  char *bufoutptr;
  int cursorx, cursory;

public:
  VNCViewer()
  {
    encodingsString = strdup("hextile corre rre raw copyrect");
    shareDesktop = True;
    viewOnly = False;
    fullScreen = False;
    passwordFile = strdup("passwd");
    passwordDialog = False;
    useBGR233 = False;
    nColours = 256;
    useSharedColours = True;
    forceOwnCmap = False;
    forceTrueColour = False;
    requestedDepth = 0;
    wmDecorationWidth = 4;
    wmDecorationHeight = 24;
    popupButtonCount = 0;
    debug = False;
    rawDelay = 0;
    copyRectDelay = 0;
    bumpScrollTime = 25;
    bumpScrollPixels = 20;
    compressLevel   = 4;
    qualityLevel    = 2;
    useRemoteCursor = True;

    vncServerPort = 0;
    serverCutText = NULL;
    newServerCutText = False;
    buffered = 0;
    bufoutptr = buf;
    cursorx = -1;
    cursory = -1;
  };
  ~VNCViewer()
  {};

  void GetArgsAndResources(int argc, char **argv);
  Bool ConnectToRFBServer();
  Bool InitialiseRFBConnection(char *pass);
  Bool SetFormatAndEncodings();
  Bool SendIncrementalFramebufferUpdateRequest();
  Bool SendFramebufferUpdateRequest(int x, int y, int w, int h,
                                    Bool incremental);
  Bool SendPointerEvent(int x, int y, int buttonMask);
  Bool SendKeyEvent(CARD32 key, Bool down);
  Bool SendClientCutText(char *str, int len);

  void PrintPixelFormat(rfbPixelFormat *format);
  void SetVisualAndCmap();

  Bool HandleCursorShape(int xhot, int yhot, int width, int height, CARD32 enc);
  Bool HandleCursorPos(int x, int y);

  Bool HandleCoRRE8  (sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleCoRRE16 (sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleCoRRE32 (sgVNCViewer *ct, int rx, int ry, int rw, int rh);

  Bool HandleHextile8  (sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleHextile16 (sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleHextile32 (sgVNCViewer *ct, int rx, int ry, int rw, int rh);

  Bool HandleRRE8  (sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleRRE16 (sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleRRE32 (sgVNCViewer *ct, int rx, int ry, int rw, int rh);

  Bool HandleZlib8(sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleZlib16(sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleZlib32(sgVNCViewer *ct, int rx, int ry, int rw, int rh);

  Bool HandleTight8(sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleTight16(sgVNCViewer *ct, int rx, int ry, int rw, int rh);
  Bool HandleTight32(sgVNCViewer *ct, int rx, int ry, int rw, int rh);


  void FilterGradient24 (int numRows, CARD32 *dst);

  int  InitFilterCopy8 (int rw, int rh);
  void FilterCopy8 (int numRows, CARD8 *dst);
  int  InitFilterGradient8 (int rw, int rh);
  void FilterGradient8 (int numRows, CARD8 *dst);
  int  InitFilterPalette8 (int rw, int rh);
  void FilterPalette8 (int numRows, CARD8 *dst);

  int  InitFilterCopy16 (int rw, int rh);
  void FilterCopy16 (int numRows, CARD16 *dst);
  int  InitFilterGradient16 (int rw, int rh);
  void FilterGradient16 (int numRows, CARD16 *dst);
  int  InitFilterPalette16 (int rw, int rh);
  void FilterPalette16 (int numRows, CARD16 *dst);

  int  InitFilterCopy32 (int rw, int rh);
  void FilterCopy32 (int numRows, CARD32 *dst);
  int  InitFilterGradient32 (int rw, int rh);
  void FilterGradient32 (int numRows, CARD32 *dst);
  int  InitFilterPalette32 (int rw, int rh);
  void FilterPalette32 (int numRows, CARD32 *dst);

  long ReadCompactLen (void);
  Bool DecompressJpegRect8(sgVNCViewer *ct, int x, int y, int w, int h);
  Bool DecompressJpegRect16(sgVNCViewer *ct, int x, int y, int w, int h);
  Bool DecompressJpegRect32(sgVNCViewer *ct, int x, int y, int w, int h);

  Bool ReadFromRFBServer(char *out, unsigned int n);
  Bool HandleRFBServerMessage(sgVNCViewer *ct);

};



/* sockets.c */

extern Bool WriteExact(int sock, char *buf, int n);
extern int  ConnectToTcpAddr(unsigned int host, int port);
extern Bool SetNonBlocking(int sock);

extern int  StringToIPAddr(const char *str, unsigned long *addr);
extern Bool SameMachine(int sock);

#endif
