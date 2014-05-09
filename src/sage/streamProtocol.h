/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: streamProtocol.h
 * Author : Byungil Jeong, Rajvikram Singh
 * Description: This is the header file for the stream protocol modules of SAGE.
 *       It gives the interface to be provided by all the protocols included.
 * Notes : This is based on the TeraVision network protocol.
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

#ifndef _STREAM_PROTOCOL_H
#define _STREAM_PROTOCOL_H

#include "sageBase.h"

#define REG_MSG_SIZE    128

class sageBlock;
class sagePixelBlock;
class sageBlockPool;
class sageBlockGroup;

// swexp, sendPixelOnly needs this
class sageBlockFrame;

/**
 * sageNwConfig
 */
class sageNwConfig {
public:
  int rcvBufSize;
  int sendBufSize;
  int mtuSize;
  int blockSize;
  int groupSize;
  double maxBandWidth; // maximum data amount that can be sent in a micro-second on the host
  int maxCheckInterval;
  int flowWindow;

  sageNwConfig() : rcvBufSize(8388608), sendBufSize(65536), mtuSize(9000),
                   blockSize(0), groupSize(0), maxBandWidth(1000), maxCheckInterval(1000),
                   flowWindow(5) {}
};

/**
 * streamProtocol
 */
class streamProtocol{

protected:
  sageStreamMode sMode;
  nwProtocol   protocol;
  int rcvPort;
  sageNwConfig config;
  std::vector<int> rcvList;
  std::vector<int> sendList;

public:
  streamProtocol() {}

  int init(sageStreamMode m, int p, sageNwConfig &c);
  // The 1st parameter is the mode of operartion
  // The 2nd parameter is the receiver port. If a receiver cannot be
  // started at this port, a false should be returned.
  // The 3rd parameter is configuration info

  inline nwProtocol getProtocol() { return protocol; }
  inline int getRcvPort() { return rcvPort; }

  inline void setConfig(sageNwConfig c) { config = c; }
  // set network configurations for new network connections
  inline void setConfig(int port, int blockSize = 0, int groupSize = 0, int sendBufSize= 0)
  {
    rcvPort = port;
    if (blockSize > 0)
      config.blockSize = blockSize;
    if (groupSize > 0)
      config.groupSize = groupSize;
    if (sendBufSize > 0)
      config.sendBufSize = sendBufSize;
  }

  virtual void setupBlockPool(sageBlockPool *pool, int id = -1) = 0;
  virtual void setFrameSize(int id, int size) = 0;
  virtual void resetFrameSize(int id) = 0;
  virtual void setFrameRate(double rate, int id = -1) = 0;

  int duplicate(sageApiOption op);

  virtual int close(int id, int mode = -1);

  /**
   *  checking if there is a new sender trying to connect
   * if yes, create a new connection and return the sender ID
   * if no, return -1
   * memory bigger than REG_MSG_SIZE should be allocated to msg
   */
  virtual int checkConnections(char *msg = NULL, sageApiOption op = 0) = 0; // for SAGE_RCV or SAGE_BRIDGE mode


  /**
   *  This function blocks till it connects (or cannot connect)
   * create a connection to a receiver
   * and return the receiver ID
   * the length of string msg should not exceed REG_MSG_SIZE
   */
  virtual int connect(char* ip, char *msg = NULL) = 0; // for SAGE_SEND or SAGE_BRDIGE mode


  virtual int send(int id, sageBlock *sb, sageApiOption op) = 0;

  /**
   * sungwon experimental,
   * when swexp flag is set in the application's conf file,
   * This function is used to send pixels to the wall.
   */
  virtual int sendpixelonly(int id, sageBlockFrame *sb) = 0;

  virtual int recv(int id, sageBlock *sb, sageApiOption op) = 0;

  virtual int sendControl(int id, int frameID, int configID) = 0;

  virtual int sendGrp(int id, sagePixelBlock *sb, int configID) = 0;
  // id : receiver ID, sb : pixel or control Block

  virtual int recvGrp(int id, sageBlockGroup *sbg) = 0;
  // id : sender ID, sbg : vacant block group

  virtual int flush(int id, int configID) = 0;

  virtual int getRcvSockFd(int id) = 0;

  /**
   *  When called, it is expected to close all internal sockets and force any operation
   * (send/recv) to be interrupted. If this function is not called explicitly the destructor is supposed
   * to call it before cleaning up
   */
  virtual int close() = 0;
  virtual ~streamProtocol() {}
};

#endif
