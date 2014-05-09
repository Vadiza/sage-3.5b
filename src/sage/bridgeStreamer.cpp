/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: bridgeStreamer.cpp - distributes pixel blocks
 *         to multiple tiled displays
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
 * Direct questions, comments etc about SAGE to sage_users@listserv.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#include "sageStreamer.h"
#include "sageBlockPool.h"
//#include "streamInfo.h"
#include "sageBlockPartition.h"
#include "sageSharedData.h"
#include "messageInterface.h"

bridgeStreamer::bridgeStreamer(streamerConfig &conf, sageBlockBuf *buf, bridgeSharedData *sh)
{
  shared = sh;
  config = conf;
  nwObj = shared->sendObj;
  partition = new sageBlockPartition(config.blockX, config.blockY, config.totalWidth,config.totalHeight);
  partition->initBlockTable();
  blockSize = conf.blockSize;
  //   sailClient = sail;

  blockBuffer = buf;

  frameID = blockBuffer->addReader(config, frameID);


  if (config.streamerID > 0)
    updateType = SAGE_UPDATE_FRAME;
  else
    updateType = SAGE_UPDATE_FOLLOW;

  interval = 1000000.0/config.frameRate;
  firstFrame = true;
  accInterval = 0.0;

  SAGE_PRINTLOG("%s() : sibal 3\n", __FUNCTION__);
}

int bridgeStreamer::initNetworks(char *data, bool localPort)
{
  char token[TOKEN_LEN];
  sageToken tokenBuf(data);
  tokenBuf.getToken(token);

  int rcvPort = atoi(token) + (int)SAGE_UDP;
  if (!nwObj) {
    SAGE_PRINTLOG("bridgeStreamer::initNetworks : network object is NULL");
    return -1;
  }


  nwObj->setConfig(rcvPort, config.blockSize, config.groupSize);
  /*
    SAGE_PRINTLOG("bridgeStreamer : network object was initialized successfully");
    SAGE_PRINTLOG("block size = %d", config.blockSize);
    SAGE_PRINTLOG("group size = %d", nwCfg.groupSize);
  */

  //SAGE_PRINTLOG("bridgeStreamer::%s() : has set the rcvPort of nwObj %d. invoke connectToRcv(%s, %d)\n", __FUNCTION__, rcvPort, data, localPort);

  connectToRcv(tokenBuf, localPort);
  setupBlockPool();
  nwObj->setFrameRate((double)config.frameRate);
  streamTimer.reset();

  if (pthread_create(&thId, 0, nwThread, (void*)this) != 0) {
    SAGE_PRINTLOG("sageBlockStreamer : can't create nwThread");
    return -1;
  }

  SAGE_PRINTLOG("bridgeStreamer::%s() : streaming channel (bridge - displayWall) established. rcvPort %d, [%s]\n", __FUNCTION__, rcvPort, data);

  return 0;
}

void bridgeStreamer::setupBlockPool()
{
  for (int i=0; i<rcvNodeNum; i++) {
    nwObj->setupBlockPool(blockBuffer, params[i].rcvID);
  }
}

int bridgeStreamer::storeStreamConfig(char *msgStr)
{
  //SAGE_PRINTLOG("bridgeStreamer::%s() : SAIL_INIT_STREAM [%s] triggers new msg enqueued at the bridgeStreamer\n", __FUNCTION__, msgStr);

  if (config.nodeNum > 1) {
    if (config.master) {
      config.sGroup->enqueSyncMsg(msgStr);
      config.sGroup->unblockSync();
    }
    //config.syncClientObj->sendSlaveUpdate(1);
  }
  else {
    enqueMsg(msgStr);


    /* if it's imageviewer, it should send message to SAIL so that sender can send a frame
     * to trigger END_FRAME at the bridgeStreamer::streamPixelData()
     * Otherwise, reconfigureStream in the streamLoop() will never be called.
     *
     * use SAIL_RESEND_FRAME message ONLY when asyncUpdate is true.
     */
  }

  return 0;
}

