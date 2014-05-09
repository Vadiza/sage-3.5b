/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: messageInterface.cpp - communcation interface among SAGE components.
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

#include "messageInterface.h"

messageInterface::messageInterface() : msgServer(NULL), readIdx(0), maxClientSockFd(0),
                                       infEnd(false)
{
  msgClientList.clear();
  FD_ZERO(&clientFds);
}

messageInterface::~messageInterface()
{
  int numClients = msgClientList.size();

  for (int i=0; i<numClients; i++) {
    if (msgClientList[i]) {
      delete msgClientList[i];
    }
  }

  msgClientList.clear();

  if (msgServer)
    delete msgServer;
}

int messageInterface::init(msgInfConfig &conf)
{
  config = conf;

  if (config.master) {
    msgServer = new QUANTAnet_tcpServer_c;
    msgServer->init(config.serverPort);
    msgServer->setSockOptions(QUANTAnet_tcpServer_c::READ_BUFFER_SIZE, 65536);
    msgServer->setSockOptions(QUANTAnet_tcpServer_c::WRITE_BUFFER_SIZE, 65536);

    if (pthread_create(&conThreadID, 0, connectionThread, (void*)this) != 0) {
      SAGE_PRINTLOG("messageInterface: can't create message checking thread");
    }
  }
  else {
    while (connect(config.serverIP, config.serverPort) < 0) {
      sage::sleep(1);
      SAGE_PRINTLOG("messageInterface::init() : retry to connect to master");
    }
  }

  return 0;
}

int messageInterface::connect(char *ip, int port)
{
  QUANTAnet_tcpClient_c *msgClient = new QUANTAnet_tcpClient_c;
  msgClient->setSockOptions(QUANTAnet_tcpClient_c::READ_BUFFER_SIZE, 65536);
  msgClient->setSockOptions(QUANTAnet_tcpClient_c::WRITE_BUFFER_SIZE, 65536);
  if (msgClient->connectToServer(ip, port) < 0) {
    SAGE_PRINTLOG("messageInterface::connect : server is not ready");
    delete msgClient;
    return -1;
  }

  int clientID = msgClientList.size();
  msgClientList.push_back(msgClient);
  maxClientSockFd = MAX(maxClientSockFd, msgClient->getSocketId());
  FD_SET(msgClient->getSocketId(), &clientFds);

  return clientID;
}

int messageInterface::checkClients()
{
  if (!config.master) {
    SAGE_PRINTLOG("messageInterface::checkClients() : this function cannot be called by slaves");
    return -1;
  }

  QUANTAnet_tcpClient_c *msgClient;
  int numClients = msgClientList.size();

  // If a connection is established, accept it and create
  // a new socket for that connection to communicate with.

  msgClient = msgServer->checkForNewConnections();

  if (msgClient) {
    SAGE_PRINTLOG("msgClient %d connected\n", numClients);

    msgClientList.push_back(msgClient);
    maxClientSockFd = MAX(maxClientSockFd, msgClient->getSocketId());
    FD_SET(msgClient->getSocketId(), &clientFds);
  }

  return numClients;
}

// waiting for a new client to connect
int messageInterface::waitForConnection()
{
  if (!config.master) {
    SAGE_PRINTLOG("messageInterface::waitForConnection() : this function cannot be called by slaves");
    return -1;
  }

  QUANTAnet_tcpClient_c *msgClient;
  int numClients = msgClientList.size();

  // If a connection is established, accept it and create
  // a new socket for that connection to communicate with.

  msgClient = msgServer->waitForNewConnection();

  if (msgClient) {
    SAGE_PRINTLOG("messageInterface::%s() : msgClient %d connected\n", __FUNCTION__, numClients);

    msgClientList.push_back(msgClient);
    maxClientSockFd = MAX(maxClientSockFd, msgClient->getSocketId());
    FD_SET(msgClient->getSocketId(), &clientFds);
  }

  return numClients;
}

