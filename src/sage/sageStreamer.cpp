/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageStreamer.cpp - streaming pixels to displays
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

#include "sageStreamer.h"
#include "sageFrame.h"
#include "streamInfo.h"
#include "sageBlockPartition.h"

sageStreamer::sageStreamer() : params(NULL), streamerOn(true), configID(0),
                               totalBandWidth(0), frameID(1), firstConfiguration(true), timeError(0.0)
{
  //std::cerr << "init config ID " << configID << std::endl;
  msgQueue.clear();

  reconfigMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(reconfigMutex, NULL);
  pthread_mutex_lock(reconfigMutex);
}

sageStreamer::~sageStreamer()
{
  if (params)
    delete params;

  if (reconfigMutex) {
    pthread_mutex_unlock(reconfigMutex);
    free(reconfigMutex);
  }

  msgQueue.clear();
}

int sageStreamer::enqueMsg(char *data)
{
  if (!data) {
    SAGE_PRINTLOG("sageStreamer::enqueMsg : string is NULL");
    return -1;
  }

  char *msgStr = new char[strlen(data)+1];
  strcpy(msgStr, data);

  /* lock acquired at the constructor. So the first configuration will only unlock the mutex */
  if (!firstConfiguration) {
    pthread_mutex_lock(reconfigMutex);
  }
  //SAGE_PRINTLOG("enque stream info");
  msgQueue.push_back(msgStr);
  pthread_mutex_unlock(reconfigMutex);
  //SAGE_PRINTLOG("sageStreamer::%s() : msg [%s]\n", __FUNCTION__, msgStr);

  return 0;
}

int sageStreamer::initNetworks(char *data, bool localPort)
{
  char token[TOKEN_LEN];
  sageToken tokenBuf(data);
  tokenBuf.getToken(token);

  //SAGE_PRINTLOG("connection info %s", data );

  int rcvPort = atoi(token) + (int)config.protocol;

  sageTcpModule *tcpObj;
  sageUdpModule *udpObj;

  switch (config.protocol) {
  case SAGE_TCP :
    tcpObj = new sageTcpModule;
    tcpObj->init(SAGE_SEND, rcvPort, nwCfg);
    nwObj = (streamProtocol *)tcpObj;
    SAGE_PRINTLOG("sageStreamer::initNetworks : initialize TCP object");
    break;
  case SAGE_UDP :
    udpObj = new sageUdpModule;
    udpObj->init(SAGE_SEND, rcvPort, nwCfg);
    nwObj = (streamProtocol *)udpObj;
    SAGE_PRINTLOG("sageStreamer::initNetworks : initialize UDP object");
    if (config.groupSize > 65536) {
      SAGE_PRINTLOG("sageStreamer::initNetworks : block group size can't exceed 64KB for UDP");
      //return -1;
    }
    break;
  case LAMBDA_STREAM :
    break;
  default :
    std::cerr << "sageStreamer::initNetworks() - error : no network protocol specified" << std::endl;
    return -1;
    break;
  }

  SAGE_PRINTLOG("sageStreamer : network object was initialized successfully");

  connectToRcv(tokenBuf, localPort);
  setupBlockPool();
  nwObj->setFrameRate((double)config.frameRate);
  streamTimer.reset();

  if (pthread_create(&thId, 0, nwThread, (void*)this) != 0) {
    SAGE_PRINTLOG("sageBlockStreamer : can't create nwThread");
  }

  return 0;
}