int bridgeStreamer::streamLoop()
{
  //SAGE_PRINTLOG("node %d stream loop started", config.rank);
  SAGE_PRINTLOG("bridgeStreamer::%s() : streamer rank %d, streamLoop has started. tableEntryNum %d, totalBlockNum %d\n", __FUNCTION__, config.rank, partition->tableEntryNum(), partition->getTotalBlockNum());
  while (streamerOn) {
    char *msgStr = NULL;

    if (config.nodeNum > 1){
      //SAGE_PRINTLOG("node %d syncGroup %d send update %d", config.rank, config.syncID, frameID);
      if ( config.syncClientObj ) {
        config.syncClientObj->sendSlaveUpdate(frameID, config.syncID, config.nodeNum, updateType);
        updateType = SAGE_UPDATE_FOLLOW;
        syncMsgStruct *syncMsg = config.syncClientObj->waitForSync(config.syncID);
        if (syncMsg) {
          frameID = syncMsg->frameID;

          //SAGE_PRINTLOG("node %d syncGroup %d receive sync %d", config.rank, config.syncID, frameID);

          if (syncMsg->data) {
            reconfigureStreams(syncMsg->data);
            //SAGE_PRINTLOG("node %d streamer %d reconfigure streams", config.rank, config.streamerID);
          }

          delete syncMsg;
        }
      }
    }
    else {
      pthread_mutex_lock(reconfigMutex);
      //SAGE_PRINTLOG("bridgeStreamer::%s() reconfigMutex acquired. msgQueue size %d\n", __FUNCTION__, msgQueue.size());

      if (msgQueue.size() > 0) {
        msgStr = msgQueue.front();

        SAGE_PRINTLOG("bridgeStreamer::%s() : will reconfigure stream with [%s]\n", __FUNCTION__, msgStr);

        reconfigureStreams(msgStr);
        msgQueue.pop_front();
        delete [] msgStr;
        firstConfiguration = false;
        //SAGE_PRINTLOG("reconfigure bridge streamer");
      }
      pthread_mutex_unlock(reconfigMutex);
    }

    if (config.nodeNum == 1)
      checkInterval();

    /* will not return until END_FRAME */
    int totalBlockNum = streamPixelData();
    if (totalBlockNum < 0) {
      streamerOn = false;
    }
    else {
      //SAGE_PRINTLOG("bridgeStreamer::%s() : sent %d pixel blocks for frame %d\n\n", __FUNCTION__, totalBlockNum, frameID);
    }
  }

  SAGE_PRINTLOG("bridgeStreamer::%s() : network thread exit", __FUNCTION__);

  return 0;
}

