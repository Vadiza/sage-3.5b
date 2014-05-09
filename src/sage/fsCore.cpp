/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: fsCore.cpp - the core part of the Free Space Manager processing
 *         user commands to run applications or send appropriate orders
 *         to each part of SAGE.
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

#include "fsCore.h"
#include "fsManager.h"
#include "streamInfo.h"
#include "misc.h"
#include "displayInstance.h"
#include "sageVirtualDesktop.h"
#include "streamProtocol.h"
#include "sageDrawObject.h"

FILE *logFile;

fsCore::fsCore()
{
  fsm = NULL;
  numReportedReceivers = 0;
}

fsCore::~fsCore()
{
}

int fsCore::init(fsManager *m)
{
  fsm = m;
  winSteps = (int)ceil(m->winTime/20.0);
  logFile = fopen("window.log","w+");
  return 0;
}

int fsCore::initDisp(appInExec* app)
{
  displayInstance *disp = new displayInstance(fsm, fsm->m_execIndex, app);
  fsm->dispList.push_back(disp);

  return 0;
}

int fsCore::initAudio()
{
  appInExec *appExec;
  //int execNum = fsm->execList.size();

  //int msgLen = 8 + SAGE_IP_LEN * execNum;
  int msgLen = 8 + SAGE_IP_LEN * fsm->vdtList[0]->getNodeNum();
  char *msgStr = new char[msgLen];
  memset(msgStr, 0, msgLen);

  //for(int i=0; i < execNum; i++) {
  //int i = execNum -1;
  int i =  fsm->m_execIndex;
  appExec = fsm->execList[i];
  if (!appExec) {
    //continue;
  }
  else
  {
    fsm->vdtList[0]->generateAudioRcvInfo(fsm->rInfo.audioPort, msgStr);
    //fsm->vdt->generateAudioRcvInfo(appExec->renderNodeIP, fsm->rInfo.audioPort, msgStr);
    //SAGE_PRINTLOG("initaudio --> %s\n", msgStr);
    char conMessage[TOKEN_LEN];
    sprintf(conMessage, "%s %d", msgStr, i);
    if (fsm->sendMessage(appExec->sailClient, SAIL_CONNECT_TO_ARCV, conMessage) < 0) {
      SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", appExec->appName, i);
      clearAppInstance(i);
    }
  }

  return 0;
}

void fsCore::clearAppInstance(int id)
{
  int index;
  appInExec* app = findApp(id, index);
  if(!app)
  {
    SAGE_PRINTLOG("fsCore/clearAppInstance : %d app instance is not found", id);
    return;
  }

  // clear app instance on display nodes
  fsm->sendToAllRcvs(RCV_SHUTDOWN_APP, id);

  // clear app instance on audio nodes
  if (app->audioOn) {
    std::vector<int> arcvList = fsm->vdtList[0]->getAudioRcvClientList();
    int arcvNum = arcvList.size();
    for(int i=0; i<arcvNum; i++)
      fsm->sendMessage(arcvList[i], ARCV_SHUTDOWN_APP, id);
  }

  // clear app instace on ui clients
  int uiNum = fsm->uiList.size();
  for (int j=0; j<uiNum; j++) {
    if (fsm->uiList[j] < 0)
      continue;

    if (fsm->sendMessage(fsm->uiList[j], UI_APP_SHUTDOWN, id) < 0) {
      SAGE_PRINTLOG("fsCore : uiClient(%d) is stuck or shutdown", j);
      fsm->uiList[j] = -1;
    }
  }

  // Re-GENERATING ID (1/2)
  //fsm->m_execIDList[id] = 0;

  // release data structure
  fsm->execList.erase(fsm->execList.begin() + index);
  delete app;
  app = NULL;

  displayInstance* temp_disp = (displayInstance*) fsm->dispList[index];
  if (temp_disp) {
    fsm->dispList.erase(fsm->dispList.begin() + index);
    delete temp_disp;
    temp_disp = NULL;
  }

}


int fsCore::getAvailableInstID(void)
{
  for(int i=0; i<MAX_INST_NUM; i++)
  {
    if (fsm->m_execIDList[i] == 0)
      return i;
  }
  return -1;
}

appInExec* fsCore::findApp(int id, int& index)
{
  std::vector<appInExec*>::iterator iter_exec;
  appInExec* temp_exec = NULL;
  index = 0;
  for(iter_exec = fsm->execList.begin(); iter_exec != fsm->execList.end(); iter_exec++, index++)
  {
    if ((*iter_exec)->fsInstID == id)
    {
      temp_exec = (appInExec*) *iter_exec;
      break;
    }
  }
  return temp_exec;
}


appInExec* fsCore::findAppBySailID(int id, int& index)
{
  std::vector<appInExec*>::iterator iter_exec;
  appInExec* temp_exec = NULL;
  index = 0;
  for(iter_exec = fsm->execList.begin(); iter_exec != fsm->execList.end(); iter_exec++, index++)
  {
    if ((*iter_exec)->sailClient == id)
    {
      temp_exec = (appInExec*) *iter_exec;
      break;
    }
  }
  return temp_exec;
}


