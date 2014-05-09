/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlockStreamer.cpp - straightforward pixel block streaming to displays
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
//#include "streamInfo.h"
#include "sageBlockPartition.h"
#include "sageBlockPool.h"

sageBlockStreamer::sageBlockStreamer(streamerConfig &conf, int pixSize) : compFactor(1.0),
                                                                          compX(1.0), compY(1.0), doubleBuf(NULL)
{
  config = conf;
  blockSize = config.blockSize;
  bytesPerPixel = pixSize;

  interval = 1000000.0/config.frameRate;

  //int memSize = (config.resX*config.resY*bytesPerPixel + BLOCK_HEADER_SIZE
  //   + sizeof(sageMemSegment)) * 4;
  //memObj = new sageMemory(memSize);

  createDoubleBuffer();
}

int sageBlockStreamer::createDoubleBuffer()
{
  if (doubleBuf) {
    SAGE_PRINTLOG("sageStreamer::createDoubleBuffer : double buffer exist already");
    return -1;
  }

  // can create different type of pixel blocks
  sagePixelData **pixelBuf = new sagePixelData*[2];

  if (config.pixFmt == PIXFMT_DXT5 || config.pixFmt == PIXFMT_DXT5YCOCG) {
    compX = 4.0;
    compY = 4.0;
    compFactor = 16;
  }
  if (config.pixFmt == PIXFMT_DXT) {
    compX = 4.0;
    compY = 4.0;
    compFactor = 16.0;
  }

  // imageviewer(staticApp) doesn't need to have double buffer -S
  pixelBuf[0] = new sageBlockFrame(config.resX, config.resY, bytesPerPixel, compX, compY);
  *pixelBuf[0] = config.imageMap;

  if ( ! config.asyncUpdate ) {
    pixelBuf[1] = new sageBlockFrame(config.resX, config.resY, bytesPerPixel, compX, compY);
  }
  else {
    pixelBuf[1] = pixelBuf[0];
  }
  *pixelBuf[1] = config.imageMap;

  doubleBuf = new sageDoubleBuf;
  doubleBuf->init(pixelBuf);

  return 0;
}

void sageBlockStreamer::setNwConfig(sageNwConfig &nc)
{
  nwCfg = nc;

  if (config.protocol == SAGE_UDP && config.autoBlockSize) {
    double payLoadSize = nwCfg.mtuSize - BLOCK_HEADER_SIZE;
    double pixelNum = payLoadSize/bytesPerPixel*compFactor;

    config.blockX = (int)floor(sqrt(pixelNum));
    if ( (config.pixFmt == PIXFMT_DXT) || (config.pixFmt == PIXFMT_YUV) ||
         (config.pixFmt == PIXFMT_DXT5) || (config.pixFmt == PIXFMT_DXT5YCOCG) ) {
      config.blockX = (config.blockX/4)*4;
    }

    config.blockY = (int)floor(pixelNum/config.blockX);
    if ( (config.pixFmt == PIXFMT_DXT) || (config.pixFmt == PIXFMT_DXT5) || (config.pixFmt == PIXFMT_DXT5YCOCG) ) {
      config.blockY = (config.blockY/4)*4;
    }
    SAGE_PRINTLOG("sageBlockStreamer::setNwConfig> autoBlockSize: %d x %d",
                  config.blockX, config.blockY);
  }

  //   if ( config.swexp ) {
  //   // When stream to SAGENext wall, image doesn't need to be partitioned because there's only one SDM.
  //     partition = 0;
  //   }
  //   else {
  partition = new sageBlockPartition(config.blockX, config.blockY, config.totalWidth, config.totalHeight);

  sageBlockFrame *buf = (sageBlockFrame *)doubleBuf->getBuffer(0);
  buf->initFrame(partition);
  buf = (sageBlockFrame *)doubleBuf->getBuffer(1);
  buf->initFrame(partition);
  blockSize = (int)ceil(config.blockX*config.blockY*bytesPerPixel/compFactor) + BLOCK_HEADER_SIZE;
  partition->initBlockTable();
  //   }

  nwCfg.blockSize = blockSize;
  nwCfg.groupSize = config.groupSize;
  nwCfg.maxBandWidth = (double)config.maxBandwidth/8.0; // bytes/micro-second
  nwCfg.maxCheckInterval = config.maxCheckInterval;  // in micro-second
  nwCfg.flowWindow = config.flowWindow;

  //   if ( config.swexp )
  //     nbg = 0;
  //   else
  nbg = new sageBlockGroup(blockSize, doubleBuf->bufSize(), GRP_MEM_ALLOC | GRP_CIRCULAR);
}

