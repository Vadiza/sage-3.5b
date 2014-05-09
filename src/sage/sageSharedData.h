/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageSharedData.h
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
 * SOFTWARE, EVEN IF 88ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#ifndef SAGE_SHARED_DATA_H_
#define SAGE_SHARED_DATA_H_

#include "sageBase.h"
//#include <sys/time.h>

class sageDisplay;
class sageSyncServer;
class sageSyncClient;
class sageSyncBBServer;
class sageMemory;
class sageEventQueue;
class displayContext;
class streamProtocol;
class messageInterface;

/**
 * class rcvSharedData
 */
class rcvSharedData {
public:
  int nodeID; /**< a SDM ID on which sageDisplayManager is running */
  int bufSize;
  sageEventQueue *eventQueue; /**< event queue */
  sageSyncClient *syncClientObj; /**< created in the sageDisplayManager::init() */

  int nodeNum;

  /**
   * The number of active PDL in a node
   * syncServer won't have to loop until MAX_INST_NUM
   */
  int current_max_inst_num;

  // delay_compensation
#ifdef DELAY_COMPENSATION
  struct timeval localT;
  struct timeval syncMasterT;
  int latency; // in microseconds

  /**
   * syncMaster sends deltaT along with init time. Each node should wait until (deltaT - latency)
   * in Microsecond(usec)
   */
  int deltaT;
#endif

  rcvSharedData();
  ~rcvSharedData();
};

/**
 * class dispSharedData
 */
class dispSharedData : public rcvSharedData {
public:
  displayContext *context;
  sageDisplay   *displayObj; /**< created in the sageDisplayManager::init() */

  dispSharedData() : displayObj(NULL), context(NULL) {}
  ~dispSharedData();
};

/**
 * class bridgeSharedData
 */
class bridgeSharedData : public rcvSharedData {
public:
  streamProtocol *sendObj;
  messageInterface *msgInf;
  bool frameDrop;
  bridgeSharedData() : sendObj(NULL), frameDrop(true), msgInf(NULL) {}
  ~bridgeSharedData();
};

#endif
