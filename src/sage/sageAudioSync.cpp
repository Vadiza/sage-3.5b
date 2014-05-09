/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioSync.cpp - This file contains the class which provides the syncing
 *            mechanism.
 * Author : Hyejung Hur
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

#include "sageAudioSync.h"

sageAudioSync::sageAudioSync(bool audioRecv) :
  m_audioReceiver(audioRecv), m_connectedDone(false),
  m_recvPort(-1), m_sendPort(-1)
{
}

sageAudioSync::~sageAudioSync()
{
  closeAll();

  sageSyncInfo* info;
  // clear memory
  std::vector<sageSyncInfo*>::iterator iter;
  for(iter=m_syncInfoList.begin(); iter != m_syncInfoList.end(); iter++) {
    info = (sageSyncInfo*) (*iter);
    delete info;
    info = NULL;
  }
  m_syncInfoList.clear();
}

int sageAudioSync::init(int port)
{
  // connect networking....
  m_recvPort = port;
  m_sendPort = port+1;
  if(m_audioReceiver) {
    m_recvPort = port +1;
    m_sendPort = port;
  }

  if(initListener(m_recvPort) < 0) {
    return -1;
  }

  return 0;
}

int sageAudioSync::connect(char* ip)
{
  if(initSender(ip, m_sendPort) < 0) {
    return -1;
  }
  return 0;
}

void sageAudioSync::registerBuffer(sageAudioSyncType type, int sourceId, int bufferId, int keyframeNum)
{
  sageSyncInfo* info = new sageSyncInfo();
  info->m_sourceId = sourceId;
  info->m_bufferId = bufferId;
  info->m_keyframeNum = keyframeNum;
  info->m_syncType = type;
  m_syncInfoList.push_back(info);

  // for testing
  if(info->m_syncType == SAGE_SYNC_AUDIO_DRIVEN) {
    if(m_audioReceiver) {
      std::cout << "SAGE_SYNC_AUDIO_DRIVEN : audio " << std::endl;
    } else {
      std::cout << "SAGE_SYNC_AUDIO_DRIVEN : graphic " << std::endl;
    }
  } else if(info->m_syncType == SAGE_SYNC_GRAPHIC_DRIVEN) {
    if(m_audioReceiver) {
      std::cout << "SAGE_SYNC_GRAPHIC_DRIVEN : audio " << std::endl;
    } else {
      std::cout << "SAGE_SYNC_GRAPHIC_DRIVEN : graphic " << std::endl;
    }
  }

}

int sageAudioSync::removeBuffer(int bufferId)
{
  sageSyncInfo* info;
  std::vector<sageSyncInfo*>::iterator iter;
  for(iter=m_syncInfoList.begin(); iter != m_syncInfoList.end(); iter++) {
    info = (sageSyncInfo*) (*iter);
    if(info->m_bufferId == bufferId) {
      m_syncInfoList.erase(iter);
      delete info;
      info = NULL;
      return 0;
    }
  }
  return -1;
}

bool sageAudioSync::checkKeyFrame(int bufferId, int frameNum)
{
  bool found = false;
  sageSyncInfo* info;
  std::vector<sageSyncInfo*>::iterator iter;
  for(iter=m_syncInfoList.begin(); iter != m_syncInfoList.end(); iter++) {
    info = (sageSyncInfo*) (*iter);
    if(info->m_bufferId == bufferId) {
      found = true;
      break;
    }
  }

  if(found) {
    if(info->m_passNum == frameNum) return false;
    // i am not sure.. it can make bug...
    if(info->m_passNum > frameNum) {
      int diff = info->m_passNum - frameNum;
      if(diff  < info->m_keyframeNum) return false;
    }
    if((frameNum > (9999 - info->m_keyframeNum)) && (info->m_passNum < info->m_keyframeNum)) {
      return false;
    }


    if(info->m_syncType == SAGE_SYNC_NONE) return false;
    int result = frameNum % (info->m_keyframeNum);
    int gap = (info->m_passNum / info->m_keyframeNum) - (frameNum / info->m_keyframeNum);
    if((result == 0) || (gap != 0)) {
      //std::cout << "1 past: " << info->m_passNum << " 2 keyframe: " << info->m_keyframeNum << " 3 framenum: " << frameNum << std::endl;
      info->m_passNum = frameNum;
      info->m_lockFlag = true;
      doKeyFrameAction(info);
      return true;   // then buffer will set lock
    }
  }

  return false;
}

