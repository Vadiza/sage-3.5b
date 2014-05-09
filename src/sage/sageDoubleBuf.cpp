/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDoubleBuf.cpp
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

#include "sageDoubleBuf.h"
#include "sageBlock.h"

int sageDoubleBuf::init(sagePixelData **bufs)
{
  bufID = 0;
  blockBuf = bufs;

  bufLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(bufLock, NULL);
  pthread_mutex_unlock(bufLock);
  notFull = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notFull, NULL);
  notEmpty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notEmpty, NULL);

  return 0;
}//End of sageDoubleBuf()

sagePixelData* sageDoubleBuf::getBuffer(int id)
{
  if (id < 0 || id > 1) {
    SAGE_PRINTLOG("sageDoubleBuf::getBuffer : invalid buffer access");
    return NULL;
  }

  return blockBuf[id];
}

sagePixelData *sageDoubleBuf::getFrontBuffer()
{
  return blockBuf[bufID];
}

sagePixelData* sageDoubleBuf::getBackBuffer()
{
  pthread_mutex_lock(bufLock);

  while(queueLen == 0) {
    //std::cerr << "=== waiting for new frames === " << std::endl;
    pthread_cond_wait(notEmpty, bufLock);
  }

  return blockBuf[1-bufID];
}

int sageDoubleBuf::bufSize()
{
  if (blockBuf)
    return (blockBuf[0]->getBufSize() * 2);

  SAGE_PRINTLOG("sageDoubleBuf::bufSize : buffers are not initialized");
  return -1;
}

int sageDoubleBuf::releaseBackBuffer()
{
  queueLen--;
  pthread_mutex_unlock(bufLock);

  if (queueLen == 0) {
    pthread_cond_signal(notFull);
  }

  return 0;
}

int sageDoubleBuf::swapBuffer()
{
  pthread_mutex_lock(bufLock);

  while(queueLen > 0) {
    //std::cerr << "wait not full signal " << std::endl;
    pthread_cond_wait(notFull, bufLock);
  }

  if (queueLen == 0) {
    bufID = 1 - bufID;
    queueLen++;
  }

  pthread_mutex_unlock(bufLock);
  pthread_cond_signal(notEmpty);

  firstFrameReady = true;
  return 0;
}

int sageDoubleBuf::resendBuffer(int num)
{
  pthread_mutex_lock(bufLock);
  queueLen += num;
  pthread_mutex_unlock(bufLock);
  pthread_cond_signal(notEmpty);

  return 0;
}

void sageDoubleBuf::releaseLocks()
{
  pthread_mutex_lock(bufLock);
  queueLen = -1;
  pthread_cond_signal(notEmpty);
  pthread_cond_signal(notFull);
  pthread_mutex_unlock(bufLock);
}

sageDoubleBuf::~sageDoubleBuf()
{
  if (bufLock)
    free(bufLock);

  if (notFull)
    free(notFull);

  if (notEmpty)
    free(notEmpty);

  if (blockBuf) {
    if (blockBuf[0])
      delete blockBuf[0];

    if (blockBuf[1])
      delete blockBuf[1];

    // delete [] blockBuf;
  }
}//End of ~sageDoubleBuf()
