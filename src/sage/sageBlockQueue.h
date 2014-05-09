/***************************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module:  sageBlockQueue.h
 * Author : Byungil Jeong
 *
 *   Description: Queue for event driven control in a SAGE component
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

#ifndef _SAGE_BLOCK_QUEUE_H
#define _SAGE_BLOCK_QUEUE_H

#include "sageBase.h"

class sageBlock;

class sageBlockContainer {
public:
  sageBlock *block;
  sageBlockContainer *next;
  int rcnt;

  sageBlockContainer() : block(NULL), next(NULL), rcnt(0) {}
};

class sageBlockQueue {
protected:
  sageBlockContainer *head, *tail;
  sageBlockContainer invalid;
  sageBlockContainer *pointerList[MAX_ENDPOINT_NUM];

  pthread_mutex_t *queueLock;
  pthread_cond_t *notFull;

  pthread_mutex_t *listLock;
  pthread_cond_t  *notEmpty;

  int maxLen, curLen, pointerNum, maxPointerID;
  bool full;

  int lastFrame[MAX_ENDPOINT_NUM];
  int skipFrame[MAX_ENDPOINT_NUM];
  bool active;
  int montageID;
  bool pixelPassed;
  int lastPixelFrame[MAX_ENDPOINT_NUM];

public:
  sageBlockQueue(int len = 2);
  ~sageBlockQueue();

  int getLastFrame(int id = 0) { return lastFrame[id]; }
  bool isActive() { return active; }
  void setMonID(int id) { montageID = id; }
  int getMonID() { return montageID; }

  void setMaxLen(int len) { maxLen = len; }
  int addReader(int id = 0);
  int removeReader(int id, pthread_mutex_t &lock);

  sageBlock* deque(int frame, bool activeMode, int id = 0);
  void enque(sageBlock* block);
  //sageBlock* searchBlock(int x, int y);
  void skipBlocks(int frame, int id = 0);

  void initCondVar(pthread_mutex_t *lock, pthread_cond_t *cond);
  bool isEmpty()  { return (head == NULL); }
  void clear();
};

class blockQueueList {
protected:
  std::vector<sageBlockQueue*> queueList;
  pthread_mutex_t *listLock;
  pthread_cond_t *notEmpty;

public:
  blockQueueList();
  int addQueue(sageBlockQueue *q);
  sageBlockQueue* operator[](int id);
  blockQueueList& operator=(blockQueueList &list);
  int waitForData();
  void stopWaiting();
  int size() {  return  queueList.size(); }
  ~blockQueueList();
};

#endif
