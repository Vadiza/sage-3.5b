/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBridge.cpp - the SAGE component to distribute pixels to multiple
 *         SAGE displays
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

#include "sageBridge.h"
#include "sageAudioBridge.h"
#include "messageInterface.h"
#include "sageTcpModule.h"
#include "sageUdpModule.h"
#include "sageSharedData.h"
#include "appInstance.h"
#include "sageSync.h"
#include "sageBlockQueue.h"
#include "sageEvent.h"

sageBridge::~sageBridge()
{
  if (shared)
    delete shared;

  if (msgInf) {
    msgInf->shutdown();
    delete msgInf;
  }
}

sageBridge::sageBridge(int argc, char **argv) : syncPort(0), syncGroupID(0), audioPort(44000),
                                                allocPolicy(ALLOC_SINGLE_NODE), enableSync(1)
{
  nwCfg = new sageNwConfig;
  for (int i=0; i<MAX_INST_NUM; i++)
    appInstList[i] = NULL;
  instNum = 0;

  if (argc < 2) {  // master mode with default configuration file
    initMaster((char *)"sageBridge.conf");
  }
  else if (strcmp(argv[1], "slave") != 0) {  // master mode with user configuration file
    initMaster(argv[1]);
  }
  else {  // slave mode
    if (argc < 5) {
      SAGE_PRINTLOG("sageBridge::sageBridge() : more arguments are needed for SAGE bridge slaves" );
      exit(0);
    }

    shared = new bridgeSharedData;
    master = false;
    strcpy(masterIP, argv[2]);

    msgPort = atoi(argv[3]);

    msgInfConfig conf;
    conf.master = false;
    strcpy(conf.serverIP, masterIP);
    conf.serverPort = msgPort;

    msgInf = new messageInterface();
    msgInf->init(conf); //wait for TCP connection through separate thread
    msgInf->msgToServer(0, BRIDGE_REG_NODE, argv[4]);

    shared->nodeID = atoi(argv[4]);
    SAGE_PRINTLOG("sageBridge::%s() : start slave node %d", __FUNCTION__, shared->nodeID);

    tcpObj = NULL;
    udpObj = NULL;
    bridgeEnd = false;

    pthread_t thId;

    if (pthread_create(&thId, 0, msgCheckThread, (void*)this) != 0) {
      SAGE_PRINTLOG("sageBridge::%s() : can't create message checking thread", __FUNCTION__);
    }

    if (pthread_create(&thId, 0, perfReportThread, (void*)this) != 0) {
      SAGE_PRINTLOG("sageBrdige::%s() : can't create performance report thread", __FUNCTION__);
    }
  }
}

