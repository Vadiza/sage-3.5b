/************************************************************************
 *
 *  Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */


/*
 * rfbproto.c - functions to deal with client side of RFB protocol.
 */

#if defined(WIN32)
#include <io.h>
#include <Winsock2.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "sail.h"
#include "sgVNCViewer.h"
#include "vncauth.h"
// RJ added for decrypting the password received from SAGE UI
extern void decryptUIpass(unsigned char *outpass, unsigned char *inpass, unsigned char *key);
extern unsigned char fixedkey[8]; // = {23,82,107,6,35,78,88,7};

#if defined(WIN32)
extern "C"
{
#define XMD_H
#include "jpeglib.h"
}
#define strncasecmp strnicmp
#define strcasecmp stricmp
#include <zlib.h>
#else

#include <zlib.h>

extern "C" {
#include <jpeglib.h>
}

#endif


#define TRUE 1

/* The zlib encoding requires expansion/decompression/deflation of the
   compressed data in the "buffer" above into another, result buffer.
   However, the size of the result buffer can be determined precisely
   based on the bitsPerPixel, height and width of the rectangle.  We
   allocate this buffer one time to be the full size of the buffer. */

static int raw_buffer_size = -1;
static char *raw_buffer;

static z_stream decompStream;
static Bool decompStreamInited = False;


/*
 * Variables for the ``tight'' encoding implementation.
 */

/* Separate buffer for compressed data. */
#define ZLIB_BUFFER_SIZE 512
static char zlib_buffer[ZLIB_BUFFER_SIZE];

/* Four independent compression streams for zlib library. */
static z_stream zlibStream[4];
static Bool zlibStreamActive[4] = {
  False, False, False, False
};

/* Filter stuff. Should be initialized by filter initialization code. */
static Bool cutZeros;
static int rectWidth, rectColors;
static char tightPalette[256*4];
static CARD8 tightPrevRow[2048*3*sizeof(CARD16)];

/* JPEG decoder state. */
static Bool jpegError;

static void JpegInitSource(j_decompress_ptr cinfo);
static boolean JpegFillInputBuffer(j_decompress_ptr cinfo);
static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes);
static void JpegTermSource(j_decompress_ptr cinfo);
static void JpegSetSrcManager(j_decompress_ptr cinfo, CARD8 *compressedData,
                              int compressedLen);

/*
 * ConnectToRFBServer.
 */

Bool
VNCViewer::ConnectToRFBServer()
{
  unsigned long host;

  if (!StringToIPAddr(vncServerHost, &host)) {
    SAGE_PRINTLOG("VNC> Couldn't convert '%s' to host address", vncServerHost);
    return False;
  }

  rfbsock = ConnectToTcpAddr(host, vncServerPort);

  if (rfbsock < 0) {
    SAGE_PRINTLOG("VNC> Unable to connect to VNC server");
    return False;
  }
  // return SetNonBlocking(rfbsock); // does not work
  return TRUE;
}


/*
 * InitialiseRFBConnection.
 */

