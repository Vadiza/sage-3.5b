/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioBridge.cpp - the SAGE component to distribute audio samples to multiple
 *         SAGE nodes
 * Author :  Hyejung Hur, Byungil Jeong
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

#include "sageAudioBridge.h"
#include "messageInterface.h"
#include "sageTcpModule.h"
#include "sageUdpModule.h"
#include "sageBlock.h"
#include "sageEvent.h"
#include "sageSharedData.h"

sageAudioDup::sageAudioDup(int appID)
  : instID(appID), initialized(false),
    rcvNodeNum(0), bridgeEnd(false), instNWObj(NULL)
{
}

sageAudioDup::~sageAudioDup(void)
{
  shutdownAllStreams();

  std::vector<streamBridgeParam*>::iterator i;
  streamBridgeParam* param = NULL;
  for (i = paramList.begin(); i != paramList.end(); i++) {
    param = (streamBridgeParam*)*i;
    delete param;
    param = NULL;
  }
  paramList.clear();
}

int sageAudioDup::init(void)
{
  return 0;
}

int sageAudioDup::addStream(char *msg, streamProtocol *nwObj)
{
  // sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d", config.streamType, winID,
  //  config.nodeNum, blockSize, config.syncMode, (int)config.sampleFmt,
  //  config.samplingRate, config.channels, config.framePerBuffer,  config.frameRate);
  // 100 6881394 1 2176 0 1 44100 2 512 1
  sscanf(msg, "%d %d %d %d %d %d %d %d %d %d %d %d", &senderID, &streamType, &instID, &sailNodeNum, &blockSize,
         &syncType, &sampleFmt, &samplingRate, &channels, &framePerBuffer, &frameRate, &keyframe);

  std::cout << "stream info " << msg << std::endl;
  instNWObj = nwObj;
  initialized = true;

  int bytesPerSample;
  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    bytesPerSample = sizeof(float);
    break;
  case SAGE_SAMPLE_INT16 :
    bytesPerSample = sizeof(short);
    break;
  case SAGE_SAMPLE_INT8 :
    bytesPerSample = sizeof(int);
    break;
  case SAGE_SAMPLE_UINT8 :
    bytesPerSample = sizeof(unsigned int);
    break;
  default :
    break;
  }
  audioNBlock.setSampleFormat((sageSampleFmt)sampleFmt);
  audioNBlock.setSampleRate(samplingRate);
  audioNBlock.setChannel(channels);
  audioNBlock.setFramePerBuffer(framePerBuffer);
  audioNBlock.setBytesPerSample(bytesPerSample);
  audioNBlock.initBuffer(blockSize);

  char regMsg[REG_MSG_SIZE];
  std::vector<streamBridgeParam*>::iterator i;
  streamBridgeParam* param = NULL;
  sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d %d", streamType, instID, sailNodeNum, blockSize,
          syncType, (int)sampleFmt, samplingRate, channels, framePerBuffer, frameRate, keyframe);

  int sendPort;
  for (i = paramList.begin(); i != paramList.end(); i++) {
    param = (streamBridgeParam*)*i;
    if(param->active == false)
    {
      sendPort = param->sendPort + instNWObj->getProtocol();
      instNWObj->setConfig(sendPort, 0, 0);
      param->rcvID = nwObj->connect(param->rcvIP, regMsg);
      param->active = true;
      std::cout << " Add stream : port : " << sendPort << " connected "<< std::endl;
    }
  }

  nwStreamThreadParam* threadparam = new nwStreamThreadParam;
  threadparam->This = this;
  threadparam->nwObj = nwObj;
  if (pthread_create(&thId, 0, nwStreamThread, (void*)threadparam) != 0)
  {
    SAGE_PRINTLOG("sageAudioDup : can't create network stream thread");
    return -1;
  }

  return 0;
}

