/************************************************************************
 *
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
 ************************************************************************/

/*
 * colour.c - functions to deal with colour - i.e. RFB pixel formats, X visuals
 * and colormaps.  Thanks to Grant McDorman for some of the ideas used here.
 */

#include "sail.h"
#include "vncviewer.h"
#include <limits.h>


#define INVALID_PIXEL 0xffffffff
#define MAX_CMAP_SIZE 256

/*
 * SetVisualAndCmap() deals with the wonderful world of X "visuals" (which are
 * equivalent to the RFB protocol's "pixel format").  Having decided on the
 * best visual, it also creates a colormap if necessary, sets the appropriate
 * resources on the toplevel widget, and sets up the myFormat structure to
 * describe the pixel format in terms that the RFB server will be able to
 * understand.
 *
 * The algorithm for deciding which visual to use is as follows:
 *
 * If forceOwnCmap is true then we try to use a PseudoColor visual - we first
 * see if there's one of the same depth as the RFB server, followed by an 8-bit
 * deep one.
 *
 * If forceTrueColour is true then we try to use a TrueColor visual - if
 * requestedDepth is set then it must be of that depth, otherwise any depth
 * will be used.
 *
 * Otherwise, we use the X server's default visual and colormap.  If this is
 * TrueColor then we just ask the RFB server for this format.  If the default
 * isn't TrueColor, or if useBGR233 is true, then we ask the RFB server for
 * BGR233 pixel format and use a lookup table to translate to the nearest
 * colours provided by the X server.
 */

void
VNCViewer::SetVisualAndCmap()
{
  if (forceOwnCmap) {
    if (!si.format.trueColour) {
      //if (GetPseudoColorVisualAndCmap(si.format.depth)) return;
    }
    //if (GetPseudoColorVisualAndCmap(8)) return;
    SAGE_PRINTLOG("VNC> Couldn't find a matching PseudoColor visual.");
  }

  if (forceTrueColour) {
    // if (GetTrueColorVisualAndCmap(appData.requestedDepth)) return;
    SAGE_PRINTLOG("VNC> Couldn't find a matching TrueColor visual.");
  }

  /* just use default visual and colormap */

  if (!useBGR233)
  {
    myFormat.bitsPerPixel = 32;
    myFormat.depth = 24;
    myFormat.trueColour = 1;
    //myFormat.bigEndian = (ImageByteOrder(dpy) == MSBFirst);
    myFormat.bigEndian = 0;

    myFormat.redShift = 0;
    myFormat.greenShift = 8;
    myFormat.blueShift = 16;
    myFormat.redMax = 255;
    myFormat.greenMax = 255;
    myFormat.blueMax = 255;

    SAGE_PRINTLOG("VNC> Using default colormap which is TrueColor.  Pixel format:");
    PrintPixelFormat(&myFormat);
    return;
  }

  useBGR233 = True;

  myFormat.bitsPerPixel = 8;
  myFormat.depth = 8;
  myFormat.trueColour = 1;
  myFormat.bigEndian = 0;
  myFormat.redMax = 7;
  myFormat.greenMax = 7;
  myFormat.blueMax = 3;
  myFormat.redShift = 0;
  myFormat.greenShift = 3;
  myFormat.blueShift = 6;

  SAGE_PRINTLOG("VNC> Using default colormap and translating from BGR233.  Pixel format:");
  PrintPixelFormat(&myFormat);
}
