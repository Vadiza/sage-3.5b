/***************************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module:  sageBuf.h
 * Author : Byungil Jeong
 *
 *  Description: SAGE buffer classes
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

#ifndef _SAGE_BUF_H
#define _SAGE_BUF_H

#include "sageBase.h"

#define MAX_READER_NUM 100

typedef char* sageBufEntry;

/**
 * the abstract class of sage buffers
 */
class sageBuf {
protected:
  int readIdx;
  int writeIdx;
  int entryNum;
  int bufLen;
  int entrySize;
  sageBufEntry *entries;

  pthread_mutex_t bufLock;
  pthread_cond_t notEmpty;
  bool full, empty;

  void initArray(int size);

public:
  sageBuf();
  inline bool isEmpty() { return empty; }
  inline bool isFull()  { return full; }
  inline int  size()    { return bufLen; }
  inline void setEntryNum(int num) { entryNum = num; }
  inline int  getEntryNum() { return entryNum; }
  inline int  getReadIdx()  { return readIdx; }
  inline int  filled()  { return (entryNum*100)/bufLen; }

  sageBufEntry operator[](int idx);

  void reset();
  void resetIdx();
  int getStatus() { return entryNum*100/bufLen; }
  virtual void setBlockingFlag(bool blk) {}
  virtual bool pushBack(sageBufEntry entry) = 0;
  virtual sageBufEntry front() = 0;
  virtual bool next() = 0;
  virtual sageBufEntry front(int id) { return NULL; }
  virtual sageBufEntry next(int id) { return NULL; }
  //virtual int import(sageBuf *buf) = 0;
  virtual void releaseLock() = 0;
  ~sageBuf();
};

/**
 * no concurrent read and write
 */
class sageSerialBuf : public sageBuf {
public:
  sageSerialBuf(int len) { initArray(len); }
  bool pushBack(sageBufEntry entry);
  sageBufEntry front();
  bool next();
  //int import(sageBuf *buf);
  void releaseLock() {}
};

/**
 * sageCircBufSingle
 * instantiated in sageBlockBuf
 */
class sageCircBufSingle : public sageBuf {
public:
  int instID;
protected:
  bool blocking;
  bool active;

public:
  sageCircBufSingle(int len, bool blkRead) : blocking(blkRead), active(true)
  { initArray(len); }
  bool pushBack(sageBufEntry entry);
  sageBufEntry front();
  bool next();
  bool isActive()  { return active; }

  void setBlockingFlag(bool blk) { blocking = blk; }
  void releaseLock();
};

class sageRAB : public sageBuf {
protected:
  int *nextIdx;
  int head;
  int tail;
  int freeHead;
  int freeTail;

public:
  sageRAB(int len);

  bool insert(sageBufEntry entry);
  bool remove(int idx, int prev);
  inline int  start() { return head; }
  void next(int &idx);

  bool next() { return false; }
  sageBufEntry front() { return NULL; }
  bool pushBack(sageBufEntry entry) { return false; }
  void releaseLock() {}
};

class readerInfo {
public:
  int readIdx;
  bool empty;
  bool active;

  readerInfo() : readIdx(0), empty(true), active(false) {}
};

/**
 * circular buffer with multiple readers
 */
class sageCircBufMulti : public sageBuf {
private:
  sageBufEntry front() { return NULL; }
  bool next();
  //int import(sageBuf *buf) { return 0; }
  int distToWriteIdx(int ridx);

protected:
  bool blocking;
  int readerNum;
  int maxReaderID;

  readerInfo readers[MAX_READER_NUM];

public:
  sageCircBufMulti(int len, bool blkRead) : readerNum(0), blocking(blkRead), maxReaderID(-1)
  { initArray(len); }

  int addReader(int id);
  void removeReader(int id);
  inline bool isActive(int id) { return readers[id].active; }

  bool pushBack(sageBufEntry entry);
  sageBufEntry front(int id);
  sageBufEntry next(int id);

  void setBlockingFlag(bool blk) { blocking = blk; }
  void releaseLock() { blocking=false, empty=false, pthread_cond_broadcast(&notEmpty); }
};

#endif

/*
  class sageBufMulti : public sageBuf {
  protected:
  int readerNum;
  readerInfo readers[MAX_READER_NUM];

  public:
  sageBufMulti(int len) : readerNum(1) { initArray(len); }
  bool pushBack(sageBufEntry entry);
  sageBufEntry front();
  bool next();
  int import(sageBuf *buf);
  };
*/