int sageBridge::initMaster(char *cFile)
{
  shared = new bridgeSharedData;
  shared->nodeID = 0;
  master = true;
  bridgeEnd = false;

  char *sageDir = getenv("SAGE_DIRECTORY");
  if (!sageDir) {
    SAGE_PRINTLOG("sageBridge: cannot find the environment variable SAGE_DIRECTORY");
    return -1;
  }

  data_path path;
  std::string found = path.get_file(cFile);
  if (found.empty()) {
    SAGE_PRINTLOG("sageBridge: cannot find the file [%s]", cFile);
    return -1;
  }
  const char *bridgeConfigFile = found.c_str();
  SAGE_PRINTLOG("sageBridge::%s() : using [%s] configuration file", __FUNCTION__, bridgeConfigFile);

  FILE *fileBridgeConf = fopen(bridgeConfigFile, "r");

  if (!fileBridgeConf) {
    SAGE_PRINTLOG("sageBridge: fail to open SAGE Bridge config file [%s]\n",bridgeConfigFile);
    return -1;
  }

  char token[TOKEN_LEN];
  int tokenIdx = getToken(fileBridgeConf, token);

  while(tokenIdx != EOF) {
    if (strcmp(token, "masterIP") == 0) {
      getToken(fileBridgeConf, masterIP);
    }
    else if (strcmp(token, "slaveList") == 0) {
      getToken(fileBridgeConf, token);
      slaveNum = atoi(token);
      shared->nodeNum = slaveNum + 1;
      for (int i=0; i<slaveNum; i++)
        getToken(fileBridgeConf, slaveIPs[i]);
    }
    else if (strcmp(token, "streamPort") == 0) {
      getToken(fileBridgeConf, token);
      streamPort = atoi(token);
    }
    else if (strcmp(token, "msgPort") == 0) {
      getToken(fileBridgeConf, token);
      msgPort = atoi(token);
    }
    else if (strcmp(token, "syncPort") == 0) {
      getToken(fileBridgeConf, token);
      syncPort = atoi(token);
    }
    else if (strcmp(token, "audioPort") == 0) {
      getToken(fileBridgeConf, token);
      audioPort = atoi(token);
    }
    else if (strcmp(token, "rcvNwBufSize") == 0) {
      getToken(fileBridgeConf, token);
      nwCfg->rcvBufSize = getnumber(token); //atoi(token);
    }
    else if (strcmp(token, "sendNwBufSize") == 0) {
      getToken(fileBridgeConf, token);
      nwCfg->sendBufSize = getnumber(token); //atoi(token);
    }
    else if (strcmp(token, "MTU") == 0) {
      getToken(fileBridgeConf, token);
      nwCfg->mtuSize = atoi(token);
    }
    else if (strcmp(token, "maxBandWidth") == 0) {
      getToken(fileBridgeConf, token);
      nwCfg->maxBandWidth = atoi(token);
    }
    else if (strcmp(token, "maxCheckInterval") == 0) {
      getToken(fileBridgeConf, token);
      nwCfg->maxCheckInterval = atoi(token);
    }
    else if (strcmp(token, "flowWindowSize") == 0) {
      getToken(fileBridgeConf, token);
      nwCfg->flowWindow = atoi(token);
    }
    else if (strcmp(token, "memSize") == 0) {
      getToken(fileBridgeConf, token);
      shared->bufSize = atoi(token) * 1024*1024;
    }
    else if (strcmp(token, "nodeAllocation") == 0) {
      getToken(fileBridgeConf, token);
      if (strcmp(token, "single") == 0)
        allocPolicy = ALLOC_SINGLE_NODE;
      else if (strcmp(token, "balanced") == 0)
        allocPolicy = ALLOC_LOAD_BALANCING;
    }
    else if (strcmp(token, "frameDrop") == 0) {
      getToken(fileBridgeConf, token);
      if (strcmp(token, "true") == 0)
        shared->frameDrop = true;
      else
        shared->frameDrop = false;
    }
    else if (strcmp(token, "enableSync") == 0) {
      getToken(fileBridgeConf, token);
      enableSync = atoi(token);
      SAGE_PRINTLOG("sageBridge::%s() : enableSync has set to %d\n", __FUNCTION__, enableSync);
    }

    tokenIdx = getToken(fileBridgeConf, token);
  }

  msgInfConfig conf;
  conf.master = true;
  conf.serverPort = msgPort;

  msgInf = new messageInterface;
  msgInf->init(conf);
  shared->msgInf = msgInf;

  if ( enableSync ) {
    syncServerObj = new sageSyncServer;
    if (syncServerObj->init(syncPort) < 0) {
      SAGE_PRINTLOG("SAGE Bridge : Error init'ing the sync server object" );
      bridgeEnd = true;
      return -1;
    }
  }
  else
    syncServerObj = NULL;


  pthread_t thId;

  if (pthread_create(&thId, 0, msgCheckThread, (void*)this) != 0) {
    SAGE_PRINTLOG("sageBridge::initMaster : can't create message checking thread");
  }

  if (pthread_create(&thId, 0, perfReportThread, (void*)this) != 0) {
    SAGE_PRINTLOG("sageBrdige::initMaster : can't create performance report thread");
  }

  if ( enableSync ) {
    shared->syncClientObj = new sageSyncClient;
    if (shared->syncClientObj->connectToServer(strdup("127.0.0.1"), syncPort) < 0) {
      SAGE_PRINTLOG("SAGE Bridge : Fail to connect to sync master" );
      return -1;
    }
  }
  else
    shared->syncClientObj = NULL;

  launchSlaves();
  initNetworks();

  audioBridge = new sageAudioBridge(masterIP, audioPort, msgInf, shared);

  return 0;
}

int sageBridge::launchSlaves()
{
  char *sageDir = getenv("SAGE_DIRECTORY");

  if (!sageDir) {
    SAGE_PRINTLOG("sageBridge : cannot find the environment variable SAGE_DIRECTORY" );
    return -1;
  }

  for (int i=0; i<slaveNum; i++) {
    char command[TOKEN_LEN];
    sprintf(command, "%s/bin/sageBridge slave %s %d %d", sageDir, masterIP, msgPort, i+1);
    execRemBin(slaveIPs[i], command);
  }

  return 0;
}

