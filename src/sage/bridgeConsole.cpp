/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: bridgeConsole.cpp - experimental user console for SAGE Bridge
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

#include "messageInterface.h"
#include "misc.h"
#include <pthread.h>

void* readThread(void *args)
{
  messageInterface *msgInf = (messageInterface *)args;

  while(1) {
    sageMessage msg;
    //msg.init(READ_BUF_SIZE);
    if (msgInf->readMsg(&msg) > 0)
      std::cout << "Message : " << msg.getCode() << std::endl << (char *)msg.getData()
                << std::endl << std::endl;
    sage::usleep(100000);
  }

  return NULL;
}

int initInterface(messageInterface &inf, const char *configFile)
{
  msgInfConfig conf;
  conf.master = false;


  char *sageDir = getenv("SAGE_DIRECTORY");
  if (!sageDir) {
    SAGE_PRINTLOG("sageBridgeUI: cannot find the environment variable SAGE_DIRECTORY");
    return -1;
  }

  data_path path;
  std::string found = path.get_file(configFile);
  if (found.empty()) {
    SAGE_PRINTLOG("bridgeConsole: cannot find the file [%s]", configFile);
    return -1;
  }
  const char *bridgeConfigFile = found.c_str();
  SAGE_PRINTLOG("bridgeConsole: using [%s] configuration file", bridgeConfigFile);

  FILE *fileBridgeConf = fopen(bridgeConfigFile, "r");

  if (!fileBridgeConf) {
    SAGE_PRINTLOG("bridgeConsole: fail to open SAGE Bridge config file [%s]\n", bridgeConfigFile);
    return -1;
  }

  char token[TOKEN_LEN];
  int tokenIdx = getToken(fileBridgeConf, token);

  while(tokenIdx != EOF) {
    if (strcmp(token, "masterIP") == 0) {
      getToken(fileBridgeConf, conf.serverIP);
    }
    else if (strcmp(token, "msgPort") == 0) {
      getToken(fileBridgeConf, token);
      conf.serverPort = atoi(token);
    }

    tokenIdx = getToken(fileBridgeConf, token);
  }

  inf.init(conf);

  return 0;
}

int main(int argc, char *argv[])
{
  char token[TOKEN_LEN];
  bool flag = true;
  int code;

  messageInterface msgInf;

  if (argc == 2)
    initInterface(msgInf, argv[1]);
  else
    initInterface(msgInf, "sageBridge.conf");

  //msgInf.msgToServer(0, BRIDGE_UI_REG);

  pthread_t thId;

  if (pthread_create(&thId, 0, readThread, (void*)&msgInf) != 0){
    return -1;
  }

  while(flag) {
    std::cin >> token;

    if (strcmp(token, "exit") == 0 || strcmp(token, "quit") == 0) {
      flag = false;
    }
    else if (strcmp(token, "share") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, SAGE_APP_SHARE, token);
    }
    else if (strcmp(token, "perf") == 0) {
      char cmd[TOKEN_LEN];
      fgets(cmd, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, PERF_INFO_REQ, cmd);
    }
    else if (strcmp(token, "stopperf") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, STOP_PERF_INFO, token);
    }
    else if (strcmp(token, "kill") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, SHUTDOWN_APP, token);
    }
    else if (strcmp(token, "shutdown") == 0) {
      msgInf.msgToServer(0, BRIDGE_SHUTDOWN);
      _exit(0);
    }
    else if (strcmp(token, "exec") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, EXEC_APP, token);
    }
    else if (strcmp(token, "move") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, MOVE_WINDOW, token);
    }
    else if (strcmp(token, "resize") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, RESIZE_WINDOW, token);
    }
    else if (strcmp(token, "bg") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, SAGE_BG_COLOR, token);
    }
    else if (strcmp(token, "depth") == 0) {
      char cmd[TOKEN_LEN];
      fgets(cmd, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, SAGE_Z_VALUE, cmd);
    }
    else if (strcmp(token, "admin") == 0) {
      msgInf.msgToServer(0, SAGE_ADMIN_CHECK);
    }
    else if (strcmp(token, "rate") == 0) {
      fgets(token, TOKEN_LEN, stdin);
      msgInf.msgToServer(0, APP_FRAME_RATE, token);
    }


    else if (strcmp(token, "help") == 0) {
      std::cout << std::endl;
      std::cout << "exec   app_name config_num (init_x init_y) : execute an app" << std::endl;
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
