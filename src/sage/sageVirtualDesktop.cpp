/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageVirtualDesktop.cpp - providing operations needed for the desktop
 *           management for tiled display
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
#include "sageVirtualDesktop.h"
#include "streamInfo.h"
#include "fsManager.h"
#include "fsCore.h"

sageVirtualDesktop::sageVirtualDesktop(fsManager *f, int id)
{
  fsm = f;
  displayID = id;

  clientList.clear();
  audioClientList.clear();
}

sageVirtualDesktop::~sageVirtualDesktop()
{
  clientList.clear();
  audioClientList.clear();
}

int sageVirtualDesktop::getRcvId(int nodeId)
{
  for(int i=0; i<tileNodeList.size(); i++) {
    if (tileNodeList[i] == nodeId)
      return clientList[i];
  }

  return -1;
}

int sageVirtualDesktop::regRcv(int cId, int nodeID)
{
  tileNodeList.push_back(nodeID);
  clientList.push_back(cId);

  return 0;
}

int sageVirtualDesktop::getNodeIPs(int nodeId, char *ipStr)
{
  strcpy(ipStr, displayCluster[nodeId]->ip);

  return 0;
}

int sageVirtualDesktop::changeBGColor(int red, int green, int blue)
{
  for (int i=0; i<clientList.size(); i++) {
    char colorStr[TOKEN_LEN];
    sprintf(colorStr, "%d %d %d", red, green, blue);
    fsm->sendMessage(clientList[i], RCV_CHANGE_BGCOLOR, colorStr);
  }
  return 0;
}

int sageVirtualDesktop::sendToAll(int code, char *data)
{
  for (int i=0; i<clientList.size(); i++) {
    if (data)
      fsm->sendMessage(clientList[i], code, data);
    else
      fsm->sendMessage(clientList[i], code);
  }

  return 0;
}

int sageVirtualDesktop::launchReceivers(char *fsIP, int port, int syncPort, bool globalSync, int syncBarrierPort, int refreshInterval, int syncMasterPollingInterval, int syncLevel)
{
  char *sageDir = getenv("SAGE_DIRECTORY");
  if (!sageDir) {
    std::cout << "sageVirtualDesktop : cannot find the environment variable SAGE_DIRECTORY" << std::endl;
    return -1;
  }

  for (int i=0; i<displayCluster.size(); i++) {
    char command[TOKEN_LEN];

#if defined(WIN32)
    sprintf(command, "start  /D%s\\bin %s\\bin\\sageDisplayManager %s %d %d %d %d %d %d %d %d %d",
            sageDir,
            sageDir, fsIP, port, i, syncPort, displayID, (int)globalSync, syncBarrierPort, refreshInterval, syncMasterPollingInterval, syncLevel);

    std::cout << "ATTENTION: SAGE on Windows works only locally, no remote execution" << std::endl;
    std::cout << "\t" << command << std::endl;
    system( command );
#else
    sprintf(command, "SAGE_DIRECTORY=%s %s/bin/sageDisplayManager %s %d %d %d %d %d %d %d %d %d",
            sageDir, sageDir, fsIP, port, i, syncPort, displayID, (int)globalSync, syncBarrierPort, refreshInterval, syncMasterPollingInterval, syncLevel);
    if (execRemBin(displayCluster[i]->ip, command, displayCluster[i]->Xdisp) < 0)
      return -1;
#endif
  }

  if (masterIP[0] == '\0')
    getNodeIPs(0, masterIP);

  return 0;
}

int sageVirtualDesktop::getRcvInfo(int nodeID, char *info)
{
  displayNode *disp = displayCluster[nodeID];
  if (!disp) {
    SAGE_PRINTLOG("sageVirtualDesktop : Can't find the display node %d", nodeID);
    return -1;
  }

  sprintf(info, "%s %d %d %d %d %d %d", masterIP, disp->tiles[0]->width, disp->tiles[0]->height,
          disp->dimX, disp->dimY, disp->winX, disp->winY);

  char tileInfo[TOKEN_LEN];
  for (int i=0; i<disp->tiles.size(); i++) {
    int tileX = disp->tiles[i]->x;
    int tileY = disp->tiles[i]->y;
    // get the top-left corner of the tile
    int revY = convertYaxis(disp->tiles[i]->y + disp->tiles[i]->height);
    sprintf(tileInfo, " %d %d %d %d", disp->tiles[i]->tileID, tileX, tileY, revY);
    strcat(info, tileInfo);
  }

  //std::cout << "recv " << nodeID << " : " << info << std::endl;

  return 0;
}

