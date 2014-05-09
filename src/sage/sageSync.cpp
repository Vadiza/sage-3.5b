/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageSync.cpp - This file contains the class which provides the syncing
 *            mechanism for keeping all the sage processes in sync.
 * Author : Byungil Jeong, Rajvikram Singh
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

#include "sageSync.h"
#include "sageBuf.h"

#if defined(WIN32)
#define MSG_WAITALL  0x8
#endif

int syncGroup::init(int startFrame, int p, int groupID, int frameRate, int sNum)
{
  slaveNum = sNum;
  curFrame = startFrame;

  policy = p;
  id = groupID;
  setFrameRate((float)frameRate);

  syncMsgLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(syncMsgLock, NULL);
  pthread_mutex_unlock(syncMsgLock);

  pthread_mutex_init(&syncLock, NULL);
  pthread_mutex_unlock(&syncLock);
  pthread_cond_init(&resumeSync, NULL);

  timer.reset();

  return 0;
}

int syncGroup::setFrameRate(float frameRate)
{
  if (frameRate > 0)
    interval = 1000000.0/frameRate; // in micro second
  else {
    SAGE_PRINTLOG("syncGroup::setFrameRate : can't set frame rate to zero");
    interval = 16666.6;
  }

  return 0;
}

bool syncGroup::checkInterval()
{
  double curTime = timer.getTimeUS();
  double maxErr = interval*MAX_INTERVAL_ERROR;
  double err = curTime - (interval - timeError);
  if (err > -maxErr) {
    if (timeError > maxErr)
      timeError = 0.0;
    else
      timeError = err;
    timer.reset();
    waitingInterval = false;
    return true;
  }

  waitingInterval = true;
  return false;
}

int syncGroup::processUpdate(char *data)
{
  //std::cerr << "process update " << data << std::endl;

  int groupID, frameNum, updateParam, updateType;
  sscanf(data, "%d %d %d %d", &groupID, &frameNum, &updateParam, &updateType);

  //SAGE_PRINTLOG("syncGroup::processUpdate : for group %d, frameNum %d, updateParam %d, updateType %d \n", groupID, frameNum, updateParam, updateType);

  float hardness = 1.0;
  if (policy == SAGE_ASAP_SYNC_HARD)
    hardness = 1.0;
  else if (policy == SAGE_ASAP_SYNC_SOFT)
    hardness = 0.5;

  switch(updateType) {
    // used in SAGE Bridge only (no audio sync)
  case SAGE_UPDATE_FRAME: {
    noOfUpdates++;
    skipFrame = MAX(skipFrame, frameNum);

    if (noOfUpdates >= slaveNum*hardness) {
      curFrame = MAX(curFrame, skipFrame);
      noOfUpdates = 0;
      return NORMAL_SYNC;
    }

    break;
  }

  case SAGE_UPDATE_SETUP: {
    //SAGE_PRINTLOG("newFrame %d  curFrame %d\n", frameNum, curFrame);
    //sungwon
    //SAGE_PRINTLOG("syncGroup::processUpdate(SAGE_UPDATE_SETUP) : newFrame %d, curFrame %d, rcvNum %d, noOfUpdates %d\n", frameNum, curFrame, updateParam, noOfUpdates);

    if (frameNum > curFrame) {
      videoFrame = frameNum;
      slaveNum = updateParam; // the number of receivers
      noOfUpdates++;

      if (noOfUpdates >= slaveNum*hardness) {
        if (audioFrame > 0 && frameNum >= keyFrame*audioSyncCnt && audioFrame < keyFrame*audioSyncCnt) {
          waitForKeyFrame = true;
          return 0;
        }
        else {
          curFrame = frameNum; // update frame
          noOfUpdates = 0; // reset counter
          return NORMAL_SYNC; // will result calling sendSync() -> sending group's currentFrame
        }
      }
    }
    break;
  }

  case SAGE_UPDATE_FOLLOW: { // no window change
    if (frameNum > curFrame) {
      videoFrame = frameNum;
      noOfUpdates++;

      if (noOfUpdates >= slaveNum*hardness) {
        if (audioFrame > 0 && frameNum >= keyFrame*audioSyncCnt && audioFrame < keyFrame*audioSyncCnt) {
          waitForKeyFrame = true; // video waits audio
          return 0; // sendSync() is not called
        }
        else {
          curFrame = MAX(skipFrame, frameNum); // update group's current frame number
          noOfUpdates = 0; // reset counter
          return NORMAL_SYNC; // call sendSync()
        }
      }
    }

    break;
  }

  case SAGE_UPDATE_AUDIO: {
    //sungwon
    //SAGE_PRINTLOG("syncGroup::processUpdate(SAGE_UPDATE_AUDIO) : for group %d, frameNum %d, updateParam %d, updateType %d \n", groupID, frameNum, updateParam, updateType);


    audioFrame = frameNum;
    keyFrame = updateParam;
    audioSyncCnt++;
    //std::cerr << "syncGroup::processUpdate : audio " << audioFrame << " video " << videoFrame << "noOfUpdate " << noOfUpdates << "slaveNum " << slaveNum << "curFrame " << curFrame << "videoFrame " << videoFrame << std::endl;

    if (waitForKeyFrame) {
      waitForKeyFrame = false;
      if (policy == SAGE_ASAP_SYNC_HARD) {
        if (noOfUpdates == slaveNum) {
          curFrame = videoFrame;
          noOfUpdates = 0;
          return NORMAL_SYNC;
        }
      }
      else if (policy == SAGE_ASAP_SYNC_SOFT) {
        if (noOfUpdates >= slaveNum*0.5) {
          curFrame = videoFrame;
          noOfUpdates = 0;
          return NORMAL_SYNC;
        }
      }
    }
  }
  }

  return 0;
}

/*
  bool syncGroup::checkTimeOut()
  {
  if (noOfUpdates > 0 && timer.getTimeSec() > 1.0) {
  curFrame++;
  noOfUpdates = 0;
  timer.reset();
  SAGE_PRINTLOG("sync group %d : time out", id);
  return true;
  }

  return false;
  }
*/

void syncGroup::checkHold()
{
  pthread_mutex_lock(&syncLock);
  while(holdSync)
    pthread_cond_wait(&resumeSync, &syncLock);
  pthread_mutex_unlock(&syncLock);
}

void syncGroup::blockSync()
{
  pthread_mutex_lock(&syncLock);
  holdSync = true;
  pthread_mutex_unlock(&syncLock);
}

void syncGroup::unblockSync()
{
  pthread_mutex_lock(&syncLock);
  holdSync = false;
  pthread_mutex_unlock(&syncLock);
  pthread_cond_signal(&resumeSync);
}

int syncGroup::enqueSyncMsg(char *msg)
{
  char *syncMsg = new char[strlen(msg)+1];
  strcpy(syncMsg, msg);

  pthread_mutex_lock(syncMsgLock);
  syncMsgQueue.push_back(syncMsg);
  pthread_mutex_unlock(syncMsgLock);

  return 0;
}

char* syncGroup::dequeSyncMsg()
{
  char *msg = NULL;
  pthread_mutex_lock(syncMsgLock);
  if (syncMsgQueue.size() > 0) {
    msg = syncMsgQueue.front();
    syncMsgQueue.pop_front();
  }
  pthread_mutex_unlock(syncMsgLock);

  return msg;
}




//--------------------------------------------  S E R V E R   C O D E  ---------------------------------------------//
sageSyncBBServer::sageSyncBBServer(int sl) : maxSyncGroupID(-1), syncEnd(false), maxSlaveSockFd(0), maxBarrierSlaveSockFd(0)
{
  //for (int i=0; i<MAX_SYNC_GROUP; i++) {
  // syncGroupArray[i] = NULL;
  //}

  FD_ZERO(&slaveFds);
  FD_ZERO(&barrierSlaveFds);

  syncLevel = sl;

  barrierPort = 0;
  syncSlavesMap.clear();
}

int sageSyncBBServer::init(int port)
{
  int optVal, optLen;

  // create the server socket
  if((serverSockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    SAGE_PRINTLOG("sageSyncBBServer::init(): Creating server socket failed");
    return -1;
  }

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::init(): Error setting SO_REUSEADDR");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::init(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::init(): Error switching on OOBINLINE.");

#else
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, (socklen_t)optLen) !=0)
    perror("sageSyncBBServer::init():: Error setting SO_REUSEADDR");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::init(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::init(): Error switching on OOBINLINE.");

#endif

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port = htons(port);

  if (bind(serverSockFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) != 0) {
    SAGE_PRINTLOG("sageSyncBBServer::init():: Error binding server socket");
    return -1;
  }

  // put in listen mode
  listen(serverSockFd, 10);

  // create the syncServerThread
  if (pthread_create(&syncServerThreadID, NULL, syncServerThread, (void*) this) !=0) {
    SAGE_PRINTLOG("sageSyncBBServer::init(): Creating syncServerThread failed");
    return -1;
  }

  //timer.reset();

  return 0;
}

