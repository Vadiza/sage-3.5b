/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: fsClient.h
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

#ifndef _FSCLIENT_H
#define _FSCLIENT_H

#include "sage.h"
#include "QUANTAnet_tcp_c.hxx"


#define SOCK_BUF_SIZE (1024*1024*32)
#define READ_BUF_SIZE 4096
#define FS_ID 0

/**
 * class fsClient
 */
class fsClient {
protected:
  QUANTAnet_tcpClient_c *client;
  char *fsIp; /**< fsManager IP address */
  int portNum;

public:
  fsClient() {}
  ~fsClient();
  int init(char *config, char *portType);
  int init(int port);
  int connect(char *ip); /**< connects to sage */
  int getSelfIP(char *ip);
  int sendMessage(sageMessage &msg);
  int sendMessage(int code);
  int sendMessage(int code, int data);
  int sendMessage(int code, char *data); // string data
  int sendMessage(int code, int size, void *data);
  int sendMessage(int dst, int code, int data);
  int sendMessage(int dst, int code, int app, char *data);
  int sendMessage(int dst, int code, int app, int size, void *data); /**< send message to fsManager */
  int rcvMessage(sageMessage &msg);
  int rcvMessageBlk(sageMessage &msg); /**< blocking read of SAGE messages. return the message size */
};


#endif
