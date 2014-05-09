/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appInstance.h
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

#ifndef APPINSTANCE_H_
#define APPINSTANCE_H_

//#include "sageDisplay.h"
#include "sage.h"
#include "sageSync.h"
#include "sageConfig.h"
#include "sageTcpModule.h"
#include "sageUdpModule.h"
#include "streamInfo.h"
#include "sageStreamer.h"
#include <string>

class appInstance;
class sagePixelReceiver;
class bridgeSharedData;
class sageBlockPartition;

#define ALLOC_LOAD_BALANCING 0
#define ALLOC_SINGLE_NODE    1

class nodeAllocInfo {
public:
  int nodeID;
  int blockID;

  nodeAllocInfo() : nodeID(-1), blockID(-1) {}
};

class appInstance : public appInExec {
protected:
  bridgeSharedData *shared;
  sagePixelReceiver *recv;
  bridgeStreamer *streamerList[MAX_ENDPOINT_NUM];
  float slaveFRate[MAX_ENDPOINT_NUM];
  int maxStreamerIdx;

  streamerConfig sConfig;

  bool initialized;
  int instID;
  int nodeNum;
  int blockSize, groupSize;
  int blockX, blockY;
  sageBlockBuf *blockBuf;
  int blockNum;
  std::vector<nodeAllocInfo> allocInfoList;

  sageRect imageMap;
  sageMessage nodeAlloc;
  syncGroup *firstSyncGroup;
  int firstSyncID;

  std::vector<int> fsList;
  std::vector<std::string> widgetList;
  int waitNodes;
  int validIdx;  // a vaild streamer index

  sageTimer perfTimer;
  int reportRate;
  float slaveBandWidth, accBandWidth, curBandWidth;
  unsigned packetLoss;
  float accLoss, slaveLoss;
  int  reportCnt;

  int numOfSenders; /**< Is this parallel app ? */

  int sendPerformanceInfo();
  int addStreamer(int fsIdx, int orgIdx = 0, syncGroup *sGroup = NULL, int syncID = 0);

  int shutdownAllStreams();
  int startPerformanceReport(sageMessage &msg);
  int findValidIdx();

public:
  appInstance(char *msgStr, int id, bridgeSharedData *sh);
  ~appInstance();
  int init(char *msg, streamProtocol *nwObj);
  int addStream(int senderID);
  void fillAppInfo(char *msgStr);
  int connectToFSManager();
  int connectToFSManager(char *fsIP, int fsPort);
  int parseMessage(sageMessage &msg, int fsIdx);
  bool isActive();
  int accumulateBandWidth(char *data);
  int allocateNodes(int policy, int nodeID = 0);
  friend class sageBridge;
};

#endif
