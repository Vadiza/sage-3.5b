/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: messageInterface.h
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

#ifndef _MSGINTF_H
#define _MSGINTF_H

#include "sage.h"
#include "QUANTAnet_tcp_c.hxx"


class msgInfConfig {
public:
  bool   master;
  char   serverIP[SAGE_IP_LEN];
  int   serverPort;

  msgInfConfig() : serverPort(0), master(true) { serverIP[0] = '\0'; }
};

class messageInterface {
private:
  QUANTAnet_tcpServer_c *msgServer;
  std::vector<QUANTAnet_tcpClient_c *> msgClientList;
  msgInfConfig config;
  int readIdx;
  int maxClientSockFd;
  fd_set clientFds;

  /**
   * blocking waiting TCP connections.
   * It just calls waitForConnection() forever
   */
  static void* connectionThread(void *args);
  bool infEnd;
  pthread_t conThreadID;

public:
  /**
   * instantiated in the sageBridge::initMaster()
   */
  messageInterface();
  ~messageInterface();

  /**
   * creates QUANTA TCP server and connectionThread
   */
  int init(msgInfConfig &conf);
  int connect(char *ip, int port);
  int checkClients();

  /**
   * Continusously called by connectionThread
   * push_back QUANTA tcp client object ptr into msgClientList
   */
  int waitForConnection();

  /**
   * sageBridge::msgCheckThread() keeps calling this
   */
  int readMsg(sageMessage *msg, int timeOut = -1);
  // return (clientID+1) : data read, return 0 : no data, return -1 : error
  // timeOut < 0 : blocking read (default)
  // timeOut = 0 : non-blocking read
  // timeOut > 0 : blocking read with time-out in micro second

  int distributeMessage(sageMessage &msg, int *clientList, int clientNum);
  int distributeMessage(int code, int instID, int *clientList, int clientNum);
  int distributeMessage(int code, int instID, int data, int *clientList, int clientNum);
  int distributeMessage(int code, int instID, char *data, int *clientList, int clientNum);
  int msgToClient(sageMessage &msg);
  int msgToClient(int cId, int instID, int code);
  int msgToClient(int cId, int instID, int code, int data);
  int msgToClient(int cId, int instID, int code, char *data);
  int msgToClient(int cId, int instID, int code, const char *data);
  int msgToServer(int instID, int code);
  int msgToServer(int instID, int code, int data);
  int msgToServer(int instID, int code, char *data);
  int getNumClients() { return msgClientList.size(); }
  int shutdown();
};

#endif
