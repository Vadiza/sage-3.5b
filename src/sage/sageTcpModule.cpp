/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  sageTcpModule.cpp
 * Author   :   Byungil Jeong, Rajvikram Singh
 * Description:   Code file for the TCP network stream of SAGE
 * Notes   :   The class primarily uses TCP sockets with the options optimized for graphics streaming
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

#include "sageTcpModule.h"
#include "sageBlock.h"
#include "sageBlockPool.h"
#include <fcntl.h>

#include "sageFrame.h"

sageTcpModule::sageTcpModule() : returnPlace(NULL)
{
#ifdef WIN32
  // Initialize Winsock
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2,2), &wsaData);

#endif

  protocol = SAGE_TCP;
  bufList.clear();
}//End of sageTcp:sageTcp

int sageTcpModule::init(sageStreamMode m, int p, sageNwConfig &c)
{
  streamProtocol::init(m, p, c);

  // create the actual network objects
  switch(sMode)   {
  case SAGE_RCV:
  case SAGE_ARCV:
  case SAGE_BRIDGE:   // prepare a server socket
    // create the sockets
    if((serverSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("sageTcpModule::init(): Creating TCP socket failed");
      return 1;
    }

    setSockOpts(serverSockFd);

    // bind to port
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(rcvPort);

    if(bind(serverSockFd, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
      perror("sageTcpModule::init()");
      return 1;
    }

    // put in listen mode
    ::listen(serverSockFd, 5);

    break;

  case SAGE_SEND:
    return 0;
    break;
  default:   return 1;

  } //End of switch(opMode)

  return 0;
}//End of sageTcpModule::init()

int sageTcpModule::checkConnections(char *msg, sageApiOption op)
{
  if(sMode == SAGE_SEND) {
    SAGE_PRINTLOG("sageTcpModule::checkConnection() cannot be used for senders");
    return -1;
  }

  if (op & SAGE_NON_BLOCKING) {
    if (!sage::isDataReady(serverSockFd))
      return -1;
  }

  //accept client connections
  int addrLen;
  addrLen = sizeof(clientAddr);

  int clientSockFd;
  if ((clientSockFd = ::accept(serverSockFd, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1){
    perror("sageTcpModule::checkConnections()");
    return -1;
  }

  //fcntl(clientSockFd, F_SETFL, 0);
  setSockOpts(clientSockFd);

  sendList.push_back(clientSockFd);

  // read registration message
  int idx = sendList.size()-1;

  char regMsg[REG_MSG_SIZE];
  int retVal = sage::recv(clientSockFd, (void *)regMsg, REG_MSG_SIZE);
  if (retVal == -1) {
    return -1;
  }
  else if (retVal == 0) {
#ifdef WIN32
    closesocket(clientSockFd);
#else
    shutdown(clientSockFd, SHUT_RDWR);
#endif

    sendList[idx] = -1;
    return -1;
  }

  //SAGE_PRINTLOG("sageTcpModule::%s() : %d byte of reg msg received from sender  [%s] \n", __FUNCTION__, REG_MSG_SIZE, regMsg);

  if (msg) {
    sprintf(msg, "%d %s", idx, regMsg);
  }
  else {
    SAGE_PRINTLOG("sageTcpModule::checkConnections() : registraion message buffer is NULL");
    return -1;
  }

  return idx;
}//End of sageTcpModule::listen()

int sageTcpModule::connect(char *ip, char *msg)
{
  if(sMode == SAGE_RCV) {
    SAGE_PRINTLOG("sageTcpModule::connect() cannot be used for receivers");
    return -1;
  }

  // create the sockets
  int clientSockFd;
  if((clientSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1)   {
    perror("sageTcpModule::init(): Creating TCP socket failed");
    return -1;
  }

  setSockOpts(clientSockFd);

  // connect to the server
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(ip);
  serverAddr.sin_port = htons(rcvPort);

  if(::connect(clientSockFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
    perror("sageTcpModule::connect()");
    return -1;
  }

  rcvList.push_back(clientSockFd);

  if (config.blockSize > 0 && config.groupSize > 0) {
    SAGE_PRINTLOG("sageTcpModule::connect() : creating sageBlockGroup object with blocksize %d, groupsize %d", config.blockSize, config.groupSize);
    sageBlockGroup *sbg = new sageBlockGroup(config.blockSize, config.groupSize, GRP_USE_IOV);
    bufList.push_back(sbg);
  }

  if (msg) {
    if (sage::send(clientSockFd, msg, REG_MSG_SIZE) == -1) {
      perror("sageTcpModule::send()");
      return -1;
    }
  }

  return (rcvList.size()-1);
}//End of sageTcpModule::connect()

int sageTcpModule::send(int id, sageBlock *sb, sageApiOption op)
{
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::send() : invalid receiver ID %d", id);
    return -1;
  }

  int clientSockFd;
  if (op & SAGE_BACKWARD)
    clientSockFd = sendList[id];
  else
    clientSockFd = rcvList[id];

  if (!sb) {
    SAGE_PRINTLOG("sageTcpModule::send() : null sage block");
    return -1;
  }

  //SAGE_PRINTLOG("block size %d", sb->getBufSize());
  // send data
  int dataSize = sage::send(clientSockFd, sb->getBuffer(), sb->getBufSize());
  if (dataSize < 0) {
    return -1;
  }

  return dataSize;
}//End of sageTcpModule::send()

/**
 * sungwon experimental, swexp
 */
int sageTcpModule::sendpixelonly(int id, sageBlockFrame *sb) {
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::%s() : invalid receiver ID %d", __FUNCTION__, id);
    return -1;
  }

  if (!sb) {
    SAGE_PRINTLOG("sageTcpModule::%s() : null sage block", __FUNCTION__);
    return -1;
  }

  //SAGE_PRINTLOG("block size %d", sb->getBufSize());
  // send data
  assert(sb);


  int totalsent = 0;
  int dataSize = 0;
  char *buf = sb->getPixelBuffer();
  int frameSize = sb->getBufSize() - BLOCK_HEADER_SIZE;

  //  SAGE_PRINTLOG("\nTCPmodule::sendPixelOnly() : frameSize %d, Bufsize %d, BLOCK_HEADER_SIZE %d, grpsize %d", frameSize, sb->getBufSize(), BLOCK_HEADER_SIZE, config.groupSize);

  while ( totalsent < frameSize ) {

    if ( frameSize - totalsent < config.groupSize ) {
      dataSize = sage::send(rcvList[id], buf, frameSize-totalsent);
    }
    else {
      dataSize = sage::send(rcvList[id], buf, config.groupSize);
    }
    if (dataSize < 0) { return -1; }

    buf += dataSize;

    totalsent += dataSize;
  }

  /*
    int dataSize = sage::send(rcvList[id], sb->getPixelBuffer(), sb->getBufSize() - BLOCK_HEADER_SIZE);
    if (dataSize < 0) { return -1; }
  */

  return dataSize;
}


int sageTcpModule::sendControl(int id, int frameID, int configID)
{
  if (flush(id, configID) < 0) {
    return -1;
  }

  int clientSockFd = rcvList[id];

  char header[GROUP_HEADER_SIZE];
  sprintf(header, "0 %d %d", frameID, configID);
  int dataSize = sage::send(clientSockFd, header, GROUP_HEADER_SIZE);
  if (dataSize < 0) {
    return -1;
  }

  return dataSize;
}

int sageTcpModule::sendGrp(int id, sagePixelBlock *sb, int configID)
{
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::send() : invalid receiver ID %d", id);
    return -1;
  }

  int clientSockFd = rcvList[id];

  if (!sb) {
    SAGE_PRINTLOG("sageTcpModule::sendGrp() : null sage block");
    return -1;
  }

  if (config.groupSize == 0) {
    SAGE_PRINTLOG("sageTcpModule::sendGrp() : group transfer is not enabled");
    return -1;
  }

  sageBlockGroup* bGrp = bufList[id];
  bGrp->pushBack(sb);
  //SAGE_PRINTLOG("send : %s", (char *)sb->getBuffer());

  int dataSize = 0;
  if (bGrp->isFull()) {
    bGrp->genIOV();
    bGrp->setFrameID(sb->getFrameID());
    bGrp->setConfigID(configID);
    dataSize = bGrp->sendData(clientSockFd);
    //SAGE_PRINTLOG("send grp %d", dataSize);
    if (returnPlace) {
      if (returnPlace->returnBlocks(bGrp) >= 0)
        SAGE_PRINTLOG("sageTcpModule::sendGrp : block pool overflow...");
    }
    else {
      SAGE_PRINTLOG("sageTcpModule::sendGrp : no block return place");
    }
    bGrp->resetGrp();
  }

  return dataSize;
}//End of sageTcpModule::send()

int sageTcpModule::flush(int id, int configID)
{
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::send() : invalid receiver ID %d", id);
    return -1;
  }

  int clientSockFd = rcvList[id];

  if (config.groupSize == 0) {
    SAGE_PRINTLOG("sageTcpModule::sendGrp() : group transfer is not enabled");
    return -1;
  }

  sageBlockGroup* bGrp = bufList[id];
  if (bGrp->isEmpty())
    return 0;

  int dataSize = 0;
  bGrp->genIOV();
  bGrp->setConfigID(configID);
  int frameID = bGrp->front()->getFrameID();
  bGrp->setFrameID(frameID);

  dataSize = bGrp->sendData(clientSockFd);
  //SAGE_PRINTLOG("flush grp %d", dataSize);

  if (returnPlace) {
    if (returnPlace->returnBlocks(bGrp) >= 0) {
      SAGE_PRINTLOG("sageTcpModule::sendGrp : block pool overflow...");
    }
  }
  else {
    SAGE_PRINTLOG("sageTcpModule::sendGrp : no block return place");
  }

  bGrp->resetGrp();

  return dataSize;
}//End of sageTcpModule::send()

int sageTcpModule::recv(int id, sageBlock *sb, sageApiOption op)
{
  if (id < 0 || id > sendList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::recv() : invalid sender ID");
    return -1;
  }

  int clientSockFd;
  if (op & SAGE_BACKWARD)
    clientSockFd = rcvList[id];
  else
    clientSockFd = sendList[id];

  if (clientSockFd < 0) {
    //SAGE_PRINTLOG("socket closed);
    return -1;
  }

  if (op & SAGE_NON_BLOCKING) {
    if (!sage::isDataReady(clientSockFd)) {
      //SAGE_PRINTLOG("no data");
      return 0;
    }
  }

  //SAGE_PRINTLOG("block size = %d", sb->getBufSize());
  char *bufP = sb->getBuffer();
  if (!bufP) {
    SAGE_PRINTLOG("sageTcpModule::recv - block buffer is null");
    return -1;
  }

  int retVal = sage::recv(clientSockFd, (void *)bufP, sb->getBufSize());
  if (retVal < 0) {
    return -1;
  }
  else if (retVal == 0) {
#ifdef WIN32
    closesocket(sendList[id]);
#else
    shutdown(sendList[id], SHUT_RDWR);
#endif
    sendList[id] = -1;
    return -1;
  }

  sb->updateBlockConfig();

  return sb->getBufSize();
}//End of sageTcpModule::recv()

int sageTcpModule::recvGrp(int id, sageBlockGroup *sbg)
{
  if (id < 0 || id > sendList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::recv() : invalid sender ID");
    return -1;
  }

  int clientSockFd = sendList[id];

  if (clientSockFd < 0) {
    SAGE_PRINTLOG("sageTcpModule::recvGrp : the socket is closed");
    return -1;
  }

  if (!sbg) {
    SAGE_PRINTLOG("sageTcpModule::recvGrp - block group ptr is null");
    return -1;
  }

  int retVal = sbg->readData(clientSockFd);

  if (retVal < 0) {
    return -1;
  }
  else if (retVal == 0) {
#ifdef WIN32
    closesocket(sendList[id]);
#else
    shutdown(sendList[id], SHUT_RDWR);
#endif
    sendList[id] = -1;
    return -1;
  }

  sbg->updateConfig();

  return retVal;
}//End of sageTcpModule::recv()

int sageTcpModule::close()
{
  int rcvNum = rcvList.size();
  int sendNum = sendList.size();

  if (sMode != SAGE_SEND) {
#ifdef WIN32
    closesocket(serverSockFd);
#else
    shutdown(serverSockFd, SHUT_RDWR);
#endif

    for (int i=0; i<sendNum; i++) {
      if (sendList[i] < 0)
        continue;
#ifdef WIN32
      closesocket(sendList[i]);
#else
      shutdown(sendList[i], SHUT_RDWR);
#endif
    }
  }

  if (sMode != SAGE_RCV && sMode != SAGE_ARCV) {
    for (int i=0; i<rcvNum; i++) {
#ifdef WIN32
      closesocket(rcvList[i]);
#else
      shutdown(rcvList[i], SHUT_RDWR);
#endif
    }
  }

  rcvList.clear();
  sendList.clear();

  return 0;
}

int sageTcpModule::setSockOpts(int fd)
{
  int optVal, optLen;

#if defined(WIN32) || defined(__APPLE__)

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1;
  optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
  {
    SAGE_PRINTLOG("\ntvUdp::init(): Error setting SO_REUSEADDR");
    return false;
  }

  optVal= config.sendBufSize, optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&optVal, optLen) !=0)
  {
    SAGE_PRINTLOG("sageTcpModule::setSockOpts(): Error setting SO_SNDBUF to %d bytes", optVal);
    return false;
  }

  optVal= config.rcvBufSize; optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&optVal, optLen) !=0)
  {
    SAGE_PRINTLOG("sageTcpModule::setSockOpts(): Error setting SO_RCVBUF to %d bytes", optVal);
    return false;
  }

  /*
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("sageTcpModule::setSockOpts(): Error switching off Nagle's algorithm.");
  */