Bool
VNCViewer::InitialiseRFBConnection(char *pass)
{
  char *desktopName;
  rfbProtocolVersionMsg pv;
  int major,minor;
  CARD32 authScheme, reasonLen, authResult;
  char *reason;
  CARD8 challenge[CHALLENGESIZE];
  char passwd[8];
  int i;
  rfbClientInitMsg ci;

  /* if the connection is immediately closed, don't report anything, so
     that pmw's monitor can make test connections */

  if (!ReadFromRFBServer(pv, sz_rfbProtocolVersionMsg)) return False;

  pv[sz_rfbProtocolVersionMsg] = 0;

  if (sscanf(pv,rfbProtocolVersionFormat,&major,&minor) != 2) {
    SAGE_PRINTLOG("VNC> Not a valid VNC server");
    return False;
  }

  SAGE_PRINTLOG("VNC> VNC server supports protocol version %d.%d (viewer %d.%d)",
                major, minor, rfbProtocolMajorVersion, rfbProtocolMinorVersion);

  major = rfbProtocolMajorVersion;
  minor = rfbProtocolMinorVersion;

  sprintf(pv,rfbProtocolVersionFormat,major,minor);

  if (!WriteExact(rfbsock, pv, sz_rfbProtocolVersionMsg)) return False;

  if (!ReadFromRFBServer((char *)&authScheme, 4)) return False;

  authScheme = Swap32IfLE(authScheme);

  switch (authScheme) {

  case rfbConnFailed:
    if (!ReadFromRFBServer((char *)&reasonLen, 4)) return False;
    reasonLen = Swap32IfLE(reasonLen);

    reason = (char*) malloc(reasonLen);

    if (!ReadFromRFBServer(reason, reasonLen)) return False;

    SAGE_PRINTLOG("VNC> VNC connection failed: %.*s",(int)reasonLen, reason);
    return False;

  case rfbNoAuth:
    SAGE_PRINTLOG("VNC> No authentication needed");
    break;

  case rfbVncAuth:
    if (!ReadFromRFBServer((char *)challenge, CHALLENGESIZE)) return False;
    if (passwordFile) {
      strcpy(passwd, vncDecryptPasswdFromFile(passwordFile));
      if (!passwd) {
        SAGE_PRINTLOG("VNC> Cannot read valid password from file \"%s\"",
                      passwordFile);
        return False;
      }
    } else
    {
      // sageui will send the password as a DES encrypted string so decrypt it here
      //decryptUIpass((unsigned char*)passwd, (unsigned char*)pass, fixedkey);
      strcpy(passwd, pass);
    }

    if ((!passwd) || (strlen(passwd) == 0)) {
      SAGE_PRINTLOG("VNC> Reading password failed");
      return False;
    }
    if (strlen(passwd) > 8) {
      passwd[8] = '\0';
    }

    vncEncryptBytes(challenge, passwd);
    /* Lose the password from memory */
    for (i = strlen(passwd); i >= 0; i--) {
      passwd[i] = '\0';
    }

    if (!WriteExact(rfbsock, (char *)challenge, CHALLENGESIZE)) return False;

    if (!ReadFromRFBServer((char *)&authResult, 4)) return False;

    authResult = Swap32IfLE(authResult);

    switch (authResult) {
    case rfbVncAuthOK:
      SAGE_PRINTLOG("VNC> VNC authentication succeeded");
      break;
    case rfbVncAuthFailed:
      SAGE_PRINTLOG("VNC> VNC authentication failed");
      return False;
    case rfbVncAuthTooMany:
      SAGE_PRINTLOG("VNC> VNC authentication failed - too many tries");
      return False;
    default:
      SAGE_PRINTLOG("VNC> Unknown VNC authentication result: %d",
                    (int)authResult);
      return False;
    }
    break;

  default:
    SAGE_PRINTLOG("VNC> Unknown authentication scheme from VNC server: %d",
                  (int)authScheme);
    return False;
  }

  ci.shared = (shareDesktop ? 1 : 0);

  if (!WriteExact(rfbsock, (char *)&ci, sz_rfbClientInitMsg)) return False;

  if (!ReadFromRFBServer((char *)&si, sz_rfbServerInitMsg)) return False;

  si.framebufferWidth = Swap16IfLE(si.framebufferWidth);
  si.framebufferHeight = Swap16IfLE(si.framebufferHeight);
  si.format.redMax = Swap16IfLE(si.format.redMax);
  si.format.greenMax = Swap16IfLE(si.format.greenMax);
  si.format.blueMax = Swap16IfLE(si.format.blueMax);
  si.nameLength = Swap32IfLE(si.nameLength);

  desktopName = (char*) malloc(si.nameLength + 1);

  if (!ReadFromRFBServer(desktopName, si.nameLength)) return False;

  desktopName[si.nameLength] = 0;

  SAGE_PRINTLOG("VNC> Desktop name \"%s\"",desktopName);
  SAGE_PRINTLOG("VNC> Connected to VNC server, using protocol version %d.%d",
                rfbProtocolMajorVersion, rfbProtocolMinorVersion);
  SAGE_PRINTLOG("VNC> VNC server default format:");

  PrintPixelFormat(&si.format);

  return True;
}


/*
 * SetFormatAndEncodings.
 */

