/***************************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module:  sageBlockPool.h
 * Author : Byungil Jeong
 *
 *  Description: Pool (block group and block buffer) of sage pixel blocks
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
 *****************************************************************************************/

#ifndef _SAGE_BLOCK_POOL_H
#define _SAGE_BLOCK_POOL_H

#include "sageBuf.h"
#include "sageConfig.h"

#define MAX_READER_NUM 100

class sagePixelBlock;
class sageBlockGroup;

/**
 * sageBlockPool
 */
class sageBlockPool {
protected:
  bool memAlloc;

public:
  sageBlockPool()  {}
  virtual bool isFull() = 0;
  virtual bool pushBack(sagePixelBlock* block) = 0;
  virtual int returnBG(sageBlockGroup* grp) = 0;
  virtual int returnBlocks(sageBlockGroup* grp) = 0;
  virtual bool garbageCollection() { return false; }
};

// group options
#define GRP_MEM_ALLOC    1
#define GRP_CIRCULAR     2
#define GRP_MULTI_READER 4
#define GRP_USE_IOV      8

#define GROUP_HEADER_SIZE 32

/**
 * class sageBlockGroup can have many sageBlock objects ( grpSize / blkSize )
 * A single group represents a frame
 */
class sageBlockGroup : public sageBlockPool {
protected:
  sageBuf *buf; /**< the length of this buffer is grpSize / blkSize */
  int flag; /**< PIXEL_DATA, CONFIG_UPDATE, END_FRAME */
  int blockSize; /**< size of a single block(sagePixelBlock) */
  int frameSize;
  int frameID;
  int configID;
  int refCnt;
  int deRefCnt;
  char header[GROUP_HEADER_SIZE];

  int blockNum;

#ifdef WIN32
  WSABUF *iovs;
#else
  struct iovec *iovs;
#endif

public:
  static const int PIXEL_DATA;
  static const int CONFIG_UPDATE;
  static const int END_FRAME;

  sageBlockGroup() : buf(NULL), iovs(NULL), frameID(0), flag(sageBlockGroup::END_FRAME),
                     blockNum(0), refCnt(0), deRefCnt(0), frameSize(0) {}
  sageBlockGroup(int blkSize, int grpSize, char opt);
  bool pushBack(sagePixelBlock* block);
  sagePixelBlock* front();
  bool next();
  void resetGrp();
  void resetIdx();
  bool isEmpty();

  bool isFull();
  int returnBG(sageBlockGroup* grp) { return returnBlocks(grp); }
  int returnBlocks(sageBlockGroup* grp);

  int size();
  int getBlockNum();
  sagePixelBlock* operator[] (int idx);

  inline int getDataSize() { return blockSize*getBlockNum(); }
  inline int getFlag()     { return flag; }
  inline void setFlag(int f)    { flag = f; }
  inline int getConfigID() { return configID; }
  inline void setConfigID(int id) { configID = id; }
  inline int getFrameID()  { return frameID; }
  inline void setFrameID(int id) { frameID = id; }
  inline void reference(int cnt) { refCnt += cnt; }
  inline int dereference() { deRefCnt++; return (refCnt - deRefCnt); }
  inline int getRefCnt() { return (refCnt - deRefCnt); }
  inline void setFrameSize(int size) { frameSize = size; }
  inline int getFrameSize() { return frameSize; }

  void clearHeaders();
  void clearBuffers();
  bool updateConfig();
  bool genIOV();
  bool setRefCnt();

  //bool dereferenceAll();
  int sendData(int sockFd);
  int readData(int sockFd);
  int sendDatagram(int sockFd);
  int readDatagram(int sockFd);
  void clear();
  ~sageBlockGroup();
};

// buffer options
#define BUF_MEM_ALLOC     1
#define BUF_MULTI_READER  2
#define BUF_BLOCKING_READ 4
#define BUF_CTRL_GROUP    8
#define INTERVAL_EVAL_COUNT 100

/**
 * sageBlockBuf can have many sageBlockGroup
 */
class sageBlockBuf : public sageBlockPool {
protected:
  sageBuf *buf;
  sageCircBufSingle *dataPool;
  sageRAB *vacantPool;
  sageCircBufSingle *ctrlPool;
  bool waitingData;
  bool multiReader;
  double frameInterval;

  sageTimer frameTimer;
  sageCounter frameCounter;
  bool firstGroup;

  int findNextFrame(int id);

public:
  sageBlockBuf(int bufSize, int grpSize, int blkSize, char opt);
  int entryNum() { return buf->getEntryNum(); }
  sageBlockGroup* getFreeBlocks();
  sageBlockGroup* getCtrlGroup(int flag);
  sageBlockGroup* front(int id = 0, int minFrame = 0);
  bool next(int id = 0);
  inline bool isEmpty() { return buf->isEmpty(); }
  bool pushBack(sageBlockGroup* grp);
  bool pushBack(sagePixelBlock* block) { return false; };

  bool finishFrame();
  int returnBG(sageBlockGroup* grp);
  int returnBlocks(sageBlockGroup* grp);

  bool isFull() { return false; }
  void getBufInfo(char *bufStatus);

  inline bool isWaitingData() { return waitingData; }

  int addReader(streamerConfig &con, int tbn);
  inline void removeReader(int id) { ((sageCircBufMulti *)buf)->removeReader(id); }
  bool clear(sageBuf *groupBuf);
  void releaseLock();

  double getFrameInterval() { return frameInterval; }

  ~sageBlockBuf();
};

#endif
