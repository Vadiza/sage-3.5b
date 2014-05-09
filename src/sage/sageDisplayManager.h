/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDisplayManager.h
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
 * Direct questions, comments etc about SAGE to sage_users@listserv.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#ifndef sageDisplayManager_H_
#define sageDisplayManager_H_

#include "fsClient.h"
#include "streamProtocol.h"

//#define DISPLAY_REFRESH_INTERVAL 8333   // 1000000us/120Hz = 8333
#define DISPLAY_REFRESH_INTERVAL 16666   // 1000000us/60Hz = 16666
//#define DISPLAY_REFRESH_INTERVAL 33333   // 30 Hz
//#define DISPLAY_REFRESH_INTERVAL 20000   // 1000000us/50Hz
#define DISPLAY_MAX_FRAME_RATE   MAX_FRAME_RATE

class sageDisplayManager;
class sageEvent;
class sageReceiver;
class sageTcpModule;
class sageUdpModule;
class dispSharedData;
class pixelDownloader;


class sageSyncBBServer;
class sageSyncServer;

typedef struct {
  sageDisplayManager *This;
  streamProtocol     *nwObj;
} nwCheckThreadParam;

/**
 * class sageDisplayManager
 */
class sageDisplayManager : public fsClient {
private:
  /**
   * This is the pointer that points eventQueue and syncClientObject which are shared to downloaderList<BR>
   * pixelDownloader also has pointer to sagePixelReceiver and it's created when pixelDownloader is created.<BR>
   * so, sageDisplayManger, pixelDownloader, and sagePixelReceiver all can access same dispSharedData
   */
  dispSharedData *shared;
  sageNwConfig nwCfg;
  sageTcpModule *tcpObj;
  sageUdpModule *udpObj;


  bool syncMaster; /**< if syncMaster then syncBBServerObj is created */
  sageSyncBBServer *syncBBServerObj; /**< instantiated if syncMaster is true */
  sageSyncServer *syncServerObj;

  int syncPort; /**< syncServer's port */
  int syncBarrierPort; /**< syncMaster's barrier port (2nd phase) */
  int syncRefreshRate; /**< This is important parameter. It determines how frequent BBS should broadcast in Hz */
  int syncMasterPollingInterval; /**< select return timer in usec */
  int syncLevel;

  //sageReceiver *receiverList[MAX_INST_NUM];
  std::vector<pixelDownloader *> downloaderList; /**< pixelDownloader object for each application */
  std::vector<char *> reconfigStr;

  int streamPort;
  int totalRcvNum; /**< total number of node in the system */
  bool rcvEnd;
  int displayID; /**< looks like ]always zero */

  /**
   * called by the mainLoop().<BR>
   * It parses sageEvent then calls appropriate functions.
   */
  int parseEvent(sageEvent *event);

  /**
   * called by parseEvent().<BR>
   * It parses sageMessage then calls appropriate functions.
   */
  int parseMessage(sageMessage *msg);
  int perfReport(); /**< calls pixelDownloader::evalPerformance() for each element in the downloaderList */
  int startPerformanceReport(sageMessage *msg);
  int stopPerformanceReport(sageMessage *msg);
  int changeBGColor(sageMessage *msg);
  int changeDepth(sageMessage *msg);

  /**
   * if appId is -1 then all applications are closed
   *
   * @param appId integer
   * @return 0 on success, -1 otherwise
   */
  int shutdownApp(int appId);

  /**
   * creates sageTcpModule, sageUdpModule, and generates nwCheckThread
   */
  int initNetworks();

  /**
   * creates pixelDownloader object if it wasn't created for this instID before.<BR>
   * If pixelDownloader object already exist then just pixelDownloader::addStream() is called.<BR>
   * pixelDownloader (per application) reads pixel data from buffer and downloads into texture memory.<BR>
   * if stream type is SAGE_BLOCK_NO_SYNC then only pixelDownloader::init() is called.<br>
   * if stream type is SAGE_BLOCK_XXX_SYNC then a syncGroup object is created and syncGroup::init() is called followed by pixelDownloader::init()<br>
   * If syncMaster is true then the newly created syncGroup object is then added by syncServerObj->addSyncGroup() <br>
   * finally, pixelDownloader::addStream() is called and this pixelDownloader instance is added into the array downloaderList
   *
   * @param msg char *
   * @param nwObj streamProtocol *
   * @return 0
   */
  int initStreams(char *msg, streamProtocol *nwObj);

  /**
   * this function is called when EVENT_SYNC_MESSAGE arrives in the parseEvent() in this class. This event is generated in the syncCheckThread() which is started in the init()<BR>
   * It finds syncGroup id and syncframe from the msg
   * then calls pixelDownloader::processSync() of an element in the downloaderList array<BR>
   * if status of this group is PDL_WAIT_SYNC then, also calls pixelDownloader.fetchSageBlocks()
   */
  //int processSync(char *msg);
  int processSync(sageEvent *e);



  /**
   * it's called when a message RCV_UPDATE_DISPLAY is received from fsManager
   */
  int updateDisplay(char *msg);
  int clearDisplay(int instID);

  char CLEAR_STR[TOKEN_LEN];

  /**
   * keeps checking network connection and generates EVENT_NEW_CONNECTION event if new connection has arrived.
   */
  static void* nwCheckThread(void *args);

  /**
   * keeps checking message from fsManager(?) by calling fsClient::rcvMessageBlk()<BR>
   * generates EVENT_NEW_MESSAGE
   */
  static void* msgCheckThread(void *args);

  /**
   * This thread creates sageEvent type syncEvent (EVENT_SYNC_MESSAGE) if sageSyncClient::waitForSync() is returned.<BR>
   * In other words, if it gets sync message from the syncMaster then it generates EVENT_SYNC_MESSAGE
   */
  static void* syncCheckThread(void *args);

  /**
   * the constructor starts this thread.<br>
   * It keeps calling perfReport()
   */
  static void* perfReportThread(void *args);

  /**
   * started by pthread_create in the init()<BR>
   * It keeps sending EVENT_REFRESH_SCREEN event. it causes calling sageDisplay::updateScreen()
   * refer DISPLAY_REFRESH_INTERVAL
   */
  static void* refreshThread(void *args);

  pixelDownloader* findApp(int id, int& index);

public:
  /**
   * if syncMaster is true, sageSyncServer object is created here, and sageSyncServer::init() is called.<BR>
   * if globalSync(which is argv[6]) is true, then both shared->nodeID and displayID must be zero to syncMaster be true.<BR>
   * Also starts msgCheckThread() and perfReportThread().
   */
  sageDisplayManager(int argc, char **argv);
  ~sageDisplayManager();

  /**
   * sageSyncClient object is created here.<BR>
   * It starts syncCheckThread() and refreshThread()<BR>
   * Also, sageDisplay is instantiated here. It draws pixel data onto screen, and all OpenGL related code resides in the sageDisplay
   */
  int init(char *msg);

  /**
   * It keeps getting an event from the eventQueue, and caliing parseEvent() with new event until rcvEnd is true
   */
  void mainLoop();
};

#endif
