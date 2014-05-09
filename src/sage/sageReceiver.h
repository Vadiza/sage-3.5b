/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageReceiver.h
 * Author : Byungil Jeong, Rajvikram Singh
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

#ifndef SAGERECEIVER_H_
#define SAGERECEIVER_H_

#include "sage.h"

#define STREAM_ACTIVE   1
#define STREAM_INACTIVE 2
#define STREAM_WAITING  3

class sageReceiver;
class sageBlockQueue;
class rcvSharedData;
class streamProtocol;

/**
 * class sageReceiver
 */
class sageReceiver {
protected:
  int instID, senderNum, blockSize;
  streamProtocol *nwObj;
  bool endFlag;
  pthread_t thId;

  /**
   * It just calls virtual member function readData()<BR>
   *
   * This thread is started in the sageAudioReceiver::addStream()<BR>
   * sagePixelReciever's Constructor
   * <BR>
   * So, when a pixelDownloader is instantiated and initiated, sagePixelReceiver starts<BR>
   * then this network thread starts.
   *
   */
  static void* nwReadThread(void *args);
  virtual int readData() = 0; /**< pure virtual member function */

public:
  sageReceiver() : endFlag(false) {}

  int getInstID() { return instID; }
  virtual int addStream(int senderID) = 0; /**< pure virtual member function */
  inline int getSenderNum() {return senderNum;}
};

class sageBlockGroup;
class sageBlockBuf;

/**
 * class streamData
 */
class streamData {
public:
  int senderID;
  int dataSockFd;
  int curFrame;
  bool dataReady;
  sageBlockGroup* bGroup;

  streamData() : curFrame(0), bGroup(NULL), dataReady(false) {}
};

/**
 * class sagePixelReceiver
 */
class sagePixelReceiver : public sageReceiver {
protected:
  rcvSharedData *shared;
  sageBlockBuf *blockBuf; /**< points datapool, ctrlpool, buf <BR> they are vector containing sageBlockGroups */
  streamData *streamList;
  int streamIdx;
  int groupSize;
  int configID;

  pthread_mutex_t streamLock;
  pthread_cond_t  connectionDone;
  bool connecting;
  int curFrame;
  fd_set streamFds;
  int maxSockFd;

  /**
   * Receiving pixel data from an application<BR>
   * Generates EVENT_READ_BLOCK
   */
  int readData();
  int checkStreams();

public:
  /**
   * constructor starts nwReadThread of the super class<BR>
   * This thread just calls virtual function readData()
   */
  sagePixelReceiver(char *msg, rcvSharedData *sh, streamProtocol *obj, sageBlockBuf *buf);

  /**
   * creates EVENT_APP_CONNECTED. This leads invoking fsClient::sendMessage() with the code DISP_APP_CONNECTED
   */
  int addStream(int senderID);
  ~sagePixelReceiver();
};

#endif
