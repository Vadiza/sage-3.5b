/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: envInterface.cpp - a part of SAGE Application Interface Library
 *            taking care of communcation with the Free Space Manager.
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

#include "envInterface.h"

envInterface::~envInterface()
{
  if (sailServer)
    delete sailServer;
}

int envInterface::init(sailConfig &conf)
{
  config = conf;

  if (config.master) {

    char *sageDir = getenv("SAGE_DIRECTORY");
    if (!sageDir) {
      SAGE_PRINTLOG("envInterface: cannot find the environment variable SAGE_DIRECTORY");
      SAGE_PRINTLOG("envInterface:     using current and home directories as default");
      //return -1;
    }

    data_path path;
    std::string found = path.get_file("fsManager.conf");
    if (found.empty()) {
      SAGE_PRINTLOG("envInterface: cannot find the file [%s]", "fsManager.conf");
      return -1;
    }
    const char *fsConfigFile = found.c_str();
    SAGE_PRINTLOG("envInterface: using [%s] configuration file", fsConfigFile);


    fsClient::init(atoi(fsConfigFile));

    while(connect(NULL) < 0)
      sage::sleep(1);

    numClients = 0;
    if (config.nodeNum > 1) {
      sailServer = new QUANTAnet_tcpServer_c;
      sailServer->init(config.msgPort + config.appID);
      sailServer->setSockOptions(QUANTAnet_tcpServer_c::READ_BUFFER_SIZE, 65536);
      sailServer->setSockOptions(QUANTAnet_tcpServer_c::WRITE_BUFFER_SIZE, 65536);
    }
    else
      sailServer = NULL;
  }
  else if (config.nodeNum > 1) {
    fsClient::init(config.msgPort + config.appID);

    while(connect(config.masterIP) < 0) {
      sage::sleep(1);
      std::cout << "envInterface::init() : retry to connect to master" << std::endl;
    }
  }

  return 0;
}

int envInterface::init(sailConfig &conf, char *ip, int port)
{
  config = conf;
  if (config.master) {
    std::cout << "try to connect to " << ip << ":" << port << std::endl;
    fsClient::init(port);
    while(connect(ip) < 0)
      sage::sleep(1);

    numClients = 0;
    if (config.nodeNum > 1) {
      sailServer = new QUANTAnet_tcpServer_c;
      sailServer->init(config.msgPort + config.appID);
      sailServer->setSockOptions(QUANTAnet_tcpServer_c::READ_BUFFER_SIZE, 65536);
      sailServer->setSockOptions(QUANTAnet_tcpServer_c::WRITE_BUFFER_SIZE, 65536);
    }
    else
      sailServer = NULL;
  }
  else if (config.nodeNum > 1) {
    fsClient::init(config.msgPort + config.appID);

    while(connect(config.masterIP) < 0) {
      sage::sleep(1);
      std::cout << "envInterface::init() : retry to connect to master" << std::endl;
    }
  }

  return 0;
}

/*
  int envInterface::checkMsg()
  {
  sageMessage msg;

  if (rcvMessage(msg) < 0)
  return -1;
  else {
  if (msg.getCode() < SAIL_MESSAGE   + 100) {
  sailPt->parseMessage(msg);
  msg.destroy();
  }
  else if (msg.getCode() < SAIL_MESSAGE + 200) {
  if (sailPt->config.master)
  distributeMessage(msg);
  sailPt->parseMessage(msg);
  msg.destroy();
  }
  else if (msg.getCode() >= APP_MESSAGE) {
  sailPt->appMsgQueue.push_back(msg);
  pthread_mutex_unlock(sailPt->msgMutex);
  }
  }

  return 0;
  }
*/

int envInterface::checkClients()
{
  if (!config.master) {
    std::cout << "envInterface::checkClients() : this function cannot be called by slaves" <<
      std::endl;
    return -1;
  }

  if (!sailServer)
    return 0;

  QUANTAnet_tcpClient_c *aClient;

  // If a connection is established, accept it and create
  // a new socket for that connection to communicate with.

  aClient = sailServer->checkForNewConnections();

  if (aClient) {
    SAGE_PRINTLOG("sailClient %d connected\n", numClients);

    sailClientList.push_back(aClient);
    numClients++;
  }

  return 0;
}


