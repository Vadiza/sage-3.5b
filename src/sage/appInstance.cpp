/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appInstance.cpp - manaing each application instance of SAGE Bridge
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

#include "appInstance.h"
#include "messageInterface.h"
#include "sageReceiver.h"
#include "sageSharedData.h"
#include "sageEvent.h"
#include "sageBlockPool.h"

appInstance::appInstance(char *msgStr, int id, bridgeSharedData *sh) : instID(id), shared(sh),
                                                                       firstSyncGroup(NULL), firstSyncID(0), initialized(false), maxStreamerIdx(0),
                                                                       blockSize(0), groupSize(0)
{
  sscanf(msgStr, "%s %d %d %d %d %d %s %d %d %d %d %d", appName, &x, &y, &width, &height,
         (int *)&sConfig.protocol, fsIP, &fsPort, &imageWidth, &imageHeight, &blockNum,
         (int *)&audioOn);

  SAGE_PRINTLOG("create app instance on node %d", shared->nodeID);

  if (shared && shared->nodeID > 0) {
    sscanf(sage::tokenSeek(msgStr, 12), "%d %d", &nodeNum, &firstSyncID);
  }

  allocInfoList.clear();
  for (int i=0; i<MAX_ENDPOINT_NUM; i++) {
    streamerList[i] = NULL;
    slaveFRate[i] = 0.0;
  }

  validIdx = 0;
  reportRate = 1;
  reportCnt = 0;
  slaveBandWidth = 0.0;
  accBandWidth = 0.0;
  curBandWidth = 0.0;
  accLoss = 0.0;
  slaveLoss = 0.0;
  packetLoss = 0;
  perfTimer.reset();

  numOfSenders = 1;
}

appInstance::~appInstance()
{
  delete recv;
  delete blockBuf;
}

int appInstance::allocateNodes(int policy, int nID)
{
  if (policy == ALLOC_LOAD_BALANCING) {
    nodeNum = shared->nodeNum;
    if (nodeNum < 1) {
      SAGE_PRINTLOG("appInstance::allocateNodes : error in bridge node number : %d", nodeNum);
      return -1;
    }
    float partition = ((float)blockNum)/nodeNum;
    for (int i=0; i<nodeNum; i++) {
      nodeAllocInfo info;
      info.nodeID = i;
      info.blockID = (int)floor(partition*(i+1) + 0.5) - 1;
      allocInfoList.push_back(info);
    }
  }
  else if (policy == ALLOC_SINGLE_NODE) {
    nodeNum = 1;
    nodeAllocInfo info;
    info.nodeID = nID;
    info.blockID = blockNum - 1;
    allocInfoList.push_back(info);
  }

  return 0;
}


/**
 * is called by EVENT_NEW_CONNECTION from sageBridge::parseEvent()
 * which is triggered by sageBridge::nwCheckThread()
 */
int appInstance::init(char *msg, streamProtocol *nwObj)
{
  initialized = true;

  int dummy;

  sscanf(msg, "%d %d %d %d %d %d %d %d %d %d",
         &dummy,
         &sConfig.streamType,
         &sConfig.frameRate,
         &dummy,
         &groupSize,
         &blockSize,
         &numOfSenders,
         (int *)&sConfig.pixFmt,
         &sConfig.blockX,
         &sConfig.blockY
         );

  /*
    char *msgPt = sage::tokenSeek(msg, 1);
    sscanf(msgPt, "%d %d", &sConfig.streamType, &sConfig.frameRate);

    msgPt = sage::tokenSeek(msg, 4);
    sscanf(msgPt, "%d %d", &groupSize, &blockSize);

    msgPt = sage::tokenSeek(msg, 7);
    sscanf(msgPt, "%d %d %d", (int *)&sConfig.pixFmt, &sConfig.blockX, &sConfig.blockY);
  */

  SAGE_PRINTLOG("block buffer size %d", shared->bufSize);

  blockBuf = new sageBlockBuf(shared->bufSize, groupSize, blockSize,
                              BUF_MEM_ALLOC | BUF_CTRL_GROUP | BUF_MULTI_READER);

  /* creates bridgeStreamer object */
  addStreamer(0, 0, firstSyncGroup, firstSyncID);

  /* pixel receiver */
  recv = new sagePixelReceiver(msg, shared, nwObj, blockBuf);

  return 0;
}

