/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: fsCore.h
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


#ifndef _FS_CORE_H
#define _FS_CORE_H

#include "sage.h"

class fsManager;
class appInExec;

/**
 * class fsCore
 * The core part of the Free Space Manager processing user commands to run applications or send appropriate orders to each part of SAGE
 */
class fsCore {
private:
  fsManager *fsm;
  double startTime;
  int winSteps;
  int numReportedReceivers;

public:
  fsCore();
  ~fsCore();
  int init(fsManager *fsm);
  int parseMessage(sageMessage &msg, int clientID);
  int sendSageStatus(int clientID);
  int sendAppStatus(int clientID, char *appName);
  int sendAppInfo(int clientID);
  int sendAppInfo(int winID, int clientID, bool dimOriginatedChange=false);
  int sendDisplayInfo(int clientID);
  int initDisp(appInExec* app);
  int initAudio();
  int windowChanged(int winID, bool dimOriginatedChange=false);
  int bringToFront(int winID);
  int pushToBack(int winID);
  int rotateWindow(char *msgStr);
  int widgetToObjID(int widgetID, int winID);
  void clearAppInstance(int id);
  int getAvailableInstID(void);
  appInExec* findApp(int id, int& index);
  appInExec* findAppBySailID(int id, int& index);


};

#endif