Bool
VNCViewer::SetFormatAndEncodings()
{
  rfbSetPixelFormatMsg spf;
  char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];
  rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
  CARD32 *encs = (CARD32 *)(&buf[sz_rfbSetEncodingsMsg]);
  int len = 0;
  Bool requestCompressLevel = False;
  Bool requestQualityLevel = False;
  Bool requestLastRectEncoding = False;

  spf.type            = rfbSetPixelFormat;
  spf.format          = myFormat;
  spf.format.redMax   = Swap16IfLE(spf.format.redMax);
  spf.format.greenMax = Swap16IfLE(spf.format.greenMax);
  spf.format.blueMax  = Swap16IfLE(spf.format.blueMax);

  if (!WriteExact(rfbsock, (char *)&spf, sz_rfbSetPixelFormatMsg))
    return False;

  se->type = rfbSetEncodings;
  se->nEncodings = 0;

  if (encodingsString)
  {
    char *encStr = encodingsString;
    int encStrLen;
    do {
      char *nextEncStr = strchr(encStr, ' ');
      if (nextEncStr)
      {
        encStrLen = nextEncStr - encStr;
        nextEncStr++;
      } else
      {
        encStrLen = strlen(encStr);
      }

      if (strncmp(encStr,"raw",encStrLen) == 0)
      {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRaw);
      } else if (strncmp(encStr,"copyrect",encStrLen) == 0)
      {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingCopyRect);
      } else if (strncasecmp(encStr,"tight",encStrLen) == 0)
      {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingTight);
        requestLastRectEncoding = True;
        if (compressLevel >= 0 && compressLevel <= 9)
          requestCompressLevel = True;
        if (qualityLevel >= 0 && qualityLevel <= 9)
          requestQualityLevel = True;
      } else if (strncmp(encStr,"hextile",encStrLen) == 0)
      {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingHextile);
      } else if (strncasecmp(encStr,"zlib",encStrLen) == 0) {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingZlib);
        if (compressLevel >= 0 && compressLevel <= 9)
          requestCompressLevel = True;
      } else if (strncmp(encStr,"corre",encStrLen) == 0)
      {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingCoRRE);
      } else if (strncmp(encStr,"rre",encStrLen) == 0)
      {
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRRE);
      } else
      {
        SAGE_PRINTLOG("VNC> Unknown encoding '%.*s'",encStrLen,encStr);
      }

      encStr = nextEncStr;
    } while (encStr && se->nEncodings < MAX_ENCODINGS);

    if (se->nEncodings < MAX_ENCODINGS && requestCompressLevel)
    {
      encs[se->nEncodings++] = Swap32IfLE(compressLevel +
                                          rfbEncodingCompressLevel0);
    }

    if (se->nEncodings < MAX_ENCODINGS && requestQualityLevel)
    {
      encs[se->nEncodings++] = Swap32IfLE(qualityLevel +
                                          rfbEncodingQualityLevel0);
    }

    if (useRemoteCursor)
    {
      if (se->nEncodings < MAX_ENCODINGS)
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingXCursor);
      if (se->nEncodings < MAX_ENCODINGS)
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRichCursor);
      if (se->nEncodings < MAX_ENCODINGS)
        encs[se->nEncodings++] = Swap32IfLE(rfbEncodingPointerPos);
    }

    if (se->nEncodings < MAX_ENCODINGS && requestLastRectEncoding) {
      encs[se->nEncodings++] = Swap32IfLE(rfbEncodingLastRect);
    }

  } else
  {
    if (SameMachine(rfbsock))
    {
      SAGE_PRINTLOG("VNC> Same machine: preferring raw encoding");
      encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRaw);
    }

    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingCopyRect);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingTight);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingHextile);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingZlib);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingCoRRE);
    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRRE);

    if (compressLevel >= 0 && compressLevel <= 9)
    {
      encs[se->nEncodings++] = Swap32IfLE(compressLevel +
                                          rfbEncodingCompressLevel0);
    }
    else
    {
      encs[se->nEncodings++] = Swap32IfLE(rfbEncodingCompressLevel1);
    }

    if (qualityLevel >= 0 && qualityLevel <= 9)
    {
      encs[se->nEncodings++] = Swap32IfLE(qualityLevel +
                                          rfbEncodingQualityLevel0);
    }

    if (useRemoteCursor)
    {
      encs[se->nEncodings++] = Swap32IfLE(rfbEncodingXCursor);
      encs[se->nEncodings++] = Swap32IfLE(rfbEncodingRichCursor);
      encs[se->nEncodings++] = Swap32IfLE(rfbEncodingPointerPos);
    }

    encs[se->nEncodings++] = Swap32IfLE(rfbEncodingLastRect);
  }

  len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;

  se->nEncodings = Swap16IfLE(se->nEncodings);

  if (!WriteExact(rfbsock, buf, len)) return False;

  return True;
}