int sageBridge::initSlave(char *data)
{
  SAGE_PRINTLOG("sageBridge : initialize slave %d", shared->nodeID);

  sscanf(data, "%d %d %d %d %d %d %d %d %d %d", &nwCfg->rcvBufSize, &nwCfg->sendBufSize,
         &nwCfg->mtuSize, (int *)&nwCfg->maxBandWidth, &nwCfg->maxCheckInterval, &nwCfg->flowWindow,
         &syncPort, &streamPort, &shared->nodeNum, &shared->bufSize);

  SAGE_PRINTLOG("slave init info %s", data);

  initNetworks();

  syncServerObj = NULL;
  shared->syncClientObj = NULL;

  if ( enableSync ) {
    shared->syncClientObj = new sageSyncClient;
    if (shared->syncClientObj->connectToServer(masterIP, syncPort) < 0) {
      SAGE_PRINTLOG("SAGE Bridge : Fail to connect to sync master");
      return -1;
    }
  }

  return 0;
}

int sageBridge::initNetworks()
{
  nwCfg->blockSize = 9000;
  nwCfg->groupSize = 65536;
  sageUdpModule *sendObj = new sageUdpModule;
  sendObj->init(SAGE_SEND, 0, *nwCfg);
  shared->sendObj = (streamProtocol *)sendObj;

  tcpObj = new sageTcpModule;
  if (tcpObj->init(SAGE_RCV, streamPort, *nwCfg) == 1) {
    SAGE_PRINTLOG("sageBridge : error in initializing TCP object");
    bridgeEnd = true;
    return -1;
  }

  SAGE_PRINTLOG("SAGE Bridge : tcp network object was initialized successfully " );

  udpObj = new sageUdpModule;
  if (udpObj->init(SAGE_RCV, streamPort+(int)SAGE_UDP, *nwCfg) == 1) {
    SAGE_PRINTLOG("sageBridge : error in initializing UDP object");
    bridgeEnd = true;
    return -1;
  }

  SAGE_PRINTLOG("SAGE Bridge : udp network object was initialized successfully" );

  pthread_t thId;
  nwCheckThreadParam *param = new nwCheckThreadParam;
  param->This = this;
  param->nwObj = tcpObj;

  if (pthread_create(&thId, 0, nwCheckThread, (void*)param) != 0) {
    SAGE_PRINTLOG("sageBridge::initNetwork : can't create network checking thread");
    return -1;
  }

  param = new nwCheckThreadParam;
  param->This = this;
  param->nwObj = udpObj;
  if (pthread_create(&thId, 0, nwCheckThread, (void*)param) != 0) {
    SAGE_PRINTLOG("sageBridge::initNetwork : can't create network checking thread");
    return -1;
  }

  return 0;
}

// Thread Functions

void* sageBridge::msgCheckThread(void *args)
{
  sageBridge *This = (sageBridge *)args;

  sageMessage *msg;

  while (!This->bridgeEnd) {
    msg = new sageMessage;
    //SAGE_PRINTLOG("waiting for message");
    int clientID = This->msgInf->readMsg(msg, 100000) - 1;

    if (clientID >= 0 && !This->bridgeEnd) {
      //SAGE_PRINTLOG("read a message");
      This->shared->eventQueue->sendEvent(EVENT_NEW_MESSAGE, clientID, (void *)msg);
    }
  }

  SAGE_PRINTLOG("sageBridge::msgCheckThread : exit");
  pthread_exit(NULL);
  return NULL;
}

void* sageBridge::perfReportThread(void *args)
{
  sageBridge *This = (sageBridge *)args;

  while (!This->bridgeEnd) {
    This->perfReport();
    sage::usleep(100000);
  }

  SAGE_PRINTLOG("sageBridge::perfReportThread : exit");
  pthread_exit(NULL);
  return NULL;
}

void* sageBridge::nwCheckThread(void *args)
{
  nwCheckThreadParam *param = (nwCheckThreadParam *)args;
  streamProtocol *nwObj = (streamProtocol *)param->nwObj;
  sageBridge *This = (sageBridge *)param->This;

  int senderID = -1;
  char regMsg[SAGE_EVENT_SIZE];

  if (nwObj) {
    while (!This->bridgeEnd) {
      senderID = nwObj->checkConnections(regMsg);
      if (!This->bridgeEnd) {
        if (senderID >= 0)
          This->shared->eventQueue->sendEvent(EVENT_NEW_CONNECTION, regMsg, (void *)nwObj);
        else
          break;
      }
    }
  }

  SAGE_PRINTLOG("sageBridge::nwCheckThread : exit");
  pthread_exit(NULL);
  return NULL;
}

int sageBridge::perfReport()
{
  for (int i=0; i<instNum; i++) {
    if (appInstList[i])
      appInstList[i]->sendPerformanceInfo();
  }

  return 0;
}