int sageVirtualDesktop::getLocalPort(int nodeID)
{
  displayNode *disp = displayCluster[nodeID];
  if (!disp) {
    SAGE_PRINTLOG("sageVirtualDesktop : Can't find the display node %d", nodeID);
    return -1;
  }

  return disp->port;
}

int sageVirtualDesktop::generateStreamInfo(streamGroup &sGrp, std::vector<int> &rcvList, int offset)
{
  for (int i=0; i<tileList.size(); i++) {
    streamInfo newStream;

    // set the stream image area to the overlap of a tile area and
    // an area mapped to a sender
    sageRect appWindow = (sageRect &)sGrp;
    if (appWindow.isOverLap(*(sageRect *)tileList[i], (sageRect &)newStream)) {
      newStream.receiverID = tileList[i]->nodeID + offset;
      //newStream.tileID = i;

      sGrp.addStream(newStream);
      int cnt = 0;
#ifdef __SUNPRO_CC
      std::count(rcvList.begin(), rcvList.end(), tileList[i]->nodeID, cnt);
#else
      cnt = std::count(rcvList.begin(), rcvList.end(), tileList[i]->nodeID);
#endif

      if (cnt == 0)
        rcvList.push_back(tileList[i]->nodeID);
    }
  }

  return 0;
}

bool sageVirtualDesktop::checkLayout(appInExec* app)
{
  int checkResult = isOverLap(*(sageRect *)app, 0.5);
  if (checkResult == ON_RECT)
    return true;
  else  {
    checkNeighbors(app, checkResult);
    return false;
  }
}

void sageVirtualDesktop::checkNeighbors(appInExec *app, int edge)
{
  //std::cerr << "check neighbor " << edge << std::endl;
  bool relocate = false;
  for (int i=0; i<connectionList.size(); i++) {
    for (int j=0; j<2; j++) {
      //std::cerr << "display " << connectionList[i]->displays[j]->displayID << std::endl;
      //std::cerr << "edge " << connectionList[i]->edges[j] << std::endl;
      if (connectionList[i]->displays[j]->displayID == displayID &&
          connectionList[i]->edges[j] == edge) {
        connectionList[i]->displays[1-j]->locateApp(app, connectionList[i], 1-j);
        relocate = true;
        app->displayID = connectionList[i]->displays[1-j]->displayID;
      }
    }
    if (relocate)
      break;
  }

  if (!relocate)
    locateApp(app, NULL);
}

int sageVirtualDesktop::getAudioRcvId(int nodeId)
{
  for(int i=0; i<audioNodeList.size(); i++) {
    if (audioNodeList[i] == nodeId)
      return audioClientList[i];
  }

  return -1;
}

int sageVirtualDesktop::regAudioRcv(int cId, int nodeId)
{
  audioNodeList.push_back(nodeId);
  audioClientList.push_back(cId);

  return 0;
}

int sageVirtualDesktop::getAudioNodeIPs(int nodeId, char *ipStr)
{
  strcpy(ipStr, audioCluster[nodeId]->ip);

  return 1;
}

int sageVirtualDesktop::launchAudioReceivers(char *fsIP, int port, int syncPort)
{
  char *sageDir;
  if (audioServer)
    sageDir = audioDir;
  else
    sageDir = getenv("SAGE_DIRECTORY");

  if (!sageDir) {
    std::cout << "sageVirtualDesktop : cannot find the environment variable SAGE_DIRECTORY" << std::endl;
    return -1;
  }

  int audioSyncPort = 14999;
  for (int i=0; i< audioCluster.size(); i++) {
    char command[TOKEN_LEN];

#if defined(WIN32)
    sprintf(command, "start /B /D%s %s\\bin\\sageAudioManager %s %d %d %d",
            sageDir, sageDir, fsIP, port, i, syncPort/*,audioSyncPort*/);

    std::cout << "ATTENTION: SAGE on Windows works only locally, no remote execution" << std::endl;
    std::cout << "\t" << command << std::endl;
    system( command );
#else
    sprintf(command, "%s/bin/sageAudioManager %s %d %d %d", sageDir, fsIP, port, i, syncPort /*, audioSyncPort*/);
    std::cout << "audio " << command << std::endl;
    if (execRemBin(audioCluster[i]->ip, command) < 0)
      return -1;
#endif

  }

  return 0;
}

