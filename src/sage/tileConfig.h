/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: tileConfig.h
 * Author : Byungil Jeong
 *
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#ifndef SAGE_TILE_CONFIG_H
#define SAGE_TILE_CONFIG_H

#include "sageBase.h"

// width : horizontal screen resolution
// height : vertical screen resolution
// left : left mullion width
// right : left mullion width
// bottom : left mullion width
// top : left mullion width

#define XID_LEN 5

class displayType : public sageRect {
public:
  char name[SAGE_NAME_LEN];
  int PPI; //pixel per inch

  displayType() : PPI(0) {}
};

// x, y : lower left corner coordinates in pixels
// width, height : screen resolution
// left, right, bottom, top : relative coordinates (0.0 - 1.0)
class tileInfo : public sageRect {
public:
  displayType *tileType;
  int nodeID, tileID;
  int xIndex, yIndex;
  int offsetX, offsetY;

  tileInfo() : tileType(NULL), nodeID(0), tileID(0), offsetX(0), offsetY(0),
               xIndex(0), yIndex(0) {}
};

class displayNode {
public:
  char ip[SAGE_IP_LEN];      //ip address of the computer the holds pixels to display
  std::vector<char*> streamIPs; // optional parameters to receive streams through separate IPs
  std::vector<tileInfo*> tiles;   // tile ID attached to this display node
  int dimX, dimY;
  int winX, winY;
  int port;
  char Xdisp[XID_LEN];

  displayNode();
  ~displayNode();
  int computeDimension();
};


class audioNode {
public:
  char ip[SAGE_IP_LEN];            //ip address of the computer the holds pixels to display
  std::vector<char*> streamIPs;      // optional parameters to receive streams through separate IPs

  // ?????????????
  //int  audioDeviceNum;
  sageSampleFmt sampleFmt;
  long samplingRate;
  int  channels;
  int  framePerBuffer;
  int  deviceId;

  audioNode();
  ~audioNode();
};

class virtualDesktop;
class appInExec;

class displayConnection {
public:
  virtualDesktop *displays[2];
  int edges[2];
  int offset;
};

// x, y : lower left corner coordinates in pixels on global desktop
// width, height : virtual desktop resolution
class virtualDesktop : public sageRect {
public:
  std::vector<tileInfo*> tileList;
  std::vector<displayNode*> displayCluster;
  std::vector<audioNode*> audioCluster;
  std::vector<displayConnection *> connectionList;

  int dimX, dimY;
  char masterIP[SAGE_IP_LEN];
  displayType globalType;
  int displayID;
  bool table;
  char audioDir[TOKEN_LEN];
  bool audioServer;

  virtualDesktop();
  bool parseConfigfile(FILE *fp, bool configBegin = false);
  int parseAudioConfigfile(FILE *fp, bool configBegin = false);
  void locateApp(appInExec *app, displayConnection *connection, int idx = 0);
  int updateDesktop();
};

#endif
