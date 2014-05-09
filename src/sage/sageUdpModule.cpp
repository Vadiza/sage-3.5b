/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  sageUdpModule.cpp
 * Author   :   Byungil Jeong, Rajvikram Singh
 * Description:   Code file for the UDP network stream of SAGE
 * Notes   :   The class uses UDP/TCP sockets with the options optimized for graphics streaming
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
 ***************************************************************************************************/

#include "sageUdpModule.h"
#include "sageBlock.h"
#include "sageBlockPool.h"
#include "sageFrame.h"

streamFlowData::streamFlowData(int wSize, sageBlockBuf *buf) : winIdx(0), frameRate(1),
                                                               frameSize(0), sentPackets(0), returnPlace(NULL), curGrp(NULL), packetSum(0),
                                                               windowTimeSum(0), actualTimeSum(0), active(true), closed(false)
{
  streamWindow = new flowHistory[wSize];
  windowSize = wSize;
  blockBuf = buf;
}

streamFlowData::~streamFlowData()
{
  if (streamWindow)
    delete [] streamWindow;

  if (blockBuf)
    delete blockBuf;
}

void streamFlowData::pushBack(sageBlockGroup *grp)
{
  if (blockBuf) {
    blockBuf->pushBack(grp);
    curGrp = blockBuf->getFreeBlocks();
  }
}

int streamFlowData::insertWindow(double aTime, double wTime)
{
  windowTimeSum -= streamWindow[winIdx].windowInterval;
  actualTimeSum -= streamWindow[winIdx].actualInterval;
  packetSum -= streamWindow[winIdx].sentPackets;

  windowTimeSum += wTime;
  actualTimeSum += aTime;
  packetSum += sentPackets;

  streamWindow[winIdx].windowInterval = wTime;
  streamWindow[winIdx].actualInterval = aTime;
  streamWindow[winIdx].sentPackets = sentPackets;

  sentPackets = 0;
  winIdx++;

  if (winIdx >= windowSize)
    winIdx = 0;

  return 0;
}

double streamFlowData::getPacketRate(int packetNum)
{
  return frameRate*packetNum/1000000.0;
}

sageUdpModule::sageUdpModule() : closeFlag(false), notStarted(true), waitData(true)
{
#ifdef WIN32
  // Initialize Winsock
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

  protocol = SAGE_UDP;
  pthread_mutex_init(&connectionLock, NULL);
  pthread_mutex_unlock(&connectionLock);
  pthread_cond_init(&streamStart, NULL);
  pthread_cond_init(&newData, NULL);

}//End of sageUdp:sageUdp

