/***************************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module:  sageDoubleBuf.h
 * Author : Byungil Jeong
 *
 *   Description: double buffer of sage pixel blocks for sageStreamer
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
 *****************************************************************************************/

#ifndef _SAGEDOUBLEBUF_H
#define _SAGEDOUBLEBUF_H

#include "sageBase.h"

class sagePixelData;

class sageDoubleBuf {
private:
  int bufID;
  sagePixelData **blockBuf;

  pthread_mutex_t *bufLock;
  pthread_cond_t *notFull;
  pthread_cond_t *notEmpty;

  bool full, empty;
  bool firstFrameReady;
  int queueLen;

public:
  sageDoubleBuf() : full(false), empty(true), queueLen(0), firstFrameReady(false) {}
  int init(sagePixelData **bufs);

  sagePixelData* getBuffer(int id);  // get buffer without checking availability
  sagePixelData* getBackBuffer();    // get the pixel block on which pixel data has been written
  int releaseBackBuffer();
  int bufSize();

  // get the empty block on which application can write safely
  sagePixelData* getFrontBuffer();

  int swapBuffer();
  int resendBuffer(int num);
  void enQueue()  { queueLen++; }
  bool isEmpty()  { return (queueLen == 0); }
  bool isFirstFrameReady() { return firstFrameReady; }
  void releaseLocks();
  ~sageDoubleBuf();
};

#endif
