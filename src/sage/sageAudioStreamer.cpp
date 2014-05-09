/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioStreamer.cpp - straightforward audio block streaming
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
#include "streamInfo.h"

using namespace std;
sageAudioStreamer::sageAudioStreamer(streamerConfig &conf, int sampleSize, sageAudioCircBuf* buff) :
  buffer(buff), bytesPerSample(sampleSize)
{
  config = conf;
  config.protocol = config.audioProtocol;
  //blockSize = config.blockSize;
  blockSize = buff->getBytesBlock() + BLOCK_HEADER_SIZE;

  if (config.protocol == SAGE_TCP) {
    blockSize = buff->getBytesBlock() + BLOCK_HEADER_SIZE; // sizeof(float) /// if not sampleBuffSize

    std::cout << "block size = " << blockSize << std::endl;
  }
  initializeBlock();
}

int sageAudioStreamer::connectToRcv(sageToken &tokenBuf, bool localPort)
{
  // 26000 1 127.0.0.1 0
  // port number ip id

  char token[TOKEN_LEN];
  tokenBuf.getToken(token);
  rcvNodeNum = atoi(token);

  params = new streamParam[rcvNodeNum];
  for (int i=0; i<rcvNodeNum; i++) {
    char rcvIP[SAGE_IP_LEN];
    tokenBuf.getToken(rcvIP);
    tokenBuf.getToken(token);
    params[i].nodeID = atoi(token);
    params[i].active = false;
    //params[i].sInfo = NULL;

    char regMsg[REG_MSG_SIZE];
    sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d %d", config.streamType, winID,
            config.nodeNum, blockSize, config.syncType, (int)config.sampleFmt,
            config.samplingRate, config.channels, config.framePerBuffer,  config.frameRate, config.audioKeyFrame);
    std::cout << "winID : " << winID << std::endl;

    cout << "------------> " << regMsg << endl;

    params[i].rcvID = nwObj->connect(rcvIP, regMsg);
  }
  std::cout << rcvNodeNum <<      " connections are established" << std::endl;

  return 0;
}


int sageAudioStreamer::sendControlBlock(int flag, int cond)
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
      // HYEJUNG NEED
      //sageAudioBlock ctrlBlock(blockSize); // ?????
      sageAudioBlock ctrlBlock; // ?????
      ctrlBlock.setFlag(flag);
      ctrlBlock.setFrameID(frameID);
      ctrlBlock.updateBufferHeader();

      int dataSize = nwObj->send(params[j].rcvID, &ctrlBlock, SAGE_BLOCKING);
      if (dataSize > 0)
        totalBandWidth += dataSize;
      else {
        SAGE_PRINTLOG("sageAudioStreamer::sendControlBlock : fail to send control block\n");
        return -1;
      }
    }
  }

  return 0;
}

void sageAudioStreamer::setNwConfig(sageNwConfig &nc)
{
  nwCfg = nc;

  if (config.protocol == SAGE_UDP) {
    blockSize = buffer->getBytesBlock() + BLOCK_HEADER_SIZE; // sizeof(float) /// if not sampleBuffSize
    std::cout << "block size = " << blockSize << std::endl;
    // HYEJUNG CHECK
  }
}

int sageAudioStreamer::sendAudioBlock()
{
  // std::cerr << "SAGE audio streamer " << config.rank << " : init Streams" << std::endl;
  // initBlock.flag = SAGE_INIT_BLOCK;

  /*aBlock.setSampleFormat(config.sampleFmt);
    aBlock.setSampleRate(config.samplingRate);
    aBlock.setChannel(config.channels);
    aBlock.setFramePerBuffer(config.framePerBuffer);
    aBlock.setBytesPerSample(bytesPerSample);
  */

  for(int i=0; i<rcvNodeNum; i++) {

    if (params[i].active) {
      //audioBlock.setTileID(sInfo->tileID);
      aBlock.setNodeID(params[i].nodeID);
      //audioBlock.updateBufferHeader(sInfo->infoString);
      //std::cout << "sending block header " << (char *)block->getBuffer() << std::endl;

      int dataSize = nwObj->send(params[i].rcvID, &aBlock, SAGE_BLOCKING);

      //std::cout << "frame  : " << aBlock.getFrameID() << " graphic frame : " << aBlock.getgFrameID() << " " << dataSize << std::endl;


      if (dataSize > 0) {
        totalBandWidth += dataSize;
        //cout << "------> datasize : " << dataSize << endl;
      }
      else {
        SAGE_PRINTLOG("sageAudioStreamer::sendAudioBlock : fail to send audio block\n");
        return -1;
      }
    }
  }

  return 0;

}