int sageUdpModule::init(sageStreamMode m, int p, sageNwConfig &c)
{
  streamProtocol::init(m, p, c);

  // create the actual network objects
  switch(sMode)   {
  case SAGE_RCV:
  case SAGE_ARCV:
  case SAGE_BRIDGE: {  // prepare a server socket
    // create the sockets
    if((serverSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
      SAGE_PRINTLOG("sageUdpModule::init(): Creating TCP socket failed");
      return -1;
    }

    setSockOpts(serverSockFd, true);

    struct sockaddr_in localAddr;
    // bind to port
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(rcvPort);

    if (bind(serverSockFd, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
      SAGE_PRINTLOG("sageUdpModule::init() : error in binding socket");
      return -1;
    }

    // put in listen mode
    ::listen(serverSockFd, 5);

    break;
  }

  case SAGE_SEND: {
    if (config.blockSize > 0 && config.groupSize > 0) {
      if (pthread_create(&thId, 0, sendingThread, (void*)this) != 0) {
        SAGE_PRINTLOG("sageUdpModule::init() : can't create sendingThread");
      }
    }
    break;
  }

  default:
    return -1;

  } //End of switch(opMode)

  return 0;
}//End of sageUdpModule::init()

void sageUdpModule::setupBlockPool(sageBlockPool *pool, int id)
{
  int flowNum = flowList.size()-1;

  if (id > flowNum) {
    SAGE_PRINTLOG("sageUdpModule::setupBlockPool() : invalid receiver ID");
    return;
  }
  else if (id == -1) {
    for (int i=0; i<flowList.size(); i++)
      flowList[i]->returnPlace = pool;
  }
  else
    flowList[id]->returnPlace = pool;
}

void sageUdpModule::setFrameSize(int id, int size)
{
  int flowNum = flowList.size()-1;

  if (id < 0 || id > flowNum) {
    SAGE_PRINTLOG("sageUdpModule::setFrameSize() : invalid receiver ID");
    return;
  }

  flowList[id]->frameSize = size;
}

void sageUdpModule::setFrameRate(double rate, int id)
{
  int flowNum = flowList.size()-1;

  if (id > flowNum) {
    SAGE_PRINTLOG("sageUdpModule::setFrameRate() : invalid receiver ID");
    return;
  }
  else if (id == -1) {
    for (int i=0; i<flowList.size(); i++)
      flowList[i]->frameRate = rate;
  }
  else
    flowList[id]->frameRate = rate;
}

int sageUdpModule::checkConnections(char *msg, sageApiOption op)
{
  if(sMode == SAGE_SEND) {
    SAGE_PRINTLOG("sageUdpModule::checkConnection() cannot be used for senders");
    return -1;
  }

  if (op & SAGE_NON_BLOCKING)
    if (!sage::isDataReady(serverSockFd)) {
      return -1;
    }

  //accept client connections
  int addrLen;
  struct sockaddr_in clientAddr;
  addrLen = sizeof(clientAddr);

  int clientSockFd;
  if((clientSockFd = ::accept(serverSockFd, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1){
    SAGE_PRINTLOG("sageUdpModule::checkConnections() - error in accepting control connection");
    return -1;
  }

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

  SAGE_PRINTLOG("sageUdpModule::%s() : received %d byte reg msg from sender [%s]\n", __FUNCTION__, REG_MSG_SIZE, regMsg);

  if (msg) {
    sprintf(msg, "%d %s", idx, regMsg);
  }
  else {
    SAGE_PRINTLOG("sageUdpModule::checkConnections() : registraion message buffer is NULL");
    return -1;
  }

  // creating UDP socket
  int udpSockFd;
  /*
    if((udpSockFd = ::accept(serverSockFd, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1){
    SAGE_PRINTLOG("sageUdpModule::checkConnections()");
    return -1;
    }
  */

  //SAGE_PRINTLOG("sageUdpModule::checkConnections() : create UDP socket");
  if ((udpSockFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    SAGE_PRINTLOG("sageUdpModule::checkConnections(): Creating UDP socket failed");
    return -1;
  }

  setSockOpts(udpSockFd, false);

  struct sockaddr_in udpLocalAddr;
  memset(&udpLocalAddr, 0, sizeof(udpLocalAddr));
  udpLocalAddr.sin_family = AF_INET;
  udpLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  //udpLocalAddr.sin_port = htons(rcvPort + 10);
  udpLocalAddr.sin_port = htons(0);

  //SAGE_PRINTLOG("sageUdpModule::checkConnections() : bind UDP socket");
  if (bind(udpSockFd, (struct sockaddr *)&udpLocalAddr, sizeof(struct sockaddr_in)) != 0) {
    SAGE_PRINTLOG("sageUdpModule::checkConnections() : Error binding UDP socket");
    return -1;
  }

  socklen_t nameLen = sizeof(udpLocalAddr);
  memset(&udpLocalAddr, 0, sizeof(udpLocalAddr));
  if (getsockname(udpSockFd, (struct sockaddr *)&udpLocalAddr, &nameLen) != 0) {
    SAGE_PRINTLOG("sageUdpModule::checkConnections() : Error getting the name of UDP socket");
    return -1;
  }

  int udpPort = (int)ntohs(udpLocalAddr.sin_port);
  char addrMsg[TOKEN_LEN];
  sprintf(addrMsg, "%d", udpPort);
  //SAGE_PRINTLOG("sageUdpModule::checkConnections() : send UDP server port %d", udpPort);

  SAGE_PRINTLOG("sageUdpModule::%s() : sending %d byte of receiver UDP port info. %d\n", __FUNCTION__, TOKEN_LEN, udpPort);

  if (sage::send(clientSockFd, (void *)addrMsg, TOKEN_LEN) < 0) {
    SAGE_PRINTLOG("sageUdpModule::checkConnections()");
    return -1;
  }

  SAGE_PRINTLOG("sageUdpModule::%s() : waiting %d byte of sender UDP port info\n", __FUNCTION__, TOKEN_LEN);

  if (sage::recv(clientSockFd, (void *)addrMsg, TOKEN_LEN) <= 0) {
    SAGE_PRINTLOG("sageUdpModule::checkConnections()");
    return -1;
  }

  sscanf(addrMsg, "%d", &udpPort);
  clientAddr.sin_port = htons(udpPort);
  //SAGE_PRINTLOG("sageUdpModule::checkConnections() : UDP client port is %d", udpPort);

  SAGE_PRINTLOG("sageUdpModule::%s() : received %d byte of sender's UDP port info. %d\n", __FUNCTION__, TOKEN_LEN, udpPort);

  if (::connect(udpSockFd, (struct sockaddr *)&clientAddr, sizeof(struct sockaddr)) == -1) {
    SAGE_PRINTLOG("sageUdpModule::checkConnections()");
    return -1;
  }

  SAGE_PRINTLOG("sageUdpModule::%s() : UDP connection with sender has established\n", __FUNCTION__);

  udpSendList.push_back(udpSockFd);
  if (idx != (udpSendList.size()-1)) {
    std::cerr << "sageUdpModule::checkConnections() - Error : socket list index mismatch" <<
      std::endl;
    return -1;
  }

  return idx;
}//End of sageUdpModule::listen()

int sageUdpModule::connect(char *ip, char *msg)
{
  if(sMode == SAGE_RCV) {
    std::cerr << "sageUdpModule::connect() cannot be used for receivers" << std::endl;
    return -1;
  }

  // create the sockets
  int clientSockFd;
  if((clientSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1)   {
    SAGE_PRINTLOG("sageUdpModule::connect(): Creating TCP socket failed");
    return -1;
  }

  setSockOpts(clientSockFd, true);

  // connect to the server
  struct sockaddr_in serverAddr;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(ip);


  /**
   * case SAIL_CONNECT_TO_RCV, rcvPort
   *
   * case SAIL_CONNECT_TO_RCV_PORT,  rcvPort+1
   *
   */
  serverAddr.sin_port = htons(rcvPort);

  if(::connect(clientSockFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
    SAGE_PRINTLOG("sageUdpModule::connect()");
    return -1;
  }

  //SAGE_PRINTLOG("sageUdpModule::%s() : TCP channel established with receiver %s:%d\n", __FUNCTION__, ip, rcvPort);

  rcvList.push_back(clientSockFd);
  //sendAck.push_back(false);

  //SAGE_PRINTLOG("UDP reg send");

  if (msg) {
    if (sage::send(clientSockFd, msg, REG_MSG_SIZE) == -1) {
      SAGE_PRINTLOG("sageUdpModule::connect()");
      return -1;
    }
    //SAGE_PRINTLOG("sageUdpModule::%s() : %d byte reg msg sent [%s] through TCP channel\n", __FUNCTION__, REG_MSG_SIZE, msg);
  }

  int idx = rcvList.size()-1;

  // creating UDP socket
  int udpSockFd;

  /*
    if((udpSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1)   {
    SAGE_PRINTLOG("sageUdpModule::connect(): Creating TCP socket failed");
    return -1;
    }

    setSockOpts(udpSockFd);


    if(::connect(udpSockFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
    SAGE_PRINTLOG("sageUdpModule::connect()");
    return -1;
    }
  */

  //SAGE_PRINTLOG("sageUdpModule::connect() : create UDP socket");
  if ((udpSockFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    SAGE_PRINTLOG("sageUdpModule::connect(): Creating UDP socket failed");
    return -1;
  }

  setSockOpts(udpSockFd);

  struct sockaddr_in udpLocalAddr;
  memset(&udpLocalAddr, 0, sizeof(udpLocalAddr));
  udpLocalAddr.sin_family = AF_INET;
  udpLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  udpLocalAddr.sin_port = htons(0);

  //SAGE_PRINTLOG("sageUdpModule::connect() : bind UDP socket");
  if (bind(udpSockFd, (struct sockaddr *)&udpLocalAddr, sizeof(struct sockaddr_in)) != 0) {
    SAGE_PRINTLOG("sageUdpModule::connect() : Error binding UDP socket");
    return -1;
  }

  socklen_t nameLen = sizeof(udpLocalAddr);
  memset(&udpLocalAddr, 0, sizeof(udpLocalAddr));
  if (getsockname(udpSockFd, (struct sockaddr *)&udpLocalAddr, &nameLen) != 0) {
    SAGE_PRINTLOG("sageUdpModule::connect() : Error getting the name of UDP socket");
    return -1;
  }

  int udpPort;
  char addrMsg[TOKEN_LEN];

  //SAGE_PRINTLOG("sageUdpModule::connect() : waiting for server...");
  //SAGE_PRINTLOG("sageUdpModule::%s() : UDP socket ready. waiting UDP Port information (%d byte) from receiver\n", __FUNCTION__, TOKEN_LEN);
  if (sage::recv(clientSockFd, (void *)addrMsg, TOKEN_LEN) < 0) {
    SAGE_PRINTLOG("sageUdpModule::connect()");
    return -1;
  }

  sscanf(addrMsg, "%d", &udpPort);
  //SAGE_PRINTLOG("sageUdpModule::connect() : UDP server port is %d", udpPort);

  serverAddr.sin_port = htons(udpPort);

  // SAGE_PRINTLOG("sageUdpModule::%s() : receiver's UDP port info received. %d\n", __FUNCTION__, udpPort);

  if (::connect(udpSockFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
    SAGE_PRINTLOG("sageUdpModule::connect()");
    return -1;
  }

  // SAGE_PRINTLOG("sageUdpModule::%s() : UDP connection established\n", __FUNCTION__);

  udpPort = (int)ntohs(udpLocalAddr.sin_port);
  sprintf(addrMsg, "%d", udpPort);
  //SAGE_PRINTLOG("sageUdpModule::connect() : send UDP client port %d", udpPort);

  //SAGE_PRINTLOG("sageUdpModule::%s() : sending sender's UDP port info %d byte [%s]\n", __FUNCTION__, TOKEN_LEN, addrMsg);
  if (sage::send(clientSockFd, (void *)addrMsg, TOKEN_LEN) <= 0) {
    SAGE_PRINTLOG("sageUdpModule::connect() - error in sending UDP port");
    return -1;
  }

  udpRcvList.push_back(udpSockFd);
  if (idx != (udpRcvList.size()-1)) {
    SAGE_PRINTLOG("sageUdpModule::connect() - socket list index mismatch");
    return -1;
  }

  if (config.blockSize > 0 && config.groupSize > 0) {
    sageBlockBuf *buf = new sageBlockBuf(config.sendBufSize, config.groupSize,
                                         config.blockSize, 0);

    streamFlowData *flowData = new streamFlowData(config.flowWindow, buf);
    flowData->curGrp = buf->getFreeBlocks();

    pthread_mutex_lock(&connectionLock);
    flowList.push_back(flowData);

    if (notStarted) {
      pthread_cond_signal(&streamStart);
      notStarted = false;
    }

    pthread_mutex_unlock(&connectionLock);
  }

  return idx;
}//End of sageUdpModule::connect()

int sageUdpModule::send(int id, sageBlock *sb, sageApiOption op)
{
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageUdpModule::send() : invalid receiver ID");
    return -1;
  }

  if (!sb) {
    SAGE_PRINTLOG("sageUdpModule::send() : null sage block");
    return 1;
  }

  int dataSize;

  if (op & SAGE_CONTROL) {
    int clientSockFd = rcvList[id];
    if (op & SAGE_BACKWARD)
      clientSockFd = sendList[id];

    if ((dataSize = sage::send(clientSockFd, sb->getBuffer(), BLOCK_HEADER_SIZE)) < 0)
      return -1;
  }
  else {
    //SAGE_PRINTLOG("block size %d bytes", sb->getBufSize());

    // send data using UDP channel
    int udpSockFd = udpRcvList[id];
    dataSize = sage::send(udpSockFd, sb->getBuffer(), sb->getBufSize());
    if (dataSize < 0)
      return -1;
    //SAGE_PRINTLOG("%d bytes sent", dataSize);
  }

  return dataSize;
}//End of sageUdpModule::send()

// swexp, Placeholder. this function won't work. DOn't use it
int sageUdpModule::sendpixelonly(int id, sageBlockFrame *sb) {
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageUdpModule::%s() : invalid receiver ID %d", __FUNCTION__, id);
    return -1;
  }

  if (!sb) {
    SAGE_PRINTLOG("sageUdpModule::%s() : null sage block", __FUNCTION__);
    return -1;
  }

  //SAGE_PRINTLOG("block size %d", sb->getBufSize());
  // send data
  int dataSize = sage::send(udpRcvList[id], sb->getPixelBuffer(), sb->getBufSize() - BLOCK_HEADER_SIZE);
  if (dataSize < 0) {
    return -1;
  }

  return dataSize;
}

int sageUdpModule::sendControl(int id, int frameID, int configID)
{
  if (flush(id, configID) < 0) {
    return -1;
  }

  sageBlockGroup *bGrp = flowList[id]->curGrp;
  bGrp->setFrameID(frameID);
  bGrp->setConfigID(configID);
  bGrp->setFrameSize(flowList[id]->frameSize);
  bGrp->genIOV();

  waitData = false;
  flowList[id]->pushBack(bGrp);
  pthread_cond_signal(&newData);

  return GROUP_HEADER_SIZE;
}

int sageUdpModule::sendGrp(int id, sagePixelBlock *sb, int configID)
{
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageUdpModule::send() : invalid receiver ID");
    return -1;
  }

  if (!sb) {
    SAGE_PRINTLOG("sageUdpModule::send() : null sage block");
    return 1;
  }

  if (config.groupSize == 0) {
    SAGE_PRINTLOG("sageUdpModule::sendGrp() : group transfer is not enabled");
    return -1;
  }

  sageBlockGroup *bGrp = flowList[id]->curGrp;
  bGrp->pushBack(sb);

  if (bGrp->isFull()) {
    bGrp->genIOV();
    bGrp->setFrameID(sb->getFrameID());
    bGrp->setFrameSize(flowList[id]->frameSize);
    bGrp->setConfigID(configID);

    waitData = false;
    flowList[id]->pushBack(bGrp);
    pthread_cond_signal(&newData);
  }

  return sb->getBufSize();
}//End of sageUdpModule::sendGrp()

int sageUdpModule::flush(int id, int configID)
{
  if (id < 0 || id > rcvList.size()-1) {
    SAGE_PRINTLOG("sageTcpModule::send() : invalid receiver ID %d", id);
    return -1;
  }

  if (config.groupSize == 0) {
    SAGE_PRINTLOG("sageTcpModule::sendGrp() : group transfer is not enabled");
    return -1;
  }

  sageBlockGroup *bGrp = flowList[id]->curGrp;
  if (bGrp->isEmpty())
    return 0;

  bGrp->genIOV();
  bGrp->setConfigID(configID);
  int frameID = bGrp->front()->getFrameID();
  bGrp->setFrameID(frameID);
  bGrp->setFrameSize(flowList[id]->frameSize);

  waitData = false;
  flowList[id]->pushBack(bGrp);
  pthread_cond_signal(&newData);

  return 0;
}//End of sageUdpModule::flush()

int sageUdpModule::sendLoop()
{
  double maxPriority = 0;
  double curTime = 0;
  double packetInterval = config.mtuSize/config.maxBandWidth;  // in micro-second
  double checkInterval = config.maxCheckInterval;              // in micro-second
  int checkPacketNum = (int)floor(checkInterval/packetInterval);
  streamFlowData totalRecords(config.flowWindow, NULL);

  pthread_mutex_lock(&connectionLock);
  if (notStarted)
    pthread_cond_wait(&streamStart, &connectionLock);
  pthread_mutex_unlock(&connectionLock);

  sageTimer flowTimer;

  while(!closeFlag) {
    pthread_mutex_lock(&connectionLock);
    int streamNum = flowList.size();
    pthread_mutex_unlock(&connectionLock);

    maxPriority = 0;
    int selectedStream = -1;

    waitData = true;

    // select a stream to send
    for (int i=0; i<streamNum; i++) {
      if (flowList[i]->active) {
        sageBlockGroup *bGrp = flowList[i]->blockBuf->front();
        int packetNum = 0;
        if (bGrp)
          packetNum = (int)ceil((double)bGrp->getFrameSize()/config.mtuSize);

        double targetRate = flowList[i]->getPacketRate(packetNum);
        int sentPackets = flowList[i]->totalSentPacketNum();
        double elapsedTime = curTime + flowList[i]->elapsedTime();
        double curRate;
        if (elapsedTime > 0)
          curRate = (double)sentPackets/elapsedTime;
        else
          curRate = 0;

        double priority;
        if (curRate > 0)
          priority = targetRate / curRate;
        else
          priority = 1.0;

        if (maxPriority < priority && !flowList[i]->blockBuf->isEmpty()) {
          maxPriority = priority;
          selectedStream = i;
        }
      }
      else if (!flowList[i]->closed) {
        while (!flowList[i]->blockBuf->isEmpty()) {
          sageBlockGroup *bGrp = flowList[i]->blockBuf->front();
          flowList[i]->blockBuf->next();

          if (bGrp && bGrp->getBlockNum() > 0)
            flowList[i]->returnPlace->returnBlocks(bGrp);
        }

        flowList[i]->closed = true;
        delete flowList[i]->blockBuf;
      }
    }

    //SAGE_PRINTLOG("selected stream %d", selectedStream);

    bool flowWindowEnd = false;
    double actualTime = 0;   // actual time to be used for sending data

    if (selectedStream == -1) {
      actualTime = flowTimer.getTimeUS(false);

      pthread_mutex_lock(&connectionLock);
      if (waitData)
        sage::condition_wait(&newData, &connectionLock, 10);
      pthread_mutex_unlock(&connectionLock);

      if (totalRecords.sentPackets > 0 || actualTime > checkInterval)
        flowWindowEnd = true;
    }
    else if (maxPriority >= 1.0) {
      sageBlockGroup *bGrp = flowList[selectedStream]->blockBuf->front();
      flowList[selectedStream]->blockBuf->next();

      int udpSockFd = udpRcvList[selectedStream];
      if (bGrp) {
        int sentSize = bGrp->sendDatagram(udpSockFd);
        if (sentSize > 0) {
          int packetNum = (sentSize+config.mtuSize-1)/config.mtuSize;
          totalRecords.sentPackets += packetNum;
          flowList[selectedStream]->sentPackets += packetNum;
          curTime += packetInterval*packetNum;
        }
        else
          flowList[selectedStream]->active = false;

        if (bGrp->getBlockNum() > 0)
          flowList[selectedStream]->returnPlace->returnBlocks(bGrp);
        bGrp->resetGrp();
        flowList[selectedStream]->blockBuf->returnBG(bGrp);
      }
      else
        SAGE_PRINTLOG("sageUdpModule::sendLoop : stream %d block buffer is empty",
                      selectedStream);
    }
    else {
      actualTime = flowTimer.getTimeUS(false);
      sage::switchThread();
      if (totalRecords.sentPackets > 0 || actualTime > checkInterval)
        flowWindowEnd = true;
    }

    if (flowWindowEnd || totalRecords.sentPackets >= checkPacketNum) {
      double windowTime = flowTimer.getTimeUS(false);  // overall time including idle time
      if (!flowWindowEnd) {
        actualTime = windowTime;
        while (windowTime < checkInterval) {
          sage::switchThread();
          windowTime = flowTimer.getTimeUS(false);
        }
      }

      totalRecords.insertWindow(actualTime, windowTime);
      for (int i=0; i<streamNum; i++)
        flowList[i]->insertWindow(actualTime, windowTime);

      double nextInterval = checkPacketNum/totalRecords.getAvePacketRate();
      checkInterval = MIN(nextInterval, config.maxCheckInterval);
      packetInterval = totalRecords.getAvePacketInterval();
      checkPacketNum = (int)floor(checkInterval/packetInterval);

      curTime = 0.0;
      flowTimer.reset();
    }
  }

  return 0;
}

void* sageUdpModule::sendingThread(void *args)
{
  sageUdpModule *This = (sageUdpModule *)args;
  This->sendLoop();

  pthread_exit(NULL);
  return NULL;
}

int sageUdpModule::recv(int id, sageBlock *sb, sageApiOption op)
{
  if (id < 0 || id > sendList.size()-1) {
    SAGE_PRINTLOG("sageUdpModule::recv() : invalid sender ID");
    return -1;
  }

  int udpSockFd = udpSendList[id];

  if (op & SAGE_NON_BLOCKING) {
    if (!sage::isDataReady(udpSockFd)) {
      return 0;
    }
  }

  char *bufP = sb->getBuffer();
  if (!bufP) {
    SAGE_PRINTLOG("sageUdpModule::recv - block buffer is null");
    return -1;
  }

  int retVal = sage::recv(udpSockFd, (void *)bufP, sb->getBufSize());

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

  return retVal;
}//End of sageUdpModule::recv()

int sageUdpModule::recvGrp(int id, sageBlockGroup *sbg)
{
  if (id < 0 || id > sendList.size()-1) {
    SAGE_PRINTLOG("sageUdpModule::recv() : invalid sender ID");
    return -1;
  }

  int udpSockFd = udpSendList[id];

  if (udpSockFd < 0) {
    SAGE_PRINTLOG("sageUdpModule::recvGrp : the socket is closed");
    return -1;
  }

  if (!sbg) {
    SAGE_PRINTLOG("sageUdpModule::recvGrp - block group ptr is null");
    return -1;
  }

  int retVal = sbg->readDatagram(udpSockFd);

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
}//End of sageUdpModule::recvGrp()

int sageUdpModule::close(int id, int mode)
{
  if (mode == -1)
    mode = sMode;

  streamProtocol::close(id, mode);

  if (mode == SAGE_RCV || mode == SAGE_ARCV) {
    if (udpSendList[id] >= 0) {
#ifdef WIN32
      closesocket(udpSendList[id]);
#else
      shutdown(udpSendList[id], SHUT_RDWR);
#endif
    }
  }

  if (mode == SAGE_SEND) {
    if (config.groupSize > 0 && flowList[id])
      flowList[id]->active = false;

    if (udpRcvList[id] >= 0) {
#ifdef WIN32
      closesocket(udpRcvList[id]);
#else
      shutdown(udpRcvList[id], SHUT_RDWR);
#endif
    }
  }

  return 0;
}

int sageUdpModule::close()
{
  closeFlag = true;

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
      closesocket(udpSendList[i]);
#else
      shutdown(sendList[i], SHUT_RDWR);
      shutdown(udpSendList[i], SHUT_RDWR);
#endif
    }
  }

  if (sMode != SAGE_RCV && sMode != SAGE_ARCV) {
    for (int i=0; i<rcvNum; i++) {
#ifdef WIN32
      closesocket(rcvList[i]);
      closesocket(udpRcvList[i]);
#else
      shutdown(rcvList[i], SHUT_RDWR);
      shutdown(udpRcvList[i], SHUT_RDWR);
#endif
    }
  }

  return 0;
}

bool sageUdpModule::setSockOpts(int fd, bool noDelay)
{
  int optVal, optLen;

#if defined(WIN32) || defined(__APPLE__) || defined(__sun)

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1;
  optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
  {
    std::cerr << "\nsageUdpModule::setSockOpts(): Error setting SO_REUSEADDR" <<  std::endl;
    return false;
  }

  optVal= config.sendBufSize, optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&optVal, optLen) !=0)
  {
    std::cerr << "sageUdpModule::setSockOpts(): Error setting SO_SNDBUF to " << optVal << " bytes";
    return false;
  }

  optVal= config.rcvBufSize; optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&optVal, optLen) !=0)
  {
    std::cerr << "sageUdpModule::setSockOpts(): Error setting SO_RCVBUF to " << optVal << " bytes";
    return false;
  }

  if (noDelay) {
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
      std::cerr << "sageUdpModule::setSockOpts(): Error switching off Nagle's algorithm." << std::endl;
  }
#else
  int reuseFlag = 1;
  optLen = sizeof(reuseFlag);
  if (setsockopt(fd, SOL_SOCKET,SO_REUSEADDR, (char *) &reuseFlag, optLen) < 0)
  {
    std::cerr << "sageUdpModule::SetSockOpts(): Cannot set SO_REUSEADDR on socket" << std::endl;
    return false;
  }

  optVal= config.sendBufSize;
  optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&optVal, (socklen_t)optLen) != 0)
  {
    std::cerr << "sageUdpModule::SetSockOpts(): Error setting SO_SNDBUF to " << config.sendBufSize << " bytes" << std::endl;
    return false;
  }

  optVal= config.rcvBufSize; optLen = sizeof(optVal);
  if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&optVal, (socklen_t)optLen) != 0)
  {
    std::cerr << "sageUdpModule::SetSockOpts(): Error setting SO_SNDBUF to " << config.rcvBufSize << " bytes" << std::endl;
    return false;
  }

  if (noDelay) {
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(fd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
      std::cerr << "sageUdpModule::setSockOpts(): Error switching off Nagle's algorithm." << std::endl;
  }
#endif

  return true;
} //End of sageUdpModule::setSockOpts()

sageUdpModule::~sageUdpModule()
{
  close();

  for (int i=0; i<flowList.size(); i++) {
    streamFlowData *fData = flowList[i];
    if (fData)
      delete fData;
  }

  flowList.clear();

  pthread_join(thId, NULL);
}