int appInstance::addStreamer(int fsIdx, int orgIdx, syncGroup *sGroup, int syncID)
{
  if (!initialized)
    return 0;

  if ( numOfSenders > 1 )
    sConfig.fromBridgeParallel = 1; //sungwon

  // add a sage streamer to the app instance
  sConfig.master = (shared->nodeID == 0);
  sConfig.nodeNum = nodeNum;
  sConfig.rank = shared->nodeID;
  sConfig.streamerID = fsIdx;
  sConfig.sGroup = sGroup;
  if (sGroup) {
    sConfig.syncID = sGroup->getSyncID();
    sGroup->setFrameRate(sConfig.frameRate);
  }
  else
    sConfig.syncID = syncID;

  sConfig.blockSize = blockSize;
  sConfig.groupSize = groupSize;
  sConfig.totalWidth = imageWidth;
  sConfig.totalHeight = imageHeight;

  sConfig.syncClientObj = shared->syncClientObj;
  if ( sConfig.syncClientObj )
    sConfig.syncClientObj->addSyncGroup(sConfig.syncID);
  sConfig.frameDrop = shared->frameDrop;

  bridgeStreamer *streamer = new bridgeStreamer(sConfig, blockBuf, shared);

  streamerList[fsIdx] = streamer;

  SAGE_PRINTLOG("appInstance::%s() : bridgeStreamer for fsIdx %d has created\n", __FUNCTION__, fsIdx);
  maxStreamerIdx = MAX(maxStreamerIdx, fsIdx);

  return 0;
}

int appInstance::addStream(int senderID)
{
  int streamNum = recv->addStream(senderID);

  return streamNum;
}

int appInstance::sendPerformanceInfo()
{
  //Calculate performance here
  //if (validIdx < 0)
  //   return 0;

  double elapsedTime = perfTimer.getTimeUS();

  if (reportRate > 0 && elapsedTime > (1000000.0 * reportRate)) {
    if (initialized) {
      unsigned long bandWidth = 0;
      for (int i=0; i<=maxStreamerIdx; i++) {
        if (streamerList[i]) {
          bandWidth += streamerList[i]->getBandWidth();
          streamerList[i]->resetBandWidth();
        }
      }

      float obsBandWidth = (float) (bandWidth * 8.0f / elapsedTime);
      //float obsLoss = (float) (packetLoss * 8.0 / elapsedTime);
      //packetLoss = 0;

      char msgStr[TOKEN_LEN];

      if (shared->nodeID == 0) {
        float totalBandWidth = slaveBandWidth+obsBandWidth;
        curBandWidth = totalBandWidth;

        //float totalLoss = slaveLoss+obsLoss;
        //float lossPercent = totalLoss*100/(totalBandWidth+totalLoss);

        for (int i=0; i<=maxStreamerIdx; i++) {
          if (streamerList[i]) {
            float frameRate = (float)((streamerList[i]->getFrameCount())*1000000.0/elapsedTime);
            streamerList[i]->resetFrameCounter();
            sprintf(msgStr, "%d %7.2f %7.2f %7.2f", fsList[i], totalBandWidth, frameRate, 0.0);
            shared->eventQueue->sendEvent(EVENT_MASTER_PERF_INFO, msgStr);
          }
        }
      }
      else {
        sageToken frameToken(TOKEN_LEN);
        int tokenCnt = 0;
        for (int i=0; i<=maxStreamerIdx; i++) {
          if (streamerList[i]) {
            float frameRate = (float)((streamerList[i]->getFrameCount())*1000000.0/elapsedTime);
            streamerList[i]->resetFrameCounter();
            char frameRateStr[TOKEN_LEN];
            sprintf(frameRateStr, "%d %7.2f", i, frameRate);
            frameToken.putToken(frameRateStr);
            tokenCnt++;
          }
        }

        //SAGE_PRINTLOG("frame token %s", frameToken.getBuffer());

        sprintf(msgStr, "%d %7.2f %7.2f %d %s", instID, obsBandWidth, 0.0, tokenCnt, frameToken.getBuffer());
        shared->eventQueue->sendEvent(EVENT_SLAVE_PERF_INFO, msgStr);
      }

      char bufInfo[TOKEN_LEN];
      blockBuf->getBufInfo(bufInfo);
      //SAGE_PRINTLOG("<buffer status> sb %d %s", shared->nodeID, bufInfo);
    }
    else {
      char msgStr[TOKEN_LEN];

      if (shared->nodeID == 0) {
        curBandWidth = slaveBandWidth;
        for (int i=0; i<fsList.size(); i++) {
          if (fsList[i] >= 0) {
            //SAGE_PRINTLOG("slave frame rate %d", slaveFRate[i]);
            sprintf(msgStr, "%d %7.2f %7.2f %7.2f", fsList[i], slaveBandWidth, slaveFRate[i], 0.0);
            shared->eventQueue->sendEvent(EVENT_MASTER_PERF_INFO, msgStr);
          }
        }
      }
    }

    perfTimer.reset();
  }


  return 0;
}