void sageAudioSync::doKeyFrameAction(sageSyncInfo* info)
{
  if(info->m_syncType == SAGE_SYNC_AUDIO_DRIVEN) {
    if(m_audioReceiver) {
      // send signal to gStreamRcv
      /** need to check */
      sendSignal(info);
    } else {
      // send signal to aStreamRcv
      /** need to check */
      sendSignal(info);
    }
  } else if(info->m_syncType == SAGE_SYNC_GRAPHIC_DRIVEN) {
    if(m_audioReceiver) {
      // nothing to do -> just return true;
    } else {
      // send signal to aStreamRcv
      sendSignal(info);
    }
  }
}

bool sageAudioSync::checkSignal(int bufferId)
{
  bool found = false;
  sageSyncInfo* info;
  std::vector<sageSyncInfo*>::iterator iter;
  for(iter=m_syncInfoList.begin(); iter != m_syncInfoList.end(); iter++) {
    info = (sageSyncInfo*) (*iter);
    if(info->m_bufferId == bufferId) {
      if(info->m_updateFlag) {
        info->m_updateFlag = false;
        return true;
      }
      found = true;
      break;
    }
  }

  if(!found) {
    return false;
  }

  if(info->m_syncType == SAGE_SYNC_NONE) return false;

  if(info->m_syncType == SAGE_SYNC_GRAPHIC_DRIVEN) {
    if (!m_audioReceiver) { // graphics receiver
      if(info->m_lockFlag == true) {
        info->m_lockFlag = false;
        return true;
      } else {
        return false;
      }
    }
  }

  char msg[REG_MSG_SIZE];
  bool retVal = false;
  int size = m_clientSockList.size();
  if(size <= 0) {
    if(info->m_clientSockFd >= 0) {
      retVal = recvSignal(info->m_clientSockFd, msg, bufferId);
      if(retVal) {
        info->m_lockFlag = false;
        return true;
      }
    }
    if(m_clientSockFd >= 0) {
      retVal = recvSignal(m_clientSockFd, msg, bufferId);
      if(retVal) {
        info->m_lockFlag = false;
        return true;
      }
    }
  } else {
    for(int i=0; i < size; i++) {
      retVal = recvSignal(m_clientSockList[i], msg, bufferId);
      if(retVal) {
        info->m_lockFlag = false;
        return true;
      }
    }
  }

  return retVal;
}

bool sageAudioSync::recvSignal(int socketFd, char* msg, int bufferId)
{
  if (!sage::isDataReady(socketFd)) {
    return false;
  }

  int retVal = sage::recv(socketFd, (void *)msg, REG_MSG_SIZE);
  int sourceID, frameNum;
  double timeStamp;
  if (retVal > 0) {
    char token[TOKEN_LEN];
    getToken(msg, token);
    sourceID = atoi(token);
    getToken(msg, token);
    frameNum = atoi(token);
    getToken(msg, token);
    timeStamp = atof(token);
  }
  else if (retVal <= 0) {
    return false;
  }

  //std::cout << "received---------> : sourceid : " << sourceID << "frameNum : " << frameNum << "timestamp : " << timeStamp <<  std::endl;
  sageSyncInfo* info;
  std::vector<sageSyncInfo*>::iterator iter;
  for(iter=m_syncInfoList.begin(); iter != m_syncInfoList.end(); iter++) {
    info = (sageSyncInfo*) (*iter);
    if(info->m_sourceId == sourceID) {
      if(info->m_bufferId == bufferId) {
        info->m_updateFlag = false;
        //std::cout << "info->m_sourceId == sourceID :: info->m_bufferId == bufferId " << std::endl;
        return true;
      }
      std::cout << "info->m_sourceId == sourceID" << std::endl;
      info->m_clientSockFd = socketFd;
      info->m_updateFlag = true;
      break;
    }
  }
  return false;
}

