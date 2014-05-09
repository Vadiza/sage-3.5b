/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageStreamer.h
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

#ifndef SAGESTREAMER_H_
#define SAGESTREAMER_H_

#include "sageConfig.h"
#include "sageBlock.h"
#include "streamProtocol.h"
#include "sageDoubleBuf.h"
#include "sageAudioCircBuf.h"
#include "sageSync.h"
#include "streamInfo.h"
#include "sageTcpModule.h"
#include "sageUdpModule.h"

#define ALL_CONNECTION       0
#define ACTIVE_CONNECTION    1
#define INACTIVE_CONNECTION  2

typedef struct {
  int  rcvID;
  int  nodeID;
  bool active;
} streamParam;

class sageBlockFrame;
class sagePixelBlock;
class sageBlockPartition;
class sageBlockGroup;

/**
 * sageStreamer
 */
class sageStreamer {
protected:
  streamerConfig config;
  streamProtocol *nwObj;
  sageNwConfig nwCfg;
  int winID;    // display window ID of the application

  int streamNum;
  int rcvNodeNum;
  bool streamerOn;

  streamParam *params;
  streamGroup sGrp;
  bridgeStreamGroup bStreamGrp;
  sageBlockPartition *partition;

  int blockSize;
  //bool reconfigured;
  //bool timeBlockFlag;
  unsigned long totalBandWidth;
  int frameID;
  int configID;

  bool firstConfiguration;
  pthread_mutex_t *reconfigMutex;
  std::deque<char *> msgQueue;
  sageTimer streamTimer;
  sageCounter frameCounter;
  double interval;
  double timeError;

  pthread_t thId;

  /**
   * invokes streamLoop
   */
  static void* nwThread(void *args);

  //int sendTimeBlock(int frame);

  virtual int streamLoop() = 0;
  virtual int reconfigureStreams(char *msgStr);

  /**
   * called by sageStreamer::initNetworks()
   * it calls nwObj->connect()
   *
   * this function makes connection between sender(SAIL) and receivers(SDM)
   * sends 1KByte of regMsg to receivers
   */
  virtual int connectToRcv(sageToken &tokenBuf, bool localPort = false);
  virtual void setupBlockPool() {}

public:
  sageStreamer();
  virtual ~sageStreamer();

  /**
   * creates sageTcpModule object. member var *nwObj will point it
   * calls connectToRcv()
   * starts nwThread
   */
  virtual int initNetworks(char *data, bool localPort = false);

  void setWinID(int id) { winID = id; }
  //void setSyncClient(sageSyncClient *sClient) { syncClientObj = sClient; }
  int getFrameID() { return frameID; }
  bool isFirstConfig() { return firstConfiguration; }
  int enqueMsg(char *data);
  void checkInterval();
  void setFrameRate(float frate) { //nwObj->setFrameRate((double)frate);
    interval = 1000000.0/frate; }
  syncGroup* getSyncGroup() { return config.sGroup; }

  virtual sageDoubleBuf* getDoubleBuffer() { return NULL; }
  unsigned long getBandWidth() { return totalBandWidth; }
  void resetBandWidth() { totalBandWidth = 0; }
  bool isActive() { return streamerOn; }
  unsigned int getFrameCount() { return frameCounter.getValue(); }
  void resetFrameCounter() { frameCounter.reset(); }
  virtual int regeneratePixelBlocks() { return 0; }

  /**
   * blockSize is calculated here. ceil((blockX * blockY * bytesPerPixel)/compFactor) + BLOCK_HEADER_SIZE
   * sageBlockGroup object is created here
   */
  virtual void setNwConfig(sageNwConfig &nc) { nwCfg = nc; }
  virtual void shutdown() { streamerOn = false; }
  inline bool isStreamerOn() { return streamerOn; }
};

/**
 * sageBlockStreamer
 * sail uses this to stream.
 *
 * This class is responsible for creating doubleBuffer for sending, getting frame from an application, and streaming it to sage
 * the size of double buffer is two times of ceil((resX*resY*bytesPerPixel)/(compX*compY)) + BLOCK_HEADER_SIZE
 */
class sageBlockStreamer : public sageStreamer {
protected:
  sageDoubleBuf  *doubleBuf;
  int bytesPerPixel;
  float compX, compY, compFactor;
  sageBlockGroup *nbg;

  /**
   * keeps calling streamPixelData() with one part of the doubleBuf
   */
  virtual int streamLoop();
  void setupBlockPool();
  int createDoubleBuffer();
  int sendPixelBlock(sagePixelBlock *block);
  int sendControlBlock(int flag, int cond);

  /**
   * keeps calling sendPixelBlock() with sagePixelBlock extracted from nbg->front()
   */
  int streamPixelData(sageBlockFrame *buf);

public:
  sageBlockStreamer(streamerConfig &conf, int pixSize);

  sageDoubleBuf* getDoubleBuffer() { return doubleBuf; }

  /**
   * creates sageBlockPartition object.
   * calls initFrame with the front and the back buffer of the double buffer
   *
   * creates sageBlockGroup object and assigns it to the member variable sageBlockGroup *nbg
   */
  virtual void setNwConfig(sageNwConfig &nc);
  void shutdown();
  ~sageBlockStreamer();
};

class sageBlockBuf;
class bridgeSharedData;

class bridgeStreamer : public sageStreamer {
protected:
  sageBlockBuf *blockBuffer;
  int updateType;
  sageTimer frameTimer;
  double accInterval;
  bool firstFrame;

  //   int sailClient; // to send message to sail
  bridgeSharedData *shared;

  virtual int streamLoop();
  int streamPixelData();
  void setupBlockPool();
  int sendPixelBlock(sagePixelBlock *block);
  int sendControlBlock(int flag, int cond);

public:
  bridgeStreamer(streamerConfig &conf, sageBlockBuf *buf, bridgeSharedData *shared);

  int initNetworks(char *data, bool localPort = false);
  int storeStreamConfig(char *msgStr);
  void shutdown();
  //void setBlockMutex(pthread_mutex_t *mutex) { blockMutex = mutex; }

  ~bridgeStreamer() {}
};

/**
 * sageAudioStreamer
 */
class sageAudioStreamer : public sageStreamer {
protected:
  sageAudioCircBuf* buffer;
  int bytesPerSample;

  virtual int streamLoop();
  virtual int reconfigureStreams(char *msgStr);
  virtual int connectToRcv(sageToken &tokenBuf, bool localPort = false);

  int sendAudioBlock();
  int sendControlBlock(int flag, int cond);
  int streamAudioData();

public:
  sageAudioStreamer(streamerConfig &conf, int sampleSize, sageAudioCircBuf* buff);

  sageAudioCircBuf* getBuffer() { return buffer; }
  virtual void setNwConfig(sageNwConfig &nc);
  ~sageAudioStreamer();
protected:
  sageAudioBlock aBlock;
  int initializeBlock();

};

#endif