int sageVirtualDesktop::getAudioRcvInfo(int nodeID, char *info)
{
  char *ipStr;

  // if (nodeID == 0)
  //        ipStr = "127.0.0.1";
  // else
  //        ipStr = audioCluster[0]->ip;
  ipStr = displayCluster[0]->ip;


  audioNode *audio = audioCluster[nodeID];

  // need to modify
  sprintf(info, "%s %d %d %ld %d %d", ipStr, audio->deviceId, (int)audio->sampleFmt, audio->samplingRate,
          audio->channels, audio->framePerBuffer);

  std::cout << "recv " << nodeID << " : " << info << std::endl;

  return 0;
}

int sageVirtualDesktop::generateAudioRcvInfo(int port, char* msgStr)
{
  int tileNum = audioCluster.size();

  char tileInfoStr[TOKEN_LEN];
  memset(tileInfoStr, 0, TOKEN_LEN);
  sprintf(tileInfoStr, "%d %d ", port, tileNum);
  strcat(msgStr, tileInfoStr);
  //SAGE_PRINTLOG("---------%s \n", tileInfoStr);

  // list ip addresses for each tile
  for (int i=0; i<tileNum; i++) {

    char tileStr[TOKEN_LEN];
    sprintf(tileStr, "%s %d ", audioCluster[i]->ip, i);
    strcat(msgStr, tileStr);
  }

  return 0;
}

int sageVirtualDesktop::generateAudioRcvInfo(char* ip, int port, char* msgStr)
{
  /*int tileNum = audioCluster.size();
    tileNum = 1;    // assumption

    char tileInfoStr[TOKEN_LEN];
    memset(tileInfoStr, 0, TOKEN_LEN);
    sprintf(tileInfoStr, "%d %d ", port, tileNum);
    strcat(msgStr, tileInfoStr);

    int attachedNum;
    // list ip addresses for each tile
    char tileStr[TOKEN_LEN];

    for (int i=0; i<tileNum; i++) {

    attachedNum = audioCluster[i]->streamIPs.size();
    for(int j =0; j < attachedNum; j++) {
    if(strcmp(audioCluster[i]->streamIPs[j], ip) == 0) {

    }
    }

    sprintf(tileStr, "%s %d ", audioCluster[i]->ip, i);
    strcat(msgStr, tileStr);
    }*/

  // new --- test
  int tileNum =1;
  char tileInfoStr[TOKEN_LEN];
  memset(tileInfoStr, 0, TOKEN_LEN);
  sprintf(tileInfoStr, "%d %d ", port, tileNum);
  //SAGE_PRINTLOG("---------%s \n", tileInfoStr);
  //SAGE_PRINTLOG("---------%d %d \n", port, tileNum);
  strcat(msgStr, tileInfoStr);

  // list ip addresses for each tile
  char tileStr[TOKEN_LEN];
  sprintf(tileStr, "%s %d ", audioCluster[0]->ip, 0);
  //audioCluster[0]->nodeCount++;
  strcat(msgStr, tileStr);

  return 0;
}

int sageVirtualDesktop::generateAudioStreamInfo(streamGroup &sGrp, std::vector<int> &rcvList)
{
  /*      for (int i=0; i<audioList.size(); i++) {
          streamInfo newStream;

          // need to modify

          // set the stream image area to the overlap of a tile area and
          // an area mapped to a sender
          sageRect appWindow = (sageRect &)sGrp;
          if (appWindow.isOverLap(*(sageRect *)tileList[i], (sageRect &)newStream)) {
          newStream.receiverId = audioList[i]->nodeID;
          newStream.tileID = i;

          sGrp.addStream(newStream);
          int cnt = 0;
          cnt = std::count(rcvList.begin(), rcvList.end(), audioList[i]->nodeID);

          if (cnt == 0)
          rcvList.push_back(audioList[i]->nodeID);
          }
          }*/

  return 0;
}