/*
 * SendIncrementalFramebufferUpdateRequest.
 */

Bool
VNCViewer::SendIncrementalFramebufferUpdateRequest()
{
  return SendFramebufferUpdateRequest(0, 0, si.framebufferWidth,
                                      si.framebufferHeight, True);
}


/*
 * SendFramebufferUpdateRequest.
 */

Bool
VNCViewer::SendFramebufferUpdateRequest(int x, int y, int w, int h,
                                        Bool incremental)
{
  Bool res;

  rfbFramebufferUpdateRequestMsg fur;

  fur.type = rfbFramebufferUpdateRequest;
  fur.incremental = incremental ? 1 : 0;
  fur.x = Swap16IfLE(x);
  fur.y = Swap16IfLE(y);
  fur.w = Swap16IfLE(w);
  fur.h = Swap16IfLE(h);

  res = WriteExact(rfbsock, (char *)&fur, sz_rfbFramebufferUpdateRequestMsg);

  if (!res)
    return False;

  return True;
}


/*
 * SendPointerEvent.
 */

Bool
VNCViewer::SendPointerEvent(int x, int y, int buttonMask)
{
  rfbPointerEventMsg pe;

  pe.type = rfbPointerEvent;
  pe.buttonMask = buttonMask;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  pe.x = Swap16IfLE(x);
  pe.y = Swap16IfLE(y);
  return WriteExact(rfbsock, (char *)&pe, sz_rfbPointerEventMsg);
}


/*
 * SendKeyEvent.
 */

Bool
VNCViewer::SendKeyEvent(CARD32 key, Bool down)
{
  rfbKeyEventMsg ke;

  ke.type = rfbKeyEvent;
  ke.down = down ? 1 : 0;
  ke.key = Swap32IfLE(key);
  return WriteExact(rfbsock, (char *)&ke, sz_rfbKeyEventMsg);
}


/*
 * SendClientCutText.
 */

Bool
VNCViewer::SendClientCutText(char *str, int len)
{
  rfbClientCutTextMsg cct;

  if (serverCutText)
    free(serverCutText);
  serverCutText = NULL;

  cct.type = rfbClientCutText;
  cct.length = Swap32IfLE(len);
  SAGE_PRINTLOG("VNC> Send SendClientCutText");
  return  (WriteExact(rfbsock, (char *)&cct, sz_rfbClientCutTextMsg) &&
           WriteExact(rfbsock, str, len));
}

/*
 * Cursor stuff
 */
Bool VNCViewer::HandleCursorPos(int x, int y)
{
  //if (appData.useX11Cursor) {
  //if (appData.fullScreen)
  //XWarpPointer(dpy, None, desktopWin, 0, 0, 0, 0, x, y);
  //
  //return True;
  //}


  if (x >= si.framebufferWidth)
    x = si.framebufferWidth - 1;
  if (y >= si.framebufferHeight)
    y = si.framebufferHeight - 1;

  cursorx = x;
  cursory = y;

  //SAGE_PRINTLOG("VNC> Cursor position %d %d", cursorx, cursory);

  //SoftCursorMove(x, y);

  return True;
}



