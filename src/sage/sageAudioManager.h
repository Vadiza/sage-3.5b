/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioManager.h
 * Author : Byungil Jeong, Rajvikram Singh
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

#ifndef _SAGE_AUDIO_MANAGER_H_
#define _SAGE_AUDIO_MANAGER_H_

#include "fsClient.h"
#include "streamProtocol.h"
#include "sageAudioModule.h"

class sageAudioManager;
class sageEvent;
class sageAudioReceiver;
class sageTcpModule;
class sageUdpModule;
class sageEventQueue;
class sageSyncClient;

typedef struct {
  sageAudioManager *This;
  streamProtocol     *nwObj;
} nwCheckThreadParam;

/**
 * \brief The binary of this class is executed by sageVirtualDesktop.launchAudioReceiver()
 *
 * This class is very similar to the class sageDisplayManager
 */
class sageAudioManager : public fsClient {
private:
  int nodeID;
  sageTcpModule *tcpObj; /**< TCP module */
  sageUdpModule *udpObj; /**< UDP module */
  sageNwConfig nwCfg;
  sageEventQueue *eventQueue; /**< event queue. continuously checked in the mainLoop() */
  sageSyncClient *syncClientObj; /**< sageSyncClient object*/

  sageAudioModule *audioModule; /**< sageAudioModule */
  sageAudioConfig audioCfg;


  std::vector<sageAudioReceiver *> receiverList; /**< a list of sageAudioReceiver object. It responsible for receiving streams from sageAudioStreamer */

  int syncPort, streamPort;
  int totalRcvNum;
  int memSize;
  bool rcvEnd;

  /**
   * handles below events <BR>
   * EVENT_NEW_CONNECTION -> initStreams()<BR>
   * EVENT_NEW_MESSAGE -> parseMessage()<BR>
   * EVENT_SYNC_MESSAGE -> processSync()
   */
  int parseEvent(sageEvent *event);
  int parseMessage(sageMessage *msg);
  int perfReport();
  int startPerformanceReport(sageMessage *msg);
  int stopPerformanceReport(sageMessage *msg);
  int shutdownApp(int appId);

  /**
   * sets tcpObj, udpObj and starts nwCheckThread() for each protocol
   */
  int initNetworks();

  /**
   * This function is called when EVENT_NEW_CONNECTION arrives<BR>
   * sageAudioCircBuffer is created and passed to sageAudioReceiver::sageAudioReceiver<BR>
   * sageAudioReceiver::addStream() is called for each element in the receiverList.<BR>
   * The first receiver object becomes Master
   *
   */
  int initStreams(char *msg, streamProtocol *nwObj);

  /**
   * calls sageAudioReceiver.processSync() for each element in the receiverList
   */
  int processSync(char *msg);
  int checkConnection();

  static void* nwCheckThread(void *args); /**< keeps checking connection, if one has found, then generates EVENT_NEW_CONNECTION */
  static void* msgCheckThread(void *args); /**< receives new message and generates EVENT_NEW_MESSAGE */
  static void* syncCheckThread(void *args); /**< calls sageSyncClient::waitForSync() then generates EVENT_SYNC_MESSAGE */
  static void* perfReportThread(void *args);
  static void* refreshThread(void *args);

public:
  sageAudioManager(int argc, char **argv);
  ~sageAudioManager();

  /**
   * creates sageAudioModule object and sageSyncClient object. calls sageSyncClient::connectToServer()<BR>
   * starts syncCheckThread() and refreshThread()
   */
  int init(char *msg);

  /**
   * keeps getting new event from the eventQueue then calls parseEvent()
   */
  void mainLoop();
};

#endif
