/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: pixelDownloader.cpp - manaing each application instance of SAGE Receiver
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

#include "pixelDownloader.h"
#include "sageDisplay.h"
#include "sageSync.h"
#include "sageBlockPool.h"
#include "sageSharedData.h"
#include "sageBlock.h"
#include "sageEvent.h"
#include "sageBlockPartition.h"
#include "sageReceiver.h"

int montagePair::init(displayContext *context, sagePixFmt pfmt, int index, float depth, int winID, int async)
{
  asyncUpdate = async;

  montage = new sageMontage(context, pfmt);

  montage->otherMontage = montage;

  montage->winID = winID;

  setLocalTileIdx(index);
  setDepth(depth);

  return 0;
}

void montagePair::swapMontage()
{
  // when a window is moved or resized, the config of back montage is
  // updated immediately, the front montage is updated when it is swapped

  if (renewMontage) {
    montage->uploadTexture();
    renewMontage = false;
  }

  frontMon = 1 - frontMon;
}

int montagePair::deleteMontage()
{
  montage->deleteTexture();

  return 0;
}

int montagePair::setDepth(float depth)
{
  montage->depth = depth;

  return 0;
}

float montagePair::getDepth()
{
  return montage->depth;
}

int montagePair::setLocalTileIdx(int idx)
{
  montage->tileIdx = idx;

  return 0;
}

montagePair::~montagePair()
{
  delete montage;
}

pixelDownloader::pixelDownloader() : reportRate(1), updatedFrame(0), curFrame(0), recv(NULL),
                                     streamNum(0), bandWidth(0), montageList(NULL), configID(0), frameCheck(false),
                                     syncFrame(0), updateType(SAGE_UPDATE_FOLLOW), activeRcvs(0), passiveUpdate(false),
                                     dispConfigID(0), displayActive(false), status(PDL_WAIT_DATA), frameBlockNum(0), frameSize(0),
                                     m_initialized(false)
{
  perfTimer.reset();
  fromBridgeParallel = 0;

  //SAGE_PRINTLOG("PDL::%s() : has created.\n", __FUNCTION__);
}

int pixelDownloader::init(char *msg, dispSharedData *sh, streamProtocol *nwObj, bool sync, int sl)
{
  char *msgPt = sage::tokenSeek(msg, 3);
  sscanf(msgPt, "%d %d %d", &instID, &groupSize, &blockSize);

  int blockX, blockY, imgWidth, imgHeight;
  sagePixFmt pixFmt;

  int asyncUpdate = 0;

  msgPt = sage::tokenSeek(msg, 7);
  sscanf(msgPt, "%d %d %d %d %d %d %d", (int *)&pixFmt, &blockX, &blockY, &imgWidth, &imgHeight, &asyncUpdate, &fromBridgeParallel);

  //SAGE_PRINTLOG("\n\n %d \n\n", fromBridgeParallel);

  partition = new sageBlockPartition(blockX, blockY, imgWidth, imgHeight);
  if (!partition) {
    SAGE_PRINTLOG("[%d,%d] PDL::init() : unable to create block partition", shared->nodeID, instID);
    return -1;
  }
  partition->initBlockTable();

  shared = sh;
  syncOn = sync;
  syncLevel = sl;

  // how many tiles a node has
  tileNum = shared->displayObj->getTileNum();

  // montage INSTANTIATION
  montageList = new montagePair[tileNum];
  float depth = 1.0f - 0.01f*instID;

  for (int i=0; i<tileNum; i++) {
    montageList[i].init(shared->context, pixFmt, i, depth, instID, asyncUpdate);
  }

  configQueue.clear();

  //SAGE_PRINTLOG("pixelDownloader::init: app %dx%d block %dx%d bufsize %d", imgWidth, imgHeight, blockX, blockY, shared->bufSize);

  /*
   * Be aware, bufSize can be smaller than blockSize - Sungwon
   */
  shared->bufSize = imgWidth * imgHeight * 1 * getPixelSize(pixFmt); // One frames worth of block -- Luc
  if ( shared->bufSize < groupSize ) {
    SAGE_PRINTLOG("pixelDownloader::init: receiver buffer size can't be smaller than group size");

    /*
     * We need receiver buffer size >= group size
     * because receiver buffer is an array where each array element is a block group.
     * array length = buffer size / group size;
     */
    shared->bufSize = groupSize;
  }
  //SAGE_PRINTLOG("pixelDownloader::init: new receiver buffer size %d Byte", shared->bufSize);

  blockBuf = new sageBlockBuf(shared->bufSize, groupSize, blockSize, BUF_MEM_ALLOC | BUF_CTRL_GROUP);
  recv = new sagePixelReceiver(msg, (rcvSharedData *)shared, nwObj, blockBuf);

  shared->displayObj->updateAppDepth(instID, montageList[0].getDepth());
  m_initialized = true;

  //SAGE_PRINTLOG("PDL %d(of SDM %d)::%s() : returning.\n", instID, shared->nodeID, __FUNCTION__);

  return 0;
}

