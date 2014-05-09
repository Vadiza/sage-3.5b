/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
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
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

#include "QUANTAnet_perfDaemon_c.hxx"
#include "QUANTAnet_datapack_c.hxx"
#include "QUANTAnet_tcp_c.hxx"

#include <string.h>

const int QUANTAnet_perfDaemon_c::OK = 0;
const int QUANTAnet_perfDaemon_c::FAILED = 1;
const int QUANTAnet_perfDaemon_c::MEM_ALLOC_ERR = 2;
const int QUANTAnet_perfDaemon_c::NEW_CONNECTION_ESTABLISHED = 4;
const int QUANTAnet_perfDaemon_c::TOO_MANY_CLIENTS = 5;
const int QUANTAnet_perfDaemon_c::NO_NEW_CONNECTION= 6;
const int QUANTAnet_perfDaemon_c::NON_BLOCKING_HAS_NO_DATA=7;
const int QUANTAnet_perfDaemon_c::SKIP_DISTRIBUTION=8;
const int QUANTAnet_perfDaemon_c::DEFAULT_PORT=PERF_DAEMON_DEFAULT_PORT;

QUANTAnet_perfDaemon_c :: QUANTAnet_perfDaemon_c(FILE* File)
{
  server = new QUANTAnet_tcpServer_c;
  interceptCallback = NULL;
  interceptUserData = NULL;
  interceptNewConnectionCallback = NULL;
  interceptNewConnectionUserData = NULL;
  LogFile = File;
}


QUANTAnet_perfDaemon_c :: ~QUANTAnet_perfDaemon_c()
{
  if (server) delete server;
  closeDownSockets();
}

int QUANTAnet_perfDaemon_c :: init(int incomingPort, int maxClients)
{
  maxNumClients = maxClients;

  // Initialize the incoming socket
  if (server && server->init(incomingPort) == 0)  {
    return QUANTAnet_perfDaemon_c::FAILED;
  }

  // Create an array of pointers to track the incoming connections.
  clients = new PerfDaemonClient*[maxNumClients];
  if (clients == NULL) return QUANTAnet_perfDaemon_c::MEM_ALLOC_ERR;

  for(int i = 0; i < maxNumClients; i++) clients[i] = NULL;

  return QUANTAnet_perfDaemon_c::OK;
}

int QUANTAnet_perfDaemon_c :: checkForNewClients()
{
  QUANTAnet_tcpClient_c *aClient;
  /* If a connection is established, accept it and create
   * a new socket for that connection to communicate with.
   *
   */
  aClient = server->checkForNewConnections();

  // If a new client connects then add it to an empty slot in the array of clients.
  if (aClient) {
    QUANTAnet_perfDaemonClient_c *newClient = new QUANTAnet_perfDaemonClient_c(aClient);

    // Turn the timeout off. In general this is necessary because in a
    // TCP stream if you lose some data you won't be able to figure out
    // where the head of the next message will be.
    aClient->setTimeOut(QUANTAnet_tcpClient_c::NO_TIME_OUT);

    for (int i = 0; i < maxNumClients; i++) {
      if (clients && clients[i] == NULL){
        clients[i] = new PerfDaemonClient;
        clients[i]->Client = newClient;
        clients[i]->IsPerfClient = 0;
        clients[i]->Started = 0;

        char remoteIP[256];
        int remotePort;
        newClient->getRemoteIP(remoteIP);
        remotePort = newClient->getRemotePort();
        printf("QUANTAnet_perfDaemon_c: New Client: %s:%d\n",remoteIP, remotePort);

        //sprintf(clients[i]->FileName, "%s_%d", remoteIP, remotePort);

        if (interceptNewConnectionCallback)
          interceptNewConnectionCallback(newClient,
                                         interceptNewConnectionUserData);

        return QUANTAnet_perfDaemon_c::NEW_CONNECTION_ESTABLISHED;
      }
    }

    // If user callback specified then call it.

    // If cannot host any more clients then report it.
    return QUANTAnet_perfDaemon_c::TOO_MANY_CLIENTS;
  }
  return QUANTAnet_perfDaemon_c::NO_NEW_CONNECTION;
}

