/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageSync.h
 * Author : Byungil Jeong, Rajvikram Singh,
 * Description : This file includes classes to synchronize multiple groups
 *               of synchronization slaves in the SAGE processes and a class
 *               to synchronize multiple threads in a SAGE process.
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

#ifndef _SAGESYNC_H
#define _SAGESYNC_H

#include "sageBase.h"
#include <list>
#include <map>
#include <bitset>

#define SAGE_SYNC_MSG_LEN  1280 // must be equal to SAGE_EVENT_SIZE
#define MAX_SYNC_GROUP     100
#define SYNC_MSG_BUF_LEN   64

#define SAGE_UPDATE_SETUP    1
#define SAGE_UPDATE_FOLLOW   2
#define SAGE_UPDATE_FRAME    3
#define SAGE_UPDATE_AUDIO    4

#define SAGE_CONSTANT_SYNC   1
#define SAGE_ASAP_SYNC_HARD  2
#define SAGE_ASAP_SYNC_SOFT  3

#define NORMAL_SYNC          1
#define SKIP_FRAME           2

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT  0
#endif

/**
 * class syncSlaveData.
 */
class syncSlaveData {
public:

  /**
   * a syncClient's socket file descriptor
   */
  int            clientSockFd;

  /**
   * another socket for refresh barrier - 2nd phase
   */
  int barrierClientSockFd;

  /**
   * a syncClient's IP address
   */
  sockaddr_in      clientAddr;


  /**
   * Sungwon
   * this will be filled in sageSyncServer::syncServerThread() when the object is created
   */
  int SDM; // sungwon

  /**
   * frame number.
   */
  int            frame;

  /**
   * Constructor.
   * When a syncClient calls sageSyncClinet::connectToServer(), sageSyncServer::syncServerThread() accepts it<BR>
   * then the thread creates syncSlaveData object for the client.
   */
  syncSlaveData() : frame(0), SDM(-1) {}
};

//forward declarations
class sageSyncClient;
class sageSyncServer;

#define MAX_INTERVAL_ERROR 0.1
#define SYNC_TIMEOUT       1667   /**< 1sec/60 * 10% = 1667 us */

/**
 * class syncGroup.
 */
class syncGroup {
protected:
  int noOfUpdates; /**< how many displays are updated ? */
  int slaveNum; /**< how many syncSlaves are involved in this sync group */
  pthread_mutex_t   *syncMsgLock;
  std::deque<char *> syncMsgQueue;

  int id, policy, curFrame, audioFrame, videoFrame, keyFrame, skipFrame, audioSyncCnt;

  /**
   * minimum sync signal interval<BR>
   * Even though a sync group is ready to proceed to the next frame,<BR>
   * the sync master waits until the minimum interval is reached.
   */
  double interval;

  double timeError;
  bool syncEnd;

  /**
   * when sageSyncServer::addSyncGroup() is called, the sync server object calling addSyncGroup() is assigned.
   */
  sageSyncServer *syncServer;
  pthread_t threadID;
  sageTimer timer;
  pthread_mutex_t  syncLock;
  pthread_cond_t   resumeSync; /**< condition variable */

  bool holdSync; /**< if true, then it will make checkHold() to wait on condition resumeSync */
  bool waitingInterval; /**< whether syncGroup is waiting to reach the interval */
  bool waitForKeyFrame;

public:
  /**
   * Constructor.
   */
  syncGroup() : id(0), noOfUpdates(0), curFrame(0), slaveNum(0), syncEnd(false), interval(0.0),
                policy(SAGE_ASAP_SYNC_SOFT), videoFrame(0), audioFrame(-1), keyFrame(100), holdSync(false),
                timeError(0.0), waitingInterval(false), waitForKeyFrame(false), skipFrame(0), audioSyncCnt(0)
  { syncMsgQueue.clear(); }

  int init(int startFrame, int _policy_, int groupID, int frameRate = 1, int sNum = 1); /**< syncGroup::init() */
  int setFrameRate(float frameRate);

  /**
   * TRUE, if this syncGroup reached the interval
   */
  bool checkInterval();