int appInstance::startPerformanceReport(sageMessage &msg)
{
  int rate = atoi((char *)msg.getData());

  reportRate = rate;
  perfTimer.reset();

  for (int i=0; i<=maxStreamerIdx; i++) {
    if (streamerList[i]) {
      streamerList[i]->resetBandWidth();
      streamerList[i]->resetFrameCounter();
    }
  }

  return 0;
}

int appInstance::accumulateBandWidth(char *data)
{
  //SAGE_PRINTLOG("slave perf data %s", data);
  float bWidth, pLoss;
  int frateNum;
  sscanf(data, "%f %f %d", &bWidth, &pLoss, &frateNum);

  accBandWidth += bWidth;
  accLoss += pLoss;
  reportCnt++;

  int perfWait = nodeNum-1;
  if (!initialized) {
    perfWait = nodeNum;
    char *frateStr = sage::tokenSeek(data, 3);

    for (int i=0; i<frateNum; i++) {
      int idx;
      float frameRate;
      sscanf(frateStr, "%d %f", &idx, &frameRate);
      //SAGE_PRINTLOG("slave %d frate %s", idx, token);

      slaveFRate[idx] = frameRate;
    }
  }

  if (reportCnt >= perfWait) {
    slaveBandWidth = accBandWidth;
    slaveLoss = accLoss;
    accBandWidth = 0.0;
    accLoss = 0.0;
    reportCnt = 0;
  }

  return 0;
}

int appInstance::findValidIdx()
{
  int i=0;
  for ( ; i<=maxStreamerIdx; i++) {
    if (streamerList[i]) {
      validIdx = i;
      break;
    }
  }

  if (i > maxStreamerIdx) {
    validIdx = -1;
  }

  return validIdx;
}