int sageAudioDup::addReceiver(char *msg, int fsIdx)
{
  // ??????

  std::cout << "fs message " << msg << std::endl;

  char token[TOKEN_LEN];
  sageToken tokenBuf(msg);
  tokenBuf.getToken(token);  // port #
  int basePort = atoi(token);
  int sendPort = basePort;
  if(initialized)
  {
    sendPort += instNWObj->getProtocol();
    instNWObj->setConfig(sendPort, 0, 0);
    std::cout << " addReceiver : port : " << sendPort << std::endl;
  }

  tokenBuf.getToken(token);  // node #
  int nodeNum = atoi(token);

  int winID = atoi(sage::tokenSeek(msg,nodeNum*2+2));
  std::cout << "winID : " << winID << std::endl;

  streamBridgeParam* param = NULL;
  char rcvIP[SAGE_IP_LEN];

  char regMsg[REG_MSG_SIZE];
  sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d %d", streamType, winID, sailNodeNum, blockSize,
          syncType, (int)sampleFmt, samplingRate, channels, framePerBuffer, frameRate, keyframe);

  char* ip = NULL;
  for (int i=0; i<nodeNum; i++) {
    param = new streamBridgeParam;
    tokenBuf.getToken(rcvIP);
    tokenBuf.getToken(token);

    param->rcvIP = (char*) malloc(sizeof(char) * SAGE_IP_LEN);
    strcpy(param->rcvIP, rcvIP);
    param->nodeID = atoi(token);
    param->fsIdx = fsIdx;
    param->sendPort = basePort;
    param->active = false;
    if (initialized ) {
      std::cout << "connect - add receiver" << std::endl;
      instNWObj->setConfig(sendPort, 0, 0);
      param->rcvID = instNWObj->connect(param->rcvIP, regMsg);
      param->active = true;
    }
    paramList.push_back(param);
  }
  rcvNodeNum += nodeNum;

  std::cout << "receiver added " << std::endl;

  return 0;
}

void* sageAudioDup::nwStreamThread(void *args)
{
  nwStreamThreadParam *argparam = (nwStreamThreadParam *)args;
  streamProtocol *nwObj = (streamProtocol *)argparam->nwObj;
  sageAudioDup *This = (sageAudioDup *)argparam->This;

  int rcvSize, streamSize;
  std::vector<streamBridgeParam*>::iterator i;
  streamBridgeParam* param= NULL;

  // receive data -> send data
  while (!This->bridgeEnd) {

    rcvSize = nwObj->recv(This->senderID, &This->audioNBlock, SAGE_BLOCKING);
    if (rcvSize <= 0) {
      SAGE_PRINTLOG("sageAudioDup::nwStreamThread : exit loop");
      break;
    }
    else {
      // send data
      for (i = This->paramList.begin(); i != This->paramList.end(); i++) {
        param = (streamBridgeParam*)*i;
        if(param == NULL) continue;
        if (param->active && param->rcvID >= 0) {
          streamSize = nwObj->send(param->rcvID, &This->audioNBlock, SAGE_BLOCKING);
        }
      }

    }

  }

  SAGE_PRINTLOG("sageAudioDup::nwStreamThread : exit");
  pthread_exit(NULL);
  return NULL;
}

void sageAudioDup::shutdownStreams(int fsIdx)
{
  streamBridgeParam* param= NULL;
  std::vector<streamBridgeParam*>::iterator i;
  for (i = paramList.begin(); i != paramList.end(); i++) {
    param = (streamBridgeParam*)*i;
    if (param->fsIdx == fsIdx && param->active) {
      param->active = false;
      instNWObj->close(param->rcvID, SAGE_SEND);
    }
  }
}

void sageAudioDup::shutdownAllStreams(void)
{
  bridgeEnd = true;
  if (instNWObj)
    instNWObj->close(senderID, SAGE_ARCV);

  streamBridgeParam* param= NULL;
  std::vector<streamBridgeParam*>::iterator i;
  for (i = paramList.begin(); i != paramList.end(); i++) {
    param = (streamBridgeParam*)*i;
    if (param->active) {
      instNWObj->close(param->rcvID, SAGE_SEND);
    }
  }

  pthread_join(thId, NULL);
}

// ip, port  : IP address and port number where listening audio stream connections
// inf  :  message communication object for sending messages to SAIL.
sageAudioBridge::sageAudioBridge(char *ip, int port, messageInterface *inf, bridgeSharedData *sh)
  : audioPort(port), bridgeEnd(false), msgInf(inf), shared(sh)
{
  strcpy(audioIP, ip);

  tcpObj = NULL;
  udpObj = NULL;
  for(int i=0; i < MAX_INST_NUM; i++) {
    dupList[i] = NULL;
  }

  initNetworks();
}

sageAudioBridge::~sageAudioBridge()
{
  for(int i=0; i < MAX_INST_NUM; i++) {
    if(dupList[i] != NULL) {
      delete dupList[i];
      dupList[i] = NULL;
    }
  }

}