void sageBlockStreamer::setupBlockPool()
{
  if (!nbg) return;
  nwObj->setupBlockPool(nbg);
}

int sageBlockStreamer::sendPixelBlock(sagePixelBlock *block)
{
  if (!partition) {
    SAGE_PRINTLOG("sageBlockStreamer::sendPixelBlock : block partition is not initialized");
    return -1;
  }

  pixelBlockMap *map = partition->getBlockMap(block->getID());

  if (!map) {
    //std::cerr << "---" <<  hostname << " pBlock " << block->getID() << " out of screen" << std::endl;
    nbg->pushBack(block);
    return 0;
  }

  block->setRefCnt(map->count);
  //while(map) {
  block->setFrameID(frameID);
  block->updateBufferHeader();

  while(map) {
    //std::cerr << "---" <<  hostname << " pBlock " << block->getBuffer() << "  sent to " <<
    //   params[map->infoID].rcvID << std::endl;

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

int sageBlockStreamer::sendControlBlock(int flag, int cond)
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
      //SAGE_PRINTLOG("send update block %d", frameID);
      int dataSize = nwObj->sendControl(params[j].rcvID, frameID, configID);

      if (dataSize > 0)
        totalBandWidth += dataSize;
      else if (dataSize < 0) {
        SAGE_PRINTLOG("sageBlockStreamer::sendControlBlock : fail to send control block");
        return -1;
      }

      //SAGE_PRINTLOG("SBS::%s() : sent ctl block to rcv %d, frameID %d, configID %d\n", __FUNCTION__, params[j].rcvID, frameID, configID);
    }
  }

  return 0;
}

int sageBlockStreamer::streamPixelData(sageBlockFrame *buf)
{
  if (streamNum < 1) {
    SAGE_PRINTLOG("sageBlockStreamer::streamPixelData : No Active Streams");
    return -1;
  }

  bool flag = true;
  buf->resetBlockIndex();

  //SAGE_PRINTLOG("%d stream frame %d", config.rank, frameID);

  int cnt = 0;
  while (flag) {
    sagePixelBlock *pBlock = nbg->front();
    nbg->next();
    if (!pBlock) {
      SAGE_PRINTLOG("sageBlockStreamer::streamPixelData : pixel block is NULL");
      continue;
    }
    //SAGE_PRINTLOG("pBlock %s", (char *)pBlock->getBuffer());

    flag = buf->extractPixelBlock(pBlock, config.rowOrd);

    if (sendPixelBlock(pBlock) < 0)
      return -1;
  }

  //std::cerr << "frame " << frameID << " transmitted" << std::endl;

  if (sendControlBlock(SAGE_UPDATE_BLOCK, ALL_CONNECTION) < 0)
    return -1;

  frameID++;
  frameCounter++;

  for (int j=0; j<rcvNodeNum; j++) {
    int dataSize = nwObj->flush(params[j].rcvID, configID);
    if (dataSize > 0) {
      totalBandWidth += dataSize;
    }
    else if (dataSize < 0) {
      SAGE_PRINTLOG("sageBlockStreamer::streamPixelData : fail to send pixel block");
      return -1;
    }
  }

  //if (config.asyncUpdate) {
  //   if (sendControlBlock(SAGE_UPDATE_BLOCK, ALL_CONNECTION) < 0)
  //      return -1;
  //}

  return 0;
}

