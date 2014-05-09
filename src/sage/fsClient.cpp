/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: fsClient.cpp - the base class of Free Space Manager Clients
 *          including the modules to communicate with Free Space Manager
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

#include "fsClient.h"

int fsClient::init(int port)
{
  client = new QUANTAnet_tcpClient_c;

  portNum = port;

  return 0;
}

int fsClient::init(char *config, char *portType)
{
  client = new QUANTAnet_tcpClient_c;

  FILE *fp = fopen(config, "r");

  if (!fp) {
    SAGE_PRINTLOG("fail to open fsClient config file\n");
    return -1;
  }

  char token[TOKEN_LEN];
  while(getToken(fp, token) != EOF) {
    if (strcmp(token, "fsManager") == 0) {
      getToken(fp, token);
      getToken(fp, token);
      fsIp = new char[strlen(token) + 1];
      strcpy(fsIp, token);
    }
    else if (strcmp(token, portType) == 0) {
      getToken(fp, token);
      portNum = atoi(token);
    }
  }

  fclose(fp);

  return 0;
}

fsClient::~fsClient()
{
  if (client)
    delete client;
}

int fsClient::connect(char *ip)
{
  int retVal = -1;

  char sIp[SAGE_IP_LEN];

  if (!ip)
    strcpy(sIp, fsIp);
  else
    strcpy(sIp, ip);

  client->setSockOptions(QUANTAnet_tcpClient_c::READ_BUFFER_SIZE, 65536);
  client->setSockOptions(QUANTAnet_tcpClient_c::WRITE_BUFFER_SIZE, 65536);

  if (client->connectToServer(sIp, portNum) < 0) {
    std::cout << "fsClient::connect : server is not ready" << std::endl;
    return -1;
  }

  //std::cout << "connected to " << ip << ":" << portNum << std::endl;

  return 0;
}

int fsClient::getSelfIP(char *ip)
{
  if (!ip) {
    std::cout << "fsClient : invalid string buffer for self IP" << std::endl;
    return -1;
  }

  client->getSelfIP(ip);
  return 0;
}

int fsClient::sendMessage(sageMessage &msg)
{
  //int dataSize = msg.getSize() + MESSAGE_HEADER_SIZE;
  int dataSize = msg.getBufSize();

  int status = client->write((const char *)msg.getBuffer(), &dataSize,
                             QUANTAnet_tcpClient_c::BLOCKING);
  if (status != QUANTAnet_tcpClient_c::OK) {
    std::cerr << "fail to send the message!" << std::endl;
    std::cerr << "\t status: " << status << std::endl;
    return -1;
  }

  return 0;
}

int fsClient::sendMessage(int code)
{
  return sendMessage(FS_ID, code, 0, 0, NULL);
}

int fsClient::sendMessage(int code, int data)
{
  char token[TOKEN_LEN];
  sprintf(token, "%d", data);
  return sendMessage(FS_ID, code, 0, strlen(token)+1, (void *)token);
}

int fsClient::sendMessage(int code, char* data)
{
  return sendMessage(FS_ID, code, 0, strlen(data)+1, (void *)data);
}

int fsClient::sendMessage(int code, int size, void *data)
{
  return sendMessage(FS_ID, code, 0, size, data);
}

int fsClient::sendMessage(int dst, int code, int data)
{
  char dataStr[TOKEN_LEN];
  sprintf(dataStr, "%d", data);

  return sendMessage(dst, code, 0, strlen(dataStr)+1, (void *)dataStr);
}

int fsClient::sendMessage(int dst, int code, int app, char *data)
{
  return sendMessage(dst, code, app, strlen(data)+1, (void *)data);
}

int fsClient::sendMessage(int dst, int code, int app, int size,
                          void *data)
{
  sageMessage msg;

  if (msg.init(dst, code, app, size, data) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int dataSize = msg.getBufSize();
  int status = client->write((const char *)msg.getBuffer(), &dataSize,
                             QUANTAnet_tcpClient_c::BLOCKING);

  msg.destroy();

  if (status != QUANTAnet_tcpClient_c::OK) {
    std::cerr << "fail to send the message!" << std::endl;
    std::cerr << "\t status: " << status << std::endl;
    return -1;
  }

  return 0;
}

int fsClient::rcvMessage(sageMessage &msg)
{
  char msgSize[MESSAGE_FIELD_SIZE];
  int dataSize = MESSAGE_FIELD_SIZE;

  memset(msgSize, 0, dataSize);

  int status = client->read(msgSize, &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);
  if (status == QUANTAnet_tcpClient_c::OK) {
    msgSize[MESSAGE_FIELD_SIZE-1] = '\0';
    dataSize = atoi(msgSize);
    msg.init(dataSize);
    dataSize = dataSize - MESSAGE_FIELD_SIZE;
    client->read((char *)msg.getBuffer()+MESSAGE_FIELD_SIZE,
                 &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
  }
  else if (status == QUANTAnet_tcpClient_c::NON_BLOCKING_HAS_NO_DATA)
    return 0;
  else
    return -1;

  return dataSize;
}

int fsClient::rcvMessageBlk(sageMessage &msg)
{
  char msgSize[MESSAGE_FIELD_SIZE];
  int dataSize = MESSAGE_FIELD_SIZE;

  memset(msgSize, 0, dataSize);

  int status = client->read(msgSize, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
  if (status != QUANTAnet_tcpClient_c::OK) {
    //SAGE_PRINTLOG("can't read messages from fsManager: got %d [%s]", status, msgSize);
#if defined(linux) || defined(__APPLE__)
    sleep(1);
#endif

#ifdef WIN32
    Sleep(1);
#endif
    return -1;
  }

  msgSize[MESSAGE_FIELD_SIZE-1] = '\0';
  dataSize = atoi(msgSize);
  msg.init(dataSize);
  dataSize = dataSize - MESSAGE_FIELD_SIZE;
  client->read((char *)msg.getBuffer()+MESSAGE_FIELD_SIZE,
               &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

  return dataSize;
}