Bool VNCViewer::HandleCursorShape(int xhot, int yhot, int width, int height, CARD32 enc)
{
  int bytesPerPixel;
  size_t bytesPerRow, bytesMaskData;
  rfbXCursorColors rgb;
  CARD32 colors[2];
  char *buf;
  CARD8 *ptr;
  int x, y, b;

  bytesPerPixel = myFormat.bitsPerPixel / 8;
  bytesPerRow = (width + 7) / 8;
  bytesMaskData = bytesPerRow * height;


  //SAGE_PRINTLOG("VNC> cursor %d %d", width, height);

  if (width * height == 0) return True;

  /* Allocate memory for pixel data and temporary mask data. */
  rcSource = (CARD8*) malloc(width * height * bytesPerPixel);
  buf = (char*)malloc(bytesMaskData);


  if (enc == rfbEncodingXCursor) {
    //if (appData.useX11Cursor) {
    //HandleXCursor(xhot, yhot, width, height);
    //return True;
    //}

    /* Read and convert background and foreground colors. */
    if (!ReadFromRFBServer((char *)&rgb, sz_rfbXCursorColors)) {
      free(rcSource);
      free(buf);
      return False;
    }

    /* Read 1bpp pixel data into a temporary buffer. */
    if (!ReadFromRFBServer(buf, bytesMaskData)) {
      free(rcSource);
      free(buf);
      return False;
    }

  } else {                      /* enc == rfbEncodingRichCursor */

    if (!ReadFromRFBServer((char *)rcSource, width * height * bytesPerPixel)) {
      free(rcSource);
      free(buf);
      return False;
    }

  }

  /* Read and decode mask data. */
  if (!ReadFromRFBServer(buf, bytesMaskData)) {
    free(rcSource);
    free(buf);
    return False;
  }


  free(rcSource);
  free(buf);
  return True;
}

/*
 * HandleRFBServerMessage.
 */