int sageBridge::findMinLoadNode()
{
  int nodeSel = 0;
  double minLoad = 0;

  for (int i=0; i<shared->nodeNum; i++) {
    double loadSum = 0.0;
    for (int j=0; j<instNum; j++) {
      if (appInstList[j] && appInstList[j]->allocInfoList[0].nodeID == i)
        loadSum += appInstList[j]->curBandWidth;
    }

    if (i == 0) {
      minLoad = loadSum;
    }
    else if (minLoad > loadSum) {
      nodeSel = i;
      minLoad = loadSum;
    }

    SAGE_PRINTLOG("node %d load %d", i, loadSum);
  }

  return nodeSel;
}

int sageBridge::regApp(sageMessage &msg, int clientID)
{
  char *data = (char *)msg.getData();

  if (master) {
    appInstance *inst = new appInstance(data, instNum, shared);
    appInstList[instNum] = inst;

    inst->sailClient = clientID;
    inst->waitNodes = slaveNum;

    if (allocPolicy == ALLOC_LOAD_BALANCING) {
      inst->allocateNodes(allocPolicy);
    }
    else  {
      int selNode = findMinLoadNode();
      SAGE_PRINTLOG("sageBridge::%s() : allocate app to node %d", __FUNCTION__, selNode);
      inst->allocateNodes(allocPolicy, selNode);
    }

    syncGroup *sGroup = NULL;
    char regStr[TOKEN_LEN];
    sprintf(regStr, "%s %d 0", data, inst->nodeNum);

    if (enableSync && inst->nodeNum > 1) {
      sGroup = addSyncGroup();
      inst->firstSyncGroup = sGroup;

      if (sGroup)
        sprintf(regStr, "%s %d %d", data, inst->nodeNum, sGroup->getSyncID());
    }

    msgInf->distributeMessage(BRIDGE_APP_REG, instNum, regStr, slaveList, slaveNum);

    char sailInitMsg[TOKEN_LEN];
    sprintf(sailInitMsg, "%d %d %d %d", instNum , nwCfg->rcvBufSize,
            nwCfg->sendBufSize, nwCfg->mtuSize);
    msgInf->msgToClient(clientID, 0, SAIL_INIT_MSG, sailInitMsg);

    if (inst->waitNodes == 0) {
      connectApp(inst);
      inst->waitNodes = inst->nodeNum;
      sendStreamInfo(inst);
      if (inst->audioOn)
        audioBridge->startAudioStream(instNum, clientID);
    }

    instNum++;
  }
  else {
    SAGE_PRINTLOG("register app on node %d", shared->nodeID);
    int instID = msg.getDest();
    appInstList[instID] = new appInstance(data, instID, shared);
    instNum = MAX(instID+1, instNum);
    msgInf->msgToServer(instID, BRIDGE_APP_INST_READY);
  }
  return 0;
}

int sageBridge::connectApp(appInstance *inst)
{
  int msgLen = 8 + SAGE_IP_LEN * inst->nodeNum;
  char *msgStr = new char[msgLen];

  sprintf(msgStr, "%d %d ", streamPort, inst->nodeNum);

  // list the ip addresses of all bridge nodes
  for (int i=0; i<inst->nodeNum; i++) {
    int nodeID = inst->allocInfoList[i].nodeID;
    char nodeStr[TOKEN_LEN];
    if (nodeID == 0)
      sprintf(nodeStr, "%s %d ", masterIP, nodeID);
    else
      sprintf(nodeStr, "%s %d ", slaveIPs[nodeID-1], nodeID);
    strcat(msgStr, nodeStr);
  }

  SAGE_PRINTLOG("app-brige connection info : %s", msgStr);

  msgInf->msgToClient(inst->sailClient, 0, SAIL_CONNECT_TO_RCV, msgStr);

  return 0;
}

int sageBridge::sendStreamInfo(appInstance *inst)
{
  int msgLen = 4 + SAGE_IP_LEN*inst->nodeNum;
  char *msgStr = new char[msgLen];

  sprintf(msgStr, "%d ", inst->nodeNum);

  for (int i=0; i<inst->nodeNum; i++) {
    char str[TOKEN_LEN];
    sprintf(str, "%d %d ", inst->allocInfoList[i].blockID, inst->allocInfoList[i].nodeID);
    strcat(msgStr, str);
  }

  msgInf->msgToClient(inst->sailClient, 0, SAIL_INIT_STREAM, msgStr);
  SAGE_PRINTLOG("sageBridge : stream info sent to app");

  return 0;
}

