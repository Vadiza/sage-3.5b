/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBuf.cpp
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

#include "sageBuf.h"

sageBuf::sageBuf() : entryNum(0), full(false), empty(true),
                     readIdx(0), writeIdx(0), bufLen(0)
{
  entrySize = sizeof(sageBufEntry);
  pthread_mutex_init(&bufLock, NULL);
  pthread_mutex_unlock(&bufLock);
  pthread_cond_init(&notEmpty, NULL);
}

void sageBuf::reset()
{
  entryNum = 0;
  full = false;
  empty = true;
  resetIdx();
}

void sageBuf::resetIdx()
{
  readIdx = 0;
  writeIdx = 0;
}

void sageBuf::initArray(int size)
{
  bufLen = size;

  if (bufLen > 0) {
    entries = new sageBufEntry[bufLen];
    for (int i=0; i<bufLen; i++)
      entries[i] = NULL;
  }
  else
    SAGE_PRINTLOG("sageBuf::initArray : invalid array length");
}

sageBufEntry sageBuf::operator[](int idx)
{
  if (!entries)
    SAGE_PRINTLOG("sageBuf::operator[] : buffer is NULL");
  else if (idx >= bufLen || idx < 0)
    SAGE_PRINTLOG("sageBuf::operator[] : invalid index %d", idx);
  else
    return entries[idx];

  return NULL;
}

sageBuf::~sageBuf()
{
  if (entries)
    delete [] entries;
}

bool sageSerialBuf::pushBack(sageBufEntry entry)
{
  bool retVal = true;

  pthread_mutex_lock(&bufLock);
  if (full) {
    retVal = false;
  }
  else {
    entries[writeIdx++] = entry;
    empty = false;

    if (writeIdx >= bufLen)
      full = true;

    entryNum++;
    retVal = true;
  }
  pthread_mutex_unlock(&bufLock);

  return retVal;
}

sageBufEntry sageSerialBuf::front()
{
  if (empty)
    return NULL;

  return entries[readIdx];
}

bool sageSerialBuf::next()
{
  if (empty) {
    SAGE_PRINTLOG("sageSerialBuf::next : buffer is empty");
    return false;
  }

  entries[readIdx] = NULL;
  readIdx++;
  if (readIdx == writeIdx)
    empty = true;

  entryNum--;

  return true;
}

/*
  int sageSerialBuf::import(sageBuf *buf)
  {
  if (full) {
  SAGE_PRINTLOG("sageSerialBuf::import : buffer is full");
  return -1;
  }

  sageSerialBuf *sBuf = (sageSerialBuf *)buf;

  int importSize = 0, retVal = -1;
  if ((bufLen-writeIdx) < sBuf->entryNum) {
  importSize = bufLen-writeIdx;
  sBuf->readIdx = importSize;
  writeIdx = bufLen;
  full = true;
  retVal = importSize;
  }
  else {
  importSize = sBuf->entryNum;
  sBuf->readIdx = 0;
  writeIdx += importSize;
  }

  entryNum += importSize;
  void *writePt = (void *)&entries[writeIdx];
  void *readPt = (void *)&sBuf->entries[sBuf->readIdx];

  memcpy(writePt, readPt, importSize*entrySize);

  return retVal;
  }
*/

bool sageCircBufSingle::pushBack(sageBufEntry entry)
{
  if (full) {
    SAGE_PRINTLOG("sageCircBufSingle::pushBack : buffer is full");
    return false;
  }

  pthread_mutex_lock(&bufLock);
  entries[writeIdx] = entry;

  writeIdx++;
  if (writeIdx >= bufLen)
    writeIdx = writeIdx - bufLen;

  if (readIdx == writeIdx)
    full = true;

  if (blocking) {
    empty = false;
    pthread_cond_signal(&notEmpty);
  }
  else
    empty = false;

  entryNum++;
  pthread_mutex_unlock(&bufLock);

  return true;
}

void sageCircBufSingle::releaseLock()
{
  blocking = false;
  empty = false;
  active = false;
  pthread_cond_signal(&notEmpty);
}

sageBufEntry sageCircBufSingle::front()
{
  if (blocking) {
    pthread_mutex_lock(&bufLock);
    while(empty) {
      pthread_cond_wait(&notEmpty, &bufLock);
    }
    pthread_mutex_unlock(&bufLock);
  }
  else if (empty)
    return NULL;

  return entries[readIdx];
}

bool sageCircBufSingle::next()
{
  if (empty) {
    return false;
  }

  pthread_mutex_lock(&bufLock);
  entries[readIdx] = NULL;
  while(!empty && !entries[readIdx]) {
    readIdx++;
    if (readIdx >= bufLen)
      readIdx = readIdx - bufLen;

    if (readIdx == writeIdx)
      empty = true;
  }

  entryNum--;
  full = false;
  pthread_mutex_unlock(&bufLock);

  return true;
}

sageRAB::sageRAB(int len) : head(-1), tail(-1), freeHead(0), freeTail(len-1)
{
  initArray(len);
  nextIdx = new int[len];
  for (int i=0; i<len; i++)
    nextIdx[i] = i+1;
  nextIdx[len-1] = -1;
}

