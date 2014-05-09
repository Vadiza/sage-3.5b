/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlockQueue.cpp
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

#include "sageBlock.h"
#include "sageBlockQueue.h"

sageBlockQueue::sageBlockQueue(int len) : full(false), curLen(0), active(true),
                                          pointerNum(0), maxPointerID(0), listLock(NULL), notEmpty(NULL), pixelPassed(false)
{
  queueLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(queueLock, NULL);
  pthread_mutex_unlock(queueLock);
  notFull = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notFull, NULL);

  maxLen = len;
  head = tail = NULL;

  for (int i=0; i<MAX_ENDPOINT_NUM; i++) {
    pointerList[i] = &invalid;
    skipFrame[i] = -1;
    lastFrame[i] = 0;
    lastPixelFrame[i] = 1;
  }

  addReader();
}

int sageBlockQueue::addReader(int id)
{
  pthread_mutex_lock(queueLock);
  pointerList[id] = NULL;
  pointerNum++;
  maxPointerID = MAX(id, maxPointerID);
  pthread_mutex_unlock(queueLock);

  return 0;
}

int sageBlockQueue::removeReader(int id, pthread_mutex_t &lock)
{
  pthread_mutex_lock(queueLock);

  while (pointerList[id]) {
    sageBlockContainer *curEntry = pointerList[id];
    sageBlock *block = curEntry->block;

    curEntry->rcnt--;
    pointerList[id] = curEntry->next;

    if (curEntry->rcnt <= 0) {
      head = curEntry->next;
      curLen--;
      delete curEntry;

      full = (curLen >= maxLen);
      if (!full)
        pthread_cond_signal(notFull);

      if (block) {
        pthread_mutex_lock(&lock);
        block->dereference();
        if (block->getRefCnt() <= 0)
          delete block;
        pthread_mutex_unlock(&lock);
      }
    }
  }

  pointerList[id] = &invalid;
  pointerNum--;
  pthread_mutex_unlock(queueLock);

  return 0;
}

void sageBlockQueue::enque(sageBlock* block)
{
  pthread_mutex_lock(queueLock);

  while (full) {
    //std::cout << "wait not full " << block->frameNum << std::endl;
    pthread_cond_wait(notFull, queueLock);
  }

  sageBlockContainer *newEntry = new sageBlockContainer;
  newEntry->block = block;
  newEntry->rcnt = pointerNum;
  block->setRefCnt(pointerNum);

  if (isEmpty()) {
    head = tail = newEntry;
  }
  else {
    tail->next = newEntry;
    tail = newEntry;
  }

  for (int i=0; i<=maxPointerID; i++) {
    if (!pointerList[i])
      pointerList[i] = newEntry;
  }

  curLen++;
  full = (curLen >= maxLen);

  pthread_mutex_unlock(queueLock);

  if (listLock && notFull) {
    pthread_mutex_lock(listLock);
    pthread_cond_broadcast(notEmpty);
    pthread_mutex_unlock(listLock);
  }
}

sageBlock* sageBlockQueue::deque(int frame, bool activeMode, int id)
{
  pthread_mutex_lock(queueLock);

  if (id > maxPointerID) {
    SAGE_PRINTLOG("sageBlockQueue::deque : invalid pointer ID");
    pthread_mutex_unlock(queueLock);
    return NULL;
  }

  if (isEmpty()) {
    pthread_mutex_unlock(queueLock);
    return NULL;
  }

  sageBlockContainer *curEntry = pointerList[id];
  sageBlock *block = NULL;

  if (curEntry) {
    block = curEntry->block;

    if (block) {
      if (pixelPassed && (lastPixelFrame[id] < block->getFrameID()))
        lastFrame[id] = block->getFrameID() - 1;

      if (activeMode && (block->getFrameID() > frame+1)) {
        pthread_mutex_unlock(queueLock);
        return NULL;
      }

      curEntry->rcnt--;
      pointerList[id] = curEntry->next;

      if (curEntry->rcnt <= 0) {
        if (curEntry != head) {
          SAGE_PRINTLOG("sageBlockQueue::deque : queueing system error");
          pthread_mutex_unlock(queueLock);
          return NULL;
        }
        else {
          head = curEntry->next;
        }

        curLen--;
        delete curEntry;
      }

      switch(block->getFlag()) {
      case SAGE_SKIP_BLOCK:
        skipFrame[id] = block->getFrameID();
        pixelPassed = false;
        break;
      case SAGE_STOP_BLOCK:
      case SAGE_CLEAR_BLOCK:
        active = false;
        pixelPassed = false;
        break;
      case SAGE_PIXEL_BLOCK:
        active = true;
        pixelPassed = true;
        lastPixelFrame[id] = block->getFrameID();
        break;
      }
    }

    full = (curLen >= maxLen);
    if (!full)
      pthread_cond_signal(notFull);
  }
  else {
    //SAGE_PRINTLOG("sageBlockQueue::deque() : no new pixel data for the endpoint");
  }

  pthread_mutex_unlock(queueLock);

  return block;
}