int sageAudioSync::sendSignal(int bufferId)
{
  bool found = false;
  sageSyncInfo* info;
  std::vector<sageSyncInfo*>::iterator iter;
  for(iter=m_syncInfoList.begin(); iter != m_syncInfoList.end(); iter++) {
    info = (sageSyncInfo*) (*iter);
    if(info->m_bufferId == bufferId) {
      found = true;
      break;
    }
  }

  if(!found) {
    return -1;
  }

  std::cerr << "called---------> : "<<  std::endl;

  char msg[REG_MSG_SIZE];
  // sourceID, frameNumber, timeStamp
  sprintf(msg, "%d %d %f", info->m_sourceId, 1, 1.0);

  if (sage::send(m_clientSockFd, msg, REG_MSG_SIZE) <= 0) {
    std::cerr << "sageAudioSync::sendSignal() : could not send message" <<  std::endl;
    return -1;
  }
  return 0;
}

int sageAudioSync::sendSignal(sageSyncInfo* info)
{
  std::cerr << "called---------> : "<<  std::endl;

  char msg[REG_MSG_SIZE];
  // sourceID, frameNumber, timeStamp
  sprintf(msg, "%d %d %f", info->m_sourceId, 1, 1.0);

  if(m_audioReceiver) { // audio receiver
    if (sage::send(m_clientSockList[0], msg, REG_MSG_SIZE) <= 0) {
      std::cerr << "sageAudioSync::sendSignal() : could not send message" <<  std::endl;
      return -1;
    }
  } else {
    if (sage::send(m_clientSockFd, msg, REG_MSG_SIZE) <= 0) {
      std::cerr << "sageAudioSync::sendSignal() : could not send message" <<  std::endl;
      return -1;
    }
  }
  return 0;
}