int pixelDownloader::addStream(int senderID)
{
  if (recv) {
    recv->addStream(senderID);
  }
  else {
    SAGE_PRINTLOG("pixelDownloader::addStream : receiver obj is NULL");
    return -1;
  }

  return 0;
}

int pixelDownloader::clearTile(int tileIdx)
{
  montagePair &monPair = montageList[tileIdx];
  sageMontage *mon = monPair.getFrontMon();
  shared->displayObj->removeMontage(mon);
  monPair.deactivate();

  return 0;
}

int pixelDownloader::clearScreen()
{
  for (int i=0; i<tileNum; i++) {
    clearTile(i);
  }

  return 0;
}

int pixelDownloader::swapMontages()
{
  bool activeMontage = false;

  for (int i=0; i<tileNum; i++) {
    montagePair &monPair = montageList[i];

    if (monPair.isActive()) {
      if (monPair.getClearFlag()) {
        clearTile(i);
      }
      else {
        monPair.swapMontage();
        shared->displayObj->replaceMontage(monPair.getFrontMon());
      }
      activeMontage = true;
    }
  }

  if (activeMontage) {
    shared->displayObj->setDirty();
  }

  return 0;
}

void pixelDownloader::processSync(int frame, int cmd)
{
  if (!syncOn)
    return;

  syncFrame = frame;
  //if (shared->nodeID == 0)
  //SAGE_PRINTLOG("receive sync %d", syncFrame);

  if (updatedFrame == syncFrame) {
    swapMontages();
  }
  else {
    bool screenUpdate = false;
    for (int i=0; i<tileNum; i++) {
      montagePair &monPair = montageList[i];
      if (monPair.getClearFlag()) {
        clearTile(i);
        screenUpdate = true;
      }
    }
    if (screenUpdate)
      shared->displayObj->setDirty();

    if (cmd == SKIP_FRAME) {
      updatedFrame = syncFrame;


    }
  }


  /**
   * SAGE display has two textures for each image fragment.
   One for display. The other to be written new pixels on.
   Two textures are swapped once a sync signal arrived
   and a new image shows up when the screen is refreshed.

   This only happens when the frame number of the image
   loaded on the new texture to be shown matches the sync
   frame number.

   It should match always for HARD-SYNC mode.
   But it may not for SOFT-SYNC mode
   for which sync frame can proceed even though some slaves
   are not ready for going to the next frame.
   But I can't guarantee the stability of the SOFT-SYNC mode
   of the current implementation (especially for parallel apps).
   CONSTANT-SYNC mode was not complete either.

   HARD-SYNC and NO-SYNC mode are mostly tested so far.
  */
}

int pixelDownloader::enqueConfig(char *data)
{
  char *configData = new char[strlen(data)+1];
  if (!configData) {
    SAGE_PRINTLOG("pixelDownloader::enqueConfig : unable to allocate memory");
    return -1;
  }

  strcpy(configData, data);
  configQueue.push_back(configData);

  return 0;
}

