/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: envInterface.h
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


#ifndef _ENVINTF_H
#define _ENVINTF_H

#include "fsClient.h"
#include "sail.h"

/**
 * class envInterface
 */
class envInterface : public fsClient {
private:
  sailConfig config;
  QUANTAnet_tcpServer_c *sailServer;
  int numClients;
  std::vector<QUANTAnet_tcpClient_c *> sailClientList;

public:
  envInterface() : sailServer(NULL) {}
  ~envInterface();
  int init(sailConfig &conf);
  int init(sailConfig &conf, char *ip, int port);
  int checkClients();
  int readClientMsg(sageMessage &msg, int idx);
  int distributeMessage(sageMessage &msg);
  int distributeMessage(int code);
  int msgToClient(sageMessage &msg);
  int msgToClient(int cId, int code);
  int msgToClient(int cId, int code, int data);
  int msgToClient(int cId, int code, char *data);
  int getNumClients() { return numClients; }

};


#endif