int appInstance::parseMessage(sageMessage &msg, int fsIdx)
{
  char *msgData = (char *)msg.getData();

  switch (msg.getCode()) {
  case SAIL_INIT_MSG : {
    if (initialized) {
      int winID;
      sageNwConfig nwCfg;
      sscanf(msgData, "%d %d %d %d", &winID, &nwCfg.rcvBufSize, &nwCfg.sendBufSize, &nwCfg.mtuSize);

      streamerList[fsIdx]->setWinID(winID);
      //streamerList[fsIdx]->setNwConfig(nwCfg);
    }
    break;
  }

  case SAIL_CONNECT_TO_RCV : {
    //SAGE_PRINTLOG("connect to rcv");
    if (initialized) {
      //SAGE_PRINTLOG("appInstance::%s() : SAIL_CONNECT_TO_RCV has received(forwarded from sageBridge) [%s]\n", __FUNCTION__, msgData);
      streamerList[fsIdx]->initNetworks(msgData);
    }
    break;
  }

  case SAIL_CONNECT_TO_RCV_PORT : {
    //SAGE_PRINTLOG("connect to rcv");
    if (initialized) {
      //SAGE_PRINTLOG("appInstance::%s() : SAIL_CONNECT_TO_RCV_PORT has received(forwarded from sageBridge) [%s]\n", __FUNCTION__, msgData);
      streamerList[fsIdx]->initNetworks(msgData, true);
    }
    break;
  }

  case SAIL_INIT_STREAM : {
    //SAGE_PRINTLOG("appInstance::%s() : instID %d. SAIL_INIT_STREAM. fsIdx %d", __FUNCTION__, instID, fsIdx);
    if (initialized) {
      streamerList[fsIdx]->storeStreamConfig(msgData);

      if ( sConfig.asyncUpdate ) {
        sageMessage resendMsg;
        resendMsg.init(64);
        resendMsg.setCode(SAIL_RESEND_FRAME);
        resendMsg.setClientID(sailClient);

        //            for ( int i=0; i<2; ++i ) {
        if ( shared->msgInf->msgToClient(resendMsg) == -1 ) {
          SAGE_PRINTLOG("appInstance::%s() : failed to send SAIL_RESEND_FRAME\n", __FUNCTION__);
        }
        //            }
        resendMsg.destroy();
      }
    }

    break;
  }

  case APP_QUIT : {
    if (initialized) {
      streamerList[fsIdx]->shutdown();
      delete streamerList[fsIdx];
      streamerList[fsIdx] = NULL;
    }

    if (shared->nodeID == 0) {
      fsList[fsIdx] = -1;
      SAGE_PRINTLOG("clear fs idx");
    }
    break;
  }

  case NOTIFY_APP_SHUTDOWN : {
    //SAGE_PRINTLOG("shutdown all streams");
    shutdownAllStreams();
    break;
  }

  case SAIL_FRAME_RATE : {
    if (initialized) {
      //float frate = atof(msgData);
      //if (nodeNum > 1 && shared->nodeID == 0)
      //   streamerList[fsIdx]->getSyncGroup()->setFrameRate(frate);
      //else
      //   streamerList[fsIdx]->setFrameRate(frate);
    }
    break;
  }

  case SAIL_PERF_INFO_REQ : {
    startPerformanceReport(msg);
    break;
  }

  case SAIL_PERF_INFO_STOP :   {
    reportRate = 0;
    break;
  }
  }

  return 0;
}

void appInstance::fillAppInfo(char *msgStr)
{
  // instID < 0 because that's how the FSM and DIM determine that the app is streamed through sageBridge
  sprintf(msgStr, "%s %d %d %d %d 0 . %d %d %d %d %d %d", appName, x, y, width, height, imageWidth,
          imageHeight, (int)audioOn, (int)sConfig.protocol, sConfig.frameRate, -1*(instID+1));
}

bool appInstance::isActive()
{
  bool active = false;
  for (int j=0; j<fsList.size(); j++) {
    if (initialized && streamerList[j])
      active = true;
    else if (shared->nodeID == 0 && fsList[j] >= 0)
      active = true;
  }

  return active;
}

int appInstance::shutdownAllStreams()
{
  for (int i=0; i<=maxStreamerIdx; i++) {
    if (shared->nodeID == 0 && fsList[i] >= 0)
      shared->eventQueue->sendEvent(EVENT_APP_SHUTDOWN, fsList[i]);

    if (streamerList[i]) {
      streamerList[i]->shutdown();
      delete streamerList[i];
      streamerList[i] = NULL;
    }
  }

  return 0;
}
