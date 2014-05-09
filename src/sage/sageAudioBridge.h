/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioBridge.h
 * Author : Hyejung Hur, Byungil Jeong
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

#ifndef _SAGE_AUDIO_BRIDGE_H_
#define _SAGE_AUDIO_BRIDGE_H_

#include "streamProtocol.h"
#include "sageBlock.h"

class sageAudioBridge;
class sageAudioDup;
class sageTcpModule;
class sageUdpModule;
class messageInterface;
class bridgeSharedData;

typedef struct {
  sageAudioBridge *This;
  streamProtocol   *nwObj;
} nwAudioCheckThreadParam;

typedef struct {
  sageAudioDup *This;
  streamProtocol   *nwObj;
} nwStreamThreadParam;

typedef struct {
  char* rcvIP;
  int  rcvID;
  int  nodeID;
  int fsIdx;
  bool active;
  int sendPort;
} streamBridgeParam;

class sageAudioDup {
private:
  int sailNodeNum;
  int streamType;
  int blockSize;
  int frameRate;
  int syncType;
  int keyframe;
  int sampleFmt;
  int samplingRate;
  int channels;
  int framePerBuffer;

  pthread_t thId;

protected:
  std::vector<streamBridgeParam*> paramList;
  int instID;
  int senderID;
  streamProtocol* instNWObj;

  bool initialized;
  bool bridgeEnd;

  int rcvNodeNum;
  sageAudioBlock audioNBlock;

  static void* nwStreamThread(void *args);

public:
  sageAudioDup(int appID);
  ~sageAudioDup(void);

  int init(void);
  int addStream(char *msg, streamProtocol *nwObj);
  int addReceiver(char *msg, int fsIdx);
  void shutdownAllStreams(void);
  void shutdownStreams(int fsIdx);

  friend class sageAudioBridge;

};

class sageAudioBridge {
private:
  sageTcpModule *tcpObj;
  sageUdpModule *udpObj;
  sageNwConfig nwCfg;

  sageAudioDup *dupList[MAX_INST_NUM];
  char audioIP[SAGE_IP_LEN];
  int audioPort;
  bool bridgeEnd;

  int initNetworks();
  messageInterface *msgInf;
  bridgeSharedData *shared;

  static void* nwCheckThread(void *args);

public:
  // ip, port  : IP address and port number where listening audio stream connections
  // inf  :  message communication object for sending messages to SAIL.
  sageAudioBridge(char *ip, int port, messageInterface *inf, bridgeSharedData *sh);

  // send SAIL_CONNECT_TO_ARCV to SAIL
  int startAudioStream(int instID, int sailClientID);

  // accept streams from app
  int initStreams(char *msg, streamProtocol *nwObj);

  // start audio stream from sageBridge to an endpoint (ARCV)
  int duplicate(int instID, char *msg, int fsIdx);

  // delete a duplicator instance
  int shutdownDup(int instID);

  // shutdown streams to an endpoint
  int shutdownStreams(int instID, int fsIdx);

  ~sageAudioBridge();
};

#endif
