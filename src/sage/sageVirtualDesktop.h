/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageVirtualDesktop.h
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

#ifndef _SAGE_VIRTUAL_DESKTOP_H
#define _SAGE_VIRTUAL_DESKTOP_H

#include "sage.h"
#include "tileConfig.h"

class streamInfo;
class streamGroup;
class fsManager;
class appInExec;

class sageVirtualDesktop : public virtualDesktop {
public:
  std::vector<int> tileNodeList;   // nodeIDs of display nodes
  std::vector<int> clientList;      // clientIDs of display nodes

  std::vector<int> audioNodeList;         // nodeIDs of audio nodes
  std::vector<int> audioClientList;       // clientIDs of audio nodes

  fsManager *fsm;

  sageVirtualDesktop(fsManager *f, int id);
  ~sageVirtualDesktop();
  int generateStreamInfo(streamGroup &sGrp, std::vector<int> &rcvList, int offset = 0);

  /**
   * launches the binary sageDisplayManager
   */
  int launchReceivers(char *fsIP, int port, int syncPort, bool globalSync = true, int syncBarrierPort=11001, int refreshInterval=60, int syncMasterPollingInterval=1000, int syncLevel=2 );

  /**
   * launches the binary sageAudioManager
   */
  int launchAudioReceivers(char *fsIP, int port, int syncPort);

  int getRcvId(int nodeId);
  int getNodeNum() { return displayCluster.size(); }
  int getTileNum() { return tileList.size(); }
  int getNodeId(int tileId) { return tileList[tileId]->nodeID; }
  int getNodeIPs(int nodeId, char *ipStr);

  int getRcvInfo(int nodeID, char *info);
  int regRcv(int cId, int nodeID);
  int getLocalPort(int nodeID);

  int getAudioRcvId(int nodeId);
  int getAudioRcvInfo(int nodeId, char *info);
  int regAudioRcv(int cId, int nodeId);
  int getAudioNodeIPs(int nodeId, char *ipStr);
  int generateAudioStreamInfo(streamGroup &sGrp, std::vector<int> &rcvList);
  int generateAudioRcvInfo(int port, char* msgStr);
  int generateAudioRcvInfo(char* ip, int port, char* msgStr);

  std::vector<int> getRcvClientList() { return clientList; }
  std::vector<int> getAudioRcvClientList() { return audioClientList; }
  int changeBGColor(int red, int green, int blue);
  int sendToAll(int code, char *data);
  int convertYaxis(int y) { return height - y; }
  bool checkLayout(appInExec* app);
  void checkNeighbors(appInExec *app, int edge);
};

#endif