int sageAudioBridge::initNetworks()
{
  // open audio port

  // nwCfg setting??????
  tcpObj = new sageTcpModule;
  if (tcpObj->init(SAGE_ARCV, audioPort, nwCfg) == 1) {
    SAGE_PRINTLOG("sageAudioBridge is already running");
    bridgeEnd = true;
    return -1;
  }
  SAGE_PRINTLOG("SAGE Audio Bridge : tcp network object was initialized successfully " );

  udpObj = new sageUdpModule;
  udpObj->init(SAGE_ARCV, audioPort+(int)SAGE_UDP, nwCfg);
  SAGE_PRINTLOG("SAGE Audio Bridge : udp network object was initialized successfully" );

  pthread_t thId;
  nwAudioCheckThreadParam *param = new nwAudioCheckThreadParam;
  param->This = this;
  param->nwObj = tcpObj;

  if (pthread_create(&thId, 0, nwCheckThread, (void*)param) != 0) {
    SAGE_PRINTLOG("sageAudioBridge::initNetwork : can't create network checking thread");
    return -1;
  }

  param = new nwAudioCheckThreadParam;
  param->This = this;
  param->nwObj = udpObj;
  if (pthread_create(&thId, 0, nwCheckThread, (void*)param) != 0) {
    SAGE_PRINTLOG("sageAudioBridge::initNetwork : can't create network checking thread");
    return -1;
  }

  return 0;
}

void* sageAudioBridge::nwCheckThread(void *args)
{
  nwAudioCheckThreadParam *param = (nwAudioCheckThreadParam *)args;
  streamProtocol *nwObj = (streamProtocol *)param->nwObj;
  sageAudioBridge *This = (sageAudioBridge *)param->This;
  int senderID = -1;
  char regMsg[SAGE_EVENT_SIZE];
  if (nwObj) {
    while (!This->bridgeEnd) {
      senderID = nwObj->checkConnections(regMsg);
      if (senderID >= 0 && !This->bridgeEnd) {
        This->shared->eventQueue->sendEvent(EVENT_AUDIO_CONNECTION, regMsg, (void *)nwObj);
        std::cout << "detect new connection " << regMsg << std::endl;
        //This->initStreams(regMsg, nwObj);
      }
    }
  }

  SAGE_PRINTLOG("sageAudioBridge::nwCheckThread : exit");
  pthread_exit(NULL);
  return NULL;
}

int sageAudioBridge::initStreams(char *msg, streamProtocol *nwObj)
{
  int senderID, instID, streamType;
  sscanf(msg, "%d %d %d", &senderID, &streamType, &instID);
  //cout << "sageAudioBridge::initStreams --> senderID=" << senderID << " instID=" << instID << endl;

  if(instID > MAX_INST_NUM) return -1;

  sageAudioDup *inst = dupList[instID];
  if(inst == NULL) return -1;

  inst->addStream(msg, nwObj);

  return 0;
}

int sageAudioBridge::startAudioStream(int instID, int sailClientID)
{
  if(instID > MAX_INST_NUM) return -1;

  sageAudioDup *inst = dupList[instID];
  if(inst == NULL)
  {
    inst = new sageAudioDup(instID);
    dupList[instID] = inst;
  }

  char audioInitMsg[TOKEN_LEN];
  sprintf(audioInitMsg, "%d 1 %s 0", audioPort, audioIP);
  std::cout << "start audio stream " << audioInitMsg <<  std::endl;
  msgInf->msgToClient(sailClientID, 0, SAIL_CONNECT_TO_ARCV, audioInitMsg);

  return 0;
}

int sageAudioBridge::duplicate(int instID, char *msg, int fsIdx)
{
  if (instID > MAX_INST_NUM) return -1;

  sageAudioDup *inst = dupList[instID];
  if (inst) {
    inst->addReceiver(msg, fsIdx);
  }

  return 0;
}

int sageAudioBridge::shutdownStreams(int instID, int fsIdx)
{
  sageAudioDup *inst = dupList[instID];
  if (inst) {
    inst->shutdownStreams(fsIdx);
  }

  return 0;
}

int sageAudioBridge::shutdownDup(int instID)
{
  // NEED TO CHANGE
  sageAudioDup *inst = dupList[instID];
  if (inst) {
    delete inst;
    dupList[instID] = NULL;
  }

  return 0;
}