void sageRAB::next(int &idx)
{
  if (idx < 0 || idx >= bufLen) {
    SAGE_PRINTLOG("sageRAB::next : invalid index");
    return;
  }

  pthread_mutex_lock(&bufLock);
  idx = nextIdx[idx];
  pthread_mutex_unlock(&bufLock);
}

bool sageRAB::insert(sageBufEntry entry)
{
  bool flag = true;

  if (freeHead > -1) {
    pthread_mutex_lock(&bufLock);
    entries[freeHead] = entry;

    if (tail == -1) {
      tail = head = freeHead;
    }
    else {
      nextIdx[tail] = freeHead;
      tail = freeHead;
    }

    freeHead = nextIdx[freeHead];
    if (freeHead < 0)
      freeTail = -1;

    nextIdx[tail] = -1;
    pthread_mutex_unlock(&bufLock);

    entryNum++;
  }
  else {
    SAGE_PRINTLOG("sageRAB::insert : buffer is full");
    flag = false;
  }

  return flag;
}

bool sageRAB::remove(int idx, int prev)
{
  pthread_mutex_lock(&bufLock);
  if (idx == head) {
    head = nextIdx[idx];
    if (idx == tail)
      tail = -1;
  }
  else {
    if (idx == tail) {
      tail = prev;
    }
    nextIdx[prev] = nextIdx[idx];
  }

  nextIdx[idx] = -1;
  entries[idx] = NULL;

  if (freeHead < 0) {
    freeHead = freeTail = idx;
  }
  else {
    nextIdx[freeTail] = idx;
    freeTail = idx;
  }
  pthread_mutex_unlock(&bufLock);

  entryNum--;

  return true;
}

int sageCircBufMulti::addReader(int id)
{
  pthread_mutex_lock(&bufLock);
  readers[id].readIdx = readIdx;
  readers[id].empty = empty;
  readers[id].active = true;
  maxReaderID = MAX(id, maxReaderID);
  pthread_mutex_unlock(&bufLock);

  return 0;
}

bool sageCircBufMulti::pushBack(sageBufEntry entry)
{
  if (full) {
    SAGE_PRINTLOG("sageCircBufMulti::pushBack : buffer is full");
    return false;
  }

  pthread_mutex_lock(&bufLock);
  entries[writeIdx] = entry;

  writeIdx++;
  if (writeIdx >= bufLen)
    writeIdx = writeIdx - bufLen;

  if (readIdx == writeIdx)
    full = true;

  empty = false;
  for (int i=0; i<=maxReaderID; i++) {
    if (readers[i].active)
      readers[i].empty = false;
  }

  if (blocking) {
    pthread_cond_broadcast(&notEmpty);
  }

  entryNum++;
  pthread_mutex_unlock(&bufLock);

  return true;
}

sageBufEntry sageCircBufMulti::front(int id)
{
  if (!readers[id].active)
    return NULL;

  if (blocking) {
    pthread_mutex_lock(&bufLock);
    while(readers[id].empty) {
      pthread_cond_wait(&notEmpty, &bufLock);
    }
    pthread_mutex_unlock(&bufLock);
  }
  else if (readers[id].empty)
    return NULL;

  return entries[readers[id].readIdx];
}

sageBufEntry sageCircBufMulti::next(int id)
{
  if (readers[id].empty || !readers[id].active) {
    return NULL;
  }

  pthread_mutex_lock(&bufLock);
  readers[id].readIdx++;
  if (readers[id].readIdx >= bufLen)
    readers[id].readIdx = readers[id].readIdx - bufLen;

  int dist = distToWriteIdx(readers[id].readIdx);
  if (dist == 0)
    readers[id].empty = true;

  int maxDist = dist;

  for (int i=0; i<=maxReaderID; i++) {
    if (readers[i].active)
      maxDist = MAX(maxDist, distToWriteIdx(readers[i].readIdx));
  }
  pthread_mutex_unlock(&bufLock);

  sageBufEntry retVal = entries[readIdx];

  if (maxDist == dist)
    next();
  else {
    retVal = NULL;
  }

  return retVal;
}

bool sageCircBufMulti::next()
{
  pthread_mutex_lock(&bufLock);
  readIdx++;
  if (readIdx >= bufLen)
    readIdx = readIdx - bufLen;

  if (readIdx == writeIdx) {
    empty = true;
  }
  entryNum--;
  full = false;
  pthread_mutex_unlock(&bufLock);

  return true;
}

int sageCircBufMulti::distToWriteIdx(int ridx)
{
  int dist = 0;

  if (writeIdx >= ridx)
    dist = writeIdx - ridx;
  else
    dist = writeIdx - ridx + bufLen;

  return dist;
}

void sageCircBufMulti::removeReader(int id)
{
  pthread_mutex_lock(&bufLock);
  readers[id].active = false;
  if (blocking && readers[id].empty) {
    readers[id].empty = false;
    pthread_cond_broadcast(&notEmpty);
  }
  pthread_mutex_unlock(&bufLock);
}