int envInterface::readClientMsg(sageMessage &msg, int idx)
{
  int dataSize;
  int status;

  if (sailClientList[idx]) {
    char msgSize[MESSAGE_FIELD_SIZE];
    dataSize = MESSAGE_FIELD_SIZE;
    status = sailClientList[idx]->read(msgSize, &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

    if (status == QUANTAnet_tcpClient_c::OK)  {
      msgSize[MESSAGE_FIELD_SIZE-1] = '\0';
      dataSize = atoi(msgSize);
      msg.init(dataSize);
      dataSize = dataSize - MESSAGE_FIELD_SIZE;
      sailClientList[idx]->read((char *)msg.getBuffer()+MESSAGE_FIELD_SIZE,
                                &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

      //SAGE_PRINTLOG("Read in message from client %d\n",idx);
      //std::cout << "messge : " << msg.getCode() << std::endl;
      //      << (char *)msg.getData() << std::endl;
      return 1;
    }
    else if (status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED) {
      std::cout << "connection to client " << idx << " was terminated" << std::endl;
      sailClientList[idx] = NULL;
      return -1;
    }
  }

  return 0;
}

int envInterface::distributeMessage(sageMessage &msg)
{
  if (!config.master) {
    std::cout << "envInterface::distributeMessage() : this function cannot be called by slaves" <<
      std::endl;
    return -1;
  }

  int dataSize = msg.getBufSize();
  for (int i = 0; i < numClients; i++) {
    if (!sailClientList[i])
      continue;

    int status = sailClientList[i]->write((char *)msg.getBuffer(), &dataSize,
                                          QUANTAnet_tcpClient_c::BLOCKING);

    if (status == QUANTAnet_tcpClient_c::OK)  {
      //std::cout << "Send message to sail client " << i << std::endl;
      //std::cout << "message : " << msg.getCode() << " "
      //      << (char *)msg.getData() << std::endl;
    }
    else
      std::cout << "envInterface::distributeMessage() : Fail to send message to sail client " << i << std::endl;
  }

  return 0;
}

int envInterface::distributeMessage(int code)
{
  sageMessage msg;

  if (msg.init(0, code, 0, 0, NULL) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int retVal = distributeMessage(msg);
  msg.destroy();

  return retVal;
}

int envInterface::msgToClient(sageMessage &msg)
{
  if (!config.master) {
    std::cout << "envInterface::msgToClient() : this function cannot be called by slaves" <<
      std::endl;
    return -1;
  }

  int dataSize = msg.getBufSize();
  int cId = msg.getDest();

  if (!sailClientList[cId])
    return -1;

  int status = sailClientList[cId]->write((char *)msg.getBuffer(), &dataSize,
                                          QUANTAnet_tcpClient_c::BLOCKING);

  if (status == QUANTAnet_tcpClient_c::OK)  {
    //std::cout << "Send message to client " << cId << std::endl;
    //std::cout << "messge : " << msg.getCode() << " "
    //      << (char *)msg.getData() << std::endl;
  }
  else
    return -1;

  return 0;
}

int envInterface::msgToClient(int cId, int code)
{
  sageMessage msg;

  if (msg.init(cId, code, 0, 0, NULL) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int retVal = msgToClient(msg);
  msg.destroy();

  return retVal;
}

int envInterface::msgToClient(int cId, int code, int data)
{
  char msgStr[TOKEN_LEN];
  sprintf(msgStr, "%d", data);

  return msgToClient(cId, code, msgStr);
}

int envInterface::msgToClient(int cId, int code, char *data)
{
  sageMessage msg;

  if (msg.init(cId, code, 0, strlen(data)+1, data) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int retVal = msgToClient(msg);
  msg.destroy();

  return retVal;
}