void QUANTAnet_perfDaemon_c :: intercept(int (*callback) (QUANTAnet_perfDaemonClient_c *, char** buffer, int *datasize, void *userData), void* userData)
{
  interceptCallback = callback;
  interceptUserData = userData;
}

void QUANTAnet_perfDaemon_c :: interceptNewConnection(void (*callback) (QUANTAnet_perfDaemonClient_c *newClient, void* userData), void *userData)
{
  interceptNewConnectionCallback = callback;
  interceptNewConnectionUserData = userData;
}


int QUANTAnet_perfDaemon_c :: process()
{
  int status;
  int incomingSize = 0;
  char *buffer;

  // Check for any new clients.
  checkForNewClients();

  // Go through array of clients and read from them.
  for (int i = 0; i < maxNumClients; i++) {
    if (clients && clients[i]) {
      status = clients[i]->Client->read(&buffer, &incomingSize,
                                        QUANTAnet_perfDaemonClient_c::NON_BLOCKING);
      if (status == QUANTAnet_perfDaemonClient_c::MEM_ALLOC_ERR)
        return QUANTAnet_perfDaemon_c::MEM_ALLOC_ERR;
      if (status == QUANTAnet_perfDaemonClient_c::OK) {
        if (clients[i]->Started == 0) {
          if (strncmp(REGISTER_PERF_CLIENT, buffer, REGISTER_PERF_CLIENT_LENGTH) == 0) {
            clients[i]->IsPerfClient = 1;
            clients[i]->Started = 1;
            delete buffer;
            return QUANTAnet_perfDaemon_c::OK;
          }
          else if (strncmp(SEND_STAT_CLIENT, buffer, SEND_STAT_CLIENT_LENGTH) == 0) {
            clients[i]->IsPerfClient = 0;
            clients[i]->Started = 1;
            delete buffer;
            return QUANTAnet_perfDaemon_c::OK;
          }
        }
        if (interceptCallback) {
          if (interceptCallback(clients[i]->Client, &buffer,
                                &incomingSize, interceptUserData)
              == QUANTAnet_perfDaemon_c::SKIP_DISTRIBUTION) {
            delete buffer;
            continue;
          }
        }

        // append to the logfile
        if (LogFile) {
          fprintf(LogFile, "%s\n", buffer);
          fflush(LogFile);
        }

        // Reflect the data to all connected clients except
        // the client that originated the message.
        distributeDataToPerfClients(buffer, incomingSize);
        delete buffer;
      }
      else if (status != QUANTAnet_perfDaemonClient_c::NON_BLOCKING_HAS_NO_DATA){
        removeClient(i);
        printf("client %d is removed.\n", i);
      }
    }
  }
  return QUANTAnet_perfDaemon_c::OK;
}

int QUANTAnet_perfDaemon_c :: distributeDataToPerfClients(char* buf, int incomingSize)
{
  // Iterate thru the array of clients and send the message to all of them
  // except the one indicated by clientToIgnore
  if (clients) {
    for (int i = 0; i < maxNumClients; i++) {
      if (clients[i]) {
        if (clients[i]->IsPerfClient
            && clients[i]->Client->write(buf, &incomingSize) != QUANTAnet_perfDaemonClient_c::OK) {
          removeClient(i);
        }
      }
    }
  }
  return QUANTAnet_perfDaemon_c::OK;
}

int QUANTAnet_perfDaemon_c::sendToAll(char* buf, int incomingSize)
{
  return distributeDataToPerfClients(buf, incomingSize);
}

void QUANTAnet_perfDaemon_c :: removeClient(int i)
{
  if (clients && clients[i])  {
    delete clients[i];
  }
  clients[i] = NULL;
}

void QUANTAnet_perfDaemon_c :: closeDownSockets()
{
  int count;

  // Iterate thru list of clients and close them down.
  if (clients) {
    for (count = 0; count < maxNumClients; count++) {
      if (clients[count]) {
        clients[count]->Client->close();
        delete clients[count];
      }
    }
    delete clients;
  }
  server->close();
  delete server;
}
