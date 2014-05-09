/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageTcpModule.h
 * Author : Byungil Jeong, Rajvikram Singh
 * Description:   This is the header file the TCP code used for trasmitting graphics.
 *                The template for the interface is given by the streamProtocol class.
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
 ***************************************************************************************************************************/

#ifndef _SAGE_TCP_MODULE_H
#define _SAGE_TCP_MODULE_H

#include "streamProtocol.h"

/**
 * sageTcpModule
 */
class sageTcpModule : public streamProtocol{
private:
  int serverSockFd;
  struct sockaddr_in localAddr, clientAddr, serverAddr;
  int setSockOpts(int);
  sageBlockPool *returnPlace;
  std::vector<sageBlockGroup*> bufList;

public:
  sageTcpModule();
  int init(sageStreamMode m, int p, sageNwConfig &c);
  int checkConnections(char *msg = NULL, sageApiOption op = 0);
  int connect(char* ip, char *msg = NULL);
  int send(int id, sageBlock *sb, sageApiOption op);

  /**
   * sungwon experimental, swexp
   */
  int sendpixelonly(int id, sageBlockFrame *sb);

  int recv(int id, sageBlock *sb, sageApiOption op);
  int sendControl(int id, int frameID, int configID);
  int sendGrp(int id, sagePixelBlock *sb, int configID);

  /**
   * class sageBlockGroup::readData() followed by sageBlockGroup::updateConfig()
   */
  int recvGrp(int id, sageBlockGroup *sbg);
  int flush(int id, int configID);
  int getRcvSockFd(int id) { return sendList[id]; }

  void setupBlockPool(sageBlockPool *pool, int id = -1) { returnPlace = pool; }
  void setFrameSize(int id, int size) {}
  void resetFrameSize(int id) {}
  void setFrameRate(double rate, int id = -1) {}

  int close();
  ~sageTcpModule();
};

#endif