  /**
   * IMPORTANT<BR>
   * This function is called by sageSyncServer::manageUpdate()<BR>
   *
   * HARD_SYNC implemented<BR>
   * A syncGroup maintains the number of syncSlaves. Whenever slave sends an update, noOfUpdates increases<BR>
   * HARD_SYNC ensures every display for an app can maintain same frame number<BR>
   * by updated frame can be propagated only when noOfUpdates == number of slaves<BR>
   * i.e. all syncSlaves for an application should sends udpate
   *
   * @return 0 or NORMAL_SYNC
   */
  int processUpdate(char *data);

  int enqueSyncMsg(char *msg); /**< enqueue sync message<BR>thread safe*/
  char* dequeSyncMsg(); /**< dequeue sync message<BR>thread sage*/
  int getSyncID() { return id; }
  bool checkTimeOut();
  void blockSync(); /**< sets holdSync = true */
  void unblockSync(); /**< sets holdSync = false */

  /**
   * waits on condition resumeSync if holdSync is true
   */
  void checkHold();
  inline bool isWaiting() { return waitingInterval; }

  friend class sageSyncServer;
  friend class sageSyncClient;
};

/**
 * class sageSyncBBServer
 */
class sageSyncBBServer {
protected:
  int serverSockFd; /**< syncServer's socket */
  int maxSyncGroupID;
  int maxSlaveSockFd;
  bool syncEnd;

  /**
   * -1 old sync
   *  0 no sync
   *  1 data sync only
   *  2 two phase
   *  3 one phase
   */
  int syncLevel;

  sockaddr_in serverAddr;
  fd_set slaveFds;

  pthread_t syncThreadID, syncServerThreadID;

  std::vector<syncSlaveData> syncSlaves; /**< a list of sync slaves */

  /**
   * a list of sync slaves
   * key is SDM id
   * value is its data(mainly socket)
   */
  std::map<int,syncSlaveData> syncSlavesMap;

  sockaddr_in barrierServerAddr;
  int barrierServerSockFd;
  int barrierPort;

  fd_set barrierSlaveFds;
  int maxBarrierSlaveSockFd;

  pthread_t syncBarrierServerThreadID;
  static void* syncBarrierServerThread(void *);

  /**
   * waits syncClient's connection by calling accept() syscall<br>
   * if one connects, then it creates syncSlaveData object for it, and adds it to the vector syncSlaves
   */
  static void* syncServerThread(void*);

  /**
   * This thread is created whenever addSyncGroup() is called and under some condition.<BR>
   * 1. when the policy is SAGE_CONSTANT_SYNC<BR>
   * 2. 1 is not true and asapSyncGroupNum == 0<BR>
   * <BR>
   * if syncGroup's policy is SAGE_CONSTANT_SYNC, then this thread periodicaly sends sync(sendSync()) signal to its syncGroup.<BR>
   * Otherwise it sends sync signal(by calling manageUpdate()) only when syncClient asks ??<BR>
   * The period is defined in the syncGroup.interval
   */
  static void* mainLoopThread(void*);

  sageTimer timer;

public:
  /**
   *  Constructor. Called in sageDisplayManager::sageDisplayManager() and sail::init()
   */
  sageSyncBBServer(int syncLevel = 2);
  ~sageSyncBBServer();

  /**
   * creates syncServer socket, and starts syncServerThread
   */
  int init(int port);

  /**
   * create barrierServerSockFd
   */
  int initBarrier(int barrierPort);

  int totalRcvNum;
  int refreshInterval;
  int syncMasterPollingInterval;
  int startManagerThread(int totalRcvNum, int refreshInterval, int syncMasterPollingInterval);

  void killAllClients();  /**< closes all the open client sockets */
  int checkTimeOut();
};



















/**
 * class sageSyncServer. It is created when syncMaster == true
 */
class sageSyncServer {
protected:
  int serverSockFd; /**< syncServer's socket */
  int asapSyncGroupNum, maxSyncGroupID;
  int maxSlaveSockFd;
  bool syncEnd;

  sockaddr_in serverAddr;
  fd_set slaveFds;

  pthread_t   syncThreadID, syncServerThreadID;
  pthread_t   groupThreadID;

  std::vector<syncSlaveData> syncSlaves; /**< a list of sync slaves */