int sageBridge::shutdownApp(int instID, bool fsmToApp)
{
  appInstance *inst = appInstList[instID];

  if (inst) {
    if (master && !inst->isActive()) {
      int clientID = inst->sailClient;

      if (fsmToApp) {
        msgInf->msgToClient(clientID, 0, APP_QUIT);
      }

      if (audioBridge && inst->audioOn)
        audioBridge->shutdownDup(instID);

      SAGE_PRINTLOG("send clear app inst message");
      msgInf->distributeMessage(CLEAR_APP_INSTANCE, instID, slaveList, slaveNum);
      appInstList[instID] = NULL;
      delete inst;
    }
  }
  else {
    SAGE_PRINTLOG("sageBridge::shutdownApp : invalid app instance ID");
    return -1;
  }

  return 0;
}

int sageBridge::forwardToSail(int instID, sageMessage &msg)
{
  appInstance *inst = appInstList[instID];

  if (inst) {
    int clientID = inst->sailClient;
    msg.setClientID(clientID);
    msgInf->msgToClient(msg);
  }

  return 0;
}

int sageBridge::shutdownAllApps()
{
  for (int i=0; i<instNum; i++) {
    if (!appInstList[i])
      continue;

    appInstance *inst = appInstList[i];
    appInstList[i] = NULL;
    msgInf->msgToClient(inst->sailClient, 0, APP_QUIT);
    inst->shutdownAllStreams();
    delete inst;
  }

  return 0;
}

/**
 *  from EVENT_NEW_CONNECTION from nwCheckThread()
 */
int sageBridge::initStreams(char *msg, streamProtocol *nwObj)
{
  int senderID, instID, frameRate, streamType;

  sscanf(msg, "%d %d %d %d", &senderID, &streamType, &frameRate, &instID);

  //SAGE_PRINTLOG("sender %d connected to node %d", senderID, shared->nodeID);
  SAGE_PRINTLOG("sageBridge::%s() : senderID %d connected to sageBridge node %d. instID %d\n", __FUNCTION__, senderID, shared->nodeID, instID);

  appInstance *inst = appInstList[instID];
  if (inst) {
    if (!inst->initialized) {
      if ( (inst->init(msg, nwObj)) == 0 ) {
        SAGE_PRINTLOG("sageBridge::%s() : appInstance %d initialized. (sageBlockBuf, sagePixelReceiver, bridgeStreamer have created)\n", __FUNCTION__, instID);
      }
      else {
        SAGE_PRINTLOG("sageBridge::%s() : appInstance %d failed initialization\n", __FUNCTION__, instID);
      }
    }
    //SAGE_PRINTLOG("node %d init instance", shared->nodeID );

    /*
     * sendEvent(EVENT_APP_CONNECTED, instID); will be issued by sageReceiver::addStream(senderID)
     */
    int streamNum = inst->addStream(senderID);
    SAGE_PRINTLOG("sageBridge::%s() : stream from sender %d added. # of streamer for this app(instID %d) is %d.\n", __FUNCTION__, senderID, instID, streamNum);
    //SAGE_PRINTLOG("node %d add stream", shared->nodeID);
  }
  else {
    SAGE_PRINTLOG("sageBridge::initStreams : appInstList[%d] is NULL. invalid instance ID %d", instID, instID);
    return -1;
  }

  return 0;
}

syncGroup* sageBridge::addSyncGroup()
{
  if (!master)
    return NULL;

  if (!enableSync)
    return NULL;

  SAGE_PRINTLOG("add sync group : sync node num %d", shared->nodeNum);

  syncGroup *sGroup = NULL;

  if (syncServerObj) {
    sGroup = new syncGroup;
    sGroup->init(0, SAGE_ASAP_SYNC_HARD, syncGroupID, MAX_FRAME_RATE, shared->nodeNum);
    syncServerObj->addSyncGroup(sGroup);
    sGroup->blockSync();
    syncGroupID++;
  }
  else
    SAGE_PRINTLOG("sageBridge::addSyncGroup : syncServerObj is NULL");

  return sGroup;
}

int sageBridge::connectToFSManager(appInstance *inst)
{
  return connectToFSManager(inst, inst->fsIP, inst->fsPort);
}

int sageBridge::connectToFSManager(appInstance *inst, char *ip, int port)
{
  SAGE_PRINTLOG("\nsageBridge::%s() connecting to FSM %s:%d for an appInstance %d\n", __FUNCTION__, ip, port, inst->instID);

  /*
   * msgInf->connect() returns msgClientList.size() after push_back QUANTA tcp client object pointer.
   * So, it returns vector index of the fsM id stored in the msgInf->msgClientList
   */
  int fsID = msgInf->connect(ip, port);
  int fsIdx = inst->fsList.size();
  inst->fsList.push_back(fsID);

  // send APP INFO to the new FSM (basically REG_APP msg)
  char msgStr[TOKEN_LEN];
  inst->fillAppInfo(msgStr);
  //SAGE_PRINTLOG("\n\nREG_APP MESSAGE TO FSM: %s\n",msgStr);
  msgInf->msgToClient(fsID, 0, REG_APP, msgStr);

  // send all the widget info to the new FSM
  for (int i=0; i<inst->widgetList.size(); i++)
    msgInf->msgToClient(fsID, 0, ADD_OBJECT, inst->widgetList[i].c_str());

  /* returns vector index of this fsManager in the appInstance::fsList */
  return fsIdx;
}