int fsCore::parseMessage(sageMessage &msg, int clientID)
{
  appInExec *app;
  displayInstance *disp;
  char *dataStr;
  char token[TOKEN_LEN];
  int tokenNum;

  if (msg.getData())
    dataStr = (char *)msg.getData();


  switch(msg.getCode()) {
  case REG_APP : {
    app = new appInExec;
    memset(app->launcherID, 0, SAGE_NAME_LEN);
    sscanf((char *)msg.getData(), "%s %d %d %d %d %d %s %d %d %d %d %d %d %s", app->appName, &app->x, &app->y,
           &app->width, &app->height, &app->bandWidth, app->renderNodeIP, &app->imageWidth,
           &app->imageHeight, (int *)&app->audioOn, (int *)&app->protocol, &app->frameRate, &app->instID, app->launcherID);

    // instID is assigned by the appLauncher... has nothing to do with winID
    // BUT!!! SageBridge also sets it to < 0... this way we know an app is run
    // through sageBridge

    app->sailClient = clientID;

    // Re-GENERATING ID (2/2)
    //fsm->m_execIndex = getAvailableInstID();
    //fsm->m_execIDList[fsm->m_execIndex] = 1;
    app->fsInstID  = fsm->m_execIndex;
    //SAGE_PRINTLOG("\nREG_APP: clientID %d, winID %d, instID %d", clientID, app->fsInstID, app->instID);

    char sailInitMsg[TOKEN_LEN];
    memset(sailInitMsg, 0, TOKEN_LEN);
    sprintf(sailInitMsg, "%d %d %d %d", fsm->m_execIndex, fsm->nwInfo->rcvBufSize,
            fsm->nwInfo->sendBufSize, fsm->nwInfo->mtuSize);

    if (fsm->sendMessage(clientID, SAIL_INIT_MSG, sailInitMsg) < 0) {
      SAGE_PRINTLOG("fsCore : %s is stuck or shutdown", app->appName);
    }
    else
      fsm->execList.push_back(app);


    if (fsm->NRM) {
      char rcvIP[SAGE_IP_LEN];
      fsm->vdtList[0]->getNodeIPs(0, rcvIP);
      char msgStr[TOKEN_LEN];
      memset(msgStr, 0, TOKEN_LEN);
      sprintf(msgStr, "%s %s %d %d", app->renderNodeIP, rcvIP, app->bandWidth, fsm->m_execIndex);

      int uiNum = fsm->uiList.size();
      for (int j=0; j<uiNum; j++) {
        if (fsm->uiList[j] < 0)
          continue;

        if (fsm->sendMessage(fsm->uiList[j], REQUEST_BANDWIDTH, msgStr) < 0) {
          SAGE_PRINTLOG("fsCore : uiClient(%d) is stuck or shutdown", j);
          fsm->uiList[j] = -1;
        }
      }
    }
    else {
      initDisp(app);
      if (app->audioOn) {
        std::cout << "initAudio is called" << std::endl;
        initAudio();
      }
      //windowChanged(fsm->execList.size()-1);
      //bringToFront(fsm->execList.size()-1);
      //SAGE_PRINTLOG("[fsCore::parseMessage] inst id : %d", app->fsInstID);
      fsm->m_execIndex++;
    }

    break;
  }

  case NOTIFY_APP_SHUTDOWN : {

    int winID, index;
    sscanf((char *)msg.getData(), "%d", &winID);
    app = findApp(winID, index);

    if (!app)
    {
      SAGE_PRINTLOG("fsCore/NOTIFY_APP_SHUTDOWN : app %d  doesn't exist", winID);
      break;
    }

    clearAppInstance(winID);
    break;
  }

  case NETWORK_RESERVED : {

    if (fsm->NRM) {
      int winID = fsm->execList.size()-1;
      app = fsm->execList[winID];

      int success = atoi(dataStr);
      if (success) {
        initDisp(app);
        if (app->audioOn)
          initAudio();
        windowChanged(fsm->execList.size()-1);
        bringToFront(fsm->execList.size()-1);
      }
      else {
        if (fsm->sendMessage(app->sailClient, APP_QUIT) < 0) {
          SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", app->appName, winID);
          clearAppInstance(winID);
        }

        fsm->execList.pop_back();
      }
    }
    break;
  }

  case REG_GRCV : {

    int nodeID, dispID;
    sscanf((char *)msg.getData(), "%d %d", &nodeID, &dispID);

    // store client ID of receivers
    fsm->vdtList[dispID]->regRcv(clientID, nodeID);

    char info[TOKEN_LEN];
    // get the tile config info of display node
    fsm->vdtList[dispID]->getRcvInfo(nodeID, info);
    int streamPort = fsm->vdtList[dispID]->getLocalPort(nodeID);
    if (streamPort < 1024)
      streamPort = fsm->rInfo.streamPort;
    else
      fsm->useLocalPort = true;

    char msgStr[TOKEN_LEN];
    memset(msgStr, 0, TOKEN_LEN);
    sprintf(msgStr, "%d %d %d %d %d %d %d %s", fsm->nwInfo->rcvBufSize,
            fsm->nwInfo->sendBufSize, fsm->nwInfo->mtuSize,
            streamPort, fsm->rInfo.bufSize, fsm->rInfo.fullScreen,
            fsm->vdtList[dispID]->getNodeNum(), info);

    if (fsm->sendMessage(clientID, RCV_INIT, msgStr) < 0) {
      SAGE_PRINTLOG("fsCore : displaynode(%d) doesn't respond", nodeID);
    }
    else {
      numReportedReceivers++;

      if(fsm->getNumStartedReceivers() <= numReportedReceivers) {
        fsm->startConnManagerMsgThread();
        fsm->startUiServer();
      }
    }
    break;
  }

  case REG_ARCV : {
    int nodeID;
    sscanf((char *)msg.getData(), "%d", &nodeID);


    // store client ID of receivers
    fsm->vdtList[0]->regAudioRcv(clientID, nodeID);

    char info[TOKEN_LEN];
    // get the tile config info of display node
    fsm->vdtList[0]->getAudioRcvInfo(nodeID, info);
    char msgStr[TOKEN_LEN];
    sprintf(msgStr, "%d %d %d %d %d %d %d %d %s", fsm->nwInfo->rcvBufSize,
            fsm->nwInfo->sendBufSize, fsm->nwInfo->mtuSize,   fsm->rInfo.audioSyncPort,
            fsm->rInfo.audioPort, fsm->rInfo.agSyncPort, fsm->rInfo.bufSize, fsm->vdtList[0]->getNodeNum(), info);

    //cout << " ----> fsCore : " << msgStr << endl;
    fsm->sendMessage(clientID, ARCV_AUDIO_INIT, msgStr);
    break;
  }

    /*case SYNC_INIT_ARCV : {
    // find gStreamRcvs connected to this aStreamRcv
    getToken((char *)msg.getData(), token);
    int nodeID = atoi(token);
    //fsm->vdt->getgRcvs(nodeID);
    char info[TOKEN_LEN];
    fsm->vdt->getAudioNodeIPs(nodeID, info);

    // send message to gStreamRcvs

    // for testing
    std::vector<int> rcvList = fsm->vdt->getRcvClientList();
    int rcvNum = rcvList.size();
    for(int i=0; i<rcvNum; i++) {
    fsm->sendMessage(rcvList[i], RCV_SYNC_INIT, info);
    }

    break;
    }*/

  case SHUTDOWN_APP : {

    int winID, index;
    sscanf((char *)msg.getData(), "%d", &winID);

    app = findApp(winID, index);

    if (!app) {
      SAGE_PRINTLOG("fsCore/SHUTDOWN_APP : app %d doesn't exist", winID);
      break;
    }

    if (fsm->sendMessage(app->sailClient, APP_QUIT) < 0) {
      SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", app->appName, winID);
    }

    clearAppInstance(winID);

    break;
  }

  case SAGE_APP_SHARE : {

    int winID, fsPort, index;
    char fsIP[SAGE_IP_LEN];
    sscanf((char *)msg.getData(), "%d %s %d", &winID, &fsIP[0], &fsPort);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/SAGE_APP_SHARE : app "<< winID << " doesn't exist" << std::endl;
      break;
    }


    char msgData[TOKEN_LEN];
    memset(msgData, 0, TOKEN_LEN);
    sprintf(msgData, "%s %d", fsIP, fsPort);

    //std::cout << fsIP << ":" << fsPort << std::endl;

    if (fsm->sendMessage(app->sailClient, SAGE_APP_SHARE, msgData) < 0) {
      SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", app->appName, winID);
      clearAppInstance(winID);
    }

    break;
  }

  case MOVE_WINDOW : {

    sageRect devRect;
    int winID, index;
    float x, y;
    sscanf((char *)msg.getData(), "%d %f %f", &winID, &x, &y);
    devRect.x = int(x);
    devRect.y = int(y);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/MOVE_WINDOW : app "<< winID << " doesn't exist" << std::endl;
      break;
    }

    //fsm->dispList[winID]->modifyStream();

    startTime = sage::getTime();
    if (fsm->winStep > 0)
      winSteps = fsm->winStep;

    if (fsm->dispList[index]->changeWindow(devRect, winSteps) < 0)
      clearAppInstance(winID);
    else
      windowChanged(winID, clientID == fsm->dim);

    break;
  }

  case ADD_OBJECT : {
    if (!msg.getData())
      break;

    char objectType[SAGE_NAME_LEN];
    int widgetID, winID, index, uniqueWidgetID;

    sscanf(dataStr, "%s %d %d", objectType, &widgetID, &winID);

    //if (winID > -1)
    //SAGE_PRINTLOG("\n\n----- Trying to add object for clientID %d: widgetID %d,  winID %d ", clientID, widgetID, winID);
    // if app was started through sageBridge, the windowIDs won't match so
    // match the sageBridge's "instID" to FSMs "winID"... through sailID
    /*if (winID > -1 && widgetID > -1) {
      app = findAppBySailID(clientID, index);
      if (!app) {
      SAGE_PRINTLOG("\nFsCore/ADD_OBJECT : app %d doesn't exist", winID);
      break;
      }

      // this means that its actually from sageBridge (instID is sageBridge's "instNum"*(-1))
      // so we have to match it to the FSM's winID
      if (app->instID <= 0)
      winID = app->fsInstID;
      }*/

    //SAGE_PRINTLOG("ADD_OBJECT for window id %d", winID);


    // in order to match widgetID/winID combo to objID, we
    // do this simple math and as long as the number of apps
    // and number of widgets per app is < 1000, it should be unique
    if (widgetID > -1)
      uniqueWidgetID = 1000000 + winID*1000000 + widgetID;
    else
      uniqueWidgetID = widgetID;

    // create an objectID for it... basically an index into drawObjectList
    // also add widgetID (only used by app requested widgets)
    int objectID = fsm->drawObjectList.size();
    fsm->drawObjectList[objectID] = uniqueWidgetID;
    if (widgetID > -1)   // reverse mapping for app-requested widgets
      fsm->appDrawObjectList[uniqueWidgetID] = objectID;


    // attach the newly created objectID in front and the modified winID
    char * msgStr = new char[msg.getSize() + 20];
    sprintf(msgStr, "%d %s %d %d\n%s", objectID, objectType, widgetID, winID, sage::tokenSeek(dataStr, 3));


    // send the message to display nodes
    fsm->sendToAllRcvs(ADD_OBJECT, msgStr);


    // inform DIM of new object (strip the image data
    // because it's usually big and unnecessary for dim)
    if(fsm->dim >= 0) {
      std::string m (msgStr);
      int begin = m.find("<graphics>");
      int end = m.find("</graphics>");
      if (begin!=std::string::npos && end!=std::string::npos) {
        m.erase(begin, end-begin+strlen("</graphics>"));
        sprintf(msgStr, "%s", m.c_str());
      }
      fsm->sendMessage(fsm->dim, UI_OBJECT_INFO, msgStr);
    }
    delete [] msgStr;

    break;
  }

  case MOVE_OBJECT : {
    if (!msg.getData())
      break;

    /*int id, dx, dy;
      sscanf(dataStr, "%d %d %d", &id, &dx, &dy);

      if (id >= fsm->drawObjectList.size())
      break;

      sageRect *obj = fsm->drawObjectList[id];
      obj->x += dx;
      obj->y += dy;

      char msgStr[TOKEN_LEN];
      sprintf(msgStr, "%d %d %d", id, obj->x, obj->y);
      fsm->sendToAllRcvs(UPDATE_OBJECT_POSITION, msgStr);*/

    break;
  }

  case REMOVE_OBJECT : {
    if (!msg.getData())
      break;

    int id, winID, objectID;
    sscanf(dataStr, "%d", &id);

    // if this request came from the app, convert the app-specific
    // widgetId to the objectId... otherwise the widgetID passed in was actually the objectID
    if (clientID != fsm->dim) {
      sscanf(dataStr, "%d %d", &id, &winID);
      id = widgetToObjID(id, winID);
    }

    objectID = id;

    if (id >= fsm->drawObjectList.size()) {
      std::cout << "fsCore: REMOVE_OBJECT - invalid object ID " << id << std::endl;
      break;
    }

    fsm->sendToAllRcvs(REMOVE_OBJECT, objectID);
    if (fsm->dim >=0)
      fsm->sendMessage(fsm->dim, UI_OBJECT_REMOVED, objectID);
    break;
  }

  case OBJECT_MESSAGE : {
    if (!msg.getData())
      break;

    int id, winID;
    int sszz = msg.getSize() + 20;
    char* msgStr = new char[sszz];

    sscanf(dataStr, "%d", &id);

    // if this request came from the app, convert the app-specific
    // widgetID to the unique objectId
    if (clientID != fsm->dim) {
      sscanf(dataStr, "%d %d", &id, &winID);
      id = widgetToObjID(id, winID);
      sprintf(msgStr, "%d %s", id, sage::tokenSeek(dataStr, 2));
    }
    else
      sprintf(msgStr, "%d %s", id, sage::tokenSeek(dataStr, 1));

    if (id >= fsm->drawObjectList.size()) {
      std::cout << "fsCore: OBJECT_MESSAGE - invalid object ID " << id << std::endl;
      break;
    }
    //SAGE_PRINTLOG( "\nObject message: %s", msgStr);
    fsm->sendToAllRcvs(OBJECT_MESSAGE, msgStr);
    if (fsm->dim >= 0 && clientID != fsm->dim)
      fsm->sendMessage(fsm->dim, APP_OBJECT_CHANGE, msgStr);

    break;
  }

  case SHOW_OBJECT : {
    if (!msg.getData())
      break;

    int id;
    sscanf(dataStr, "%d", &id);

    if (id >= fsm->drawObjectList.size()) {
      std::cout << "fsCore: SHOW_OBJECT - invalid object ID " << id << std::endl;
      break;
    }

    fsm->sendToAllRcvs(SHOW_OBJECT, dataStr);

    break;
  }

  case SAVE_SCREENSHOT : {
    if (!msg.getData())
      break;
    fsm->sendToAllRcvs(SAVE_SCREENSHOT, (char *)msg.getData());
    break;
  }

  case FS_TIME_MSG : {
    double endTime = sage::getTime();
    double bigTime = floor(endTime/1000000.0)*1000000.0;
    endTime = (endTime - bigTime)/1000.0;
    double sageLatency = (endTime - startTime)/1000.0;
    //std::cout << "total latency = " << sageLatency << "ms" << std::endl;
    std::cout << "Time Block received at " << endTime << "ms" << std::endl;

    /*
      double endTime = sageGetTime();
      double winTime = (endTime - startTime)/1000.0;
      double singleLatency = winTime / winSteps;

      //std::cout << winTime << std::endl;

      //std::cout << "total time = " << winTime << "ms for " << winSteps << " move/resizing" << std::endl;
      //std::cout << "single move/resizing time = " << singleLatency << "ms" << std::endl;
      winSteps = (int)(ceil)(fsm->winTime /singleLatency);
      //std::cout << "# of window change steps changed to " << winSteps << std::endl;
      */

    break;
  }

  case RESIZE_WINDOW : {

    int winID, index, left, right, bottom, top;
    sscanf((char *)msg.getData(), "%d %d %d %d %d", &winID, &left, &right, &bottom, &top);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/RESIZE_WINDOW : app "<< winID << " doesn't exist" << std::endl;
      break;
    }

    sageRect devRect;
    devRect.x = left - app->x;
    devRect.width = right - (app->width+app->x) - devRect.x;
    devRect.y = bottom - app->y;
    devRect.height = top - (app->y+app->height) - devRect.y;

    startTime = sage::getTime();

    if (fsm->winStep > 0)
      winSteps = fsm->winStep;

    if (fsm->dispList[index]->changeWindow(devRect, winSteps) < 0)
      clearAppInstance(winID);
    else
      windowChanged(winID, clientID==fsm->dim);

    break;
  }

  case ROTATE_WINDOW : {
    rotateWindow((char *)msg.getData());
    break;
  }

  case SAGE_UI_REG : {
    // DIM sends a string "dim" because it needs more messages than sageui
    // so it's also stored in a separate variable to receive those messages
    if (msg.getData())
      if (strcmp("dim",(char *)msg.getData()) == 0)
        fsm->dim = clientID;  // this is dim so store it separately

    fsm->uiList.push_back(clientID);
    sendDisplayInfo(clientID);
    sendSageStatus(clientID);
    sendAppInfo(clientID);
    break;
  }

  case APP_UI_REG : {
    fsm->appUiList.push_back(clientID);
    tokenNum = getToken((char *)msg.getData(), token);

    if (strlen(token) > 0) {
      sendAppStatus(clientID, token);
    }
    else {
      std::cout << "More arguments are needed for this command " << std::endl;
    }
    break;
  }

  case APP_FRAME_RATE : {

    /*
      tokenNum = getToken((char *)msg.getData(), token);
      int winID = atoi(token);
      int index;
    */
    int winID, index;
    char msgData[TOKEN_LEN];
    sscanf((char *)msg.getData(), "%d %s", &winID, msgData);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/APP_FRAME_RATE : app "<< winID << " doesn't exist" << std::endl;
      break;
    }

    //getToken((char *)msg.getData(), token);
    //SAGE_PRINTLOG( "\n\n\nSCHANGING APP FRAMERATE to: %s\n\n", msgData);
    if (fsm->sendMessage(app->sailClient, SAIL_FRAME_RATE, msgData) < 0) {
      SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", app->appName, winID);
      clearAppInstance(winID);
    }

    break;
  }

  case PERF_INFO_REQ : {
    /*
      tokenNum = getToken((char *)msg.getData(), token);
      int winID = atoi(token);
      int index;
    */

    int winID, sendingRate, index;
    sscanf((char *)msg.getData(), "%d %d", &winID, &sendingRate);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/PERF_INFO_REQ : app "<< winID << " doesn't exist" << std::endl;
      break;
    }
    disp = fsm->dispList[index];

    /*         int sendingRate;

               if (tokenNum < 1) {
               std::cout << "More arguments are needed for this command " << std::endl;
               break;
               }

               tokenNum = getToken((char *)msg.getData(), token);
               sendingRate = atoi(token);
    */

    if (disp->requestPerformanceInfo(sendingRate) < 0)
      clearAppInstance(winID);

    break;
  }

  case STOP_PERF_INFO : {
    /*
      tokenNum = getToken((char *)msg.getData(), token);
      int winID = atoi(token);
      int index;
    */
    int winID, index;
    sscanf((char *)msg.getData(), "%d", &winID);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/STOP_PERF_INFO : app "<< winID << " doesn't exist" << std::endl;
      break;
    }
    disp = fsm->dispList[index];

    if (disp->stopPerformanceInfo() < 0)
      clearAppInstance(winID);
    break;
  }

  case SAGE_BG_COLOR : {

    int red, green, blue;
    sscanf((char *)msg.getData(), "%d %d %d", &red, &green, &blue);

    for (int i=0; i<fsm->vdtList.size(); i++)
      fsm->vdtList[i]->changeBGColor(red, green, blue);

    break;
  }

  case UPDATE_WIN_PROP : {

    int winID, index;
    sscanf((char *)msg.getData(), "%d", &winID);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/UPDATE_WIN_PROP : app "<< winID << " doesn't exist" << std::endl;
      break;
    }

    //disp->updateWinProp((char *)msg.getData());
    windowChanged(winID, clientID==fsm->dim);
    break;
  }

  case BRING_TO_FRONT : {
    int winID;
    sscanf((char *)msg.getData(), "%d", &winID);

    bringToFront(winID);

    break;
  }

  case PUSH_TO_BACK : {
    int winID;
    sscanf((char *)msg.getData(), "%d", &winID);

    pushToBack(winID);

    break;
  }

  case SAGE_FLIP_WINDOW : {
    int winID, index;
    sscanf((char *)msg.getData(), "%d", &winID);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/SAGE_FLIP_WINDOW : app "<< winID << " doesn't exist" << std::endl;
      break;
    }

    if (fsm->sendMessage(app->sailClient, SAIL_FLIP_WINDOW) < 0) {
      SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", app->appName, winID);
      clearAppInstance(winID);
    }

    break;
  }

  case SAGE_CHECK_LATENCY : {
    int winID, index;
    sscanf((char *)msg.getData(), "%d", &winID);

    app = findApp(winID, index);

    if (!app) {
      std::cout << "FsCore/SAGE_CHECK_LATENCY : app "<< winID << " doesn't exist" << std::endl;
      break;
    }

    if (fsm->sendMessage(app->sailClient, SAIL_SEND_TIME_BLOCK) < 0) {
      SAGE_PRINTLOG("fsCore : %s(%d) is stuck or shutdown", app->appName, winID);
      clearAppInstance(winID);
    }
    startTime = sage::getTime();

    break;
  }

  case SAGE_Z_VALUE : {
    fsm->sendToAllRcvs(RCV_CHANGE_DEPTH, dataStr);

    int uiNum = fsm->uiList.size();

    for (int j=0; j<uiNum; j++) {
      if (fsm->uiList[j] < 0)
        continue;

      if (fsm->sendMessage(fsm->uiList[j], Z_VALUE_RETURN, dataStr) < 0) {
        SAGE_PRINTLOG("fsCore : uiClient(%d) is stuck or shutdown", j);
        fsm->uiList[j] = -1;
      }
    }


    int numOfChange, winID, zValue;
    int index=0;
    char seps[] = " ,\t\n";
    char *token;

    token = strtok(dataStr, seps);
    numOfChange = atoi(token);

    while(token=strtok(NULL, seps)) {
      winID = atoi(token);
      zValue = atoi(strtok(NULL, seps));

      app = findApp(winID, index);
      if(!app) continue;

      fsm->dispList[index]->setZValue(zValue);
    }

    break;
  }

  case SAGE_ADMIN_CHECK : {
    int execNum = fsm->execList.size();

    for (int i=0; i<execNum; i++) {
      app = fsm->execList[i];
      if (!app)
        continue;

      displayInstance *disp = fsm->dispList[i];

      int left = app->x;
      int bottom = app->y;
      int right = app->x + app->width;
      int top = app->y + app->height;

      int dispNodes = 0; //disp->getReceiverNum();
      int renderNodes = 0;//app->nodeNum;
      int numStream = 0; //disp->getStreamNum();

      char msgStr[512];
      memset(msgStr, 0, 512);
      sprintf(msgStr, "App Name : %s\n   App Instance ID : %d\n",
              app->appName,  app->fsInstID);
      sprintf(token, "   position (left, right, bottom, top) : ( %d , %d , %d , %d )\n",
              left, right, bottom, top);
      strcat(msgStr, token);
      sprintf(token, "   Z-Value : %d\n", disp->getZValue());
      strcat(msgStr, token);
      sprintf(token, "   %d Rendering Nodes    %d Display Nodes    %d Streams\n",
              renderNodes, dispNodes, numStream );
      strcat(msgStr, token);

      fsm->sendMessage(clientID, UI_ADMIN_INFO, msgStr);
    }

    break;
  }

  case SAGE_SHUTDOWN : {
    int execNum = fsm->execList.size();

    for (int i=0; i<execNum; i++) {
      app = fsm->execList[i];
      if (!app)
        continue;

      fsm->sendMessage(app->sailClient, APP_QUIT);
    }

    fsm->sendToAllRcvs(SHUTDOWN_RECEIVERS);

    std::vector<int> arcvList = fsm->vdtList[0]->getAudioRcvClientList();
    int arcvNum = arcvList.size();
    for(int i=0; i<arcvNum; i++)
      fsm->sendMessage(arcvList[i], SHUTDOWN_RECEIVERS);

    fsm->fsmClose = true;
    break;
  }
  }

  return 0;
}

