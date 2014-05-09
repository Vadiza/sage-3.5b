/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: fsServer.cpp - a component of the Free Space Manager which takes care
 *         of communication with Free Space Manager Clients
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

#include "fsServer.h"
#include "fsManager.h"

fsServer::fsServer()
{
  sysServer = NULL;
  uiServer = NULL;
  fsm = NULL;

  //maxNumOfApp = 0;
}

fsServer::~fsServer()
{
  if (sysServer)
    delete sysServer;

  if (uiServer)
    delete uiServer;
}

int fsServer::init(fsManager *m)
{
  fsm = m;
  numSysClients = 0;
  numUiClients = 0;

  sysServer = new QUANTAnet_tcpServer_c;
  sysServer->init(fsm->getSysPort());
  sysServer->setSockOptions(QUANTAnet_tcpServer_c::READ_BUFFER_SIZE, 65536);
  sysServer->setSockOptions(QUANTAnet_tcpServer_c::WRITE_BUFFER_SIZE, 65536);

  return 0;
}


// start the ui server only after all the nodes have reported back
int fsServer::startUiServer()
{
  uiServer = new QUANTAnet_tcpServer_c;
  uiServer->init(fsm->getUiPort());
  uiServer->setSockOptions(QUANTAnet_tcpServer_c::READ_BUFFER_SIZE, 65536);
  uiServer->setSockOptions(QUANTAnet_tcpServer_c::WRITE_BUFFER_SIZE, 65536);

  return 0;
}