  /**
   * key - sdm id
   */
  std::map<int,syncSlaveData> syncSlavesMap;


  //syncGroup *syncGroupArray[MAX_SYNC_GROUP]; /**< an array of sync groups */
  std::vector<syncGroup *> syncGroupArray; /**< an array of sync groups */

  /**
   * waits syncClient's connection by calling accept() syscall<br>
   * if one connects, then it creates syncSlaveData object for it, and adds it to the vector syncSlaves
   */
  static void* syncServerThread(void*);

  /**
   * This thread is created whenever addSyncGroup() is called and under some condition.<BR>
   * 1. when the policy is SAGE_CONSTANT_SYNC<BR>
   * 2. 1 is not true and asapSyncGroupNum == 0<BR>
   * <BR>
   * if syncGroup's policy is SAGE_CONSTANT_SYNC, then this thread periodicaly sends sync(sendSync()) signal to its syncGroup.<BR>
   * Otherwise it sends sync signal(by calling manageUpdate()) only when syncClient asks ??<BR>
   * The period is defined in the syncGroup.interval
   */
  static void* managerThread(void*);
  static void* managerConstantThread(void*);

  /**
   * It seems that each syncGroup has corresponding managerThread.<BR>
   *
   * This function waits this syncServer object's syncSlaves' update from socket connection<BR>
   * then, extracts and identifies groupID from a slave's message<BR>
   * then, calls corresponding syncGroup's syncGroup::processUpdate()<BR>
   * If syncGroup's processUpdate() returns NORMAL_SYNC<BR>
   * then, syncGroup::checkInterval(), syncGroup::checkHold(), syndSync() could be called.
   *
   * respond with sync signal ( sendSync() ) when a syncClient sends sageSyncClient::sendSlaveUpdate().
   */
  int manageUpdate();

  /**
   * sends sync message (group's current frame: grp->curFrame) to the syncSlaves.<BR>
   * sync message includes grp->id, grp->curFrame, dataLen, cmd<BR>
   * if grp->syncGroup::dequeSyncMsg() returns data then sends the data as well
   */
  int sendSync(syncGroup *grp, int cmd = NORMAL_SYNC);
  sageTimer timer;

  syncGroup* findSyncGroup(int id, int& index);

public:
  /**
   *  Constructor. Called in sageDisplayManager::sageDisplayManager() and sail::init()
   */
  sageSyncServer();
  ~sageSyncServer();

  /**
   * creates syncServer socket, and starts syncServerThread
   */
  int init(int port);

  /**
   * Adds syncGroup into syncGroupArray and creates corresponding managerThread() if group's policy is SAGE_CONSTANT_SYNC<BR>
   * or policy is not SAGE_CONSTANT_SYNC and asapSyncGroupNum == 0<BR>
   * triggered by sageDisplayManager::initStreams() and sail
   */
  int addSyncGroup(syncGroup *grp);

  int removeSyncGroup(int id);

  void killAllClients();  /**< closes all the open client sockets */
  int checkTimeOut();
}; // End of sageSyncServer

class sageCircBufSingle;

/**
 * class syncMsgStruct
 */
class syncMsgStruct {
public:
  int frameID;
  char *data;

  syncMsgStruct() : frameID(0), data(NULL) {}
  syncMsgStruct(int size) : frameID(0) { data = new char[size]; }
  ~syncMsgStruct() { if (data) delete [] data; }
};

/**
 * class sageSyncClient
 */
class sageSyncClient {
private:
  int syncLevel;

  int clientSockFd;

  int barrierClientSockFd;
  int refreshBarrierDeltaT;

  int maxGroupID;
  //sageCircBufSingle *syncMsgBuf[MAX_SYNC_GROUP];
  std::vector<sageCircBufSingle *> syncMsgBuf;
  bool syncEnd;
  pthread_t syncThreadID;

  /**
   * keeps trying to receive message from syncServer by calling readSyncMsg()<BR>
   * started by addSyncGroup()
   */
  static void* syncClientThread(void *args);

  /**
   * receive messages from sageSyncServer::sendSync()<BR>
   * receives message(syncgroupID, frameNum, dataLen) from syncServer by calling sage::recv() and reads sync data.<BR>
   * This function is continuously called by syncClientThread().
   */
  int readSyncMsg();