int fsCore::widgetToObjID(int widgetID, int winID)
{
  int id = -1;
  // if winID > -1, this is a widget ID then and the request
  // came from the app itself
  if (winID != -1) {
    int uniqueWidgetID = winID*1000000 + widgetID + 1000000;
    if (fsm->appDrawObjectList.count(uniqueWidgetID) > 0)
      id = fsm->appDrawObjectList[uniqueWidgetID];
  }
  return id;
}

int fsCore::rotateWindow(char *msgStr)
{
  int winID, rotation;
  sscanf(msgStr, "%d %d", &winID, &rotation);

  appInExec *app = NULL;
  int index;
  app = findApp(winID, index);

  if (!app) {
    SAGE_PRINTLOG("fsCore::rotateWindow : app %d doesn't exist", winID);
    return -1;
  }


  switch(rotation) {
  case 90:
    app->rotate(CCW_90);
    break;
  case 180:
    app->rotate(CCW_180);
    break;
  case 270:
    app->rotate(CCW_270);
    break;
  case -90:
    app->rotate(CCW_270);
    break;
  case -180:
    app->rotate(CCW_180);
    break;
  case -270:
    app->rotate(CCW_90);
    break;
  default:
    SAGE_PRINTLOG("rotation commnad error : invalid rotation degree");
    break;
  }

  if (fsm->dispList[index]->modifyStream() < 0)
    clearAppInstance(winID);
  else
    windowChanged(winID);

  return 0;
}