bool pixelDownloader::reconfigDisplay(int confID)
{
  if (dispConfigID >= confID) {
    SAGE_PRINTLOG("[%d,%d] PDL::reconfigDisplay(%d) : configuration ID error", shared->nodeID, instID, confID);
    return false;
  }

  char *configStr = NULL;
  while (dispConfigID < confID) {
    if (configQueue.size() == 0)
      return false;

    configStr = configQueue.front();
    dispConfigID++;
    configQueue.pop_front();
  }

  int oldRcvs = activeRcvs;
  displayActive = false;

  sageRotation orientation;
  sscanf(configStr, "%d %d %d %d %d %d", &windowLayout.x, &windowLayout.y,
         &windowLayout.width, &windowLayout.height, &activeRcvs, (int *)&orientation);
  windowLayout.setOrientation(orientation);

  // now update the drawObjects dependent on the application position
  /*shared->displayObj->updateAppBounds(instID, windowLayout.x, windowLayout.y,
    windowLayout.width, windowLayout.height,
    windowLayout.getOrientation());
  */
  // when the window locates on a neighbor tiled display
  if (windowLayout.width == 0 || windowLayout.height == 0) {
    if (syncOn) {
      for (int i=0; i<tileNum; i++)
        montageList[i].clear();
    }
    else {
      clearScreen();
      shared->displayObj->setDirty();
    }
    return true;
  }

  partition->setDisplayLayout(windowLayout);
  partition->clearBlockTable();

  for (int i=0; i<tileNum; i++) {
    montagePair &monPair = montageList[i];
    sageMontage *mon = NULL;

    sageRect tileRect = shared->displayObj->getTileRect(i);
    if (!tileRect.crop(windowLayout)) {
      if (syncOn)
        monPair.clear();
      else {
        clearTile(i);
        shared->displayObj->setDirty();
      }
      continue;
    }

    displayActive = true;

    partition->setTileLayout(tileRect);
    sageRect viewPort = partition->getViewPort();
    sageRect blockLayout = partition->getBlockLayout();

    viewPort.moveOrigin(blockLayout);

    if (monPair.isActive()) {
      //if (shared->nodeID == 5)
      //   std::cerr << "montage active" << std::endl;
      mon = monPair.getBackMon();
      *(sageRect *)mon = tileRect;
      mon->init(viewPort, blockLayout, orientation);
      monPair.renew();
    }
    else {
      //if (shared->nodeID == 5)
      //   std::cerr << "montage inactive" << std::endl;
      mon = monPair.getFrontMon();
      //*(sageRect *)mon = tileRect;
      //mon->init(viewPort, blockLayout, orientation);
      int monIdx = shared->displayObj->addMontage(mon);

      mon = monPair.getBackMon();
      *(sageRect *)mon = tileRect;
      mon->init(viewPort, blockLayout, orientation);
      mon->monIdx = monIdx;
      monPair.renew();

      monPair.activate();
    }

    partition->genBlockTable(i);
  }

  frameSize = blockSize * partition->tableEntryNum();

  if (oldRcvs != activeRcvs)
    updateType = SAGE_UPDATE_SETUP;

  return true;
}

int pixelDownloader::downloadPixelBlock(sagePixelBlock *block, montagePair &monPair)
{
  sageMontage *mon = monPair.getBackMon();

  mon->loadPixelBlock(block);
  //monPair.update();

  return 0;
}