int fsServer::checkClients()
{
  QUANTAnet_tcpClient_c *aClient;

  // If a connection is established, accept it and create
  // a new socket for that connection to communicate with.

  aClient = sysServer->checkForNewConnections();

  if (aClient) {
    //
    // askees - apps (imageviewer, pdfviewer, etc) and ui connect here
    //
    //SAGE_PRINTLOG("ASKEES: NEW CONNECTION: SDM: %d", fsm->getSysPort());

    SAGE_PRINTLOG("fsServer::checkClients() : sysClient (SDM) %d connected to fsManager\n", numSysClients);
    aClient->setTimeOut(1);
    sysClientList.push_back(aClient);
    numSysClients++;
  }

  if (uiServer != NULL) {
    aClient = uiServer->checkForNewConnections();

    if (aClient) {
      //
      // askees - apps (imageviewer, pdfviewer, etc) and ui connect here
      //
      //SAGE_PRINTLOG("ASKEES: NEW CONNECTION: UI: %d", fsm->getUiPort());

      SAGE_PRINTLOG("fsServer::checkClients() : UI %d connected to fsManager\n", numUiClients);
      aClient->setTimeOut(1);
      uiClientList.push_back(aClient);
      numUiClients++;
    }
  }

  sageMessage msg;

  int dataSize;
  int status;

  for (int i = 0; i < numSysClients; i++) {
    if (sysClientList[i]) {
      char msgSize[MESSAGE_FIELD_SIZE];
      dataSize = MESSAGE_FIELD_SIZE;
      status = sysClientList[i]->read(msgSize, &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

      if (status == QUANTAnet_tcpClient_c::OK)  {
        msgSize[MESSAGE_FIELD_SIZE-1] = '\0';
        dataSize = atoi(msgSize);
        msg.init(dataSize);
        dataSize = dataSize - MESSAGE_FIELD_SIZE;
        sysClientList[i]->read((char *)msg.getBuffer()+MESSAGE_FIELD_SIZE, &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

        /*
          SAGE_PRINTLOG("Read in message from client %d\n",i+SYSTEM_CLIENT_BASE);
          std::cout << "\tmessage code: " << msg.getCode() << std::endl;
          if (msg.getData())
          std::cout << "\t" << (char *)msg.getData() << std::endl;
          else
          std::cout << std::endl;
        */

        /**
           if ( msg.getCode() == REG_APP ) {
           maxNumOfApp++;
           SAGE_PRINTLOG("fsServer::checkClients() : maxNumOfApp %u\n", maxNumOfApp);
           if ( maxNumOfApp > (SAGE_SYNC_MSG_LEN/sizeof(int))/2 ) {
           SAGE_PRINTLOG("fsServer::checkClients() : max num of app %d has reached. \n", (SAGE_SYNC_MSG_LEN/sizeof(int))/2);
           sendMessage(i+SYSTEM_CLIENT_BASE, APP_QUIT);
           msg.destroy();
           break;
           }
           else {
           }
           }
        **/


        if (msg.getCode() < DISP_MESSAGE) {
          fsm->msgToCore(msg, i+SYSTEM_CLIENT_BASE);
        }
        else if (msg.getCode() < GRCV_MESSAGE) {
          fsm->msgToDisp(msg, i+SYSTEM_CLIENT_BASE);
        }
        else
          sendMessage(msg);

        msg.destroy();
      }
      else if (status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED) {
        SAGE_PRINTLOG("\nfsServer::checkClients() : Can't read a message from SDM %d. Connection was terminated\n", i);
        sysClientList[i] = NULL;

        /*
          maxNumOfApp--;
          SAGE_PRINTLOG("fsServer::checkClients() : maxNumOfApp %u\n", maxNumOfApp);
        */

        return 1;
      }
    }
  }

  for (int i = 0; i < numUiClients; i++) {
    if (uiClientList[i]) {
      bool flag = true;
      while(flag) {
        flag = false;
        char msgSize[MESSAGE_FIELD_SIZE];
        dataSize = MESSAGE_FIELD_SIZE;

        status = uiClientList[i]->read(msgSize, &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

        if (status == QUANTAnet_tcpClient_c::OK)  {
          flag = true;
          msgSize[MESSAGE_FIELD_SIZE-1] = '\0';
          dataSize = atoi(msgSize);
          msg.init(dataSize);
          dataSize = dataSize - MESSAGE_FIELD_SIZE;
          uiClientList[i]->read((char *)msg.getBuffer()+MESSAGE_FIELD_SIZE,
                                &dataSize, QUANTAnet_tcpClient_c::BLOCKING);

          //SAGE_PRINTLOG("Read in message from client %d\n",i);
          //std::cout << "messge : " << msg.getCode() << std::endl;
          //if (msg.getData())
          //   std::cout << (char *)msg.getData() << std::endl;
          //else
          //   std::cout << std::endl;

          if (msg.getCode() < DISP_MESSAGE) {
            fsm->msgToCore(msg, i);
          }
          else
            sendMessage(msg);

          msg.destroy();
        }
        else if (status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED) {
          std::cout << "fsCore::checkClients() : connection to client " << i << " was terminated" << std::endl;
          uiClientList[i] = NULL;
          return 1;
        }
      } // end while
    } // end if
  } // end for

  return 0;
}

int fsServer::sendMessage(int cId, int code, int data)
{
  QUANTAnet_tcpClient_c *aClient = NULL;
  if (cId >= SYSTEM_CLIENT_BASE)
    aClient = sysClientList[cId-SYSTEM_CLIENT_BASE];
  else
    aClient = uiClientList[cId];

  if (!aClient) {
    return -1;
  }

  sageMessage msg;
  char msgStr[TOKEN_LEN];
  sprintf(msgStr, "%d", data);

  if (msg.init(cId, code, 0, strlen(msgStr)+1, msgStr) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int dataSize = msg.getBufSize();
  int status = aClient->write((char *)msg.getBuffer(), &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

  if (status == QUANTAnet_tcpClient_c::TIMED_OUT ||
      status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED)  {
    if (cId >= SYSTEM_CLIENT_BASE)
      sysClientList[cId-SYSTEM_CLIENT_BASE] = NULL;
    else
      uiClientList[cId] = NULL;
    msg.destroy();
    return -1;
  }

  msg.destroy();

  return 0;
}

int fsServer::sendMessage(int cId, int code, char* data)
{
  QUANTAnet_tcpClient_c *aClient = NULL;
  if (cId >= SYSTEM_CLIENT_BASE)
    aClient = sysClientList[cId-SYSTEM_CLIENT_BASE];
  else
    aClient = uiClientList[cId];

  if (!aClient) {
    return -1;
  }

  sageMessage msg;

  if (msg.init(cId, code, 0, strlen(data)+1, data) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int dataSize = msg.getBufSize();
  int status = aClient->write((char *)msg.getBuffer(), &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

  if (status == QUANTAnet_tcpClient_c::TIMED_OUT ||
      status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED)  {
    if (cId >= SYSTEM_CLIENT_BASE)
      sysClientList[cId-SYSTEM_CLIENT_BASE] = NULL;
    else
      uiClientList[cId] = NULL;
    msg.destroy();
    return -1;
  }

  msg.destroy();

  return 0;
}

int fsServer::sendMessage(int cId, int code)
{
  QUANTAnet_tcpClient_c *aClient = NULL;
  if (cId >= SYSTEM_CLIENT_BASE)
    aClient = sysClientList[cId-SYSTEM_CLIENT_BASE];
  else
    aClient = uiClientList[cId];

  if (!aClient) {
    return -1;
  }

  sageMessage msg;

  if (msg.init(cId, code, 0, 0, NULL) < 0) {
    std::cerr << "fail to init the message!" << std::endl;
    return -1;
  }

  int dataSize = msg.getBufSize();
  int status = aClient->write((char *)msg.getBuffer(), &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

  if (status == QUANTAnet_tcpClient_c::TIMED_OUT ||
      status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED)  {
    if (cId >= SYSTEM_CLIENT_BASE)
      sysClientList[cId-SYSTEM_CLIENT_BASE] = NULL;
    else
      uiClientList[cId] = NULL;
    msg.destroy();
    return -1;
  }

  msg.destroy();

  return 0;
}

int fsServer::sendMessage(sageMessage &msg)
{
  int dataSize = msg.getBufSize();
  int cId = msg.getDest();

  QUANTAnet_tcpClient_c *aClient = NULL;
  if (cId >= SYSTEM_CLIENT_BASE)
    aClient = sysClientList[cId-SYSTEM_CLIENT_BASE];
  else
    aClient = uiClientList[cId];

  if (!aClient) {
    return -1;
  }

  /*
    char abc[1024];
    aClient->getRemoteIP(abc);
    SAGE_PRINTLOG("fsServer::sendMessage() : write to %s, data %s\n", abc, (char*)msg.getData());
  */

  int status = aClient->write((char *)msg.getBuffer(), &dataSize, QUANTAnet_tcpClient_c::NON_BLOCKING);

  if (status == QUANTAnet_tcpClient_c::TIMED_OUT ||
      status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED)  {
    if (cId >= SYSTEM_CLIENT_BASE)
      sysClientList[cId-SYSTEM_CLIENT_BASE] = NULL;
    else
      uiClientList[cId] = NULL;
    return -1;
  }

  return 0;
}
