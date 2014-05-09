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


#ifdef QUANTA_USE_PTHREADS
#include <pthread.h>
#endif

#include <errno.h>
#include <string.h>

#include "QUANTAnet_perfClient_c.hxx"
#include "QUANTAnet_perfMonitor_c.hxx"

const int QUANTAnet_perfMonitor_c::FAILED = 0;
const int QUANTAnet_perfMonitor_c::OK = 1;

const int _DEFAULT_PERF_BUFFER_SIZE = 1024;
const int _STREAM_INFO_SIZE = 256;

QUANTAnet_perfMonitor_c::QUANTAnet_perfMonitor_c()
{
  resultantStats = new char[_DEFAULT_PERF_BUFFER_SIZE];
  resultantStats[0] = '\0';
  selfIp = new char[_STREAM_INFO_SIZE];
  remoteIp = new char[_STREAM_INFO_SIZE];
  aClient = NULL;
}

// css: The copy constructor is required due to operations like
// QUANTAnet_udp_c::clone() which copy one class to another. By default, C++
// only does a shallow copy. This means that the pointers are copied rather
// than the values they contain. As a result, the cloned object can easily
// lead to memory corruption. This copy constructor initializes the new
// object and copies values from the original one.
QUANTAnet_perfMonitor_c::QUANTAnet_perfMonitor_c(const QUANTAnet_perfMonitor_c& original)
{
  resultantStats = new char[_DEFAULT_PERF_BUFFER_SIZE];
  resultantStats[0] = '\0';
  selfIp = new char[_STREAM_INFO_SIZE];
  remoteIp = new char[_STREAM_INFO_SIZE];

  // With everything allocated, now do a deep copy.
  aClient = original.aClient;
  strncpy(selfIp, original.selfIp, _STREAM_INFO_SIZE);
  strncpy(remoteIp, original.remoteIp, _STREAM_INFO_SIZE);
}

QUANTAnet_perfMonitor_c::~QUANTAnet_perfMonitor_c()
{
  if (resultantStats)
    delete[] resultantStats;
  if (selfIp)
    delete[] selfIp;
  if (remoteIp)
    delete[] remoteIp;
  if (aClient)
    delete aClient;
}

void QUANTAnet_perfMonitor_c::setIPs(const char* _selfIp, const char* _remoteIp)
{
  if (_selfIp)
    strncpy(this->selfIp, _selfIp, _STREAM_INFO_SIZE);

  if (_remoteIp)
    strncpy(this->remoteIp, _remoteIp, _STREAM_INFO_SIZE);
}

void QUANTAnet_perfMonitor_c::setPorts(int _selfPort, int _remotePort)
{
  selfPort = _selfPort;
  remotePort = _remotePort;
}

void QUANTAnet_perfMonitor_c::showStats(char* streamInfo, char* comment)
{
  //update the resultant buffer with the calculated information
  updateStats(streamInfo, comment);
  printf("\n%s\n", resultantStats);
}

int QUANTAnet_perfMonitor_c::logStats(char* streamInfo, char* comment, FILE* filePtr)
{
  if (filePtr) {
    //update the resultant buffer with the calculated information
    updateStats(streamInfo, comment);

    if ((fprintf(filePtr, "%s\n", resultantStats)) != (strlen(resultantStats) + 1)) {
      printf("\nWarning: Error writing to logfile - data may be truncated or  not written at all\n");
      return QUANTAnet_perfMonitor_c::FAILED;
    }
    fflush(filePtr);
  }
  else {
    printf("File pointer must be given to use logStats.\n");
    return QUANTAnet_perfMonitor_c::FAILED;
  }
  return QUANTAnet_perfMonitor_c::OK;
}

int QUANTAnet_perfMonitor_c::initSendStats(const char* monitorClientIP, int port)
{
  if (monitorClientIP) {
    if (aClient) {
      delete aClient;
      aClient = NULL;// close current connection first.
    }
    aClient = new QUANTAnet_perfDaemonClient_c;
    if (aClient == NULL) {
      printf("Cannot create perfDaemonClient.\n");
      return QUANTAnet_perfMonitor_c::FAILED;
    }

    if (aClient->connectToServer(monitorClientIP, port) == QUANTAnet_perfDaemonClient_c::FAILED) {
      printf("\nCannot connect to perf_daemon\n");
      delete aClient;
      aClient = NULL;
      return QUANTAnet_perfMonitor_c::FAILED;
    }
    int len = SEND_STAT_CLIENT_LENGTH + 1; // including terminating null char
    if (aClient->write(SEND_STAT_CLIENT, &len) != QUANTAnet_perfDaemonClient_c::OK) {
      printf("Cannot connect to perf_daemon\n");
      delete aClient;
      aClient = NULL;
      return QUANTAnet_perfMonitor_c::FAILED;
    }
  }
  else {
    printf("Invalid ip address for perf_daemon is given. Please verify the address again.\n");
    return QUANTAnet_perfMonitor_c::FAILED;
  }
  return QUANTAnet_perfMonitor_c::OK;
}