int fsCore::windowChanged(int winID, bool dimOriginatedChange)
{
  // update the window borders on the display side
  appInExec *app;
  int index;
  app = findApp(winID, index);
  if(!app)
  {
    std::cout << "FsCore/windowChanged ::Invalid App ID " << winID << std::endl;
    return 1;
  }

  char appBoundsStr[TOKEN_LEN];
  memset(appBoundsStr, 0, TOKEN_LEN);
  sprintf(appBoundsStr, "%d %d %d %d %d %d",
          winID, app->x, app->y, app->width, app->height, 90 * (int)app->getOrientation());
  fsm->sendToAllRcvs(RCV_CHANGE_APP_BOUNDS, appBoundsStr);


  // update the UI clients with the new bounds of the app
  int uiNum = fsm->uiList.size();
  for (int j=0; j<uiNum; j++) {
    if (fsm->uiList[j] < 0)
      continue;

    if (sendAppInfo(winID, fsm->uiList[j], dimOriginatedChange) < 0) {
      SAGE_PRINTLOG("fsCore : uiClient(%d) is stuck or shutdown", j);
      fsm->uiList[j] = -1;
    }
  }

  return 0;
}

int fsCore::sendSageStatus(int clientID)
{
  /*
    int numApps = fsm->appDataList.size();

    char statusStr[SAGE_MESSAGE_SIZE], token[TOKEN_LEN];

    memset(statusStr, 0, SAGE_MESSAGE_SIZE);
    sprintf(statusStr, "%d\n", numApps);

    for (int i=0; i<numApps; i++) {
    appDataNode *dataNode = fsm->appDataList[i];
    int instNum = dataNode->instList.size();
    int execNum = dataNode->execs.size();

    sprintf(token, "%s %d %d ", dataNode->app, execNum, instNum);
    strcat(statusStr, token);

    for (int j=0; j<instNum; j++) {
    sprintf(token, "%d ", dataNode->instList[j]);
    strcat(statusStr, token);
    }
    strcat(statusStr, "\n");
    }

    fsm->sendMessage(clientID, SAGE_STATUS, statusStr);

    memset(statusStr, 0, SAGE_MESSAGE_SIZE);

    for (int i=0; i<numApps; i++) {
    appDataNode *dataNode = fsm->appDataList[i];
    int execNum = dataNode->execs.size();

    sprintf(statusStr, "%s\n", dataNode->app);

    for(int j=0; j<execNum; j++) {
    sprintf(token,  "config %s : nodeNum=%d initX=%d initY=%d dimX=%d dimY=%d\n",
    dataNode->execs[j].name, dataNode->execs[j].nodeNum, dataNode->execs[j].posX,
    dataNode->execs[j].posY, dataNode->execs[j].dimX,
    dataNode->execs[j].dimY);
    strcat(statusStr, token);

    int cmdNum = dataNode->execs[j].cmdList.size();
    for (int k=0; k<cmdNum; k++) {
    sprintf(token, "exec %s %s\n", dataNode->execs[j].cmdList[k].ip,
    dataNode->execs[j].cmdList[k].cmd);
    strcat(statusStr, token);
    }
    sprintf(token, "nwProtocol=%s\n", dataNode->execs[j].nwProtocol);
    strcat(statusStr, token);
    sprintf(token, "bufNum=%d\n", dataNode->execs[j].bufNum);
    strcat(statusStr, token);
    sprintf(token, "syncMode=%d\n", dataNode->execs[j].syncMode);
    strcat(statusStr, token);
    }

    fsm->sendMessage(clientID, APP_EXEC_CONFIG, statusStr);
    }
  */
  return 0;
}

