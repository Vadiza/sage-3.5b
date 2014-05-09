/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: monitorPerf.cpp - monitoring performance data of SAGE
 * Author : Luc Renambot, Byungil Jeong
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

#include "suil.h"
#include "misc.h"
#include <pthread.h>

char *gr_server;
QUANTAnet_tcpClient_c *draw[20];
int timestep = 1;


void* readThread(void *args)
{
  int status, dataSize;
  char buffer[65];

  suil *uiLib = (suil *)args;

  while(1) {
    sageMessage msg;

    msg.init(READ_BUF_SIZE);
    uiLib->rcvMessageBlk(msg);

    if (msg.getCode() == APP_INFO_RETURN)
    {
      char appname[256], cmd[256];
      int ret;
      int appID, left, right, bottom, top, sailID, zValue;

      ret = sscanf((char*)msg.getData(), "%s %d %d %d %d %d %d %d",
                   appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);

      if (ret != 8)
        SAGE_PRINTLOG("Something wrong, got only %d fields\n", ret);
      else
      {
        if (draw[appID*2] == NULL)
        {
          SAGE_PRINTLOG("New application: %d %s\n", appID, appname);

          memset(cmd, 0, 256);
          sprintf(cmd, "%d", appID);
          uiLib->sendMessage(STOP_PERF_INFO, cmd);

          memset(cmd, 0, 256);
          sprintf(cmd, "%d %d", appID, 1); // every second
          uiLib->sendMessage(PERF_INFO_REQ, cmd);

          draw[appID*2] = new QUANTAnet_tcpClient_c;

          if (draw[appID*2]->connectToServer(gr_server, 50008) < 0)
          {
            SAGE_PRINTLOG("Cannot connecy to drawing server\n");
            exit(1);
          }

          memset(buffer, 0, 65);
          sprintf(buffer, "%64s", "lines");
          dataSize = 64;
          status = draw[appID*2]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error1: %d\n", status);

          memset(buffer, 0, 65);
          sprintf(buffer, "%37s - Display Bandwidth (Mbps)", appname);
          dataSize = 64;
          status = draw[appID*2]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error2: %d\n", status);


          memset(buffer, 0, 65);
          sprintf(buffer, "%64s", "Time (sec)");
          dataSize = 64;
          status = draw[appID*2]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error3: %d\n", status);


          memset(buffer, 0, 65);
          sprintf(buffer, "%6d", 1);
          dataSize = 6;
          status = draw[appID*2]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error4: %d\n", status);


          memset(buffer, 0, 65);
          sprintf(buffer, "%64s", "Display Bandwidth (Mbps)");
          dataSize = 64;
          status = draw[appID*2]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error5: %d\n", status);

          dataSize = 1;
          memset(buffer, 0, 65);
          status = draw[appID*2]->read(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

          if (status == QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Got [%s]\n", buffer);
          else
            SAGE_PRINTLOG("Error7: %d\n", status);


          draw[appID*2+1] = new QUANTAnet_tcpClient_c;

          if (draw[appID*2+1]->connectToServer(gr_server, 50008) < 0)
          {
            SAGE_PRINTLOG("Cannot connecy to drawing server\n");
            exit(1);
          }

          memset(buffer, 0, 65);
          sprintf(buffer, "%64s", "lines");
          dataSize = 64;
          status = draw[appID*2+1]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error1: %d\n", status);

          memset(buffer, 0, 65);
          sprintf(buffer, "%43s - Display Rate (fps)", appname);
          dataSize = 64;
          status = draw[appID*2+1]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error2: %d\n", status);


          memset(buffer, 0, 65);
          sprintf(buffer, "%64s", "Time (sec)");
          dataSize = 64;
          status = draw[appID*2+1]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error3: %d\n", status);


          memset(buffer, 0, 65);
          sprintf(buffer, "%6d", 1);
          dataSize = 6;
          status = draw[appID*2+1]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error4: %d\n", status);


          memset(buffer, 0, 65);
          sprintf(buffer, "%64s", "Display Rate (fps)");
          dataSize = 64;
          status = draw[appID*2+1]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
          if (status != QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Error6: %d\n", status);

          dataSize = 1;
          memset(buffer, 0, 65);
          status = draw[appID*2+1]->read(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

          if (status == QUANTAnet_tcpClient_c::OK)
            SAGE_PRINTLOG("Got [%s]\n", buffer);
          else
            SAGE_PRINTLOG("Error7: %d\n", status);
        }
      }
    }
    else if (msg.getCode() == UI_PERF_INFO)
    {
      char appname[256], cmd[256];
      int ret;
      int appID, dispNodes, numStreamD, renderNodes, numStreamR;
      float dispBandWidth, dispFrameRate, renderBandWidth, renderFrameRate;

      ret = sscanf((char*)msg.getData(), "%d\nDisplay %f %f %d %d\nRendering %f %f %d %d",
                   &appID,
                   &dispBandWidth, &dispFrameRate, &dispNodes, &numStreamD,
                   &renderBandWidth, &renderFrameRate, &renderNodes, &numStreamR);

      if (ret != 9)
        SAGE_PRINTLOG("Something wrong, got only %d fields\n", ret);
      else
      {
        //SAGE_PRINTLOG("displayBw %f displayFPS %f, renderBW %f, renderFPS %f\n",
        // dispBandWidth, dispFrameRate, renderBandWidth, renderFrameRate);

        memset(buffer, 0, 65);
        sprintf(buffer, "%12.6f %12.6f", (float)timestep, dispBandWidth);
        dataSize = 2*12+1;
        status = draw[appID*2]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
        if (status != QUANTAnet_tcpClient_c::OK)
          SAGE_PRINTLOG("Error\n");

        memset(buffer, 0, 65);
        sprintf(buffer, "%12.6f %12.6f", (float)timestep, dispFrameRate);
        dataSize = 2*12+1;
        status = draw[appID*2+1]->write(buffer, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);
        if (status != QUANTAnet_tcpClient_c::OK)
          SAGE_PRINTLOG("Error\n");

        timestep += 1;
      }
    }
    else
    {
      std::cout << "Message : " << msg.getCode() << std::endl << (char *)msg.getData() << std::endl << std::endl;
    }
  }
}

int main(int argc, char **argv)
{
  char token[TOKEN_LEN];
  bool flag = true;
  int code;

  int status, dataSize;
  char buffer[65];

  suil uiLib;
  uiLib.init("fsManager.conf");
  uiLib.connect(NULL);

  uiLib.sendMessage(SAGE_UI_REG," ");

  pthread_t thId;

  if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0){
    return -1;
  }

  gr_server = strdup(argv[1]);

  for (int k=0;k<20;k++) draw[k] = NULL;

  while(flag) {
    std::cin >> token;

    if (strcmp(token, "exit") == 0) {
      flag = false;
    }
    else if (strcmp(token, "exec") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      uiLib.sendMessage(EXEC_APP, token);
    }
    else if (strcmp(token, "move") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      uiLib.sendMessage(MOVE_WINDOW, token);
    }
    else if (strcmp(token, "resize") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      uiLib.sendMessage(RESIZE_WINDOW, token);
    }
    else if (strcmp(token, "bg") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      uiLib.sendMessage(SAGE_BG_COLOR, token);
    }
    else if (strcmp(token, "depth") == 0) {
      char cmd[TOKEN_LEN];
      fgets(cmd, TOKEN_LEN, stdin);
      uiLib.sendMessage(SAGE_Z_VALUE, cmd);
    }
    else if (strcmp(token, "perf") == 0) {
      char cmd[TOKEN_LEN];
      fgets(cmd, TOKEN_LEN, stdin);
      uiLib.sendMessage(PERF_INFO_REQ, cmd);
    }
    else if (strcmp(token, "stopperf") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      uiLib.sendMessage(STOP_PERF_INFO, token);
    }
    else if (strcmp(token, "kill") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      uiLib.sendMessage(SHUTDOWN_APP, token);
    }
    else if (strcmp(token, "shutdown") == 0) {
      uiLib.sendMessage(SAGE_SHUTDOWN);
      _exit(0);
    }
    else if (strcmp(token, "admin") == 0) {
      uiLib.sendMessage(SAGE_ADMIN_CHECK);
    }
    else if (strcmp(token, "help") == 0) {
      std::cout << std::endl;
      std::cout << "exec   app_name init_x init_y        : execute an app" << std::endl;
      std::cout << "move   app_id dx dy                  : move window of an app" << std::endl;
      std::cout << "resize app_id left right bottom top  : resize window of an app" << std::endl;
      std::cout << "bg     red green blue                : change background color" << std::endl;
      std::cout << "depth  num_of_change app_id zVal...  : change z-order of windows" << std::endl;
      std::cout << "perf   app_id report_period(sec)     : request performance info" << std::endl;
      std::cout << "stopperf  app_id                     : stop performance info report" << std::endl;
      std::cout << "kill      app_id                     : shutdown an app" << std::endl;
      std::cout << "shutdown                             : shutdown SAGE" << std::endl;
      std::cout << "admin                                : get administrative info" << std::endl;
    }

    std::cout << std::endl;
  }
}
