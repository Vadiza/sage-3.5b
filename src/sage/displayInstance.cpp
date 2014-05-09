/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: displayInstance.cpp - a part of free space manager controlling each
 *         application instance.
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

#include "streamInfo.h"
#include "displayInstance.h"
#include "fsManager.h"
#include "fsCore.h"
#include "sageVirtualDesktop.h"

displayInstance::displayInstance(fsManager *f, int id, appInExec* app) : fsm(f), winID(id), appExec(app)
{
  //appExec = fsm->execList[id];
  receiverList.clear();
  sailClient = appExec->sailClient;
  zValue = 0;

  //imageSize = appExec->imageWidth * appExec->imageHeight * appExec->bytesPerPixel;
  //if (imageSize == 0) {
  //   SAGE_PRINTLOG("displayInstance::displayInstance : image_size = 0, using default size");
  //   imageSize = DEFAULT_IMAGE_SIZE;
  //}

  streamNum = 0;
  rcvFrate = 0.0;
  rcvBwidth = 0.0;
  accBwidth = 0.0;
  accFSize = 0;
  sendFrate = appExec->frameRate;
  dispNodeNum = 0;
  tileNum = 0;

  for (int i=0; i<fsm->vdtList.size(); i++) {
    dispNodeNum += fsm->vdtList[i]->getNodeNum();
    tileNum += fsm->vdtList[i]->getTileNum();
  }

  waitNodes = dispNodeNum;
  rcvFlagCnt = 0;

  SAGE_PRINTLOG("Establishing network connections for streams......");
  connectSenders();
  initStreams();

  memset(winTitle, 0, SAGE_NAME_LEN);
}

void displayInstance::generateStreamInfo(void)
{
  fsm->vdtList[appExec->displayID]->checkLayout(appExec);
  streamGrp = *(sageRect *)appExec;

  int offset = 0;
  for (int i=0; i<appExec->displayID; i++)
    offset += fsm->vdtList[i]->getNodeNum();
  fsm->vdtList[appExec->displayID]->generateStreamInfo(streamGrp, receiverList, offset);
  streamGrp.setRcvNum(receiverList.size());
}

int displayInstance::streamInfoToSender(void)
{
  // send new stream info to recevers
  char msgStr[TOKEN_LEN];
  streamGrp.createRcvMsg(winID, msgStr);
  fsm->sendToVDT(appExec->displayID, RCV_UPDATE_DISPLAY, msgStr);
  fsm->sendToOtherVDT(appExec->displayID, RCV_CLEAR_DISPLAY, winID);

  //SAGE_PRINTLOG("displayInstance : send stream info to SAIL");
  sageMessage msg;
  streamGrp.createMessage(msg, SAIL_INIT_STREAM);
  msg.setDest(sailClient);

  if (fsm->sendMessage(msg) < 0) {
    SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
    return -1;
  }

  return 0;
}

int displayInstance::connectSenders()
{
  int msgLen = 8 + SAGE_IP_LEN * dispNodeNum;
  char *msgStr = new char[msgLen];

  sprintf(msgStr, "%d %d ", fsm->rInfo.streamPort, dispNodeNum);
  int nodeID = 0;

  for (int j=0; j<fsm->vdtList.size(); j++) {
    // list ip addresses for each display node
    for (int i=0; i<fsm->vdtList[j]->getNodeNum(); i++) {
      char ipStr[TOKEN_LEN];
      fsm->vdtList[j]->getNodeIPs(i, ipStr);

      char tileStr[TOKEN_LEN];

      if (fsm->useLocalPort) {
        int localPort = fsm->vdtList[j]->getLocalPort(i);
        sprintf(tileStr, "%s %d %d ", ipStr, localPort, nodeID++);
      }
      else
        sprintf(tileStr, "%s %d ", ipStr, nodeID++);

      strcat(msgStr, tileStr);
    }
  }

  int connectMsg = SAIL_CONNECT_TO_RCV;
  if (fsm->useLocalPort)
    connectMsg = SAIL_CONNECT_TO_RCV_PORT;

  //SAGE_PRINTLOG("displayInstance::%s() : sending SAIL_CONNECT_TO_RCV(_PORT), streamPort dispNodeNum (ipStr localPort? nodeID)+ [%s]\n", __FUNCTION__, msgStr);

  if (fsm->sendMessage(sailClient, connectMsg, msgStr) < 0) {
    SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
    return -1;
  }

  SAGE_PRINTLOG("displayInstance::%s() : SAIL_CONNECT_TO_RCV(or _PORT) sent to SAIL", __FUNCTION__);
  return 0;
}

int displayInstance::initStreams()
{
  generateStreamInfo();
  if (streamInfoToSender() < 0)
    return -1;

  SAGE_PRINTLOG("init stream message sent");

  return 0;
}

int displayInstance::modifyStream(void)
{
  streamGrp.init();
  receiverList.clear();
  generateStreamInfo();
  if (streamInfoToSender() < 0)
    return -1;

  if (appExec->protocol == SAGE_UDP) {
    sendFrate = appExec->frameRate;
    if (fsm->sendMessage(sailClient, SAIL_FRAME_RATE, appExec->frameRate) < 0) {
      SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
      return -1;
    }
  }

  return 0;
}

