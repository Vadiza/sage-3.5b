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
 * argsresources.c - deal with command-line args and resources.
 */
#include "sail.h"
#include "vncviewer.h"


void
VNCViewer::GetArgsAndResources(int argc, char **argv)
{
  int i;
  char *vncServerName;

  //encodingsString = strdup("tight copyrect");
  //encodingsString = strdup("zlib copyrect");
  //encodingsString = strdup("copyrect hextile");
  //default : raw copyrect hextile corre rre
  //encodingsString = strdup("hextile corre rre raw copyrect");
  encodingsString = strdup("tight hextile copyrect");

  shareDesktop = True;
  viewOnly = False;
  fullScreen = False;

  //passwordFile = strdup("passwd");
  passwordFile = NULL;

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

  compressLevel   = 6;
  qualityLevel    = 4;
  useRemoteCursor = True;
  cursorx = -1;
  cursory = -1;

  if (argc == 2)
  {
    vncServerName = argv[1];

    if (vncServerName[0] == '-')
    {
      SAGE_PRINTLOG("VNC> invalid command line argument\n");
      exit(-1);
    }
  }

  if (strlen(vncServerName) > 255)
    SAGE_PRINTLOG("VNC> VNC server name too long\n");

  for (i = 0; vncServerName[i] != ':' && vncServerName[i] != 0; i++);

  memset(vncServerHost, 0, 256);
  strncpy(vncServerHost, vncServerName, i);

  if (vncServerName[i] == ':')
    vncServerPort = atoi(&vncServerName[i+1]);
  else
    vncServerPort = 0;

  if (vncServerPort < 100)
    vncServerPort += SERVER_PORT_OFFSET;
}
