/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: displayInstance.h
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

#ifndef _DISPLAY_MANAGER_H
#define _DISPLAY_MANAGER_H

#include "sage.h"
#include "sageConfig.h"

class fsManager;
class appInExec;
class sageVirtualDesktop;
class streamInfo;
class connectionInfo;

/**
 * class displayInstance
 */
class displayInstance {
public:
  int winID;

private:
  fsManager *fsm;
  appInExec *appExec;
  sageVirtualDesktop *vdt;

  std::vector<int> receiverList;
  streamGroup streamGrp;

  int tileNum;
  int dispNodeNum;
  int streamNum;
  int sailClient;
  int zValue;
  int rcvFlagCnt;
  int syncMode;
  int waitNodes;
  int imageSize;
  float rcvFrate, rcvBwidth, rcvLoss, accBwidth, accLoss;
  float sendFrate;
  int accFSize;

  char winTitle[SAGE_NAME_LEN];
  int borderColor[3];
  int titleColor[3];

  bool *rcvFlag;

  void reportPerformance(char *sailPerf);
  int accumulateBandwidth(char *rcvBand);
  void generateStreamInfo(void);
  int streamInfoToSender(void);

  int shutdownApp();

public:
  displayInstance(fsManager *f, int id, appInExec* app);
  ~displayInstance() {}
  int getReceiverNum() { return receiverList.size(); }
  int getReceiver(int i) { return receiverList[i]; }
  int getStreamNum() { return streamNum; }
  int getSailClient() { return sailClient; }
  int parseMsg(sageMessage &msg);
  int getWinID() { return winID; }
  //int shutdownReceivers();
  int changeWindow(sageRect &devRect, int steps);
  int modifyStream(void);
  int connectSenders(); /**< send messages to sail client */
  int initStreams();
  int getZValue() { return zValue; }
  int setZValue(int z) { zValue = z; return 1;}
  int increaseZ() { zValue++; return 1;}
  int decreaseZ() { zValue--; return 1;}
  int requestPerformanceInfo(int rate);
  int stopPerformanceInfo();
  int updateWinProp(char *info);
  int updateBorders(char *title);

  friend class fsCore;
};

#endif