int QUANTAnet_perfMonitor_c::sendStats(char* streamInfo, char* comment)
{
  if (aClient == NULL) {
    printf("PerfDaemonClient is not initialized. No data will be sent to perf_daemon\n");
    return QUANTAnet_perfMonitor_c::FAILED;
  }

  //update the resultant buffer with the calculated information
  updateStats(streamInfo, comment);

  int nbytes = strlen(resultantStats) + 1;

  //transmitting data to perfdaemon
  int status = aClient->write(resultantStats, &nbytes);
  if (status != QUANTAnet_perfDaemonClient_c::OK) {
    aClient->showStatus(status, nbytes);
    delete aClient;
    aClient = NULL;
    return QUANTAnet_perfMonitor_c::FAILED;
  }

  return QUANTAnet_perfMonitor_c::OK;
}

void QUANTAnet_perfMonitor_c::exitSendStats()
{
  if (aClient) {
    delete aClient;
    aClient = NULL;
  }
}

void QUANTAnet_perfMonitor_c::updateStats(char* streamInfo, char* comment)
{
  //update the resultant stats buffer
  if (resultantStats) {
    char *tempStreamInfo, *tempComment;
    char tempMsg[] = "NULL";

    tempComment = (comment == NULL) ? tempMsg : comment;
    tempStreamInfo = (streamInfo == NULL) ? tempMsg : streamInfo;

    // css: When copying into the resultantStats buffer, try to avoid a
    // buffer overflow by limiting the operation with snprintf().
#ifdef WIN32
    _snprintf(resultantStats, _DEFAULT_PERF_BUFFER_SIZE,
              "TIME=%lf SELF_IP=%s REMOTE_IP=%s SELF_PORT=%d " \
              "REMOTE_PORT=%d STREAM_INFO=%s COMMENT=%s MIN_LAT=%lf " \
              "AVG_LAT=%lf MAX_LAT=%lf INST_LAT=%lf STAL=%lf JITTER=%lf " \
              "MIN_IMD=%lf AVG_IMD=%lf MAX_IMD=%lf INST_IMD=%lf " \
              "AVG_RBW=%lf INST_RBW=%lf AVG_SBW=%lf INST_SBW=%lf " \
              "STAB=%lf BURSTINESS=%lf TOTAL_READ=%d TOTAL_SENT=%d " \
              "PACKETS_READ=%ld PACKETS_SENT=%ld", getTimeInSecs(),
              selfIp, remoteIp, selfPort, remotePort, tempStreamInfo,
              tempComment,  getMinLatency(), getAverageLatency(),
              getMaxLatency(), getInstantLatency(), getSTALatency(),
              getJitter(),  getMinInterMesgDelay(),
              getAverageInterMesgDelay(), getMaxInterMesgDelay(),
              getInstantInterMesgDelay(), getAverageReceiveBandwidth(),
              getInstantReceiveBandwidth(),  getAverageSendBandwidth(),
              getInstantSendBandwidth(), getSTABandwidth(),
              getBurstiness(), getTotalDataRead(),  getTotalDataSent(),
              getPacketsRead(), getPacketsSent());
#else /* WIN32 */
    snprintf(resultantStats, _DEFAULT_PERF_BUFFER_SIZE,
             "TIME=%lf SELF_IP=%s REMOTE_IP=%s SELF_PORT=%d " \
             "REMOTE_PORT=%d STREAM_INFO=%s COMMENT=%s MIN_LAT=%lf " \
             "AVG_LAT=%lf MAX_LAT=%lf INST_LAT=%lf STAL=%lf JITTER=%lf " \
             "MIN_IMD=%lf AVG_IMD=%lf MAX_IMD=%lf INST_IMD=%lf " \
             "AVG_RBW=%lf INST_RBW=%lf AVG_SBW=%lf INST_SBW=%lf " \
             "STAB=%lf BURSTINESS=%lf TOTAL_READ=%d TOTAL_SENT=%d " \
             "PACKETS_READ=%ld PACKETS_SENT=%ld", getTimeInSecs(),
             selfIp, remoteIp, selfPort, remotePort, tempStreamInfo,
             tempComment,  getMinLatency(), getAverageLatency(),
             getMaxLatency(), getInstantLatency(), getSTALatency(),
             getJitter(),  getMinInterMesgDelay(),
             getAverageInterMesgDelay(), getMaxInterMesgDelay(),
             getInstantInterMesgDelay(), getAverageReceiveBandwidth(),
             getInstantReceiveBandwidth(),  getAverageSendBandwidth(),
             getInstantSendBandwidth(), getSTABandwidth(),
             getBurstiness(), getTotalDataRead(),  getTotalDataSent(),
             getPacketsRead(), getPacketsSent());
#endif /* WIN32 */
  }
}
