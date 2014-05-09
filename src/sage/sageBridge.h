/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBridge.h
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

#ifndef SAGEBRIDGE_H_
#define SAGEBRIDGE_H_

#include "sage.h"

#define MAX_BRIDGE_NODE 100

class messageInterface;
class sageBridge;
class streamProtocol;
class bridgeSharedData;
class sageTcpModule;
class sageUdpModule;
class sageNwConfig;
class appInstance;
class sageSyncServer;
class syncGroup;
class sageEvent;
class sageAudioBridge;


typedef struct {
  sageBridge     *This;
  streamProtocol *nwObj;
} nwCheckThreadParam;

class sageBridge {
protected:
  bridgeSharedData *shared;
  sageTcpModule *tcpObj;
  sageUdpModule *udpObj;
  sageNwConfig *nwCfg;
  sageAudioBridge *audioBridge;

  bool master;
  char masterIP[SAGE_IP_LEN];
  int streamPort, msgPort, syncPort, audioPort;
  int slaveNum;
  int memSize;
  int allocPolicy;
  char slaveIPs[MAX_BRIDGE_NODE][SAGE_IP_LEN];
  int slaveList[MAX_BRIDGE_NODE];

  messageInterface *msgInf;
  appInstance *appInstList[MAX_INST_NUM];
  int instNum;     // RJ: basically a winID for sageBridge... same as fsm->m_execIndex
  sageSyncServer *syncServerObj;
  //sageSyncClient *syncClientObj;

  bool bridgeEnd;
  int syncGroupID;
  int enableSync;

  int initMaster(char *cFile);
  int initSlave(char *data);
  int launchSlaves();
  int regApp(sageMessage &msg, int clientID);
  int shareApp(char *msgData, int clientID);
  int connectApp(appInstance *inst);
  int sendStreamInfo(appInstance *inst);

  int connectToFSManager(appInstance *inst);
  int connectToFSManager(appInstance *inst, char *ip, int port);
  syncGroup* addSyncGroup();

  appInstance* clientIDtoAppInstance(int clientID, int &orgIdx);
  appInstance* forwardToAppinstance(sageMessage &msg, int clientID);
  appInstance* findAppInstance(int instID);
  appInstance* delieverMessage(sageMessage &msg, int clientID);
  int parseMessage(sageMessage &msg, int clientID);
  int parseEvent(sageEvent *event);

  int findMinLoadNode();
  int startPerformanceReport(sageMessage &msg);
  int stopPerformanceReport(sageMessage &msg);
  int perfReport();
  int shutdownApp(int instID, bool fsmToApp);
  int forwardToSail(int instID, sageMessage &msg);
  int shutdownAllApps();

  int initNetworks();
  int checkNetworks();
  int initStreams(char *msg, streamProtocol *nwObj);


  static void* msgCheckThread(void *args);
  static void* perfReportThread(void *args);
  static void* nwCheckThread(void *args);

public:
  sageBridge(int argc, char **argv);
  ~sageBridge();
  void mainLoop();

  friend class appInstance;
};

#endif