int fsCore::sendDisplayInfo(int clientID)
{
  char displayStr[STRBUF_SIZE];
  memset(displayStr, 0, STRBUF_SIZE);
  displayStr[0] = '\0';

  for (int i=0; i<fsm->vdtList.size(); i++) {
    int tileNum = fsm->vdtList[i]->getTileNum();
    int dimX = fsm->vdtList[i]->dimX;
    int dimY = fsm->vdtList[i]->dimY;
    int vdtWidth = fsm->vdtList[i]->width;
    int vdtHeight = fsm->vdtList[i]->height;
    int tileWidth = fsm->vdtList[i]->tileList[0]->width;
    int tileHeight = fsm->vdtList[i]->tileList[0]->height;
    int id = fsm->vdtList[i]->displayID;

    char info[STRBUF_SIZE];
    memset(info, 0, STRBUF_SIZE);
    sprintf(info, "%d %d %d\n%d %d\n%d %d %d\n", tileNum, dimX, dimY, vdtWidth, vdtHeight,
            tileWidth, tileHeight, id);
    strcat(displayStr, info);
  }

  fsm->sendMessage(clientID, SAGE_DISPLAY_INFO, displayStr);

  memset(displayStr, 0, STRBUF_SIZE);
  displayStr[0] = '\0';

  for (int i=0; i<fsm->dispConnectionList.size(); i++) {
    int disp0 = fsm->dispConnectionList[i]->displays[0]->displayID;
    int disp1 = fsm->dispConnectionList[i]->displays[1]->displayID;
    int edge0 = fsm->dispConnectionList[i]->edges[0];
    int edge1 = fsm->dispConnectionList[i]->edges[1];
    int offset = fsm->dispConnectionList[i]->offset;

    char info[STRBUF_SIZE];
    memset(info, 0, STRBUF_SIZE);
    sprintf(info, "%d %d %d %d %d\n", disp0, edge0, disp1, edge1, offset);
    strcat(displayStr, info);
  }

  if (fsm->dispConnectionList.size() > 0)
    fsm->sendMessage(clientID, DISP_CONNECTION_INFO, displayStr);

  return 0;
}

