/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: pixelDownloader.h
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

#ifndef PIXEL_DOWNLOADER_H_
#define PIXEL_DOWNLOADER_H_

#include "sage.h"
#include "sageSync.h"


#include "sageEvent.h"

class pixelDownloader;
class dispSharedData;
class sageMontage;
class sageBlockBuf;
class sagePixelBlock;
class displayContext;
class sageBlockPartition;
class sagePixelReceiver;
class streamProtocol;

/**
 * class montagePair
 */
class montagePair {
private:
  sageMontage *montage;
  int frontMon;
  bool renewMontage;
  bool active;
  bool clearFlag;
  int asyncUpdate; /**< if staticApp, back montage won't be created */

public:
  montagePair() : renewMontage(false), frontMon(0), active(false),
                  clearFlag(false) {}
  int init(displayContext *context, sagePixFmt pfmt, int index, float depth, int winID, int asyncUpdate);
  inline sageMontage* getFrontMon() { return montage; }
  inline sageMontage* getBackMon() { return montage; }
  int setDepth(float depth);
  float getDepth();
  int setLocalTileIdx(int idx);

  /**
   * when a window is moved or resized, the config of back montage is updated immediately,<BR>
   * the front montage is updated when it is swapped
   */
  void swapMontage();

  int deleteMontage();
  inline bool isActive()  { return active; }
  inline void activate()  { active = true, clearFlag = false; }
  inline void deactivate(){ active = false, clearFlag = false; }
  inline void clear()     { clearFlag = true; }
  inline bool getClearFlag() { return clearFlag; }

  inline void renew()     { renewMontage = true; }
  inline bool isRenewed() { return renewMontage; }

  ~montagePair();
};

#define PDL_WAIT_CONFIG 1
#define PDL_WAIT_DATA   2
#define PDL_WAIT_SYNC   3

/**
 * \brief class pixelDownloader (per application). It reads pixel data from buffer and downloads into texture memory.
 */
class pixelDownloader {
public:
  int instID; /**< application ID */
protected:
  int streamNum;
  int tileNum; /**< how many tiles a single node has ? */
  int updatedFrame, curFrame, syncFrame;
  int configID, dispConfigID;
  int groupSize, blockSize;
  dispSharedData *shared; /**< dispSharedData *. This is assigned at the sageDisplayManager */
  bool frameCheck;
  bool syncOn; /**< whether we ensure sync b/w tiles or not */
  int syncLevel;
  bool displayActive;
  int  status; /**< PDL_WAIT_CONFIG 1, PDL_WAIT_DATA 2, and PDL_WAIT_SYNC 3*/
  bool m_initialized;

  sagePixelReceiver *recv; /**< sagePixelReceiver */
  sageBlockBuf *blockBuf; /**< sageBlockBuf */
  montagePair *montageList;

  sageCounter frameCounter;
  int reportRate;
  sageTimer perfTimer;
  unsigned long bandWidth;
  unsigned packetLoss;
  int frameBlockNum;
  int frameSize;

  int fromBridgeParallel;

  sageBlockPartition *partition;
  sageRect windowLayout;

  /**
   * how many sageReceiver involves ?
   */
  int activeRcvs;

  /**
   * SAGE_UPDATE_SETUP is set when activeRcvs has changed <BR>
   * SAGE_UPDATE_FOLLOW is set right after sendSlaveUpdate() is called <BR>
   */
  int updateType;
  bool passiveUpdate;

  std::deque<char *> configQueue;

  //int sendPerformanceInfo();
  int downloadPixelBlock(sagePixelBlock *block, montagePair &monPair);
  int clearTile(int tileIdx);

  /**
   * for each tileNum, clearTile() is called
   */
  int clearScreen();
  int setupSyncInfo(sagePixelBlock *block);

public:
  /**
   * starts with updateType = SAGE_UPDATE_FOLLOW, status = PDL_WAIT_DATA
   */
  pixelDownloader();

  /**
   * creates montagePair, sageBlockPartition, sageBlockBuf, sagePixelReceiver objects<BR>
   * this is invoked by initStreams in the sageDisplayManager (when EVENT_NEW_CONNECTION)
   */
  int init(char *msg, dispSharedData *sh, streamProtocol *nwObj, bool sync, int syncLevel);

  /**
   * Called when EVENT_READ_BLOCK, EVENT_SYNC_MESSAGE, and RCV_UPDATE_DISPLAY<BR>
   * This function is the one which calls sageSyncClient::sendSlaveUpdate()<BR>
   * When syncOn is true and updateFrame > curFrame<BR>
   * <BR>
   * It first fetches block data from the block buffer by calling sageBlockBuf.front()<BR>
   * then calculates packet loss.. then..
   * ..
   * ..
   *
   */
  int fetchSageBlocks();

  /**
   * It looks like this function is responsible for syncing b/w tiles<BR>
   * The first argument frame is the frame that we want to sync to<BR>
   * <BR>
   * if this syncSlave's updated frame is equal to the sync frame(argument),
   * then it swaps texture (swapMontage()) => HARD_SYNC
   */
  void processSync(int frame, int cmd = 0);

  int enqueConfig(char *data);
  int setDepth(float depth);

  /**
   * just calls sagePixelReceiver::addStream()<BR>
   * called by sageDisplayManager::initStream()
   */
  int addStream(int senderID);

  /**
   * When a window is moved or resized, the config of back montage is
   * updated immediately, the front montage is updated when it is swapped.
   */
  int swapMontages();
  bool reconfigDisplay(int id);
  int evalPerformance(char **frameStr, char **bandStr); /**< evaluate performance */
  inline void setReportRate(int rate) { reportRate = rate; }
  inline void resetTimer() { perfTimer.reset(); }
  inline void resetBandWidth() { bandWidth = 0; }
  inline void resetFrame() { frameCounter.reset(); }
  inline int getInstID() { return instID; }
  inline int getStatus() { return status; }
  bool isInitialized(void) { return m_initialized; }

  ~pixelDownloader();
};

#endif