int sageStreamer::connectToRcv(sageToken &tokenBuf, bool localPort)
{
  char token[TOKEN_LEN];
  tokenBuf.getToken(token);

  int temp = atoi(token);

  //
  // SAGENext's fsManager will send -1 for rcvNodeNum
  //
  if (temp == -1) {
    // then this means SAGENext
    rcvNodeNum = 1; // There always is only one receiver in SAGENext
    config.swexp = 1; // this will cause sageTcpModule::sendpixelonly() called for actual streaming
    SAGE_PRINTLOG("sageStreamer::connectToRcv() : SAGENext detected !!");
  }

  //
  // normal SAGE operation
  //
  else {
    rcvNodeNum = temp;
  }
  //rcvNodeNum = atoi(token);

  //SAGE_PRINTLOG("sageStreamer::%s() : will connect to %d receivers. protocol %d, tokenBuf [%s]\n", __FUNCTION__, rcvNodeNum, nwObj->getProtocol(), tokenBuf.getBuffer());

  params = new streamParam[rcvNodeNum];
  for (int i=0; i<rcvNodeNum; i++) {
    char rcvIP[SAGE_IP_LEN];
    tokenBuf.getToken(rcvIP);

    if (localPort) {
      tokenBuf.getToken(token);

      /**
       * THIS F*CKING BUG TOOK ME NEARLY 20 HOURS
       */
      //int port = atoi(token) + (int)config.protocol;
      int port = atoi(token) + nwObj->getProtocol();

      nwObj->setConfig(port);
      //SAGE_PRINTLOG("sageStreamer::%s() : rcv %d/%d; because of SAIL_CONNECT_TO_RCV_PORT, localPort is on. port %d. protocol %d\n", __FUNCTION__, i+1, rcvNodeNum, port, nwObj->getProtocol());
    }

    tokenBuf.getToken(token);
    params[i].nodeID = atoi(token);
    params[i].active = false;

    char regMsg[REG_MSG_SIZE];
    sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
            config.streamType,
            config.frameRate,
            winID,
            config.groupSize,
            blockSize,
            config.nodeNum,
            (int)config.pixFmt,
            config.blockX,
            config.blockY,
            config.totalWidth,
            config.totalHeight,
            (int)config.asyncUpdate,
            config.fromBridgeParallel);

    SAGE_PRINTLOG("sageStreamer::%s() : connecting to receiver %d/%d. [%s:%d]\n", __FUNCTION__, i+1, rcvNodeNum, rcvIP, nwObj->getRcvPort());

    params[i].rcvID = nwObj->connect(rcvIP, regMsg);

    if (params[i].rcvID >= 0)
      SAGE_PRINTLOG("ageStreamer::%s() : Rcv %d/%d; Connected using protocol %d to %s:%d", __FUNCTION__, i+1, rcvNodeNum, nwObj->getProtocol(), rcvIP, nwObj->getRcvPort());
    else
      SAGE_PRINTLOG("sageStreamer::%s() : Failed to connect to %s:%d", __FUNCTION__, rcvIP, nwObj->getRcvPort());
  }

  SAGE_PRINTLOG("sageStreamer::%s() : %d connections are established.", __FUNCTION__, rcvNodeNum);

  return 0;
}

int sageStreamer::reconfigureStreams(char *msgStr)
{
  if (!partition) {
    SAGE_PRINTLOG("sageStreamer::reconfigureStreams : block partition is not initialized");
    return -1;
  }
  partition->clearBlockTable();

  for(int j=0; j<rcvNodeNum; j++) {
    params[j].active = false;
  }

  if (config.bridgeOn) {
    //SAGE_PRINTLOG("config message %s", msgStr);
    //SAGE_PRINTLOG("sageStreamer::%s() : sageBridge is on. msg [%s]\n", __FUNCTION__, msgStr);
    bStreamGrp.parseMessage(msgStr);
    streamNum = bStreamGrp.streamNum;
  }
  else {
    // parsing the information of application window on tiled display
    sGrp.parseMessage(msgStr);
    sageRect imageRect(0, 0, config.totalWidth, config.totalHeight);
    sGrp.addImageInfo(imageRect);

    streamNum = sGrp.streamNum();
  }

  for(int j=0; j<rcvNodeNum; j++) {
    int blockNum = 0;
    for(int i=0; i<streamNum; i++) {
      if (config.bridgeOn) {
        bridgeStreamInfo *sInfo = &bStreamGrp.streamList[i];

        // pick connections for streaming
        if (sInfo->receiverID == params[j].nodeID) {
          //SAGE_PRINTLOG("map %d %d , %d", j, sInfo->firstID, sInfo->lastID);
          //SAGE_PRINTLOG("sageStreamer::%s() : sageBridge is on. rcv %d should receive block range %d ~ %d\n", __FUNCTION__, j, sInfo->firstID, sInfo->lastID);
          blockNum += partition->setStreamInfo(j, sInfo->firstID, sInfo->lastID);
        }
      }
      else {
        streamInfo *sInfo = sGrp.getStream(i);

        // pick connections for streaming
        if (sInfo->receiverID == params[j].nodeID) {
          blockNum += partition->setStreamInfo(j, sInfo->imgCoord);
        }
      }
    }

    if (blockNum > 0)
      nwObj->setFrameSize(params[j].rcvID, blockNum*blockSize);
    else
      nwObj->setFrameSize(params[j].rcvID, blockSize);
  }

  configID++;

  return 0;
}

void sageStreamer::checkInterval()
{
  double curTime = streamTimer.getTimeUS();
  double maxErr = interval*MAX_INTERVAL_ERROR;
  double err = curTime - (interval - timeError);

  while (err < -maxErr) {
    sage::usleep(SYNC_TIMEOUT);
    curTime = streamTimer.getTimeUS();
    err = curTime - (interval - timeError);
  }

  if (timeError > maxErr)
    timeError = 0.0;
  else
    timeError = err;

  streamTimer.reset();
}

void* sageStreamer::nwThread(void *args)
{
  sageStreamer *This = (sageStreamer *)args;
  This->streamLoop();

  pthread_exit(NULL);
  return NULL;
}