int fsCore::sendAppInfo(int winID, int clientID, bool dimOriginatedChange)
{
  appInExec *app;
  int index;
  app = findApp(winID, index);
  if(!app)
  {
    std::cout << "FsCore/sendAppInfo ::Invalid App ID " << winID << std::endl;
    return 1;
  }

  int zValue = 0;
  char appInfoStr[TOKEN_LEN];
  char dimAppInfoStr[TOKEN_LEN];  // when messages are sent to DIM
  zValue = fsm->dispList[index]->getZValue();

  int left = app->x;
  int bottom = app->y;
  int right = app->x + app->width;
  int top = app->y + app->height;
  int sailID = app->sailClient;
  int degree = 90 * (int)app->getOrientation();

  memset(appInfoStr, 0, TOKEN_LEN);
  /*sprintf(appInfoStr, "%s %d %d %d %d %d %d %d %d %d %d %s %s", app->appName, winID,
    left, right, bottom, top, sailID, zValue, degree, app->displayID,
    app->instID, app->launcherID, fsm->dispList[index]->winTitle);*/

  if (dimOriginatedChange)
    sprintf(appInfoStr, "%s %d %d %d %d %d %d %d %d %d %d %s %d %d 1", app->appName, winID,
            left, right, bottom, top, sailID, zValue, degree, app->displayID,
            app->instID, app->launcherID, app->imageWidth, app->imageHeight);
  else
    sprintf(appInfoStr, "%s %d %d %d %d %d %d %d %d %d %d %s %d %d 0", app->appName, winID,
            left, right, bottom, top, sailID, zValue, degree, app->displayID,
            app->instID, app->launcherID, app->imageWidth, app->imageHeight);


  return fsm->sendMessage(clientID, APP_INFO_RETURN, appInfoStr);
}