appInstance* sageBridge::findAppInstance(int instID)
{
  //if (!appInstList[instID])
  //SAGE_PRINTLOG("sageBridge : can't find app instance" );

  return appInstList[instID];
}

appInstance* sageBridge::clientIDtoAppInstance(int clientID, int &orgIdx)
{
  for (int i=0; i<instNum; i++) {
    appInstance *inst = appInstList[i];
    if (inst) {
      int fsNum = inst->fsList.size();
      for (int j=0; j<fsNum; j++) {
        if (inst->fsList[j] == clientID) {
          orgIdx = j;
          return inst;
        }
      }

      if (inst->sailClient == clientID) {
        return inst;
      }
    }
  }

  return NULL;
}

appInstance* sageBridge::forwardToAppinstance(sageMessage &msg, int clientID)
{
  for (int i=0; i<instNum; i++) {
    appInstance *inst = appInstList[i];
    if (inst) {

      // for each fsManager of this app
      int fsNum = inst->fsList.size();
      for (int j=0; j<fsNum; j++) {
        if (inst->fsList[j] == clientID) {
          inst->parseMessage(msg, j);
          msg.setDest(inst->instID);
          msg.setAppCode(j);
          return inst;
        }
      }

      if (inst->sailClient == clientID) {
        inst->parseMessage(msg, 0);
        msg.setDest(inst->instID);
        msg.setAppCode(0);
        return inst;
      }
    }
  }

  return NULL;
}

appInstance* sageBridge::delieverMessage(sageMessage &msg, int clientID)
{
  appInstance *inst = NULL;

  if (master)   {
    inst = forwardToAppinstance(msg, clientID);
    //if (!inst)
    //   SAGE_PRINTLOG("sageBridge : can't forward to an app instance : %d", msg.getCode());

    msgInf->distributeMessage(msg, slaveList, slaveNum);
  }
  else {
    int instID = msg.getDest();
    inst = findAppInstance(instID);
    int fsIdx = msg.getAppCode();
    if (inst)
      inst->parseMessage(msg, fsIdx);
  }

  return inst;
}

int sageBridge::shareApp(char *msgData, int clientID)
{
  if (master)   {
    int fsPort;
    char fsIP[SAGE_IP_LEN];
    sscanf(msgData, "%s %d", fsIP, &fsPort);
    //SAGE_PRINTLOG("app share message : %s", msgData);

    int orgIdx = 0;
    appInstance *inst = clientIDtoAppInstance(clientID, orgIdx);

    if (!inst)
      return -1;

    int newIdx = connectToFSManager(inst, fsIP, fsPort);
    SAGE_PRINTLOG("sageBridge::%s() : instID %d, connected to fsManager %s:%d", __FUNCTION__, inst->instID, fsIP, fsPort);

    syncGroup *sGroup = NULL;
    if (enableSync && inst->nodeNum > 1)
      sGroup = addSyncGroup();
    else
      sGroup = 0;

    int syncID = 0;
    if (sGroup) {
      syncID = sGroup->getSyncID();
    }

    //SAGE_PRINTLOG("added sync group %d for app instance %d", syncID, inst->instID);
    inst->addStreamer(newIdx, orgIdx, sGroup);
    SAGE_PRINTLOG("sageBridge::%s() : instID %d, new streamer added for %s:%d", __FUNCTION__, inst->instID, fsIP, fsPort);

    char msgStr[TOKEN_LEN];
    sprintf(msgStr, "%d %d %d %d", inst->instID, orgIdx, newIdx, syncID);
    msgInf->distributeMessage(SAGE_APP_SHARE, 0, msgStr, slaveList, slaveNum);
  }
  else {
    int instID, orgIdx, newIdx, syncID;
    sscanf(msgData, "%d %d %d %d", &instID, &orgIdx, &newIdx, &syncID);
    appInstance *inst = findAppInstance(instID);
    if (inst)
      inst->addStreamer(newIdx, orgIdx, NULL, syncID);
  }

  return 0;
}