#else
  int reuseFlag =1;
  optLen = sizeof(reuseFlag);
  if (setsockopt(fd, SOL_SOCKET,SO_REUSEADDR, (char *) &reuseFlag, optLen) < 0)
  {
    SAGE_PRINTLOG("sageTcpModule::SetSockOpts(): Cannot set SO_REUSEADDR on socket");
    return false;
  }

  optVal= config.sendBufSize, optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&optVal, (socklen_t)optLen) != 0)
  {
    SAGE_PRINTLOG("sageTcpModule::SetSockOpts(): Error setting SO_SNDBUF to %d bytes", config.sendBufSize);
    return false;
  }

  optVal= config.rcvBufSize; optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&optVal, (socklen_t)optLen) != 0)
  {
    SAGE_PRINTLOG("sageTcpModule::SetSockOpts(): Error setting SO_SNDBUF to %d bytes", config.rcvBufSize);
    return false;
  }

  /*
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(fd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    SAGE_PRINTLOG("sageTcpModule::setSockOpts(): Error switching off Nagle's algorithm.");
  */
#endif

  return true;
} //End of sageTcpModule::setSockOpts()

sageTcpModule::~sageTcpModule()
{
  close();

  for (int i=0; i<bufList.size(); i++) {
    sageBlockGroup *sbg = bufList[i];
    if (sbg)
      delete sbg;
  }

  bufList.clear();
}//End of sageTcpModule::~sageTcpModule()