int sageAudioStreamer::streamAudioData()
{
  if (rcvNodeNum < 1) {
    SAGE_PRINTLOG("sageAudioStreamer::streamAudioData : No Active Streams\n");
    return -1;
  }

  bool flag = true;

  //std::cout << config.rank << " stream frame " << frameID << std::endl;

  aBlock.setFrameID(buffer->getReadIndex());

  audioBlock *bufferBlock = buffer->readBlock();
  if(bufferBlock == NULL) return 0;

  if(bufferBlock->reformatted != 1) return 0;
  char* tempbuff = aBlock.getAudioBuffer();
  memcpy(tempbuff,  bufferBlock->buff, buffer->getBytesBlock());

  aBlock.setgFrameID(bufferBlock->gframeIndex);

  aBlock.updateBufferHeader();

  int cnt = 0;

  if (sendAudioBlock() < 0)
    return -1;

  //std::cout << "frame " << frameID << " transmitted" << std::endl;

  if (sendControlBlock(SAGE_STOP_BLOCK, INACTIVE_CONNECTION) < 0)
    return -1;
  frameID++;
  bufferBlock->reformatted = 0;
  buffer->updateReadIndex();

  /*
    if (streamTimer.getTimeUS() < 25000)
    sage::usleep(25000 - (int)streamTimer.getTimeUS());
    streamTimer.reset();
  */

  //if (sendControlBlock(SAGE_UPDATE_BLOCK, ALL_CONNECTION) < 0)
  //   return -1;
  return 0;
}

int sageAudioStreamer::streamLoop()
{
  //streamTimer.reset();

  while (streamerOn) {
    //std::cout << "pt0" << std::endl;
    //std::cout << "get a new frame" << config.nodeNum << std::endl;

    int syncFrame = 0;
    char *msgStr = NULL;
    if (config.nodeNum > 1) {
      if(config.syncClientObj != NULL)
      {
        syncFrame = config.syncClientObj->waitForSyncData(msgStr);
        if (msgStr) {
          std::cout << "reconfigure " << msgStr << std::endl;
          reconfigureStreams(msgStr);
        }
      }
    }
    else {
      pthread_mutex_lock(reconfigMutex);
      if (msgQueue.size() > 0) {
        msgStr = msgQueue.front();
        reconfigureStreams(msgStr);
        msgQueue.pop_front();
        firstConfiguration = false;
      }
      pthread_mutex_unlock(reconfigMutex);
    }

    if (streamAudioData() < 0) {
      streamerOn = false;
      //std::cout << "pt1" << std::endl;
    }

    //std::cout << "pt1" << std::endl;

  }

  std::cout << "sageAudioStreamer : network thread exit" << std::endl;
  //SAGE_PRINTLOG("sageAudioStreamer : network thread exit");

  return 0;
}

sageAudioStreamer::~sageAudioStreamer()
{
  streamerOn = false;
}

int sageAudioStreamer::initializeBlock()
{
  aBlock.setFlag(SAGE_AUDIO_BLOCK);
  aBlock.setSampleFormat(config.sampleFmt);
  aBlock.setSampleRate(config.samplingRate);
  aBlock.setChannel(config.channels);
  aBlock.setFramePerBuffer(config.framePerBuffer);
  aBlock.setBytesPerSample(bytesPerSample);
  aBlock.initBuffer(blockSize);
  return 0;
}

int sageAudioStreamer::reconfigureStreams(char *msgStr)
{
  for(int j=0; j<rcvNodeNum; j++)
  {
    //params[j].sInfo = NULL;
    params[j].active = true;
  }

  //cout << "------> " << msgStr << endl;
  // 26000 1 127.0.0.1 0
  // port number ip id

  return 0;
}