Bool
VNCViewer::HandleRFBServerMessage(sgVNCViewer *ct)
{
  rfbServerToClientMsg msg;

  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(rfbsock,&fds);

  struct timeval timeout;
  //timeout.tv_usec = 5000000;
  timeout.tv_usec = 500000;
  timeout.tv_sec  = 0;

  if (!select(rfbsock+1, &fds, NULL, NULL, &timeout))
  {
    //fprintf(stderr, "VNC Timeout --> closing VNCViewer.\n");
    //return False;
    ;
  }

  if (!ReadFromRFBServer((char *)&msg, 1))
  {
    SAGE_PRINTLOG("VNC> VNC> Failed to read message");
    return False;
  }

  switch (msg.type)
  {
  case rfbSetColourMapEntries:
    break;

  case rfbFramebufferUpdate:
    rfbFramebufferUpdateRectHeader rect;
    int linesToRead;
    int bytesPerLine;
    int i;

    if (!ReadFromRFBServer(((char *)&msg.fu) + 1,
                           sz_rfbFramebufferUpdateMsg - 1))
      return False;

    msg.fu.nRects = Swap16IfLE(msg.fu.nRects);

    for (i = 0; i < msg.fu.nRects; i++)
    {
      if (!ReadFromRFBServer((char *)&rect, sz_rfbFramebufferUpdateRectHeader))
        return False;

      rect.encoding = Swap32IfLE(rect.encoding);

      if (rect.encoding == rfbEncodingLastRect)
        break;

      rect.r.x = Swap16IfLE(rect.r.x);
      rect.r.y = Swap16IfLE(rect.r.y);
      rect.r.w = Swap16IfLE(rect.r.w);
      rect.r.h = Swap16IfLE(rect.r.h);

      if (rect.encoding == rfbEncodingXCursor ||
          rect.encoding == rfbEncodingRichCursor) {
        if (!HandleCursorShape(rect.r.x, rect.r.y, rect.r.w, rect.r.h,
                               rect.encoding)) {
          return False;
        }
        continue;
      }

      if (rect.encoding == rfbEncodingPointerPos) {
        if (!HandleCursorPos(rect.r.x, rect.r.y)) {
          return False;
        }
        continue;
      }

      if ((rect.r.x + rect.r.w > si.framebufferWidth) ||
          (rect.r.y + rect.r.h > si.framebufferHeight))
      {
        SAGE_PRINTLOG("VNC> Rect too large: %dx%d at (%d, %d)",
                      rect.r.w, rect.r.h, rect.r.x, rect.r.y);
        continue;
        //return False;
      }

      if ((rect.r.h * rect.r.w) == 0) {
        SAGE_PRINTLOG("VNC> Zero size rect - ignoring");
        continue;
      }

      // If RichCursor encoding is used, we should prevent collisions
      // between framebuffer updates and cursor drawing operations.
      //                  SoftCursorLockArea(rect.r.x, rect.r.y, rect.r.w, rect.r.h);

      switch (rect.encoding)
      {
      case rfbEncodingRaw:
        bytesPerLine = rect.r.w * myFormat.bitsPerPixel / 8;
        linesToRead = BUFFER_SIZE / bytesPerLine;

        while (rect.r.h > 0)
        {
          if (linesToRead > rect.r.h)
            linesToRead = rect.r.h;

          if (!ReadFromRFBServer(buffer,
                                 bytesPerLine*linesToRead))
            return False;

          ct->CopyDataToScreen(buffer,
                               rect.r.x, rect.r.y, rect.r.w,
                               linesToRead);

          rect.r.h -= linesToRead;
          rect.r.y += linesToRead;

        }
        break;

      case rfbEncodingCopyRect:
        {
          rfbCopyRect cr;

          if (!ReadFromRFBServer((char *)&cr, sz_rfbCopyRect))
            return False;

          cr.srcX = Swap16IfLE(cr.srcX);
          cr.srcY = Swap16IfLE(cr.srcY);

          // If RichCursor encoding is used, we should extend our
          //  "cursor lock area" (previously set to destination
          // rectangle) to the source rectangle as well.
          //  SoftCursorLockArea(cr.srcX, cr.srcY, rect.r.w, rect.r.h);

          ct->RectDataToScreen(rect.r.x, rect.r.y,
                               rect.r.w, rect.r.h,
                               cr.srcX, cr.srcY);
          break;
        }

      case rfbEncodingRRE:
        switch (myFormat.bitsPerPixel)
        {
        case 8:
          if (!HandleRRE8(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 16:
          if (!HandleRRE16(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 32:
          if (!HandleRRE32(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        }
        break;

      case rfbEncodingCoRRE:
        switch (myFormat.bitsPerPixel)
        {
        case 8:
          if (!HandleCoRRE8(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 16:
          if (!HandleCoRRE16(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 32:
          if (!HandleCoRRE32(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        }
        break;

      case rfbEncodingHextile:
        switch (myFormat.bitsPerPixel)
        {
        case 8:
          if (!HandleHextile8(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 16:
          if (!HandleHextile16(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 32:
          if (!HandleHextile32(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        }
        break;

      case rfbEncodingZlib:
        switch (myFormat.bitsPerPixel) {
        case 8:
          if (!HandleZlib8(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 16:
          if (!HandleZlib16(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 32:
          if (!HandleZlib32(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        }
        break;

      case rfbEncodingTight:
        switch (myFormat.bitsPerPixel) {
        case 8:
          if (!HandleTight8(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 16:
          if (!HandleTight16(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        case 32:
          if (!HandleTight32(ct, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return False;
          break;
        }
        break;

      default:
        SAGE_PRINTLOG("VNC> Unknown rect encoding %d", (int)rect.encoding);
        return False;
      }
      // Now we may discard "soft cursor locks".
      //  SoftCursorUnlockScreen();

    }

    if (!SendIncrementalFramebufferUpdateRequest())
    {
      SAGE_PRINTLOG("VNC> VNC> !SendIncrementalFramebufferUpdateRequest() == true");
      return False;
    }

    break;

  case rfbBell:
    break;

  case rfbServerCutText:
    if (!ReadFromRFBServer(((char *)&msg) + 1,
                           sz_rfbServerCutTextMsg - 1))
      return False;

    msg.sct.length = Swap32IfLE(msg.sct.length);

    if (serverCutText)
      free(serverCutText);

    serverCutText = (char*) malloc(msg.sct.length+1);

    if (!ReadFromRFBServer(serverCutText, msg.sct.length))
      return False;

    serverCutText[msg.sct.length] = 0;

    newServerCutText = True;

    break;

  default:
    SAGE_PRINTLOG("VNC> Unknown message type %d from VNC server",msg.type);
    //return False;
  }

  return True;
}


#define GET_PIXEL8(pix, ptr) ((pix) = *(ptr)++)

#define GET_PIXEL16(pix, ptr) (((CARD8*)&(pix))[0] = *(ptr)++,  \
                               ((CARD8*)&(pix))[1] = *(ptr)++)

#define GET_PIXEL32(pix, ptr) (((CARD8*)&(pix))[0] = *(ptr)++,  \
                               ((CARD8*)&(pix))[1] = *(ptr)++,  \
                               ((CARD8*)&(pix))[2] = *(ptr)++,  \
                               ((CARD8*)&(pix))[3] = *(ptr)++)

/* CONCAT2 concatenates its two arguments.  CONCAT2E does the same but also
   expands its arguments if they are macros */

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)

#define BPP 8
#include "rre.cpp"
#include "corre.cpp"
#include "hextile.cpp"
#include "zlib.cpp"
#include "tight.cpp"
#undef BPP
#define BPP 16
#include "rre.cpp"
#include "corre.cpp"
#include "hextile.cpp"
#include "zlib.cpp"
#include "tight.cpp"
#undef BPP
#define BPP 32
#include "rre.cpp"
#include "corre.cpp"
#include "hextile.cpp"
#include "zlib.cpp"
#include "tight.cpp"
#undef BPP


/*
 * PrintPixelFormat.
 */

void
VNCViewer::PrintPixelFormat(rfbPixelFormat *format)
{
  if (format->bitsPerPixel == 1)
  {
    SAGE_PRINTLOG("VNC>   Single bit per pixel.");
    SAGE_PRINTLOG("VNC>  %s significant bit in each byte is leftmost on the screen.",
                  (format->bigEndian ? "Most" : "Least"));
  }
  else
  {
    SAGE_PRINTLOG("VNC>   %d bits per pixel.",format->bitsPerPixel);
    if (format->bitsPerPixel != 8)
    {
      SAGE_PRINTLOG("VNC>   %s significant byte first in each pixel.",
                    (format->bigEndian ? "Most" : "Least"));
    }
    if (format->trueColour)
    {
      SAGE_PRINTLOG("VNC>   True colour: max red %d green %d blue %d",
                    format->redMax, format->greenMax, format->blueMax);
      SAGE_PRINTLOG("VNC> , shift red %d green %d blue %d",
                    format->redShift, format->greenShift, format->blueShift);
    }
    else
    {
      SAGE_PRINTLOG("VNC>   Colour map (not true colour).");
    }
  }
}

long
VNCViewer::ReadCompactLen (void)
{
  long len;
  CARD8 b;

  if (!ReadFromRFBServer((char *)&b, 1))
    return -1;
  len = (int)b & 0x7F;
  if (b & 0x80) {
    if (!ReadFromRFBServer((char *)&b, 1))
      return -1;
    len |= ((int)b & 0x7F) << 7;
    if (b & 0x80) {
      if (!ReadFromRFBServer((char *)&b, 1))
        return -1;
      len |= ((int)b & 0xFF) << 14;
    }
  }
  return len;
}

/*
 * JPEG source manager functions for JPEG decompression in Tight decoder.
 */

static struct jpeg_source_mgr jpegSrcManager;
static JOCTET *jpegBufferPtr;
static size_t jpegBufferLen;

static void
JpegInitSource(j_decompress_ptr cinfo)
{
  jpegError = False;
}

static boolean
JpegFillInputBuffer(j_decompress_ptr cinfo)
{
  jpegError = True;
  jpegSrcManager.bytes_in_buffer = jpegBufferLen;
  jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;

  return TRUE;
}

static void
JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
  if (num_bytes < 0 || num_bytes > jpegSrcManager.bytes_in_buffer) {
    jpegError = True;
    jpegSrcManager.bytes_in_buffer = jpegBufferLen;
    jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;
  } else {
    jpegSrcManager.next_input_byte += (size_t) num_bytes;
    jpegSrcManager.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void
JpegTermSource(j_decompress_ptr cinfo)
{
  /* No work necessary here. */
}

static void
JpegSetSrcManager(j_decompress_ptr cinfo, CARD8 *compressedData,
                  int compressedLen)
{
  jpegBufferPtr = (JOCTET *)compressedData;
  jpegBufferLen = (size_t)compressedLen;

  jpegSrcManager.init_source = JpegInitSource;
  jpegSrcManager.fill_input_buffer = JpegFillInputBuffer;
  jpegSrcManager.skip_input_data = JpegSkipInputData;
  jpegSrcManager.resync_to_restart = jpeg_resync_to_restart;
  jpegSrcManager.term_source = JpegTermSource;
  jpegSrcManager.next_input_byte = jpegBufferPtr;
  jpegSrcManager.bytes_in_buffer = jpegBufferLen;

  cinfo->src = &jpegSrcManager;
}