int fsCore::sendAppInfo(int clientID)
{

  int appInstNum = fsm->execList.size();
  appInExec *app  = NULL;
  int retVal = 0;
  for (int i=0; i<appInstNum; i++)
  {
    app = (appInExec*) fsm->execList[i];
    if (!app) continue;
    if (sendAppInfo(app->fsInstID, clientID) < 0)
      retVal = 1;
  }

  return retVal;
}

int fsCore::sendAppStatus(int clientID, char *appName)
{
  int appInstNum = fsm->execList.size();
  appInExec *app;
  int instNum = 0;
  char statusMsg[TOKEN_LEN];
  statusMsg[0] = '\0';

  for (int i=0; i<appInstNum; i++) {
    app = fsm->execList[i];
    if (!app)
      return -1;
    if (strcmp(app->appName, appName) == 0) {
      instNum++;
      char instStr[TOKEN_LEN];
      sprintf(instStr, " %d %d", app->fsInstID, app->sailClient);
      strcat(statusMsg, instStr);
    }
  }

  char msgStr[TOKEN_LEN];
  memset(msgStr, 0, TOKEN_LEN);
  sprintf(msgStr, "%d%s", instNum, statusMsg);
  fsm->sendMessage(clientID, APP_STATUS, msgStr);

  return 0;
}

