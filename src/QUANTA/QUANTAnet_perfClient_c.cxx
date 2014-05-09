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

#include "QUANTAnet_perfClient_c.hxx"
#include "QUANTAnet_datapack_c.hxx"
#include "QUANTAnet_tcp_c.hxx"

#include <string.h>
#ifndef WIN32
#include <sys/errno.h>
#ifndef errno
extern int errno;
#endif /* errno */
#endif

const int QUANTAnet_perfDaemonClient_c::OK = 1;
const int QUANTAnet_perfDaemonClient_c::FAILED = 2;
const int QUANTAnet_perfDaemonClient_c::MEM_ALLOC_ERR = 3;
const int QUANTAnet_perfDaemonClient_c::NON_BLOCKING_HAS_NO_DATA = 7;
const int QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED=8;
const int QUANTAnet_perfDaemonClient_c::NOT_READY=9;
const int QUANTAnet_perfDaemonClient_c::READY_TO_READ=10;
const int QUANTAnet_perfDaemonClient_c::READY_TO_WRITE=11;
const int QUANTAnet_perfDaemonClient_c::READY_TO_READ_AND_WRITE=12;
const int QUANTAnet_perfDaemonClient_c::BLOCKING=QUANTAnet_tcpClient_c::BLOCKING;
const int QUANTAnet_perfDaemonClient_c::NON_BLOCKING=QUANTAnet_tcpClient_c::NON_BLOCKING;
const int QUANTAnet_perfDaemonClient_c::NO_TIME_OUT=QUANTAnet_tcpClient_c::NO_TIME_OUT;

QUANTAnet_perfDaemonClient_c :: QUANTAnet_perfDaemonClient_c(int isPerfClient)
{
  connectionAlive = 0;
  client = new QUANTAnet_tcpClient_c;
  perfClient = isPerfClient;
  // Turn the timeout off. In general this is necessary because in a
  // TCP stream if you lose some data you won't be able to figure out
  // where the head of the next message will be.
  if (client) client->setTimeOut(QUANTAnet_perfDaemonClient_c::NO_TIME_OUT);
  timeOut = QUANTAnet_perfDaemonClient_c::NO_TIME_OUT;
}

QUANTAnet_perfDaemonClient_c::QUANTAnet_perfDaemonClient_c(QUANTAnet_tcpClient_c *aclient, int isPerfClient)
{
  connectionAlive = 1;
  client = aclient;
  perfClient = isPerfClient;
  if (client) client->setTimeOut(QUANTAnet_perfDaemonClient_c::NO_TIME_OUT);
  timeOut = QUANTAnet_perfDaemonClient_c::NO_TIME_OUT;
}

void QUANTAnet_perfDaemonClient_c :: setTimeOut(int timeout)
{
  if (client) {
    client->setTimeOut(timeout);
    timeOut =  timeout;
  }
}

QUANTAnet_perfDaemonClient_c :: ~QUANTAnet_perfDaemonClient_c()
{
  if (client)
    delete client;
}

int QUANTAnet_perfDaemonClient_c :: connectToServer(const char *ip, int port)
{
  if (client && (client->connectToServer(ip,port) >= 0)) {
    connectionAlive = 1;

    if (perfClient) {
      int len = strlen(REGISTER_PERF_CLIENT) + 1;

      if (write(REGISTER_PERF_CLIENT, &len) != QUANTAnet_perfDaemonClient_c::OK) {
        printf("Cannot connect to perf_daemon\n");
        connectionAlive = 0;
        return QUANTAnet_perfDaemonClient_c::FAILED;
      }
    }

    return QUANTAnet_perfDaemonClient_c::OK;
  }
  else {
    printf("Cannot connect to perf_daemon\n");
    connectionAlive = 0;
    return QUANTAnet_perfDaemonClient_c::FAILED;
  }
}


int QUANTAnet_perfDaemonClient_c :: read(char **ret, int *nbytes, int blockingType)
{
  if (connectionAlive == 0 || client == NULL) return QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED;

  QUANTAnet_datapack_c packer;
  char *read_buffer;
  int dataSize;
  int status;
  //  char header[sizeof(int)];

  // Read the header.
  dataSize = packer.sizeof_int();
  read_buffer = new char[dataSize];
  status = client->read((char*) read_buffer, &dataSize, blockingType);
  if (status == QUANTAnet_tcpClient_c::OK)  {
    // The header is simply the # of bytes that follows.
    int incomingSize = 0;

    packer.initUnpack((char *)read_buffer,dataSize);
    packer.unpackInt(&incomingSize);
    delete[] read_buffer;

    char *buffer = new char[incomingSize];
    if (buffer == NULL) {
      *ret = NULL;
      connectionAlive = 0;
      return QUANTAnet_perfDaemonClient_c::MEM_ALLOC_ERR;
    }

    // Read the body
    status = client->read(buffer, &incomingSize, QUANTAnet_tcpClient_c::BLOCKING);
    if (status == QUANTAnet_tcpClient_c::OK)  {
      *nbytes = incomingSize;
      *ret = buffer;
      return QUANTAnet_perfDaemonClient_c::OK;
    } else {
      connectionAlive = 0;
      *nbytes = 0;
      delete[] buffer;
      *ret = NULL;
      if (status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED)
        return QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED;
      else
        return QUANTAnet_perfDaemonClient_c::FAILED;
    }
  }

  if (read_buffer)
    delete[] read_buffer;
  *nbytes = 0;
  *ret = NULL;
  if (status == QUANTAnet_tcpClient_c::NON_BLOCKING_HAS_NO_DATA)
    return QUANTAnet_perfDaemonClient_c::NON_BLOCKING_HAS_NO_DATA;
  if (status == QUANTAnet_tcpClient_c::CONNECTION_TERMINATED) {
    connectionAlive = 0;
    return QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED;
  }
  if (status == QUANTAnet_tcpClient_c::TIMED_OUT) {
    connectionAlive = 0;
    return QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED;
  }

  return QUANTAnet_perfDaemonClient_c::FAILED;

}