int bridgeStreamer::streamPixelData()
{

  if (streamNum < 1) {
    SAGE_PRINTLOG("bridgeStreamer::streamPixelData : No Active Streams");
    return -1;
  }

  // fetch block data from block buffer
  bool loop = true;
  int curFrame = 0;

  int TotalBlockNumCounter = 0;

  while (loop) {
    //      SAGE_PRINTLOG("waiting for data");
    sageBlockGroup *sbg = blockBuffer->front(config.streamerID, frameID);
    //      SAGE_PRINTLOG("get data");

    while (streamerOn && !sbg) {
      blockBuffer->next(config.streamerID);
      sbg = blockBuffer->front(config.streamerID);
    }

    if (!sbg)
      return -1;



    pthread_mutex_lock(reconfigMutex);
    if (msgQueue.size() > 0) {
      char *msgStr = msgQueue.front();

      //SAGE_PRINTLOG("bridgeStreamer::%s() : reconfigure stream\n", __FUNCTION__);

      reconfigureStreams(msgStr);
      msgQueue.pop_front();
      delete [] msgStr;
    }
    pthread_mutex_unlock(reconfigMutex);





    int flag = sbg->getFlag();

    if (flag == sageBlockGroup::PIXEL_DATA) {
      curFrame = sbg->getFrameID();
      for (int i=0; i<sbg->getBlockNum(); i++) {
        sagePixelBlock *block = (*sbg)[i];

        if (!block)
          continue;

        sendPixelBlock(block);
        ++TotalBlockNumCounter; // increment for each pixel block
      }
    }
    else if (flag == sageBlockGroup::END_FRAME) {
      //loop = false;
      //SAGE_PRINTLOG("bridgeStreamer::%s() : END_FRAME flag\n", __FUNCTION__);
    }

    /* this might not work fine with parallel sender */
    if ( !config.fromBridgeParallel  &&  TotalBlockNumCounter >= partition->getTotalBlockNum() ) {
      loop = false;
      //SAGE_PRINTLOG("bridgeStreamer::%s() : totalBlockNum %d reached\n", __FUNCTION__, TotalBlockNumCounter);
    }

    blockBuffer->next(config.streamerID);
  }

  if (sendControlBlock(SAGE_UPDATE_BLOCK, ALL_CONNECTION) < 0)
    return -1;

  //   SAGE_PRINTLOG("controlblock sent\n");

  frameID = curFrame + 1;

  if (firstFrame) {
    firstFrame = false;
    frameTimer.reset();
    accInterval = 0.0;
  }

  /* WHAT THE F*CK IS THIS */
  //   else if (config.frameDrop && config.nodeNum == 1) {
  //      accInterval += blockBuffer->getFrameInterval();
  //      //accInterval += 1000000.0/config.frameRate;
  //
  //      if (frameTimer.getTimeUS() > accInterval+blockBuffer->getFrameInterval()) {
  //         SAGE_PRINTLOG("drop frame %d", frameID);
  //         frameID++;
  //
  //         accInterval = 0.0;
  //         frameTimer.reset();
  //      }
  //   }

  frameCounter++;

  //   SAGE_PRINTLOG("flushing\n");

  for (int j=0; j<rcvNodeNum; j++) {
    if (!params[j].active)
      nwObj->setFrameSize(params[j].rcvID, blockSize);

    int dataSize = nwObj->flush(params[j].rcvID, configID);
    if (dataSize > 0) {
      totalBandWidth += dataSize;
    }
    else if (dataSize < 0) {
      SAGE_PRINTLOG("bridgeStreamer::streamPixelData : fail to flush pixel block group");
      return -1;
    }
  }

  //SAGE_PRINTLOG("bridgeStreamer::%s() : returning. frameID is %d\n", __FUNCTION__, frameID);
  return TotalBlockNumCounter;
}

int bridgeStreamer::sendPixelBlock(sagePixelBlock *block)
{
  if (!partition)
    SAGE_PRINTLOG("bridgeStreamer::sendPixelBlock : block partition is not initialized");
  pixelBlockMap *map = partition->getBlockMap(block->getID());

  if (map)
    block->getGroup()->reference(map->count);

  while(map) {
    params[map->infoID].active = true;
    int dataSize = nwObj->sendGrp(params[map->infoID].rcvID, block, configID);
    if (dataSize > 0) {
      totalBandWidth += dataSize;
    }
    else if (dataSize < 0) {
      SAGE_PRINTLOG("sageBlockStreamer::sendPixelBlock : fail to send pixel block");
      return -1;
    }
    map = map->next;
  }

  return 0;
}

int bridgeStreamer::sendControlBlock(int flag, int cond)
{
  for (int j=0; j<rcvNodeNum; j++) {
    bool sendCond = false;

    switch(cond) {
    case ALL_CONNECTION :
      sendCond = true;
      break;
    case ACTIVE_CONNECTION :
      sendCond = params[j].active;
      break;
    case INACTIVE_CONNECTION :
      sendCond = !params[j].active;
      break;
    }

    if (sendCond) {
      int dataSize = nwObj->sendControl(params[j].rcvID, frameID, configID);
      if (dataSize > 0)
        totalBandWidth += dataSize;
      else if (dataSize < 0) {
        SAGE_PRINTLOG("bridgeStreamer::sendControlBlock : fail to send control block");
        return -1;
      }
    }
  }

  return 0;
}

void bridgeStreamer::shutdown()
{
  streamerOn = false;
  if (config.nodeNum > 1  &&  config.syncClientObj){
    config.syncClientObj->removeSyncGroup(config.syncID);
  }
  blockBuffer->removeReader(config.streamerID);
  pthread_join(thId, NULL);

  for (int j=0; j<rcvNodeNum; j++)
    nwObj->close(params[j].rcvID, SAGE_SEND);

  SAGE_PRINTLOG("< bridgeStreamer shutdown >");
}
