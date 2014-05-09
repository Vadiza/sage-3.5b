/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageConfig.h
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

#ifndef SAGE_CONFIG_H_
#define SAGE_CONFIG_H_

#include "sage.h"

class sageSyncServer;
class sageSyncClient;
class syncGroup;

class streamerConfig {
public:
  int   blockX, blockY, blockSize;
  nwProtocol protocol;

  int   totalWidth, totalHeight;
  int   rank;      // the rank of this node
  int   resX;      // the width of application buffer in pixels
  int   resY;      // the height of application buffer in pixels
  sageRect   imageMap;  // the location of sub-image rendered by this node in the whole app image
  sagePixFmt pixFmt;    // pixel format defined in sage.h
  int   nodeNum;     // number of rendering nodes for an app instance
  int   rowOrd;      // row order flag
  bool  master;      // is master node or not
  bool  asyncUpdate;
  sageCompressType compression;
  int  frameRate;
  int  syncMode;
  int  streamType;
  int  streamerID;
  int  groupSize;
  int  maxBandwidth;  // maximum hardware bandwidth of sending node
  int  maxCheckInterval;
  int  flowWindow;
  bool autoBlockSize;
  bool frameDrop;

  ////////
  long totalFrames;
  bool bridgeOn;
  bool audioOn;                   // audio on/off flag
  nwProtocol audioProtocol;
  int audioKeyFrame;
  sageAudioMode audioMode;        // audio mode : capture, play, read, write
  int  audioPort;                 // audio port number
  int  audioDeviceNum;            // device number
  sageSampleFmt sampleFmt;        // audio sampling format : float, short, char, unsigned char
  long samplingRate;              // audio sampling rate : default-44100
  int  channels;                  // number of audio channel
  int  framePerBuffer;            // audio frames per buffer
  int  bytesPerSample;
  sageAudioSyncType syncType;
  char  audioFileName[SAGE_NAME_LEN];  // application name
  int   audioBuffSize;

  /**
   * Is this stream coming from sageBridge and originally parallel app?
   */
  int fromBridgeParallel;

  int  syncPolicy, syncID;
  syncGroup *sGroup;

  sageSyncClient *syncClientObj;

  /*!
   * This is to stream to SAGENext wall.
   * If this flag is set at the application's conf file,
   * ...
   */
  bool swexp; // sungwon experimental.

  char mediaFileName[SAGE_NAME_LEN];

  streamerConfig();
};

class sailConfig : public streamerConfig {
public:
  char  appName[SAGE_NAME_LEN];  // application name
  char  masterIP[SAGE_IP_LEN];  // the SAIL Master IP
  int   nwID;
  char  fsIP[SAGE_IP_LEN];
  int   fsPort;
  char    bridgeIP[SAGE_IP_LEN];
  int    bridgePort;

  int   syncPort;
  int   msgPort;
  int   appID;   // ID for each app instance
  char  launcherID[SAGE_NAME_LEN];  // which appLauncher started this app

  int   winX, winY, winWidth, winHeight; //window position and size on tiled display

  int   syncMode;
  int   streamMode;
  int   rcvBufSize;  // stream receiving buffer size

  bool  rendering;   // rendering or control only
  char streamIP[SAGE_IP_LEN];

  sailConfig();
  int init(char *fname); //initialize app configuration by config file
  int setAppName(char *name);
};

class appInExec : public sageRect {
public:
  int uiClient;
  int sailClient;
  char appName[SAGE_NAME_LEN];
  int instID;
  int fsInstID;   // basically the windowID
  char launcherID[SAGE_NAME_LEN];
  nwProtocol protocol;
  int nwID;
  char fsIP[SAGE_IP_LEN];
  int fsPort;
  int imageWidth, imageHeight;
  int bandWidth;
  int frameRate;
  int displayID;
  bool audioOn;
  char renderNodeIP[SAGE_IP_LEN];

  appInExec() : displayID(0) {}
};

#endif