int sageBridge::parseMessage(sageMessage &msg, int clientID)
{
  if (clientID >= 0){
    char *msgData;
    if (msg.getData())
      msgData = (char *)msg.getData();
    else
      msgData = (char *)"\0";

    sageToken tokenBuf(msgData);
    char token[TOKEN_LEN];

    switch (msg.getCode()) {
    case BRIDGE_REG_NODE : {
      tokenBuf.getToken(token);
      int slaveID = atoi(token);
      // store client ID of a slave
      slaveList[slaveID-1] = clientID;

      char msgStr[TOKEN_LEN];
      sprintf(msgStr, "%d %d %d %d %d %d %d %d %d %d", nwCfg->rcvBufSize, nwCfg->sendBufSize,
              nwCfg->mtuSize, (int)nwCfg->maxBandWidth, nwCfg->maxCheckInterval, nwCfg->flowWindow,
              syncPort, streamPort, shared->nodeNum, shared->bufSize);
      msgInf->msgToClient(clientID, 0, BRIDGE_SLAVE_INIT, msgStr);
      break;
    }

    case BRIDGE_SLAVE_INIT : {
      initSlave(msgData);
      break;
    }

    case BRIDGE_APP_REG : {
      regApp(msg, clientID);
      break;
    }

    case BRIDGE_SLAVE_READY : {
      int instID = msg.getDest();
      appInstance *inst = findAppInstance(instID);

      if (inst) {
        inst->waitNodes--;
        // make sure if app instances are ready on all slaves
        if (inst->waitNodes == 0) {
          inst->waitNodes = slaveNum;
          connectToFSManager(inst);
        }
      }
      else {
        SAGE_PRINTLOG("sageBridge::parseMessage : invalid instance ID");
      }

      break;
    }
    case ADD_OBJECT: {
      // store the widget info in the list so we can later forward it to all FSMs
      // stored per appInstance
      int orgIdx = 0;
      appInstance *inst = clientIDtoAppInstance(clientID, orgIdx);

      if (!inst) {
        SAGE_PRINTLOG("\n\n============ NO APP INSTANCE FOR CLIENT ID: %d", clientID);
        return -1;
      }

      inst->widgetList.push_back(std::string(msgData));

      // send to the already existing FSMs...
      for (int i=0; i<inst->fsList.size(); i++) {
        msgInf->msgToClient(inst->fsList[i], 0, ADD_OBJECT, msgData);
      }

      break;
    }

    case BRIDGE_SHUTDOWN : {
      SAGE_PRINTLOG("shuting down sage bridge....");
      if (master)
        msgInf->distributeMessage(msg, slaveList, slaveNum);

      shutdownAllApps();
      shared->eventQueue->sendEvent(EVENT_BRIDGE_SHUTDOWN);
      break;
    }

    case BRIDGE_SLAVE_PERF : {
      int instID = msg.getDest();
      appInstance *inst = findAppInstance(instID);
      if (inst)
        inst->accumulateBandWidth(msgData);
      break;
    }

    case BRIDGE_APP_INST_READY : {
      int instID = msg.getDest();
      appInstance *inst = findAppInstance(instID);

      if (inst) {
        inst->waitNodes--;
        // make sure if app instances are ready on all slaves
        if (inst->waitNodes == 0) {
          connectApp(inst);
          sendStreamInfo(inst);
          inst->waitNodes = inst->nodeNum;
          if (inst->audioOn)
            audioBridge->startAudioStream(instID, inst->sailClient);
        }
      }
      else {
        SAGE_PRINTLOG("sageBridge::parseMessage : invalid instance ID");
      }

      break;
    }

    case SAGE_APP_SHARE : {
      shareApp(msgData, clientID);
      break;
    }

    case APP_QUIT : {
      appInstance *inst = delieverMessage(msg, clientID);
      if (inst) {
        if (audioBridge && inst->audioOn) {
          int fsIdx;
          clientIDtoAppInstance(clientID, fsIdx);
          audioBridge->shutdownStreams(inst->instID, fsIdx);
        }

        shutdownApp(inst->instID, true);
      }

      break;
    }

    case SAIL_CONNECT_TO_ARCV : {
      int fsIdx;
      appInstance *inst = clientIDtoAppInstance(clientID, fsIdx);
      if (inst && audioBridge) {
        audioBridge->duplicate(inst->instID, msgData, fsIdx);
      }
      break;
    }

    case NOTIFY_APP_SHUTDOWN : {
      appInstance *inst = delieverMessage(msg, clientID);
      if (inst)
        shutdownApp(inst->instID, false);
      break;
    }

    case CLEAR_APP_INSTANCE : {
      int instID = msg.getDest();
      appInstance *inst = findAppInstance(instID);
      appInstList[instID] = NULL;
      if (inst)
        delete inst;
      SAGE_PRINTLOG("app instance cleared at node %d", shared->nodeID);
      break;
    }

    case SAIL_SEND_TIME_BLOCK : {
      appInstance *inst = delieverMessage(msg, clientID);
      if (inst)
        msgInf->msgToClient(inst->sailClient, 0, SAIL_SEND_TIME_BLOCK);
      break;
    }

    case SAIL_INIT_STREAM : {
      appInstance *appInst = delieverMessage(msg, clientID);
      break;
    }

    default : {
      appInstance *inst = delieverMessage(msg, clientID);
      break;
    }
    }

    // forward app events
    if (APP_MESSAGE <= msg.getCode() && msg.getCode() < APP_MESSAGE+1000) {
      appInstance *inst = delieverMessage(msg, clientID);
      if (inst)
        forwardToSail(inst->instID, msg);
    }
  }

  msg.destroy();

  return 0;
}