  sageCircBufSingle* findSyncGroup(int id, int& index);

public:
  /**
   * creates socket(clientSockFd) and sets TCP_NODELAY, SO_OOBINLINE socket option
   */
  sageSyncClient(int syncLevel = -1);

  /**
   * shuts down the socket, and calls pthread_join
   */
  ~sageSyncClient();

  /**
   * connects to a syncServer
   *
   * @param syncServerIP a char *
   * @param port an integer
   * @return -1 on error, 0 otherwise
   */
  int connectToServer(char *serverIP, int port, int SDMnum=-1);

  int connectToBarrierServer(char *serverIP, int port, int SDMnum=-1);


  /**
   * creates sageCircBufSingle object, assigns it into syncMsgBuf array<BR>
   * starts syncClientThread() if no other syncgroup was added.<BR>
   * So, syncClientThread() is started when the first syncgroup is added.
   *
   * @param id an integer, sync group id
   * @return -1 on error, 0 otherwise
   */
  int addSyncGroup(int id);


  /**
   * removes element(id) from syncMsgBuf array
   *
   * @param id an integer
   * @return -1 on error, 0 otherwise
   */
  int removeSyncGroup(int id);


  /**
   * sends update msg to the syncServer.<BR>
   * sageSyncServer::manageUpdate() will respond to this.<BR>
   *
   * Triggered by below events and message<BR>
   * EVENT_READ_BLOCK (pixelDownloader::fetchSageBlocks()) <BR>
   * EVENT_SYNC_MESSAGE (sageDisplayManager::processSync() -> pixelDownloader::fetchSageBlocks()) <BR>
   * RCV_UPDATE_DISPLAY (sageDisplayManager::updateDisplay() -> pixelDownloader::fetchSageBlocks()) <BR>
   * <BR>
   * in the sageAudioManager::initStream()<BR>
   * in the sageAudioCircBuf::updateReadIndex() <- (sageAudioReceiver::readData(), sageAudioStreamer::streamLoop())<BR>
   *
   *
   * @param frame an integer frame number
   * @param id an integer, default 0
   * @param rcvNum an integer, default 0
   * @param type an integer, default SAGE_UPDATE_FOLLOW
   * @return -1 on error, 0 otherwise
   */
  int sendSlaveUpdate(int frame, int id = 0, int rcvNum = 0, int type = SAGE_UPDATE_FOLLOW, int nodeID=-1);

  /**
   * when a PDL received new frame (END_FRAME flag) it reports to the syncMaster before doing swapMontage()
   */
  int sendSlaveUpdateToBBS(int frame, int id = 0, int rcvNum = 0, int SDMnum = -1, int delayCompenLatency=0);

  /** THE FINAL CASE , sageSync_theFinal.cpp */
  int sendRefreshBarrier(int nodeID);
  int recvRefreshBarrier(bool nonblock=false); // block or nonblock
  /** end THE FINAL CASE */

  /**
   * receives sync message without additional data
   *
   * @param msg a char *
   * @return -1 on error, 0 otherwise
   */
  int waitForSync(char* msg, int len = -1);

  /**
   * It uses MSG_PEEK to find out the length of sync message
   *
   * @param void
   * @return -1 on error, message size otherwise
   */
  int waitForSyncPeek();

  /**
   * receives sync message with group ID and additional data
   *
   * @param id an integer
   * @return syncMsg, NULL on error
   */
  syncMsgStruct* waitForSync(int id);

  /**
   * receives sync message with additional data
   *
   * @param data a char *
   * @return -1 on error, 0 otherwise
   */
  int waitForSyncData(char* &data);


  /**
   * ch
   */
  int checkSync(char* &msg);
}; // End of tvSyncClient


/**
 * For synchronization among master and slave threads
 */
class sageThreadSync {
private:
  pthread_mutex_t   *slaveLock;
  pthread_mutex_t   *masterLock;
  int slaveNum;

public:
  sageThreadSync(int sNum = 1);
  ~sageThreadSync();

  int synchronize(int rank = 0);
};

#endif