void* messageInterface::connectionThread(void *args)
{
  messageInterface *This = (messageInterface *)args;

  while (!This->infEnd) {
    This->waitForConnection();
  }

  SAGE_PRINTLOG("messageInterface::connectionThread : exit");
  pthread_exit(NULL);
  return NULL;
}

int messageInterface::readMsg(sageMessage *msg, int timeOut)
{
  int dataSize, status;
  fd_set readFds = clientFds;

  struct timeval *pTimeOut = NULL;
  if (timeOut >= 0) {
    pTimeOut = new struct timeval;
    pTimeOut->tv_sec = 0;
    pTimeOut->tv_usec = timeOut;
  }

  int ready = select(maxClientSockFd+1, &readFds, NULL, NULL, pTimeOut);
  if (ready < 0) {
    SAGE_PRINTLOG("messageInterface::readMsg : error in waiting update message");
    return -1;
  }
  else if (ready == 0)
    return 0;

  int numClients = msgClientList.size();
  for (int i=readIdx; i<numClients; i++) {
    if (msgClientList[i]) {
      char msgSize[MESSAGE_FIELD_SIZE];
      dataSize = MESSAGE_FIELD_SIZE;

      if (FD_ISSET(msgClientList[i]->getSocketId(), &readFds)) {
        status = msgClientList[i]->read(msgSize, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

        if (status == QUANTAnet_tcpClient_c::OK) {
          msgSize[MESSAGE_FIELD_SIZE-1] = '\0';
          dataSize = atoi(msgSize);
          msg->init(dataSize);
          dataSize = dataSize - MESSAGE_FIELD_SIZE;
          msgClientList[i]->read((char *)msg->getBuffer()+MESSAGE_FIELD_SIZE,
                                 &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

          //if (msg->getCode() != ADD_OBJECT)   // too many printfs otherwise
          //SAGE_PRINTLOG("messageInterface::%s() : received msg code %d [%s] \n", __FUNCTION__, msg->getCode(), (char *)msg->getData());

          readIdx = (i+1) % numClients;
          //SAGE_PRINTLOG("Read in message from client %d\n",i);
          //SAGE_PRINTLOG("messge : " << msg->getCode() << std::endl;
          //      << (char *)msg->getData() << std::endl;
          return i+1;
        }
        else {
          if (status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED)
            SAGE_PRINTLOG("messageInterface::readMsg : connection to client %d was shutdown", i);
          else
            SAGE_PRINTLOG("messageInterface::readMsg : error in the connection to client %d", i);
          FD_CLR(msgClientList[i]->getSocketId(), &clientFds);
          msgClientList[i] = NULL;
          readIdx = (i+1) % numClients;
          return -1;
        }
      }
    }
  }

  readIdx = 0;

  return 0;
}

int messageInterface::distributeMessage(sageMessage &msg, int *clientList, int clientNum)
{
  if (!config.master) {
    SAGE_PRINTLOG("messageInterface::distributeMessage() : this function cannot be called by slaves");
    return -1;
  }

  int dataSize = msg.getBufSize();
  for (int i = 0; i < clientNum; i++) {
    int clientID = clientList[i];
    if (!msgClientList[clientID])
      continue;

    int status = msgClientList[clientID]->write((char *)msg.getBuffer(), &dataSize,
                                                QUANTAnet_tcpClient_c::BLOCKING);

    SAGE_PRINTLOG("messageInterface::%s() : msg code %d [%s] sent to msgClient %d\n", __FUNCTION__, msg.getCode(), (char *)msg.getData(), clientID);

    if (status == QUANTAnet_tcpClient_c::OK)  {
      //SAGE_PRINTLOG("Send message to msg client " << i << std::endl;
      //SAGE_PRINTLOG("message : " << msg.getCode() << " "
      //      << (char *)msg.getData() << std::endl;
    }
    else
      SAGE_PRINTLOG("messageInterface::distributeMessage() : Fail to send message to msg client ", i);
  }

  return 0;
}

int messageInterface::distributeMessage(int code, int instID, int *clientList, int clientNum)
{
  sageMessage msg;

  if (msg.init(instID, code, 0, 0, NULL) < 0) {
    SAGE_PRINTLOG("fail to init the message!");
    return -1;
  }

  int retVal = distributeMessage(msg, clientList, clientNum);
  msg.destroy();

  return retVal;
}

int messageInterface::distributeMessage(int code, int instID, char *data, int *clientList, int clientNum)
{
  sageMessage msg;

  if (msg.init(instID, code, 0, strlen(data)+1, data) < 0) {
    SAGE_PRINTLOG("fail to init the message!");
    return -1;
  }

  int retVal = distributeMessage(msg, clientList, clientNum);
  msg.destroy();

  return retVal;
}

int messageInterface::distributeMessage(int code, int instID, int data, int *clientList, int clientNum)
{
  char msgStr[TOKEN_LEN];
  sprintf(msgStr, "%d", data);

  return distributeMessage(code, instID, msgStr, clientList, clientNum);
}

int messageInterface::msgToClient(sageMessage &msg)
{
  int dataSize = msg.getBufSize();
  int cId = msg.getClientID();

  if (!msgClientList[cId])
    return -1;

  int status = msgClientList[cId]->write((char *)msg.getBuffer(), &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

  // SAGE_PRINTLOG("messageInterface::%s() : msg code %d [%s] sent to msgClient %d\n", __FUNCTION__, msg.getCode(), (char *)msg.getData(), cId);

  if (status == QUANTAnet_tcpClient_c::OK)  {
    //SAGE_PRINTLOG("Send message to client " << cId << std::endl;
    //SAGE_PRINTLOG("messge : " << msg.getCode() << " "
    //      << (char *)msg.getData() << std::endl;
  }
  else
    return -1;

  return 0;
}

int messageInterface::msgToClient(int cId, int instID, int code)
{
  sageMessage msg;

  if (msg.init(instID, code, 0, 0, NULL) < 0) {
    SAGE_PRINTLOG("fail to init the message!");
    return -1;
  }

  msg.setClientID(cId);

  int retVal = msgToClient(msg);
  msg.destroy();

  return retVal;
}

int messageInterface::msgToClient(int cId, int instID, int code, int data)
{
  char msgStr[TOKEN_LEN];
  sprintf(msgStr, "%d", data);

  return msgToClient(cId, instID, code, msgStr);
}

int messageInterface::msgToClient(int cId, int instID, int code, const char *data)
{
  sageMessage msg;

  if (msg.init(instID, code, 0, strlen(data)+1, data) < 0) {
    SAGE_PRINTLOG("fail to init the message!");
    return -1;
  }

  msg.setClientID(cId);

  int retVal = msgToClient(msg);
  msg.destroy();

  return retVal;
}

int messageInterface::msgToClient(int cId, int instID, int code, char *data)
{
  const char * const_data = data;
  msgToClient(cId, instID, code, const_data);
}

int messageInterface::msgToServer(int instID, int code)
{
  sageMessage msg;

  if (msg.init(instID, code, 0, 0, NULL) < 0) {
    SAGE_PRINTLOG("fail to init the message!");
    return -1;
  }

  msg.setClientID(0);

  int retVal = msgToClient(msg);
  msg.destroy();

  return retVal;
}

int messageInterface::msgToServer(int instID, int code, int data)
{
  char msgStr[TOKEN_LEN];
  sprintf(msgStr, "%d", data);

  return msgToServer(instID, code, msgStr);
}

int messageInterface::msgToServer(int instID, int code, char *data)
{
  sageMessage msg;

  if (msg.init(instID, code, 0, strlen(data)+1, data) < 0) {
    SAGE_PRINTLOG("fail to init the message!");
    return -1;
  }

  msg.setClientID(0);

  int retVal = msgToClient(msg);
  msg.destroy();

  return retVal;
}

int messageInterface::shutdown()
{
  infEnd = true;

  if (msgServer) {
    msgServer->close();
    delete msgServer;
  }

  //pthread_join(conThreadID, NULL);

  return 0;
}