int sageBridge::parseEvent(sageEvent *event)
{
  if (!event) {
    SAGE_PRINTLOG("sageBridge::parseEvent : event object is Null");
    return -1;
  }

  switch (event->eventType) {
    /**
     * from sageBridge::nwCheckThread()
     */
  case EVENT_NEW_CONNECTION : {
    initStreams(event->eventMsg, (streamProtocol *)event->param);
    break;
  }

    /**
     * from sageBridge::msgCheckThread()
     */
  case EVENT_NEW_MESSAGE : {
    int clientID = atoi(event->eventMsg);
    sageMessage *msg = (sageMessage *)event->param;

    //if (msg->getCode() != ADD_OBJECT)   // too many printfs otherwise
    //SAGE_PRINTLOG("sageBridge::%s() : envInf received new message code %d [%s]\n", __FUNCTION__, msg->getCode(), (char *)msg->getData());

    parseMessage(*msg, clientID);
    delete msg;
    break;
  }

    /*
     * EVENT_NEW_CONNECTION by sageBridge::nwCheckThread() triggers appInstance::addStream() which triggers sageReceiver::addStream()
     * In there, this event is generated
     */
  case EVENT_APP_CONNECTED : {
    int instID;
    sscanf(event->eventMsg, "%d", &instID);

    appInstance *inst = appInstList[instID];
    //SAGE_PRINTLOG("\n\nEVENT_APP_CONNECTED: instID %d, clientID %d\n\n", instID, inst->sailClient);

    if (inst) {
      if (master) {
        inst->waitNodes--;
        if (inst->waitNodes == 0) {
          connectToFSManager(inst);
          inst->waitNodes = slaveNum;
        }
      }
      else
        msgInf->msgToServer(instID, BRIDGE_SLAVE_READY);
    }
    break;
  }

  case EVENT_SLAVE_PERF_INFO : {
    int instID;
    sscanf(event->eventMsg, "%d", &instID);
    char *msgStr = sage::tokenSeek(event->eventMsg, 1);
    msgInf->msgToServer(instID, BRIDGE_SLAVE_PERF, msgStr);
    break;
  }

  case EVENT_MASTER_PERF_INFO : {
    int fsClientID;
    sscanf(event->eventMsg, "%d", &fsClientID);
    char *msgStr = sage::tokenSeek(event->eventMsg, 1);
    msgInf->msgToClient(fsClientID, 0, DISP_SAIL_PERF_RPT, msgStr);
    //SAGE_PRINTLOG("bridge perf %s", msgStr);
    break;
  }

  case EVENT_APP_SHUTDOWN : {
    int fsClientID;
    sscanf(event->eventMsg, "%d", &fsClientID);
    msgInf->msgToClient(fsClientID, 0, NOTIFY_APP_SHUTDOWN);
    break;
  }

  case EVENT_BRIDGE_SHUTDOWN : {
    bridgeEnd = true;
    delete tcpObj;
    delete udpObj;
    if (syncServerObj)
      delete syncServerObj;

    break;
  }

  case EVENT_AUDIO_CONNECTION : {
    if (audioBridge)
      audioBridge->initStreams(event->eventMsg, (streamProtocol *)event->param);
    break;
  }
  }

  delete event;

  return 0;
}

void sageBridge::mainLoop()
{
  while(!bridgeEnd) {
    sageEvent *newEvent = shared->eventQueue->getEvent();
    //SAGE_PRINTLOG("get the event %d", newEvent->eventType);
    parseEvent(newEvent);
    sage::usleep(100);
  }
}

int main(int argc, char*argv[])
{
  sage::initUtil();
  sageBridge bridge(argc, argv);
  bridge.mainLoop();
  _exit(0);
}