int fsCore::bringToFront(int winID)
{
  appInExec *app;

  int execNum = fsm->execList.size();
  int numOfWin = 0;
  char depthStr[TOKEN_LEN];
  depthStr[0] = '\0';

  for (int i=0; i<execNum; i++) {
    app = fsm->execList[i];
    if (!app)
      continue;

    displayInstance *disp = fsm->dispList[i];
    if (disp->winID != winID)
      disp->increaseZ();
    else
      disp->setZValue(0);

    numOfWin++;
    char zVal[TOKEN_LEN];
    sprintf(zVal, "%d %d ", disp->winID, disp->getZValue());
    strcat(depthStr, zVal);
  }

  char msgStr[TOKEN_LEN];
  memset(msgStr, 0, TOKEN_LEN);
  sprintf(msgStr, "%d %s", numOfWin, depthStr);

  fsm->sendToAllRcvs(RCV_CHANGE_DEPTH, msgStr);


  int uiNum = fsm->uiList.size();

  for (int j=0; j<uiNum; j++) {
    if (fsm->uiList[j] < 0)
      continue;

    if (fsm->sendMessage(fsm->uiList[j], Z_VALUE_RETURN, msgStr) < 0) {
      SAGE_PRINTLOG("fsCore : uiClient(%d) is stuck or shutdown", j);
      fsm->uiList[j] = -1;
    }
  }

  return 0;
}


int fsCore::pushToBack(int winID)
{
  appInExec *app;

  int execNum = fsm->execList.size();
  int numOfWin = 0;
  char depthStr[TOKEN_LEN];
  depthStr[0] = '\0';

  displayInstance *toBack = NULL;

  int max = -1;

  for (int i=0; i<execNum; i++) {
    app = fsm->execList[i];
    if (!app)
      continue;

    displayInstance *disp = fsm->dispList[i];

    if (max < disp->getZValue())
      max = disp->getZValue();

    if (disp->winID == winID)
      toBack = disp;
  }

  if (toBack) {
    numOfWin++;
    toBack->setZValue(max+1);
    char zVal[TOKEN_LEN];
    sprintf(zVal, "%d %d ", winID, max+1);
    strcat(depthStr, zVal);
  }

  // distributed the new z order to displays and UIs
  char msgStr[TOKEN_LEN];
  memset(msgStr, 0, TOKEN_LEN);
  sprintf(msgStr, "%d %s", numOfWin, depthStr);

  fsm->sendToAllRcvs(RCV_CHANGE_DEPTH, msgStr);

  int uiNum = fsm->uiList.size();

  for (int j=0; j<uiNum; j++) {
    if (fsm->uiList[j] < 0)
      continue;

    if (fsm->sendMessage(fsm->uiList[j], Z_VALUE_RETURN, msgStr) < 0) {
      SAGE_PRINTLOG("fsCore : uiClient(%d) is stuck or shutdown", j);
      fsm->uiList[j] = -1;
    }
  }

  return 0;
}