int sageBlockStreamer::streamLoop()
{
  while (streamerOn) {

    //int syncFrame = 0;
    //      SAGE_PRINTLOG("\n========= wait for a frame ========\n");
    sageBlockFrame *buf = (sageBlockFrame *)doubleBuf->getBackBuffer(); // wait on notEmpty condition
    //      SAGE_PRINTLOG("\n========= got a frame ==========\n");

    /* sungwon experimental */

    /*
      if ( 0 affinityFlag ) {
      #if ! defined (__APPLE__)
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);

      pthread_mutex_lock(&affinityMutex);
      std::list<int>::iterator it;
      for ( it=cpulist.begin(); it!=cpulist.end(); it++) {
      CPU_SET((*it), &cpuset);
      }
      affinityFlag = false; // reset flag
      pthread_mutex_unlock(&affinityMutex);

      if ( pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0 ) {
      perror("pthread_setaffinity_np");
      }
      if ( pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0 ) {
      perror("pthread_getaffinity_np");
      }
      else {
      SAGE_PRINTLOG("SBS::%s() : cpu affinity : ", __FUNCTION__);
      for (int i=0; i<CPU_SETSIZE; i++) {
      if (CPU_ISSET(i, &cpuset)) {
      SAGE_PRINTLOG("%d ", i);
      }
      }
      SAGE_PRINTLOG("\n");
      }
      #endif
      }
    */

    if ( config.swexp ) {
      //buf->updateBufferHeader(frameID, config.resX, config.resY);
      if ( nwObj->sendpixelonly(0, buf) <= 0 ) {
        streamerOn = false;
      }
      else {
        //          SAGE_PRINTLOG("sageBlockStreamer::%s() : frame %d sent \n", __FUNCTION__, frameID);
      }
      doubleBuf->releaseBackBuffer();
      frameID++;
      frameCounter++;
      continue;
    }

    char *msgStr = NULL;
    if (config.nodeNum > 1) {
      config.syncClientObj->sendSlaveUpdate(frameID);
      //SAGE_PRINTLOG("send update %d", config.rank);
      config.syncClientObj->waitForSyncData(msgStr);
      //SAGE_PRINTLOG("receive sync %d", config.rank);
      if (msgStr) {
        //SAGE_PRINTLOG("reconfigure %s", msgStr);
        reconfigureStreams(msgStr);
        //firstConfiguration = false;
      }
    }
    else {
      pthread_mutex_lock(reconfigMutex);
      if (msgQueue.size() > 0) {
        msgStr = msgQueue.front();
        reconfigureStreams(msgStr);
        //SAGE_PRINTLOG("config ID : %d", configID);
        msgQueue.pop_front();
        firstConfiguration = false;
      }
      pthread_mutex_unlock(reconfigMutex);
    }

    if (config.nodeNum == 1)
      checkInterval();

    if (streamPixelData(buf) < 0) {
      streamerOn = false;
    }

    // signal notFull condition
    doubleBuf->releaseBackBuffer();
    //      SAGE_PRINTLOG("releaseBackBuffer() returned");
  }

  // for quiting other processes waiting a sync signal
  if (config.nodeNum > 1) {
    config.syncClientObj->sendSlaveUpdate(frameID);
  }

  SAGE_PRINTLOG("sageStreamer : network thread exit");

  return 0;
}

void sageBlockStreamer::shutdown()
{
  streamerOn = false;
  if (doubleBuf)
    doubleBuf->releaseLocks();

  pthread_join(thId, NULL);
}

sageBlockStreamer::~sageBlockStreamer()
{
  if (doubleBuf)
    delete doubleBuf;

  if (nwObj)
    delete nwObj;
}