void sageBlockQueue::skipBlocks(int frame, int id)
{
  /*
    pthread_mutex_lock(queueLock);

    if (isEmpty()) {
    pthread_mutex_unlock(queueLock);
    return;
    }

    if (id > maxPointerID) {
    SAGE_PRINTLOG("sageBlockQueue::skipBlocks : invalid pointer ID");
    pthread_mutex_unlock(queueLock);
    return;
    }

    sageBlockContainer *curEntry = pointerList[id];
    sageBlock *block = NULL;

    while (curEntry) {
    block = curEntry->block;
    if (block) {
    lastFrame[id] = block->getFrameID() - 1;
    if (block->getFlag() == SAGE_STOP_BLOCK)
    active = false;
    else
    active = true;

    if (block->getFrameID() >= frame)
    break;
    }

    curEntry->rcnt--;

    if (curEntry->rcnt <= 0) {
    if (curEntry != head) {
    SAGE_PRINTLOG("sageBlockQueue::skipBlocks : queueing system error");
    pthread_mutex_unlock(queueLock);
    return;
    }
    else {
    head = curEntry->next;
    }

    curLen--;
    delete curEntry;
    if (block)
    delete block;
    }

    curEntry = curEntry->next;
    }

    pointerList[id] = curEntry;
    full = (curLen >= maxLen);
    if (!full)
    pthread_cond_signal(notFull);

    pthread_mutex_unlock(queueLock);
  */
}

/*
  sageBlock* sageBlockQueue::searchBlock(int x, int y)
  {
  if (!head)
  return NULL;

  pthread_mutex_lock(queueLock);

  sageBlockContainer *curEntry = head;
  sagePixelBlock *block = NULL;

  while(curEntry) {
  block = (sagePixelBlock *)curEntry->block;
  if (block && (block->getFlag() == SAGE_PIXEL_BLOCK) && block->isInRect(x,y)) {
  pthread_mutex_unlock(queueLock);
  return block;
  }
  curEntry = curEntry->next;
  }

  pthread_mutex_unlock(queueLock);

  return NULL;
  }
*/

void sageBlockQueue::clear()
{
  pthread_mutex_lock(queueLock);
  while(!isEmpty()) {
    sageBlockContainer *curEntry = head;
    head = head->next;
    curLen--;

    if (curEntry) {
      sageBlock *block = curEntry->block;
      if (block)
        delete block;
      delete curEntry;
    }
  }

  tail = NULL;
  pthread_mutex_unlock(queueLock);

  full = false;
  pthread_cond_signal(notFull);
}

void sageBlockQueue::initCondVar(pthread_mutex_t *lock, pthread_cond_t *cond)
{
  listLock = lock;
  notEmpty = cond;
}

sageBlockQueue::~sageBlockQueue()
{
  if (queueLock)
    free(queueLock);

  if (notFull)
    free(notFull);
}


blockQueueList::blockQueueList()
{
  queueList.clear();

  listLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(listLock, NULL);
  pthread_mutex_unlock(listLock);
  notEmpty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notEmpty, NULL);
}

int blockQueueList::addQueue(sageBlockQueue *q)
{
  q->initCondVar(listLock, notEmpty);
  queueList.push_back(q);

  return queueList.size();
}

sageBlockQueue* blockQueueList::operator[](int id)
{
  return queueList[id];
}

blockQueueList& blockQueueList::operator=(blockQueueList &list)
{
  queueList = list.queueList;
  listLock = list.listLock;
  notEmpty = list.notEmpty;

  return *this;
}

int blockQueueList::waitForData()
{
  int streamNum = queueList.size();
  int empty = true;

  pthread_mutex_lock(listLock);

  for (int i=0; i<streamNum; i++) {
    empty = empty && queueList[i]->isEmpty();
  }

  if (empty)
    pthread_cond_wait(notEmpty, listLock);

  pthread_mutex_unlock(listLock);

  return 0;
}

void blockQueueList::stopWaiting()
{
  pthread_mutex_lock(listLock);
  pthread_cond_broadcast(notEmpty);
  pthread_mutex_unlock(listLock);
}

blockQueueList::~blockQueueList()
{
  queueList.clear();
}