int sageSyncBBServer::initBarrier(int barrierPort)
{
  int optVal, optLen;

  // create the server socket
  if((barrierServerSockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    SAGE_PRINTLOG("sageSyncBBServer::initBarrier(): Creating server socket failed");
    return -1;
  }

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
  if(setsockopt(barrierServerSockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::initBarrier(): Error setting SO_REUSEADDR");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(barrierServerSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::initBarrier(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(barrierServerSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::initBarrier(): Error switching on OOBINLINE.");

#else
  if(setsockopt(barrierServerSockFd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, (socklen_t)optLen) !=0)
    perror("sageSyncBBServer::initBarrier():: Error setting SO_REUSEADDR");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(barrierServerSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::initBarrier(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(barrierServerSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncBBServer::initBarrier(): Error switching on OOBINLINE.");
#endif

  memset(&barrierServerAddr, 0, sizeof(barrierServerAddr));
  barrierServerAddr.sin_family = AF_INET;
  barrierServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  barrierServerAddr.sin_port = htons(barrierPort);

  if (bind(barrierServerSockFd, (struct sockaddr *)&barrierServerAddr, sizeof(struct sockaddr_in)) != 0) {
    SAGE_PRINTLOG("sageSyncBBServer::initBarrier():: Error binding server socket");
    return -1;
  }

  // put in listen mode
  listen(barrierServerSockFd, 10);

  // create the syncBarrierServerThread
  if (pthread_create(&syncBarrierServerThreadID, NULL, syncBarrierServerThread, (void*) this) !=0) {
    SAGE_PRINTLOG("sageSyncBBServer::initBarrier(): Creating syncBarrierServerThread failed");
    return -1;
  }

  return 0;
}

// The sync server thread, which is responsible for listening for new sync clients
// This thread also polls each connected slave to look for frame updates
// and then updates the slave data vector

void* sageSyncBBServer :: syncServerThread(void *args)
{
  sageSyncBBServer *This;
  This = (sageSyncBBServer *)args;

  int tempSockFd;
  int tempBarrierSockFd;
  int addrLen;
  int optVal,optLen;

  sockaddr_in clientAddr;

  // Bug Fix:
  //   wait for fsManager to be ready
  sage::sleep(3);

  while (!This->syncEnd)   {
    // accept connections
    addrLen = sizeof(clientAddr);
    memset(&clientAddr, 0, addrLen);

    if ((tempSockFd = accept(This->serverSockFd, (struct sockaddr *)&clientAddr,(socklen_t*)&addrLen)) == -1) {
      SAGE_PRINTLOG("sageSyncBBServer::syncServerThread()::Quitting sync server thread - accept failed");
      pthread_exit(NULL);
      continue;
    }

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer: Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer: Error switching on OOBINLINE.");

#else
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer: Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer: Error switching on OOBINLINE.");

#endif

    // add the client's info to the list
    syncSlaveData newClient;
    newClient.clientSockFd = tempSockFd;
    memcpy((void *)&(newClient.clientAddr), (void *)&clientAddr, sizeof(clientAddr));
    This->maxSlaveSockFd = MAX(This->maxSlaveSockFd, tempSockFd);
    FD_SET(tempSockFd, &This->slaveFds);

    //This->syncSlaves[ This->syncSlaves.size() - 1 ];
    // syncSlave sends its SDM number
    sage::recv(tempSockFd, (void*)&newClient.SDM, sizeof(int), MSG_WAITALL);

    //This->syncSlaves.push_back(newClient); // copy occurs here
    This->syncSlavesMap[newClient.SDM] = newClient; // copy occurs

    SAGE_PRINTLOG("\nsageSyncBBServer::syncServerThread() : SDM %d (socket %d) has connected. MaxFD is %d", newClient.SDM, This->syncSlavesMap[newClient.SDM].clientSockFd, This->maxSlaveSockFd);

  } // end of while(!This->syncEnd)
  //pacify compiler
  return NULL;
} // end of sageSyncBBServer :: syncServerThread()

void* sageSyncBBServer::syncBarrierServerThread(void *args)
{
  SAGE_PRINTLOG("sageSyncBBServer::syncBarrierServerThread() : started.");

  sageSyncBBServer *This = (sageSyncBBServer *)args;

  int tempBarrierSockFd;
  int addrLen;
  int optVal,optLen;

  sockaddr_in clientAddr;
  int sdm = 0;

  //
  // wait  before syncClients are ready to connect
  // Without this, accept() will return with error EINTR (interrupted system call)
  //
  sage::sleep(2);

  while (!This->syncEnd)   {
    // accept connections
    addrLen = sizeof(clientAddr);

    if ((tempBarrierSockFd = accept(This->barrierServerSockFd, (struct sockaddr *)&clientAddr,(socklen_t*)&addrLen)) == -1) {
      SAGE_PRINTLOG("sageSyncBBServer::syncBarrierServerThread()::Quitting sync server thread.");
      pthread_exit(NULL);
    }

    /**
     * this could be an alternate solution instead of sage::sleep(1) up there
     *
     while(1) {
     tempBarrierSockFd = ::accept4(This->barrierServerSockFd, (struct sockaddr *)&clientAddr,(socklen_t*)&addrLen, SOCK_NONBLOCK);
     if (tempBarrierSockFd > 0) {
     break;
     }
     ::usleep(10000);
     }
    **/

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempBarrierSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempBarrierSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : Error switching on OOBINLINE.");

#else
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempBarrierSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempBarrierSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : Error switching on OOBINLINE.");

#endif

    sage::recv(tempBarrierSockFd, (void*)&sdm, sizeof(int), MSG_WAITALL);

    ((This->syncSlavesMap)[sdm]).barrierClientSockFd = tempBarrierSockFd;

    This->maxBarrierSlaveSockFd = MAX(This->maxBarrierSlaveSockFd, tempBarrierSockFd);

    FD_SET(tempBarrierSockFd, &This->barrierSlaveFds);

    SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : SDM %d (socket %d) has connected. MaxFD is %d", sdm, This->syncSlavesMap[sdm].barrierClientSockFd, This->maxBarrierSlaveSockFd);


    /*
      std::map<int, syncSlaveData*>::iterator it;
      it = This->syncSlavesMap.find( sdm );

      if ( it != This->syncSlavesMap.end() ) {
      if ( (it->second)->SDM == sdm ) {
      (it->second)->barrierClientSockFd = tempBarrierSockFd;
      This->maxBarrierSlaveSockFd = MAX(This->maxBarrierSlaveSockFd, tempBarrierSockFd);
      FD_SET(tempBarrierSockFd, &This->barrierSlaveFds);

      SAGE_PRINTLOG("sageSyncBBServer::syncBarrierServerThread() : syncSlave %d has registered to barrier with socket(%d), Max is %d\n", it->second->SDM, it->second->barrierClientSockFd, This->maxBarrierSlaveSockFd);
      }
      else {
      SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : nodeID in the map is different from that of syncSlaveData\n");
      }
      }
      else {
      SAGE_PRINTLOG("\nsageSyncBBServer::syncBarrierServerThread() : map iterator hit the end with SDM %d\n", sdm);
      }
    */
  } // end of while(!This->syncEnd)
  //pacify compiler
  return NULL;
} // end of sageSyncBBServer :: syncBarrierServerThread()

int sageSyncBBServer::startManagerThread(int totalRcvNum, int refreshInterval, int syncMasterPollingInterval) {
  this->totalRcvNum = totalRcvNum;
  this->refreshInterval = refreshInterval;
  this->syncMasterPollingInterval = syncMasterPollingInterval;

  while(1) {
    sage::sleep(1);
    if ( totalRcvNum == syncSlavesMap.size() ) {
      SAGE_PRINTLOG("sageSyncBBServer::startManagerThread() : All %d sync clients have connected.\n", syncSlavesMap.size());
      sage::sleep(1);
      break;
    }
  }


  if (pthread_create(&syncThreadID, NULL, mainLoopThread, (void*) this) !=0) {
    SAGE_PRINTLOG("sageSyncBBServer::startManagerThread(): Creating mainLoopThread failed");
    return -1;
  }
}

void* sageSyncBBServer::mainLoopThread(void *args)
{
  sageSyncBBServer *This = (sageSyncBBServer *)args;

  /**
   * speed of syncMaster
   */
  double interval = 1.0 / (double)(This->refreshInterval); // second

  /**
   * polling interval
   */
  struct timeval selTimer;
  selTimer.tv_sec = 0;
  selTimer.tv_usec = This->syncMasterPollingInterval;
  //selTimer.tv_usec = 1000; // 1msec

  SAGE_PRINTLOG("\n\nsageSyncBBServer::mainLoopThread() has started !! TotalRcvNum %d, refreshInterval %d Hz(%.6f sec), select polling interval %ld usec\n", This->totalRcvNum, This->refreshInterval, interval, selTimer.tv_usec);
  if ( This->syncLevel == 1 ) {
    SAGE_PRINTLOG("Data Sync Only\n");
  }
  else if ( This->syncLevel == 2 ) {
    SAGE_PRINTLOG("Two-Phase : SwapBuffer Barrier\n");
  }
  else if ( This->syncLevel == 3 ) {
    SAGE_PRINTLOG("One-Phase : SYNC with NTP\n");
  }

#ifdef PROFILING_SYNCMASTER
  FILE *profile = NULL;
  char fname[10];
  sprintf(fname, "/home/evl/snam5/synchProfile/syncMaster_%d", This->totalRcvNum);
  profile = fopen(fname, "w+");
  struct timeval pTimer;
  unsigned long loopCounter = 0;
#endif

  /**
   * for selective barrier
   */
  /*
    bool activeNodes[This->totalRcvNum];
    for ( int i=0; i<This->totalRcvNum; i++ ) {
    activeNodes[i] = false;
    }
  */

  //bool SDMlistBoolset[MAX_INST_NUM][This->totalRcvNum]; // faster

  //std::vector<int> listOfApps[This->totalRcvNum];

  /**
   * number of current total SDM invovled in the application indexed by array index, array index is APP id(PDL id)
   */
  //int slaveNumArray[MAX_INST_NUM];



  /**
   * list of SDM involved in the application indexed by array index,
   * array index is APP id(PDL id)
   * SDMlistBitset[0].set(3) means SDM 3 has PDL 0
   */
  //std::bitset<50> SDMlistBitset[MAX_INST_NUM]; // space efficient

  std::map<int, std::bitset<100> > SDMlistBitsetMap;
  SDMlistBitsetMap.clear();

  /**
   * The syncFrame of each App(PDL)
   */
  //int syncFrameArray[MAX_INST_NUM];
  std::map<int, int> syncFrameMap; // appID, frameNumber
  syncFrameMap.clear();

  // max frame number for each APP to determine who's leading or lagging
  //int maxFrameNum[MAX_INST_NUM];

  /**
   * Is this application ready ?
   * Or  slaveNum(activeRcvs) == SDMlistBitset[App].count() then it's ready
   */
  //bool isReadyToSwapMon[MAX_INST_NUM];
  std::map<int,bool> isReadyToSwapMonMap; // appID, value
  isReadyToSwapMonMap.clear();

  /**
   * init data structure
   */
  /*
    for ( int i=0; i<MAX_INST_NUM; i++ ) {
    //slaveNumArray[i] = 0;
    syncFrameArray[i] = 0;
    isReadyToSwapMon[i] = false;
    SDMlistBitset[i].reset();
    }
  */

  /**
   * message container for receiving PDL's update
   */
  char msg[SAGE_SYNC_MSG_LEN]; // this is still needed to receive sync message from clients
  //std::map<int,int> msgMap; // appID, syncFrame
  //msgMap.clear();

  /**
   * message container for broadcasting to nodes
   * it tells which app is ready for which frame
   * intMsg[4] == 1 then PDL 4 is ready AND intMsg[5] = syncFrame
   * In 64bit machine, MAX_INST_NUM should be less than or equal to 80 if SAGE_SYNC_MSG_LEN is 1280 Byte
   */
#ifdef DELAY_COMPENSATION
  long long intMsg[ SAGE_SYNC_MSG_LEN / sizeof(long long) ]; // 1280 / 8 = 160
#else
  //int intMsg[ SAGE_SYNC_MSG_LEN / sizeof(int) ]; // 1280 / 4 = 320 or 1280 / 8 = 160
  //int intMsg[ 8000 / sizeof(int) ];
#endif

  int sdm,pdl,slaveNum,updatedFrame;

  /**
   * Delay compensation thing
   */
  int nodeLatency; // delay compensation. in usec
  int maxNodeLatency = 0;

  int barrierCount = 0;
  int selectRetval;
  int netStatus;
  bool swapMontageReady = false;

  struct timeval tvs,tve;
  double elapsed = 0.0;

  int numUpdatedApps = 0; // how many apps are updated in this round
  int *intMsg = NULL;
  int intMsg_byteLen = 0;

  gettimeofday(&tvs, NULL);

  while(!This->syncEnd) {
    while(1) {

      fd_set rfds = This->slaveFds;
      selectRetval = select( This->maxSlaveSockFd+1, &rfds, NULL, NULL, &selTimer ); // polling
      if ( selectRetval < 0 ) {
        SAGE_PRINTLOG("sageSyncBBServer::managerThread() : select error\n");
        continue;
      }

      // for all node
      if ( selectRetval > 0 ) {
        for ( int i=0; i<This->syncSlavesMap.size(); i++ ) {
          if ( FD_ISSET( This->syncSlavesMap[i].clientSockFd, &rfds ) ) {

            //status = recv(array[i], (void*)dataArray, 4*sizeof(int), MSG_WAITALL);
            netStatus = sage::recv(This->syncSlavesMap[i].clientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN);
            if ( netStatus == 0 ) {
              // close the socket and mark it as dead
#ifdef WIN32
              closesocket(This->syncSlaves[i].clientSockFd);
#else
              shutdown(This->syncSlavesMap[i].clientSockFd, SHUT_RDWR);
              close(This->syncSlavesMap[i].clientSockFd);
#endif
              FD_CLR(This->syncSlavesMap[i].clientSockFd, &(This->slaveFds));
              This->syncSlavesMap[i].clientSockFd = -1;
            }
            else {
              //id, frame, rcvNum, type, SDMnum
              // receive message from a node
              sscanf(msg, "%d %d %d %d %d", &pdl, &updatedFrame, &slaveNum, &sdm, &nodeLatency);

#ifdef DELAY_COMPENSATION
              maxNodeLatency = MAX(maxNodeLatency, nodeLatency);
#endif

              //SAGE_PRINTLOG("\t SDM %d PDL %d : upFrm %d, actRcvs %d\n", sdm, pdl, updatedFrame, slaveNum);
#ifdef DEBUG_SYNC
              SAGE_PRINTLOG("\trecved update : [%d,%d] updF %d, activeRcv %d\n", sdm, pdl, updatedFrame, slaveNum);
#endif

              // add this node to the activeNode per application
              // update reported node list for this application(pdl id)
              //SDMlistBitset[pdl].set(sdm, 1);

              // bitset will be initialized with zeros
              (SDMlistBitsetMap[pdl]).set(sdm, 1);

              // update frame number of each PDL of each SDM
              //frameNumOfEachSDM[pdl][sdm] = updatedFrame;

              // find number of SDM involved in this application
              // update number of active node for this application(pdl id)
              //slaveNumArray[pdl] = slaveNum;

              // this should be same for all nodes of this application
              //syncFrameArray[pdl] = updatedFrame;
              syncFrameMap[pdl] = updatedFrame;

              //frameNumOfEachSDM[pdl][sdm] = updatedFrame;

              // list of apps that this node has
              //listOfApps[sdm].push_back(pdl);

              // if all nodes of this application reported
              //if ( slaveNumArray[pdl] <= SDMlistBitset[pdl].count() ) {
              if ( slaveNum == (SDMlistBitsetMap[pdl]).count() ) {
                swapMontageReady = true;
                numUpdatedApps++;


#ifdef DEBUG_SYNC
                SAGE_PRINTLOG("\tIt's ready : App %d is ready. SF %d\n", pdl, syncFrameMap[pdl]);
#endif

                // then they can do swapMontage
                //isReadyToSwapMon[pdl] = true;
                isReadyToSwapMonMap[pdl] = true;

                // reset data structure for next round
                // comment this out for SELECTIVE barrier
                (SDMlistBitsetMap[pdl]).reset(); // must be here for BROADCAST barrier, comment out for selective barrier

                // lots of contention will be happening
                //if ( This->syncLevel == 1 ) break;
              }
              else if ( slaveNum < (SDMlistBitsetMap[pdl]).count() ) {
                SAGE_PRINTLOG("\n\t ActRcv %d > SDMlist %d\n", slaveNum, (SDMlistBitsetMap[pdl]).count() );
              }
            } // if ( netStatus > 0) // sage::recv() returned with data
            //break; // exit for loop -> time check after every message receive
          } // if (FD_ISSET)
        } // foreach node
      } // if (selectRetval > 0)

      // still contention
      //if ( This->syncLevel == 1 && swapMontageReady ) break;

      gettimeofday(&tve, NULL);
      elapsed += ((double)tve.tv_sec + 0.000001*(double)tve.tv_usec)-((double)tvs.tv_sec + 0.000001*(double)tvs.tv_usec);
      tvs = tve;
      //SAGE_PRINTLOG("%.6f\n", elapsed);
      if ( elapsed >= interval ) {
        // 0.0083 sec =  8.3 msec = 120Hz
        // 0.0167 sec = 16.7 msec =  60Hz
        //SAGE_PRINTLOG("%.6f msec \n", elapsed*1000.0);

        elapsed = 0.0;
        break; // break while(1) loop
      }
    } // end while(1)

    //SAGE_PRINTLOG("\nTIMEOUT\n");

    // for each application
    // it memset with zeros, then SDM::processSync will read wrong value
    // if it's unsigned type, then it will become UINT_MAX, ULONG_MAX, or ULLONG_MAX
    intMsg_byteLen = sizeof(int) + (sizeof(int) * numUpdatedApps * 2); // sizeof(int) byte is for MSG_PEEK at the SDM
    intMsg = (int *)malloc(intMsg_byteLen);
    memset(intMsg, -1, intMsg_byteLen);
    intMsg[0] = intMsg_byteLen;

    if ( swapMontageReady ) {
      // then we need to prepare message for them
      swapMontageReady = false; // reset

      // for each App
      int intMsgIndex = 0; // the first index is byte length.
      for ( std::map<int,bool>::iterator it=isReadyToSwapMonMap.begin(); it!=isReadyToSwapMonMap.end(); it++ ) {
        int appID = (*it).first;
        // if it's ready ( all node that have this app reported )

        if ( (*it).second ) {
          isReadyToSwapMonMap[ appID ] = false; // reset

          intMsg[2*intMsgIndex + 1] = appID;
          intMsg[2*intMsgIndex + 2] = syncFrameMap[appID];
          intMsgIndex++;
        }
        if ( intMsgIndex >= numUpdatedApps ) {
          break;
        }
      }
    }

#ifdef DEBUG_SYNC
    SAGE_PRINTLOG("\tUpdatedApps : ");
    for ( int i=0; i<numUpdatedApps; i++ ) {
      SAGE_PRINTLOG("(%d,%d) ", intMsg[2*i], intMsg[2*i+1]);
    }
    SAGE_PRINTLOG("\n");
#endif
    numUpdatedApps = 0; // reset

    /** temporary delat_compensation for 1st phase only */
    if ( This->syncLevel == 3 ) {
      //int intMsg[ SAGE_SYNC_MSG_LEN / sizeof(int) ];
      //int index = SAGE_SYNC_MSG_LEN / sizeof(int); // 320 when 4byte int
      int index = SAGE_SYNC_MSG_LEN / sizeof(long long); // 160

      struct timeval initT;
      gettimeofday(&initT, NULL);
      //sprintf(msg, "%d %d %d", initT.tv_sec, initT.tv_usec, maxDeltaT);

      //intMsg[index-1] = maxNodeLatency; // in usec
      intMsg[index-1] = 7000; // Presentation Time Offset in usec
      intMsg[index-2] = initT.tv_sec;
      intMsg[index-3] = initT.tv_usec;
      maxNodeLatency = 0;
    }
    /** temporary delat_compensation for 1st phase only */

    // Broadcast -> will trigger EVENT_SYNC_MESSAGE on all node
#ifdef PROFILING_SYNCMASTER
    // node%d:pdl%d:frame%d:ITEM:%d:%d
    gettimeofday(&pTimer, NULL);
    fprintf(profile, "%d:%d:%lu:Broadcast_B:%ld:%ld\n", -1,-1,loopCounter,pTimer.tv_sec, pTimer.tv_usec);
#endif
    for ( int i=0; i<This->syncSlavesMap.size(); i++ ) {
      if ( sage::send(This->syncSlavesMap[i].clientSockFd, (void*)intMsg, intMsg_byteLen) < intMsg_byteLen ) {
        SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : send() error at the 1st phase\n");
      }
    }
#ifdef PROFILING_SYNCMASTER
    // node%d:pdl%d:frame%d:ITEM:%d:%d
    gettimeofday(&pTimer, NULL);
    fprintf(profile, "%d:%d:%lu:Broadcast_E:%ld:%ld\n", -1,-1,loopCounter,pTimer.tv_sec, pTimer.tv_usec);
#endif
    if (intMsg) {
      free(intMsg);
      intMsg = NULL;
    }

    /**
     * Barrier Before SwapBuffer
     */
    //SAGE_PRINTLOG("\nBARRIER\n");

    /* SELECTIVE BARRIER */
    /**
       int activeNodeSize = 0;
       for ( int i=0; i<MAX_INST_NUM; i++ ) {
       if ( isReadyToSwapMon[i] ) {
       isReadyToSwapMon[i] = false;
       for ( int j=0; j<SDMlistBitset[i].size(); j++ ) {
       if ( SDMlistBitset[i].test(j) ) {
       activeNodes[j] = true;
       activeNodeSize++;
       }
       }
       SDMlistBitset[i].reset(); // bitset is being reset here !!
       }
       }
       while ( barrierCount < activeNodeSize ) {
       fd_set rfds2 = This->barrierSlaveFds;
       selectRetval = select( This->maxBarrierSlaveSockFd+1, &rfds2, NULL, NULL, NULL ); // blocking
       if ( selectRetval < 0 ) {
       SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : select(refresh barrier) error\n");
       exit(1);
       }
       for ( int i=0; i<This->syncSlavesMap.size(); i++ ) {

       if ( !activeNodes[i] ) continue;

       if ( FD_ISSET( This->syncSlavesMap[i].barrierClientSockFd, &rfds2 ) ) {

       netStatus = sage::recv(This->syncSlavesMap[i].barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN);
       //netStatus = ::recv(This->syncSlavesMap[i].barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN, 0);

       if ( netStatus <= 0 ) {
       SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : recv error at the barrier entry phase\n");
       }
       barrierCount++;
       }
       }
       }
       for ( int i=0; i<This->syncSlavesMap.size(); i++ ) {
       if ( activeNodes[i] ) {
       activeNodes[i] = false;

       // NON_BLOCKING
       //netStatus = ::send(This->syncSlavesMap[i].barrierClientSockFd, msg, SAGE_SYNC_MSG_LEN, MSG_DONTWAIT);

       //SAGE_PRINTLOG("BARRIER send to node%d\n", i);
       //netStatus = ::send(This->syncSlavesMap[i].barrierClientSockFd, msg, SAGE_SYNC_MSG_LEN, 0);


       // BLOCING
       netStatus = sage::send(This->syncSlavesMap[i].barrierClientSockFd, msg, SAGE_SYNC_MSG_LEN);

       if ( netStatus <= 0 ) {
       SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : send error when barrier broadcasting\n");
       }
       }
       }
       barrierCount = 0;
       activeNodeSize = 0;
    **/
    /* Ends SELECTIVE barrier */



    /** BROADCASTING BARRIER ***/
    if ( This->syncLevel == 2 ) {

#ifdef PROFILING_SYNCMASTER
      // node%d:pdl%d:frame%d:ITEM:%d:%d
      gettimeofday(&pTimer, NULL);
      fprintf(profile, "%d:%d:%lu:Barrier_B:%ld:%ld\n", -1,-1,loopCounter,pTimer.tv_sec, pTimer.tv_usec);
#endif
      int nodeID,deltaT; // in usec
      int maxDeltaT = 0;

      while(barrierCount < This->syncSlavesMap.size()) {
        fd_set rfds2 = This->barrierSlaveFds;
        selectRetval = select( This->maxBarrierSlaveSockFd+1, &rfds2, NULL, NULL, NULL ); // blocking
        if ( selectRetval < 0 ) {
          SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : select(refresh barrier) error\n");
          exit(1);
        }
        for ( int i=0; i<This->syncSlavesMap.size(); i++ ) {
          if ( FD_ISSET( This->syncSlavesMap[i].barrierClientSockFd, &rfds2 ) ) {
            netStatus = sage::recv(This->syncSlavesMap[i].barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN);
            /*
              #ifdef DELAY_COMPENSATION
              sscanf(msg, "%d %d", &nodeID, &deltaT);
              maxDeltaT = MAX(maxDeltaT, deltaT);
              #endif
            */
            if ( netStatus <= 0 ) {
              SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : recv error at the barrier entry phase\n");
            }
            barrierCount++;
          }
        }
      } // end barrier loop
      barrierCount = 0;

      /*
        #ifdef DELAY_COMPENSATION
        struct timeval initT;
        gettimeofday(&initT, NULL);
        sprintf(msg, "%d %d %d", initT.tv_sec, initT.tv_usec, maxDeltaT);
        #endif
      */
      for ( int i=0; i<This->syncSlavesMap.size(); i++ ) {
        netStatus = ::send(This->syncSlavesMap[i].barrierClientSockFd, msg, SAGE_SYNC_MSG_LEN, MSG_DONTWAIT); // non-block
        //netStatus = ::send(This->syncSlavesMap[i].barrierClientSockFd, msg, SAGE_SYNC_MSG_LEN, 0); // blocking
        if ( netStatus <= 0 ) {
          SAGE_PRINTLOG("sageSyncBBServer::mainLoopThread() : Refresh Barrier send error to node %d", This->syncSlavesMap[i].SDM);
        }
      }
    } // if ( syncLevel == 2)
#ifdef PROFILING_SYNCMASTER
    // node%d:pdl%d:frame%d:ITEM:%d:%d
    gettimeofday(&pTimer, NULL);
    fprintf(profile, "%d:%d:%lu:Barrier_E:%ld:%ld\n", -1,-1,loopCounter,pTimer.tv_sec, pTimer.tv_usec);
#endif
    /* BROADCAST Barrier ends */

#ifdef PROFILING_SYNCMASTER
    loopCounter++;
#endif
  } // end while(syncEnd)

  pthread_exit(NULL);
  return NULL;
}

void sageSyncBBServer::killAllClients()
{
  int noOfSyncSlaves = syncSlaves.size();
  int i;

  for(i=0; i< noOfSyncSlaves; i++)   {
    // Check to see if the client has been removed
    if(syncSlaves[i].clientSockFd == -1) continue;

#ifdef WIN32
    closesocket(syncSlaves[i].clientSockFd);
#else
    shutdown(syncSlaves[i].clientSockFd, SHUT_RDWR);
    close(syncSlaves[i].clientSockFd);
#endif
  }

  noOfSyncSlaves = 0;
}

sageSyncBBServer :: ~sageSyncBBServer()
{
  syncEnd = true;
#ifdef WIN32
  closesocket(serverSockFd);
  closesocket(barrierServerSockFd);
#else
  shutdown(serverSockFd, SHUT_RDWR);
  shutdown(barrierServerSockFd, SHUT_RDWR);
  close(serverSockFd);
  close(barrierServerSockFd);
#endif

  int noOfSyncSlaves = syncSlaves.size();
  int i;

  for (i=0; i< noOfSyncSlaves; i++)   {
    // Check to see if the client has been removed
    if(syncSlaves[i].clientSockFd == -1)
      continue;

#ifdef WIN32
    closesocket(syncSlaves[i].clientSockFd);
#else
    shutdown(syncSlaves[i].clientSockFd, SHUT_RDWR);
    shutdown(syncSlaves[i].barrierClientSockFd, SHUT_RDWR);
    close(syncSlaves[i].clientSockFd);
    close(syncSlaves[i].barrierClientSockFd);
#endif

  }

  //wait for the thread to quit
  pthread_join(syncThreadID, NULL);

} // End of ~sageSyncBBServer()





sageSyncServer::sageSyncServer() : asapSyncGroupNum(0), maxSyncGroupID(-1), syncEnd(false),
                                   maxSlaveSockFd(0)
{
  FD_ZERO(&slaveFds);
}

int sageSyncServer::init(int port)
{
  int optVal, optLen;

  // create the server socket
  if((serverSockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    SAGE_PRINTLOG("sageSyncServer::init(): Creating server socket failed");
    return -1;
  }

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncServer::init(): Error setting SO_REUSEADDR");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncServer::init(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncServer::init(): Error switching on OOBINLINE.");

#else
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, (socklen_t)optLen) !=0)
    perror("sageSyncServer::init():: Error setting SO_REUSEADDR");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncServer::init(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(serverSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncServer::init(): Error switching on OOBINLINE.");

#endif

  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port = htons(port);

  if (bind(serverSockFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) != 0) {
    SAGE_PRINTLOG("sageSyncServer::init():: Error binding server socket");
    return -1;
  }

  // put in listen mode
  listen(serverSockFd, 10);

  // create the syncServerThread
  if (pthread_create(&syncThreadID, NULL, syncServerThread, (void*) this) !=0) {
    SAGE_PRINTLOG("sageSyncServer::init(): Creating sync thread failed");
    return -1;
  }

  if (pthread_create(&groupThreadID, NULL, managerThread, (void*)this) !=0) {
    SAGE_PRINTLOG("sageSyncServer::init(): Creating manager thread failed");
    return -1;
  }

  //timer.reset();

  return 0;
}

// The sync server thread, which is responsible for listening for new sync clients
// This thread also polls each connected slave to look for frame updates
// and then updates the slave data vector

void* sageSyncServer :: syncServerThread(void *args)
{
  sageSyncServer *This;
  This = (sageSyncServer *)args;

  int tempSockFd;
  int addrLen;
  int optVal,optLen;

  sockaddr_in clientAddr;

  while (!This->syncEnd)   {

    // accept connections
    addrLen = sizeof(clientAddr);

    if ((tempSockFd = accept(This->serverSockFd, (struct sockaddr *)&clientAddr,
                             (socklen_t*)&addrLen)) == -1) {
      SAGE_PRINTLOG("sageSyncServer::syncServerThread()::Quitting sync server thread.");
      pthread_exit(NULL);
      continue;
    }

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncServer: Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncServer: Error switching on OOBINLINE.");

#else
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncServer: Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(tempSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncServer: Error switching on OOBINLINE.");

#endif

    // add the client's info to the list
    syncSlaveData newClient;
    newClient.clientSockFd = tempSockFd;
    memcpy((void *)&(newClient.clientAddr), (void *)&clientAddr, sizeof(clientAddr));
    This->syncSlaves.push_back(newClient);
    This->maxSlaveSockFd = MAX(This->maxSlaveSockFd, tempSockFd);
    FD_SET(tempSockFd, &This->slaveFds);
  }

  //pacify compiler
  return NULL;
} // end of sageSyncServer :: syncServerThread()

int sageSyncServer::sendSync(syncGroup *grp, int cmd)
{
  if (!grp) {
    SAGE_PRINTLOG("sageSyncServer::sendSync : invalid sync group ID");
    return -1;
  }

  char *data = grp->dequeSyncMsg();

  int msgSize = SAGE_SYNC_MSG_LEN;
  char msg[SAGE_SYNC_MSG_LEN];
  int dataLen = 0;

  if (data)
    dataLen = strlen(data) + 1;
  sprintf(msg, "%d %d %d %d", grp->id, grp->curFrame, dataLen, cmd);

  //std::cout << "send sync " << msg << std::endl;

  int noOfSyncSlaves = syncSlaves.size();

  // send the sync message to all the slaves listed with the server
  for (int i=0; i<noOfSyncSlaves; i++) {
    // Check to see if the client has been removed
    if (syncSlaves[i].clientSockFd == -1)
      continue;


    //sungwon
    //SAGE_PRINTLOG("sageSyncServer::sendSync: sending sync msg to slave %d, (%s)\n", i, msg);

    /// sends sync message here
    int sentSize = sage::send(syncSlaves[i].clientSockFd, (void *)msg, msgSize);

    if (sentSize == 0) {
      SAGE_PRINTLOG("sageSyncServer::sendSync : connection shutdown - client ",
                    (char *)inet_ntoa(syncSlaves[i].clientAddr.sin_addr));
      // Assuming that something went wrong with the socket connection. Closing it and marking it as dead
#ifdef WIN32
      closesocket(syncSlaves[i].clientSockFd);
#else
      shutdown(syncSlaves[i].clientSockFd, SHUT_RDWR);
      close(syncSlaves[i].clientSockFd);
#endif
      syncSlaves[i].clientSockFd = -1;
      return -1;
    }
    else if (sentSize < 0) {
      return -1;
    }
    else if (data) {

      //sungwon
      //SAGE_PRINTLOG("sageSyncServer::sendSync: syncGroup %d has data(%s) in sync msg\n", grp->id, data);

      sage::send(syncSlaves[i].clientSockFd, (void *)data, dataLen);
    }
  }

  if (data)
    delete [] data;

  return 0;
}

int sageSyncServer::removeSyncGroup(int id)
{
  int index;
  syncGroup* grp= findSyncGroup(id, index);
  if(grp)
  {
    //std::cout << "sageSyncServer: found... trying to kill... thread.." << id << " " << index << std::endl;
    if (grp->policy == SAGE_CONSTANT_SYNC) {
      // kill thread...
      grp->syncEnd = true;
      pthread_join(grp->threadID, NULL);
    }

    // remove object
    delete grp;
    grp = NULL;

    syncGroupArray.erase(syncGroupArray.begin() + index);
    //std::cout << "---------------------" << std::endl;
    //asapSyncGroupNum--;
  }
  return 0;
}

int sageSyncServer::addSyncGroup(syncGroup *grp)
{
  grp->syncServer = (sageSyncServer *)this;
  if (grp->id >= 0) {
    syncGroupArray.push_back(grp);
  }
  else {
    SAGE_PRINTLOG("sageSyncServer::addSyncGroup : invalid sync group ID");
    return -1;
  }

  maxSyncGroupID = MAX(grp->id, maxSyncGroupID);

  if (grp->policy == SAGE_CONSTANT_SYNC) {
    if (pthread_create(&grp->threadID, NULL, managerConstantThread, (void*)grp) !=0) {
      SAGE_PRINTLOG("sageSyncServer::init(): Creating manager thread failed");
      return -1;
    }
  }

  return 0;
}

void* sageSyncServer::managerConstantThread(void *args)
{
  syncGroup *grp = (syncGroup *)args;
  sageSyncServer *This = (sageSyncServer *)grp->syncServer;

  while (!grp->syncEnd) {
    This->sendSync(grp);
    sage::usleep((int)floor(grp->interval+0.5));
  }

  pthread_exit(NULL);
  return NULL;
}

void* sageSyncServer::managerThread(void *args)
{
  sageSyncServer *This;
  This = (sageSyncServer *)args;

  while (!This->syncEnd)   {
    This->manageUpdate();
  }

  pthread_exit(NULL);
  return NULL;
}

syncGroup* sageSyncServer::findSyncGroup(int id, int& index)
{
  syncGroup *grp = NULL;
  index =0;
  std::vector<syncGroup *>::iterator iter;
  for(iter = syncGroupArray.begin(); iter != syncGroupArray.end(); iter++, index++)
  {
    if((*iter)->getSyncID() == id)
    {
      grp = (syncGroup*) (*iter);
      return grp;
    }
  }
  index = -1;
  return NULL;
}

int sageSyncServer::manageUpdate()
{
  int maxSyncGroup = syncGroupArray.size();
  if (maxSyncGroup == 0) return -1;

  int noOfSyncSlaves = syncSlaves.size();
  char data[SAGE_SYNC_MSG_LEN];
  fd_set readFds = slaveFds;

  struct timeval timeOut;
  timeOut.tv_sec = 0;
  timeOut.tv_usec = SYNC_TIMEOUT;

  //std::cout << "waiting update" << std::endl;
  int returnVal = select(maxSlaveSockFd+1, &readFds, NULL, NULL, &timeOut);

  if (returnVal < 0) {
    SAGE_PRINTLOG("sageSyncServer::manageUpdate : error in waiting update message");
    return -1;
  }

  /**
   * Each sync group has its own maximum frame rate (or minimum interval).
   Even though a sync group is ready to proceed to the next frame,
   the sync master waits until the minimum interval is reached.
   Thus, a sync group can have two states:
   waiting for updates from slaves or waiting for the minimum interval.
   This code fragment was inserted to take care of the latter cases.

   If syncGroup::isWaiting() is true, the sync group has the latter state.
   syncGroup::checkInterval() returns if it reaches the interval or not.

   This code checks there is a sync group
   which is waiting for the interval.
   If yes and the interval is reached,
   it sends sync signal to the group members immediately
   because all the sync slaves are already ready to proceed the next frame.
  */
  //for (int i=0; i<=maxSyncGroupID; i++) {
  //   syncGroup *grp = syncGroupArray[i];
  syncGroup *grp = NULL;
  for (int i=0; i< maxSyncGroup; i++) {
    grp = syncGroupArray[i];
    if(!grp) continue;

    // if it's waiting to reach the interval
    if (grp && grp->isWaiting()) {

      // interval reached?
      if (grp->checkInterval()) {

        // if this syncGroup's sync is holding then waits on the condition resumeSync
        grp->checkHold();

        // send sync signal
        sendSync(grp);
      }
    }
  }

  for (int i=0; i < noOfSyncSlaves; i++)   {
    // Check to see if the client has been removed
    if (syncSlaves[i].clientSockFd == -1)
      continue;

    if (FD_ISSET(syncSlaves[i].clientSockFd, &readFds)) {
      int status = sage::recv(syncSlaves[i].clientSockFd, (void*)data, SAGE_SYNC_MSG_LEN);

      if (status == 0) {
        // close the socket and mark it as dead
#ifdef WIN32
        closesocket(syncSlaves[i].clientSockFd);
#else
        shutdown(syncSlaves[i].clientSockFd, SHUT_RDWR);
        close(syncSlaves[i].clientSockFd);
#endif
        FD_CLR(syncSlaves[i].clientSockFd, &slaveFds);
        syncSlaves[i].clientSockFd = -1;
      }
      else {
        int groupID;
        sscanf(data, "%d", &groupID);
        int index;
        syncGroup *grp = findSyncGroup(groupID, index);

        if (!grp) {
          SAGE_PRINTLOG("sageSyncServer::manageUpdate : invalid groupID %d", groupID);
          return -1;
        }

        //sungwon
        //SAGE_PRINTLOG("sageSyncServer::manageUpdate: calling grp %d's processUpdate\n", grp->id);

        int cmd = grp->processUpdate(data);
        if (cmd > 0) {
          //SAGE_PRINTLOG("send sync %d\n", grp->curFrame);
          if (grp->checkInterval()) {
            grp->checkHold();

            //sungwon
            //SAGE_PRINTLOG("sageSyncServer::manageUpdate: calling sendSync(%d, %d)\n", grp->id, cmd);

            sendSync(grp, cmd);
          }
        }
      }
    } // end if
  } // end for

  //if (timer.getTimeSec() > 1.0) {
  //   checkTimeOut();
  //   timer.reset();
  //}

  return 0;
}

/*
  int sageSyncServer::checkTimeOut()
  {
  for (int i=0; i<=maxSyncGroupID; i++) {
  syncGroup *grp = syncGroupArray[i];
  if (grp && grp->checkTimeOut()) {
  grp->checkHold();
  sendSync(grp);
  }
  }

  return 0;
  }

  int sageSyncServer::queueData(char *data, int id)
  {
  char *syncMsg = new char[strlen(data)+1];
  strcpy(syncMsg, data);
  syncGroupList[id]->syncMsgQueue.push_back(syncMsg);

  return 0;
  }
*/

void sageSyncServer::killAllClients()
{
  int noOfSyncSlaves = syncSlaves.size();
  int i;

  for(i=0; i< noOfSyncSlaves; i++)   {
    // Check to see if the client has been removed
    if(syncSlaves[i].clientSockFd == -1) continue;

#ifdef WIN32
    closesocket(syncSlaves[i].clientSockFd);
#else
    shutdown(syncSlaves[i].clientSockFd, SHUT_RDWR);
    close(syncSlaves[i].clientSockFd);
#endif

  }

  noOfSyncSlaves = 0;
}


sageSyncServer :: ~sageSyncServer()
{
  syncEnd = true;
#ifdef WIN32
  closesocket(serverSockFd);
#else
  shutdown(serverSockFd, SHUT_RDWR);
  close(serverSockFd);
#endif

  int noOfSyncSlaves = syncSlaves.size();
  int i;

  for (i=0; i< noOfSyncSlaves; i++)   {
    // Check to see if the client has been removed
    if(syncSlaves[i].clientSockFd == -1)
      continue;

#ifdef WIN32
    closesocket(syncSlaves[i].clientSockFd);
#else
    shutdown(syncSlaves[i].clientSockFd, SHUT_RDWR);
    close(syncSlaves[i].clientSockFd);
#endif

  }

  //wait for the thread to quit
  pthread_join(syncThreadID, NULL);
  pthread_join(groupThreadID, NULL);

} // End of ~sageSyncServer()


//--------------------------------------------  C L I E N T   C O D E  ---------------------------------------------//




sageSyncClient::sageSyncClient(int sl) : maxGroupID(-1), syncEnd(false)
{
  syncLevel = sl;
  int optVal, optLen;

  // create the client socket
  if((clientSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("sageSyncClient::sageSyncClient(): Creating server socket failed");
  }

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)

    SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching on OOBINLINE.");

#else

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching off Nagle's algorithm.");

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching on OOBINLINE.");

#endif

  if ( syncLevel == 2 ) {
    if((barrierClientSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("sageSyncClient::sageSyncClient(): Creating server socket failed");
    }

    // loosen the rules for check during bind to allow mutiple binds on the same port
    optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined (__sun)
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(barrierClientSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(barrierClientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)

      SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching on OOBINLINE.");
#else
    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(barrierClientSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching off Nagle's algorithm.");

    optVal = 1;
    optLen=sizeof(optVal);
    if(setsockopt(barrierClientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
      SAGE_PRINTLOG("\nsageSyncClient::sageSyncClient(): Error switching on OOBINLINE.");

#endif
  }
} // End of sageSyncClient :: sageSyncClient()

int sageSyncClient::connectToServer(char *syncServIP, int port, int SDMnum)
{
  int noOfConnAttempts = 0;
  sockaddr_in tempAddr;

  memset(&tempAddr, 0, sizeof(tempAddr));
  tempAddr.sin_family = AF_INET;
  tempAddr.sin_addr.s_addr = inet_addr(syncServIP);
  tempAddr.sin_port = htons(port);

  // try a few times before giving up
  //connect to the server
  while (noOfConnAttempts < 50) {
    if (connect(clientSockFd, (struct sockaddr *)&tempAddr, sizeof(struct sockaddr)) != 0) {
      char msg[TOKEN_LEN];
      sprintf(msg, "sageSyncClient :: connectToServer(): sync server %s : %d is not ready\n",
              syncServIP, port);
      SAGE_PRINTLOG(msg);
      noOfConnAttempts++;
    }
    else {
      if ( SDMnum != -1 ) { // it means new sync
        //SAGE_PRINTLOG("sageSyncClient::connectToServer() : Sending SDM %d\n", SDMnum);
        sage::send(clientSockFd, (void *)&SDMnum, sizeof(int));
      }
      return 0;
    }
    sage::sleep(1);
  }

  SAGE_PRINTLOG("sageSyncClient :: connectToServer(): Giving up after trying a few times");

  return -1;
}

int sageSyncClient::connectToBarrierServer(char *syncServIP, int port, int SDMnum)
{
  if ( syncLevel != 2 ) {
    SAGE_PRINTLOG("sageSyncClient::connectToBarrierServer() : syncLevel id %d. No swapbuffer barrier\n", syncLevel);
    return -1;
  }
  int noOfConnAttempts = 0;
  sockaddr_in tempAddr;

  memset(&tempAddr, 0, sizeof(tempAddr));
  tempAddr.sin_family = AF_INET;
  tempAddr.sin_addr.s_addr = inet_addr(syncServIP);
  tempAddr.sin_port = htons(port);

  // try a few times before giving up
  //connect to the server
  while (noOfConnAttempts < 50) {
    if (connect(barrierClientSockFd, (struct sockaddr *)&tempAddr, sizeof(struct sockaddr)) != 0) {
      char msg[TOKEN_LEN];
      sprintf(msg, "sageSyncClient :: connectToBarrierServer(): sync server %s : %d is not ready\n",syncServIP, port);
      SAGE_PRINTLOG(msg);
      noOfConnAttempts++;
    }
    else {
      sage::send(barrierClientSockFd, (void *)&SDMnum, sizeof(int));
      return 0;
    }
    sage::sleep(1);
  }
  SAGE_PRINTLOG("sageSyncClient :: connectToBarrierServer(): Giving up after trying a few times");
  return -1;
}

int sageSyncClient::addSyncGroup(int id)
{
  if (id < 0) {
    SAGE_PRINTLOG("sageSyncClient::addSyncGroup : group ID is out of scope");
    return -1;
  }

  sageCircBufSingle* buf = new sageCircBufSingle(SYNC_MSG_BUF_LEN, true);
  buf->instID = id;
  syncMsgBuf.push_back(buf);

  if (maxGroupID < 0) {
    maxGroupID = id;

    //sungwon
    //SAGE_PRINTLOG("sageSyncClient::addSyncGroup: starting syncClientThread for the group %d\n", id);

    if (pthread_create(&syncThreadID, NULL, syncClientThread, (void*) this) !=0) {
      SAGE_PRINTLOG("sageSyncClient::addSyncGroup(): Creating sync client thread failed");
      return -1;
    }
  }
  else
    maxGroupID = MAX(maxGroupID, id);

  std::cout << "added sync group " << id << std::endl;

  return 0;
}

int sageSyncClient::removeSyncGroup(int id)
{
  if (id < 0) {
    SAGE_PRINTLOG("sageSyncClient::removeSyncGroup : group ID is out of scope");
    return -1;
  }

  int index;
  sageCircBufSingle* buf = findSyncGroup(id, index);
  if(buf)
  {
    buf->releaseLock();
    delete buf;
    buf = NULL;
    syncMsgBuf.erase(syncMsgBuf.begin() + index);
  }


  return 0;
}

sageCircBufSingle* sageSyncClient::findSyncGroup(int id, int& index)
{
  sageCircBufSingle *buf = NULL;
  index =0;
  std::vector<sageCircBufSingle *>::iterator iter;
  for(iter = syncMsgBuf.begin(); iter != syncMsgBuf.end(); iter++, index++)
  {
    if((*iter)->instID == id)
    {
      buf = (sageCircBufSingle*) (*iter);
      return buf;
    }
  }
  index = -1;
  return NULL;
}

void* sageSyncClient::syncClientThread(void* args)
{
  //sungwon
  //SAGE_PRINTLOG("sageSyncClient::syncClientThread started\n");

  sageSyncClient *This = (sageSyncClient *)args;

  while (!This->syncEnd) {
    if (This->readSyncMsg() < 0)
      This->syncEnd = true;
  }

  pthread_exit(NULL);
  return NULL;
}

int sageSyncClient::sendSlaveUpdate(int frame, int id, int rcvNum, int type, int SDMnum)
{
  int dataSize = SAGE_SYNC_MSG_LEN;
  char msg[SAGE_SYNC_MSG_LEN];
  sprintf(msg, "%d %d %d %d %d", id, frame, rcvNum, type, SDMnum);

  int status = sage::send(clientSockFd, (void *)msg, dataSize);

  if (status !=  dataSize) {
    perror("sageSyncClient :: sendSlaveUpdate(): Error sending update message to sync master");
    return -1;
  }

  return 0;
}


int sageSyncClient::sendSlaveUpdateToBBS(int frame, int id, int rcvNum, int SDMnum, int delayCompenLatency)
{
  int dataSize = SAGE_SYNC_MSG_LEN;
  char msg[SAGE_SYNC_MSG_LEN];
  sprintf(msg, "%d %d %d %d %d", id, frame, rcvNum, SDMnum, delayCompenLatency);
#ifdef DEBUG_SYNC
  SAGE_PRINTLOG("[%d,%d] sageSyncClient::sendSlaveUpdateToBBS() : updateFrame %d\n", SDMnum, id, frame);
#endif

  //SAGE_PRINTLOG("SDM %d PDL %d sendUp frm %d rcvNum %d\n", SDMnum, id, frame, rcvNum);

  int status = sage::send(clientSockFd, (void *)msg, dataSize);

  if (status !=  dataSize) {
    perror("sageSyncClient :: sendSlaveUpdate(): Error sending update message to sync master");
    return -1;
  }

  return 0;
}

int sageSyncClient::sendRefreshBarrier(int nodeID) {
  char msg[SAGE_SYNC_MSG_LEN];
  sprintf(msg, "%d %d", nodeID, refreshBarrierDeltaT);

  // barrierClient->getSocketId();
  //int status = ::send(barrierClientSockFd, (void *)msg, SAGE_SYNC_MSG_LEN, 0);

  //SAGE_PRINTLOG("SDM%d send delta %d\n", nodeID, refreshBarrierDeltaT);

  int status = sage::send(barrierClientSockFd, (void *)msg, SAGE_SYNC_MSG_LEN);

  //if (status != SAGE_SYNC_MSG_LEN) {
  //SAGE_PRINTLOG("sageSyncClient::reportMontageUpdate(): SDM %d PDL %d, Error sending update message to barrier", nodeID, groupID);
  //return -1;
  //}
  return status;
}
int sageSyncClient::recvRefreshBarrier(bool nonblock) {
  char msg[SAGE_SYNC_MSG_LEN];

  //int status = ::recv(barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN, MSG_DONTWAIT);
  //int status = ::recv(barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN, MSG_WAITALL);
  //int status = ::recv(barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN, 0);

  int status = 0;
  if ( nonblock ) {
    //status = ::recv(barrierClientSockFd, (void*)msg, SAGE_SYNC_MSG_LEN, MSG_DONTWAIT);
    status = ::recv(barrierClientSockFd, (char*)msg, SAGE_SYNC_MSG_LEN, MSG_DONTWAIT);
  }
  else {
    status = ::recv(barrierClientSockFd, (char*)msg, SAGE_SYNC_MSG_LEN, 0);
  }
  return status;
}



int sageSyncClient::readSyncMsg()
{
  int dataSize = SAGE_SYNC_MSG_LEN;
  char msg[SAGE_SYNC_MSG_LEN];
  int groupID = -1;

  // read a message but leave it in socket buffer
  int status = sage::recv(clientSockFd, (void*)msg, dataSize);

  //sungwon
  //SAGE_PRINTLOG("sageSyncClient::readSyncMsg: recved msg (%s)\n", msg);

  if (status !=  dataSize) {
    perror("sageSyncClient :: waitForSync(): Error receiving sync from master");
    sage::sleep(1);
    return -1;
  }

  int frameNum, dataLen;
  sscanf(msg, "%d %d %d", &groupID, &frameNum, &dataLen);

  if (groupID >= 0) {
    syncMsgStruct *syncMsg;

    if (dataLen > 0) {
      syncMsg = new syncMsgStruct(dataLen);
      sage::recv(clientSockFd, (void*)syncMsg->data, dataLen);
    }
    else {
      syncMsg = new syncMsgStruct;
    }

    syncMsg->frameID = frameNum;

    int index;
    sageCircBufSingle *msgBuf = findSyncGroup(groupID, index);
    if (msgBuf && msgBuf->isActive()) {
      if (!msgBuf->pushBack((sageBufEntry)syncMsg)) {
        SAGE_PRINTLOG("sageSyncClient::readSyncMsg : sync message buffer is full");
        delete syncMsg;
      }
    }
    else
      delete syncMsg;
  }
  else
    SAGE_PRINTLOG("sageSyncClient::readSyncMsg : invalid group ID");

  return 0;
}

// receive sync message with group ID and additional data
syncMsgStruct* sageSyncClient::waitForSync(int id)
{
  if (id < 0) {
    SAGE_PRINTLOG("sageSyncClient::waitForSync : group ID is out of scope");
    return NULL;
  }

  int index;
  sageCircBufSingle *msgBuf = findSyncGroup(id, index);

  if (msgBuf) {
    syncMsgStruct *syncMsg = (syncMsgStruct *)msgBuf->front();
    if (msgBuf->isActive()) {
      if (!syncMsg) {
        SAGE_PRINTLOG("sageSyncClient::waitForSync : syncMsg is NULL");
      }
      msgBuf->next();

      return syncMsg;
    }
  }

  return NULL;
}

// receive sync message with additional data
int sageSyncClient::waitForSyncData(char* &data)
{
  int dataSize = SAGE_SYNC_MSG_LEN;
  char msg[SAGE_SYNC_MSG_LEN];
  int groupID = -1;
  int frameNum, dataLen;
  int status = sage::recv(clientSockFd, (void*)msg, dataSize);

  if (status !=  dataSize) {
    SAGE_PRINTLOG("sageSyncClient :: waitForSync(): Error receiving sync from master");
    sage::sleep(1);
    return -1;
  }

  sscanf(msg, "%d %d %d", &groupID, &frameNum, &dataLen);

  if (dataLen > 0) {
    data = new char[dataLen];
    sage::recv(clientSockFd, (void*)data, dataLen);
  }

  return frameNum;
}

int sageSyncClient::waitForSyncPeek() {
  int size = 0;
  int status = ::recv(clientSockFd, (char *)&size, sizeof(int), MSG_PEEK);

  if ( status != sizeof(int) ) {
    //perror("sageSyncClient::waitForSyncPeek() : recv");
    return -1;
  }
  return size;
}

// receive sync message with no additional data
int sageSyncClient::waitForSync(char* msg, int len)
{
  int dataSize = 0;
  if ( syncLevel == -1 ) {
    dataSize = SAGE_SYNC_MSG_LEN;
  }
  else {
    if ( len > 0 )
      dataSize = len;
    else
      dataSize = SAGE_SYNC_MSG_LEN;
  }

  int status = sage::recv(clientSockFd, (void*)msg, dataSize);

  if (status !=  dataSize) {
    SAGE_PRINTLOG("sageSyncClient::waitForSync(): syncLevel %d, msgLen %d, Error receiving sync from master",syncLevel, len);
    sage::sleep(1);
    return -1;
  }

  return 0;
}

int sageSyncClient::checkSync(char* &msg)
{
  int dataSize = SAGE_SYNC_MSG_LEN;

  if (sage::isDataReady(clientSockFd)) {
    int status=sage::recv(clientSockFd, (void*)msg, dataSize);
    if (status !=  dataSize) {
      perror("sageSyncClient :: checkSync(): Error receiving sync from master");
      sage::sleep(1);
      return -1;
    }

    return 1;
  }
  else
    return 0;
}

sageSyncClient :: ~sageSyncClient()
{
  syncEnd = true;

#ifdef WIN32
  closesocket(clientSockFd);
#else
  shutdown(clientSockFd, SHUT_RDWR);
  close(clientSockFd);
#endif

  pthread_join(syncThreadID, NULL);

  int maxGroupID = syncMsgBuf.size();
  for (int i=0; i<maxGroupID; i++)
    if (syncMsgBuf[i])
      delete syncMsgBuf[i];
}

sageThreadSync::sageThreadSync(int sNum)
{
  slaveNum = sNum;
  masterLock = new pthread_mutex_t[slaveNum];
  slaveLock = new pthread_mutex_t[slaveNum];

  for (int i=0; i<slaveNum; i++) {
    pthread_mutex_init(&masterLock[i], NULL);
    pthread_mutex_lock(&masterLock[i]);
  }

  for (int i=0; i<slaveNum; i++) {
    pthread_mutex_init(&slaveLock[i], NULL);
    pthread_mutex_lock(&slaveLock[i]);
  }
}

sageThreadSync::~sageThreadSync()
{
  if (masterLock)
    delete [] masterLock;

  if (slaveLock)
    delete [] slaveLock;
}

int sageThreadSync::synchronize(int rank)
{
  //SAGE_PRINTLOG("rank " << rank << std::endl << "slave num " << slaveNum);
  if (rank == 0) {
    for (int i=0; i<slaveNum; i++) {
      pthread_mutex_unlock(&slaveLock[i]);
    }
    //SAGE_PRINTLOG("unlock slave lock");
    //SAGE_PRINTLOG("waiting master lock");
    for (int i=0; i<slaveNum; i++) {
      pthread_mutex_lock(&masterLock[i]);
    }
    //SAGE_PRINTLOG("obtain master lock");
  }
  else if (rank <= slaveNum) {
    //SAGE_PRINTLOG("==== waiting slave lock");
    pthread_mutex_lock(&slaveLock[rank-1]);
    //SAGE_PRINTLOG("==== obtain slave lock");
    pthread_mutex_unlock(&masterLock[rank-1]);
    //SAGE_PRINTLOG("==== unlock master lock");
  }

  return 0;
}