int pixelDownloader::fetchSageBlocks()
{

  // fetch block data from the block buffer
  sageBlockGroup *sbg;

  // will use this instead of END_FRAME flag
  bool useLastBlock = true; // must be TCP
  bool proceedSwap = false;

  while (sbg = blockBuf->front()) {

    /**
     * the difference between updatedFrame and syncFrame should always 1
     * because the new frame it gets is always right next frame of current frame
     * otherwise, PDL should WAIT until others catch up
     *
     * This is the most important pre-requisite of the sync algorithm
     */
    if ( syncOn && (sbg->getFrameID() > syncFrame + 1) )   {
      status = PDL_WAIT_SYNC; // wait for others to catch up
      return status;
    }

    //SAGE_PRINTLOG("PDL %d(of SDM %d)::%s() : sbgFlag %d, sbgConfigID %d, sbgFrameID %d\n", instID, shared->nodeID, __FUNCTION__, sbg->getFlag(), sbg->getConfigID(), sbg->getFrameID());

    //
    // pixelReceiver received entire frame
    //
    /** lastblock used... what about UDP?
        if (sbg->getFlag() == sageBlockGroup::END_FRAME) { // END_FRAME flag is set at the sagePixelReceiver::readData()
        // now the most recent frame I got(curFrame) becomes updateFrame.
        // this means that because of swapMontages() curFrames will become front montage which means it can be displayed
        // therefore, it's updatedFrame
        updatedFrame = curFrame;

        //SAGE_PRINTLOG("SDM %d PDL %d END_FRAME, Frame %d, syncFrame %d\n", shared->nodeID, instID, updatedFrame, syncFrame);

        frameCounter++;

        // calculate packet loss
        packetLoss += frameSize-(frameBlockNum*blockSize);
        frameBlockNum = 0;

        if (syncOn) {
        //if (updatedFrame <= syncFrame) {
        //swapMontages();
        //}
        //else {
        if ( updatedFrame > syncFrame) {
        // must wait for others

        // frame, id, rcvNum, type
        // activeRcv is the number of receiver(number of SDM nodes)

        //SAGE_PRINTLOG("PDL::fetch() : SDM %d PDL %d beforeUpdate, Frame %d, syncFrame %d, activeRcvs %d\n", shared->nodeID, instID, updatedFrame, syncFrame, activeRcvs);

        #ifdef DELAY_COMPENSATION
        shared->syncClientObj->sendSlaveUpdateToBBS(updatedFrame, instID, activeRcvs, shared->nodeID, shared->latency, shared->current_max_inst_num);
        #else
        shared->syncClientObj->sendSlaveUpdateToBBS(updatedFrame, instID, activeRcvs, shared->nodeID, 0, shared->current_max_inst_num);
        #endif

        updateType = SAGE_UPDATE_FOLLOW;
        status = PDL_WAIT_SYNC;

        blockBuf->next();

        // if PIXEL_DATA, sbg is stored into blockBuf->dataPool
        // otherwise, blockBuf->ctrlPool
        blockBuf->returnBG(sbg);

        return status;
        }
        else {
        SAGE_PRINTLOG("\nPDL::fetchSageBlocks() : Fatal Error! SDM %d PDL %d, updatedFrame %d <= syncFrame %d\n", shared->nodeID, instID, updatedFrame, syncFrame);
        }
        }
        else {
        swapMontages();
        }
        } // end of if(END_FRAME)
    **/


    //
    // pixelReceiver received new CONFIG
    //
    if (sbg->getFlag() == sageBlockGroup::CONFIG_UPDATE) {

#ifdef DEBUG_PDL
      //SAGE_PRINTLOG("[%d,%d] PDL::fetch() : flag CONFIG_UPDATE, curFrame %d, updatedFrame %d, config %d\n", shared->nodeID, instID,curFrame, updatedFrame, configID);
#endif

      if (configID < sbg->getConfigID()) {
        if (reconfigDisplay(sbg->getConfigID())) {
          configID = sbg->getConfigID();

#ifdef DEBUG_PDL
          SAGE_PRINTLOG("[%d,%d] PDL::fetch() : CONFIG_UPDATE, curF %d, updF %d, cfg %d\n", shared->nodeID, instID,curFrame, updatedFrame, configID);
#endif
        }
        else {
          // config id is updated but didn't receive config information yet
          status = PDL_WAIT_CONFIG; // related with sageDisplayManager::updateDisplay()

#ifdef DEBUG_PDL
          SAGE_PRINTLOG("\t[%d,%d] waits for new config %d, PDL_WAIT_CONFIG.  current config %d\n", shared->nodeID, instID, sbg->getConfigID(), configID);
#endif

          return status;
        }
      }
      else {
#ifdef DEBUG_PDL
        SAGE_PRINTLOG("\tflag was CONFIG_UPDATE. but configID %d >= sbg->getConfigID() %d\n", configID, sbg->getConfigID());
#endif
      }
    }

    //
    // continuous next frame received (it means this node was displaying this app already)
    //
    else if (sbg->getFlag() == sageBlockGroup::PIXEL_DATA && sbg->getFrameID() > updatedFrame) {
#ifdef DEBUG_PDL
      //SAGE_PRINTLOG("[%d,%d] PDL::fetch() : new PIXEL_DATA; sbg->getFrameID() %d > updatedFrame %d\n", shared->nodeID, instID, sbg->getFrameID(), updatedFrame);
#endif
      // see if config changed
      if (configID < sbg->getConfigID()) {
        if (reconfigDisplay(sbg->getConfigID())) {
          configID = sbg->getConfigID();
#ifdef DEBUG_PDL
          SAGE_PRINTLOG("[%d,%d] PDL::fetch() : PIXEL_DATA with new Config %d; curFrame is now %d\n", shared->nodeID, instID, sbg->getConfigID(), sbg->getFrameID());
#endif
        }
        else {
          status = PDL_WAIT_CONFIG;
#ifdef DEBUG_PDL
          SAGE_PRINTLOG("[%d,%d] recondigDisplay(%d) returned 0. wait for new config. will PDL_WAIT_CONFIG current cfgID %d\n", shared->nodeID, instID, sbg->getConfigID(), configID);
#endif
          return status;
        }
      }

      bandWidth += sbg->getDataSize() + GROUP_HEADER_SIZE;
      curFrame = sbg->getFrameID();
      frameBlockNum += sbg->getBlockNum();

      //SAGE_PRINTLOG("Numblocks: %d",sbg->getBlockNum());
      for (int i=0; i<sbg->getBlockNum(); i++) {
        sagePixelBlock *block = (*sbg)[i];

        if (!block)
          continue;

        //SAGE_PRINTLOG("block header %s", (char *)block->getBuffer());

        blockMontageMap *map = (blockMontageMap *)partition->getBlockMap(block->getID());
        int bx = block->x, by = block->y;

        while(map) {
          block->translate(map->x, map->y);
          //SAGE_PRINTLOG("block montage %d id %d pos %d , %d", map->infoID, block->getID(), block->x, block->y);
          downloadPixelBlock(block, montageList[map->infoID]);
          block->x = bx;
          block->y = by;
          map = (blockMontageMap *)map->next;
        }
      } // end of foreach block

      if ( recv->getSenderNum() == 1  &&  !fromBridgeParallel) {
        if ( partition && frameBlockNum >= partition->tableEntryNum() ) { // whole frame received
          useLastBlock = true; // setting flag for swapMontages to be executed, since END_FRAME
          proceedSwap = true;
#ifdef DEBUG_PDL
          //SAGE_PRINTLOG("\t\t[%d,%d] The Last block group for frame %d received. frameBlockNum %d, LastBlockNum %d\n", shared->nodeID, instID, fnum, frameBlockNum, lastBlockNum);
#endif
        }
        else {
          useLastBlock = false;
        }
      }

      //SAGE_PRINTLOG("\t[%d,%d] FBN %d, actualFBN %d, LBN %d\n", shared->nodeID, instID, frameBlockNum, actualFrameBlockNum, lastBlockNum);
    }

    // if UDP, lastBlock checking method could fail
    else if (sbg->getFlag() == sageBlockGroup::END_FRAME) { // END_FRAME flag is set at the sagePixelReceiver::readData()
      //SAGE_PRINTLOG("[%d,%d] PDL::fetch() : END_FRAME; updF %d, curF %d, syncF %d\n", shared->nodeID, instID, updatedFrame, curFrame, syncFrame);

      if ( updatedFrame == curFrame ) {
        // already swapMontage-ed
        // if syncOn then, updatedF == curF == synchF
      }
      else if ( updatedFrame > curFrame ) {
        // something is badly wrong
        SAGE_PRINTLOG("[%d,%d] PDL::fetch() : END_FRAME flag!!! FATAL_ERROR!!! curF %d, updF %d, syncF %d\n", shared->nodeID, instID, curFrame, updatedFrame, syncFrame);
      }
      else {
#ifdef DEBUG_PDL
        SAGE_PRINTLOG("[%d,%d] PDL::fetch() : END_FRAME flag!!! Before proceeding, curF %d, updF %d, syncF %d\n", shared->nodeID, instID, curFrame, updatedFrame, syncFrame);
#endif
        proceedSwap = true;
        useLastBlock = false;
      }
    }

    //
    // unexpected case
    //
    else {
      //SAGE_PRINTLOG("\n[%d,%d] PDL::fetch() : invalid block order.",shared->nodeID, instID);
#ifdef DEBUG_PDL
      SAGE_PRINTLOG("\tcurF %d, updF %d, syncF %d, curF %d, cfgID %d\n",curFrame, updatedFrame, syncFrame, configID);
      SAGE_PRINTLOG("\tsbg->getFlag() %d, sbg->getFrameID() %d, sbg->getConfigID() %d\n", sbg->getFlag(), sbg->getFrameID(), sbg->getConfigID());
#endif
    }


    // to fix END_FRAME recognition.
    // Originally, frame n is recognized as complete (END_FRAME) when blocks of frame n+1 is received
    // This causes frame being displayed is always behind actual config ->  config l is applied but frame l-1 is displayed
    if ( proceedSwap ) {
      proceedSwap = false;

      // now the most recent frame I got(curFrame) becomes updateFrame.
      // this means that because of swapMontages() curFrames will become front montage which means it can be displayed
      // therefore, it's updatedFrame
      updatedFrame = curFrame;
#ifdef DEBUG_PDL
      //if (useLastBlock)
      //SAGE_PRINTLOG("[%d,%d] PDL::fetch() : !!! ProceedSwap !!! using LastBlock fBN %d of %d; updF %d, syncF %d, cfID %d\n", shared->nodeID, instID, frameBlockNum, partition->tableEntryNum(), updatedFrame, syncFrame, configID);
      //else
      //SAGE_PRINTLOG("[%d,%d] PDL::fetch() : !!! ProceedSwap !!! using END_FRAME fBN %d of %d; updF %d, syncF %d, cfID %d\n", shared->nodeID, instID, frameBlockNum, partition->tableEntryNum(), updatedFrame, syncFrame, configID);
#endif
      frameCounter++;

      // calculate packet loss
      packetLoss += frameSize-(frameBlockNum*blockSize);
      frameBlockNum = 0; //reset
      //actualFrameBlockNum = 0;

      if (syncOn) {
        if ( updatedFrame > syncFrame) {
          // if this is the case, I'm too fast. I must wait for others

#ifdef DELAY_COMPENSATION
          //shared->syncClientObj->sendSlaveUpdateToBBS(updatedFrame, instID, activeRcvs, shared->nodeID, shared->latency);
#else
          if ( syncLevel == -1 ) {
            shared->syncClientObj->sendSlaveUpdate(updatedFrame, instID, activeRcvs, updateType);
          }
          else {
            shared->syncClientObj->sendSlaveUpdateToBBS(updatedFrame, instID, activeRcvs, shared->nodeID, 0);
          }
#endif
          updateType = SAGE_UPDATE_FOLLOW;
          status = PDL_WAIT_SYNC;

          blockBuf->next();
          blockBuf->returnBG(sbg);

          return status;
        }
        else if ( updatedFrame == syncFrame ) {
#ifdef DEBUG_PDL
          SAGE_PRINTLOG("\nPDL::fetch() : [%d,%d] updatedFrame == synchFrame %d, don't we need swapMontages() ? \n", syncFrame);
#endif
          //swapMontages();
        }
        else {
          SAGE_PRINTLOG("\nPDL::fetch() : [%d,%d] FatalError! updF %d , syncF %d\n", shared->nodeID, instID, updatedFrame, syncFrame);
        }
      }
      else {
#ifdef DEBUG_PDL
        //SAGE_PRINTLOG("[%d,%d] PDL::fetch() : NO_SYNC; swapMont() frame %d, config %d\n\n", shared->nodeID, instID, updatedFrame, configID);
#endif
        swapMontages();
      }
    } // end of if(isLastBlock)





    blockBuf->next();
    blockBuf->returnBG(sbg);




  } // end of while(blockBuf->front())

  status = PDL_WAIT_DATA;
  //SAGE_PRINTLOG("exit fetch");

  //SAGE_PRINTLOG("\nPDL::fetch() returning with PDL_WAIT_DATA. curFrame %d, updatedFrame %d, configID %d\n", curFrame, updatedFrame, configID);

  return status;
}