int QUANTAnet_perfDaemonClient_c :: write(const char *ptr, int *nbytes)
{
  QUANTAnet_datapack_c packer;
  char *send_buffer;

  if (connectionAlive == 0 || client == NULL) return QUANTAnet_perfDaemonClient_c::FAILED;

  int  dataSize = packer.sizeof_int();
  int status;

  // Send the header
  send_buffer = new char[dataSize];
  packer.initPack(send_buffer, dataSize);
  packer.packInt(*nbytes);
  status = client->write(send_buffer,&dataSize,QUANTAnet_tcpClient_c::BLOCKING);
  delete[] send_buffer;
  if (status != QUANTAnet_tcpClient_c::OK) {
    connectionAlive = 0;
    return QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED;
  }

  status = client->write(ptr, nbytes, QUANTAnet_tcpClient_c::BLOCKING);
  if (status != QUANTAnet_tcpClient_c::OK) {
    connectionAlive = 0;
    return QUANTAnet_perfDaemonClient_c::CONNECTION_TERMINATED;
  }

  return QUANTAnet_perfDaemonClient_c::OK;
}

int QUANTAnet_perfDaemonClient_c :: isReadyToRead()
{

  if (connectionAlive == 0 || client == NULL) return QUANTAnet_perfDaemonClient_c::NOT_READY;

  int status= client->isReadyToRead();

  if (QUANTAnet_tcpClient_c::READY_TO_READ == status)
    return QUANTAnet_perfDaemonClient_c::READY_TO_READ;

  else //we got QUANTAnet_tcpClient_c::NOT_READY
    return QUANTAnet_perfDaemonClient_c::NOT_READY;
}


int QUANTAnet_perfDaemonClient_c :: isReadyToWrite() {

  if (connectionAlive == 0 || client == NULL) return QUANTAnet_perfDaemonClient_c::NOT_READY;

  int status =  client->isReadyToWrite();

  if (QUANTAnet_tcpClient_c::READY_TO_WRITE == status)
    return QUANTAnet_perfDaemonClient_c::READY_TO_WRITE;

  else // we got QUANTAnet_tcpClient_c::NOT_READY
    return QUANTAnet_perfDaemonClient_c::NOT_READY;


}

int QUANTAnet_perfDaemonClient_c :: isReady() {

  if (connectionAlive == 0 || client == NULL) return QUANTAnet_perfDaemonClient_c::NOT_READY;

  int status =  client->isReady();

  if (QUANTAnet_tcpClient_c::READY_TO_READ == status)
    return QUANTAnet_perfDaemonClient_c::READY_TO_READ;

  else if (QUANTAnet_tcpClient_c::READY_TO_WRITE == status)
    return QUANTAnet_perfDaemonClient_c::READY_TO_WRITE;

  else if (QUANTAnet_tcpClient_c::READY_TO_READ_AND_WRITE == status)
    return QUANTAnet_perfDaemonClient_c::READY_TO_READ_AND_WRITE;

  else //we got QUANTAnet_tcpClient_c::NOT_READY
    return QUANTAnet_perfDaemonClient_c::NOT_READY;

}

void QUANTAnet_perfDaemonClient_c::showStatus(int status, int nbytes)
{
  switch (status) {
  case CONNECTION_TERMINATED:
    printf("QUANTAnet_perfDaemonClient_c::showStatus: Connection terminated with %d bytes transacted.\n", nbytes);
    break;
  case NON_BLOCKING_HAS_NO_DATA:
    printf("QUANTAnet_perfDaemonClient_c::showStatus: Non-blocking read found no data on stream.\n");
    break;
  case OK:
    printf("QUANTAnet_perfDaemonClient_c::showStatus: Transaction ok.\n");
    break;
  default:
#ifdef WIN32
    printf("QUANTAnet_perfDaemonClient_c::showStatus: UNIX Socket error no: %d ",WSAGetLastError());
#else
    printf("QUANTAnet_perfDaemonClient_c::showStatus: UNIX Socket error no: %d ",errno);
    perror("");
#endif
    break;
  }
  fflush(stdout);
}

unsigned int QUANTAnet_perfDaemonClient_c::getRemoteIP()
{
  return (client) ? client->getRemoteIP() : 0;
};

void QUANTAnet_perfDaemonClient_c::getRemoteIP(char* name)
{
  if (client) client->getRemoteIP(name);
}

int QUANTAnet_perfDaemonClient_c::getSelfPort()
{
  return (client) ? client->getSelfPort() : 0;
}

int QUANTAnet_perfDaemonClient_c::getRemotePort()
{
  return (client) ? client->getRemotePort() : 0;
}

void QUANTAnet_perfDaemonClient_c::close()
{
  connectionAlive = 0;
  if (client) client->close();
}