int displayInstance::changeWindow(sageRect &devRect, int steps)
{
  sageRect deltaRect = devRect/steps;
  sageRect initWindow = *(sageRect *)appExec;

  for (int i=0; i<steps; i++) {
    streamGrp.init();
    receiverList.clear();

    if (i == steps-1) {
      *(sageRect *)appExec = initWindow + devRect;
    }
    else {
      *(sageRect *)appExec += deltaRect;
    }

    bool inScope = fsm->vdtList[appExec->displayID]->checkLayout(appExec);

    streamGrp = *(sageRect*)appExec;
    int offset = 0;
    for (int i=0; i<appExec->displayID; i++)
      offset += fsm->vdtList[i]->getNodeNum();

    fsm->vdtList[appExec->displayID]->generateStreamInfo(streamGrp, receiverList, offset);
    streamGrp.setRcvNum(receiverList.size());
    if (streamInfoToSender() < 0)
      return -1;
    if (!inScope)
      break;
  }

  if (appExec->protocol == SAGE_UDP) {
    sendFrate = appExec->frameRate;
    if (fsm->sendMessage(sailClient, SAIL_FRAME_RATE, appExec->frameRate) < 0) {
      SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
      return -1;
    }
  }

  return 0;
}

int displayInstance::accumulateBandwidth(char *rcvBand)
{
  char token[TOKEN_LEN];
  getToken(rcvBand, token);
  int instID = atoi(token);
  getToken(rcvBand, token);
  float bWidth = atof(token);
  getToken(rcvBand, token);
  float pLoss = atof(token);
  getToken(rcvBand, token);
  int fSize = atoi(token);

  accBwidth += bWidth;
  accLoss += pLoss;
  accFSize += fSize;

  //SAGE_PRINTLOG("frame size %d", fSize);

  rcvFlagCnt++;

  if (rcvFlagCnt >= dispNodeNum) {
    rcvBwidth = accBwidth;
    rcvLoss = accLoss;
    accBwidth = 0.0;
    accLoss = 0.0;
    rcvFlagCnt = 0;

    if (rcvLoss > 0 && appExec->protocol == SAGE_UDP) {
      float newFrameRate = (rcvBwidth*990000.0)/(accFSize*8.0);
      newFrameRate = MAX(newFrameRate, sendFrate*0.9);
      newFrameRate = MIN(newFrameRate, sendFrate*1.1);
      sendFrate = newFrameRate;

      SAGE_PRINTLOG("=== %s %d ===", appExec->appName, winID);
      SAGE_PRINTLOG("Data loss = %d Mbps", rcvLoss);
      SAGE_PRINTLOG("New Frame Rate = %d fps", newFrameRate);

      char frateStr[TOKEN_LEN];
      sprintf(frateStr, "%7.2f", newFrameRate);

      if (fsm->sendMessage(sailClient, SAIL_FRAME_RATE, frateStr) < 0) {
        SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
        return -1;
      }
    }

    accFSize = 0;
  }

  return 0;
}

void displayInstance::reportPerformance(char *sailPerf)
{
  float packetLoss = 0;
  if (rcvBwidth + rcvLoss > 0)
    packetLoss = rcvLoss*100/(rcvBwidth + rcvLoss);

  int receiverNum = receiverList.size();

  char msgStr[TOKEN_LEN];

  sprintf(msgStr, "%d\nDisplay %7.2f %7.2f %7.2f %d\nRendering %s", winID,
          rcvBwidth, rcvFrate, packetLoss, receiverNum, sailPerf);

  int uiNum = fsm->uiList.size();
  for (int j=0; j<uiNum; j++) {
    if (fsm->uiList[j] < 0)
      continue;

    if (fsm->sendMessage(fsm->uiList[j], UI_PERF_INFO, msgStr) < 0) {
      SAGE_PRINTLOG("displayInstance : uiClient(%d) is stuck or shutdown", j);
      fsm->uiList[j] = -1;
    }
  }
}

int displayInstance::requestPerformanceInfo(int rate)
{
  char perfStr[TOKEN_LEN];
  sprintf(perfStr, "%d %d", winID, rate);

  fsm->sendToAllRcvs(RCV_PERF_INFO_REQ, perfStr);
  if (fsm->sendMessage(sailClient, SAIL_PERF_INFO_REQ, rate) < 0) {
    SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
    return -1;
  }

  return 0;
}

int displayInstance::stopPerformanceInfo()
{
  //fsm->sendToAllRcvs(RCV_PERF_INFO_STOP, winID);
  if (fsm->sendMessage(sailClient, SAIL_PERF_INFO_STOP) < 0) {
    SAGE_PRINTLOG("displayInstance : %s(%d) is stuck or shutdown", appExec->appName, winID);
    return -1;
  }

  return 0;
}

int displayInstance::parseMsg(sageMessage &msg)
{
  switch (msg.getCode()) {
  case DISP_RCV_FRATE_RPT : {
    int instID;
    sscanf((char *)msg.getData(), "%d %f", &instID, &rcvFrate);
    /*         char token[TOKEN_LEN];
               getToken((char *)msg.getData(), token);
               rcvFrate = atof(token);*/
    break;
  }

  case DISP_RCV_BANDWITH_RPT : {
    accumulateBandwidth((char *)msg.getData());
    break;
  }

  case DISP_SAIL_PERF_RPT : {
    reportPerformance((char *)msg.getData());
    break;
  }

  case DISP_APP_CONNECTED : {
    waitNodes--;
    if (waitNodes == 0) {
      fsm->core->windowChanged(winID);
      fsm->core->bringToFront(winID);
    }
    break;
  }
  }

  return 0;
}