int sageAudioSync::initListener(int port)
{
  int optVal, optLen;
  // open listener socket

  // create the server socket
  if((m_serverSockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    std::cerr << "sageAudioSync::init(): Creating socket failed" <<  std::endl;
    return -1;
  }

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined(__sun)
  if(setsockopt(m_serverSockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error setting SO_REUSEADDR" <<  std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_serverSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching off Nagle's algorithm." << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_serverSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching on OOBINLINE." << std::endl;

#else
  if(setsockopt(m_serverSockFd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, (socklen_t)optLen) !=0)
    std::cerr << "sageAudioSync::init():: Error setting SO_REUSEADDR" << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_serverSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching off Nagle's algorithm." << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_serverSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching on OOBINLINE." << std::endl;

#endif

  memset(&m_localAddr, 0, sizeof(sockaddr_in));
  m_localAddr.sin_family = AF_INET;
  m_localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  m_localAddr.sin_port = htons(port);

  if (bind(m_serverSockFd, (struct sockaddr *)&m_localAddr, sizeof(struct sockaddr_in)) != 0) {
    std::cerr << "sageAudioSync::init(): Error binding server socket" << std::endl;
    return -1;
  }

  // put in listen mode
  listen(m_serverSockFd, 5);
  std::cout << "sageAudioSync::initListener() : initialized for listen : port " << port << std::endl;

  return 0;

}

int sageAudioSync::initSender(char* ip, int port)
{
  int optVal, optLen;

  // open sender/client socket
  if((m_clientSockFd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    std::cerr << "sageAudioSync::initSender(): Creating server socket failed" << std::endl;
    return -1;
  }

  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined(__sun)
  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_clientSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::initSender(): Error switching off Nagle's algorithm." << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_clientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)

    std::cerr << "sageAudioSync::initSender(): Error switching on OOBINLINE." << std::endl;
#else

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_clientSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    std::cerr << "sageAudioSync::initSender(): Error switching off Nagle's algorithm." << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(m_clientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::initSender(): Error switching on OOBINLINE." << std::endl;

#endif
  // connect to the server
  memset(&m_serverAddr, 0, sizeof(sockaddr_in));
  m_serverAddr.sin_family = AF_INET;
  m_serverAddr.sin_addr.s_addr = inet_addr(ip);
  m_serverAddr.sin_port = htons(port);

  if(::connect(m_clientSockFd, (struct sockaddr *)&m_serverAddr, sizeof(struct sockaddr)) == -1) {
    std::cerr << "sageAudioSync::initSender() : Fail to connect" << std::endl;
    return -1;
  }

  // check message
  // source id
  char msg[REG_MSG_SIZE];
  // sourceID, frameNumber, timeStamp
  sprintf(msg, "%d %d %f", -1, 1, 1.0);
  if (sage::send(m_clientSockFd, msg, REG_MSG_SIZE) == -1) {
    std::cerr << "sageAudioSync::initSender() : send message  " << std::endl;
    return -1;
  }
  std::cout << "sageAudioSync::initSender() : sended message for initialize : port " << port << std::endl;

  return 0;
}

int sageAudioSync::checkConnections()
{
  if(!m_connectedDone) {
    /*if(initSender(ip, port) < 0) {
      return -1;
      }*/
    m_connectedDone = true;
    return -1;
  }
  if (!sage::isDataReady(m_serverSockFd)) {
    return -1;
  }

  //accept client connections
  int addrLen;
  sockaddr_in clientAddr;
  addrLen = sizeof(clientAddr);

  int clientSockFd;
  if((clientSockFd = ::accept(m_serverSockFd, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1){
    std::cerr << "sageAudioSync::checkConnections() : accept error" << std::endl;
    return -1;
  }
  int optVal, optLen;
  // loosen the rules for check during bind to allow mutiple binds on the same port
  optVal=1; optLen = sizeof(optVal);

#if defined(WIN32) || defined(__APPLE__) || defined(__sun)
  if(setsockopt(clientSockFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error setting SO_REUSEADDR" <<  std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, IPPROTO_TCP, TCP_NODELAY, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching off Nagle's algorithm." << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching on OOBINLINE." << std::endl;

#else
  if(setsockopt(clientSockFd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, (socklen_t)optLen) !=0)
    std::cerr << "sageAudioSync::init():: Error setting SO_REUSEADDR" << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, SOL_TCP, TCP_NODELAY, (void*)&optVal, (socklen_t)optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching off Nagle's algorithm." << std::endl;

  optVal = 1;
  optLen=sizeof(optVal);
  if(setsockopt(clientSockFd, SOL_SOCKET, SO_OOBINLINE, (const char*)&optVal, optLen) !=0)
    std::cerr << "sageAudioSync::init(): Error switching on OOBINLINE." << std::endl;

#endif

  m_clientSockList.push_back(clientSockFd);

  char msg[REG_MSG_SIZE];
  int retVal = sage::recv(clientSockFd, (void *)msg, REG_MSG_SIZE);
  if (retVal == -1) {
    std::cerr << "sageAudioSync::checkConnections() : receive error" << std::endl;
    return -1;
  }
  else if (retVal == 0) {
    return -1;
  }

  std::cout << "sageAudioSync::checkConnections() : new client is connected " << std::endl;

  return 0;
}

int sageAudioSync::closeAll()
{
  int rcvNum = m_clientSockList.size();

#ifdef WIN32
  closesocket(m_clientSockFd);

  for (int i=0; i< rcvNum ; i++) {
    if (m_clientSockList[i] < 0) continue;
    closesocket(m_clientSockList[i]);
  }
  closesocket(m_serverSockFd);
#else
  shutdown(m_serverSockFd, SHUT_RDWR);
  close(m_serverSockFd);

  for (int i=0; i< rcvNum ; i++) {
    if (m_clientSockList[i] < 0) continue;
    shutdown(m_clientSockList[i], SHUT_RDWR);
    close(m_clientSockList[i]);
  }
  shutdown(m_clientSockFd, SHUT_RDWR);
  close(m_clientSockFd);
#endif

  return 0;
}