int pixelDownloader::evalPerformance(char **frameStr, char **bandStr)
{
  //Calculate performance here
  double elapsedTime = perfTimer.getTimeUS();

  if (elapsedTime > 1000000.0*reportRate && reportRate > 0) {
    *bandStr = new char[TOKEN_LEN];

    float obsBandWidth = (float) (bandWidth * 8.0 / (elapsedTime));
    float obsLoss = (float) (packetLoss * 8.0 / (elapsedTime));
    bandWidth = 0;
    packetLoss = 0;
    sprintf(*bandStr, "%d %7.2f %7.2f %d", instID, obsBandWidth, obsLoss, frameSize);

    if (displayActive) {
      *frameStr = new char[TOKEN_LEN];
      float frameRate = (float) (frameCounter.getValue()*1000000.0/elapsedTime);
      frameCounter.reset();
      sprintf(*frameStr, "%d %f", instID, frameRate);
    }

    perfTimer.reset();
  }

  return 0;
}

int pixelDownloader::setDepth(float depth)
{
  for (int i=0; i<tileNum; i++) {
    montageList[i].setDepth(depth);
  }

  // now update the drawObjects dependent on the application position
  shared->displayObj->updateAppDepth(instID, depth);

  return 0;
}

pixelDownloader::~pixelDownloader()
{
  for (int i=0; i<tileNum; i++) {
    montagePair &monPair = montageList[i];
    sageMontage* mon = monPair.getFrontMon();
    shared->displayObj->removeMontage(mon);
    monPair.deleteMontage();
  }

  shared->displayObj->setDirty();

  delete [] montageList;
  delete recv;
  delete blockBuf;

  for (int i=0; i<configQueue.size(); i++) {
    char *configData = configQueue.front();
    configQueue.pop_front();
    delete [] configData;
  }
}
