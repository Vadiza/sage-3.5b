/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageReceiver.cpp
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

#include "sageReceiver.h"
#include "streamProtocol.h"
#include "sageBlock.h"
#include "sageBlockPool.h"
#include "sageSharedData.h"
#include "sageEvent.h"

void* sageReceiver::nwReadThread(void *args)
{
  sageReceiver *This = (sageReceiver *)args;

  This->readData();

  pthread_exit(NULL);
  return NULL;
}

sagePixelReceiver::sagePixelReceiver(char *msg, rcvSharedData *sh,
                                     streamProtocol *obj, sageBlockBuf *buf)
{
  char *msgPt = sage::tokenSeek(msg, 3);
  sscanf(msgPt, "%d %d %d %d", &instID, &groupSize, &blockSize, &senderNum);

  nwObj = obj;
  shared = sh;
  blockBuf = buf;
  FD_ZERO(&streamFds);
  maxSockFd = 0;
  streamList = new streamData[senderNum];
  streamIdx = 0;
  configID = 0;
  curFrame = 1;

  connecting = true;

  pthread_mutex_init(&streamLock, NULL);
  pthread_mutex_unlock(&streamLock);
  pthread_cond_init(&connectionDone, NULL);

  if (pthread_create(&thId, 0, nwReadThread, (void*)this) != 0) {
    SAGE_PRINTLOG("sagePixelReceiver : can't create network reading thread");
  }
}

int sagePixelReceiver::addStream(int senderID)
{
  if (streamIdx >= senderNum) {
    SAGE_PRINTLOG("sagePixelReceiver::addStream : stream number exceeded correct number");
    return -1;
  }

  streamList[streamIdx].senderID = senderID;
  streamList[streamIdx].dataSockFd = nwObj->getRcvSockFd(senderID);
  maxSockFd = MAX(maxSockFd, streamList[streamIdx].dataSockFd);
  FD_SET(streamList[streamIdx].dataSockFd, &streamFds);

  streamIdx++;

  if (streamIdx == senderNum) {
    pthread_mutex_lock(&streamLock);
    connecting = false;
    pthread_cond_signal(&connectionDone);
    pthread_mutex_unlock(&streamLock);

    shared->eventQueue->sendEvent(EVENT_APP_CONNECTED, instID);
  }

  return streamIdx;
}

int sagePixelReceiver::checkStreams()
{
  fd_set sockFds = streamFds;
  int retVal = select(maxSockFd+1, &sockFds, NULL, NULL, NULL);
  if (retVal <= 0) {
    SAGE_PRINTLOG("sagePixelReceiver::checkStreams : error in stream checking");
    return -1;
  }

  for (int i=0; i<senderNum; i++) {
    if (FD_ISSET(streamList[i].dataSockFd, &sockFds)) {
      streamList[i].dataReady = true;
    }
  }

  return 0;
}

int sagePixelReceiver::readData()
{
  if (!shared) {
    SAGE_PRINTLOG("sagePixelReceiver::readData : shared object is null");
    return -1;
  }

  if (!nwObj)  {
    SAGE_PRINTLOG("sagePixelReceiver::readData : network object is null");
    return -1;
  }

  if (!blockBuf)  {
    SAGE_PRINTLOG("sagePixelReceiver::readData : block buffer is null");
    return -1;
  }

  pthread_mutex_lock(&streamLock);
  if (connecting)
    pthread_cond_wait(&connectionDone, &streamLock);
  pthread_mutex_unlock(&streamLock);

  bool reuseBlockGroup = false;
  bool updated = false;

  sageBlockGroup *sbg = NULL;

  while(!endFlag) {
    if (checkStreams() < 0)
      return -1;

    //SAGE_PRINTLOG("pt4");

    int nextFrame = SAGE_INT_MAX;

    for (int i=0; i<senderNum; i++) { // if parallel app, senderNum > 1
      if (streamList[i].dataReady) {
        if (!reuseBlockGroup) {

          // retrieve free space for a group
          sbg = blockBuf->getFreeBlocks(); // dataPool->front(); dataPool->next()
          if (!sbg) {
            endFlag = true;
            return -1;
          }
        }
        else {
          reuseBlockGroup = false;
        }

        int rcvSize = nwObj->recvGrp(streamList[i].senderID, sbg); // sageBlockGroup::readData()
        streamList[i].dataReady = false;

        if (rcvSize > 0) {
          //
          // What I received is continuous data of the current frame
          //
          if (sbg->getFrameID() == curFrame) {

            // and it is PIXEL
            if (sbg->getFlag() == sageBlockGroup::PIXEL_DATA) {
              blockBuf->pushBack(sbg); // the sbg will be stored in 'buf'

              // Aug 2009 BJ fix (cube rev 7167)
              if (curFrame == 1 && configID == 0)
                configID = sbg->getConfigID();

              //std::cerr << "sagePixelReceiver::readData() : pushback a group with frame id " << sbg->getFrameID() << std::endl;

              updated = true;
              if (blockBuf->isWaitingData()) {
                //SAGE_PRINTLOG("\t and sending EVENT_READ_BLOCK to inst %d as well", instID);
                shared->eventQueue->sendEvent(EVENT_READ_BLOCK, instID);
              }
            }
            else {
              /* SDM must receive CONFIG_UPDATE sbg when an app disappeared from this screen
               * so that the app screen can be cleared
               */
              reuseBlockGroup = true;
              if ( sbg->getConfigID() > configID ) {
                //SAGE_PRINTLOG("[%d,%d] sagePixelReceiver::%s() : new CONFIG_UPDATE sbg arrived. new configID %d\n", shared->nodeID, instID, __FUNCTION__, sbg->getConfigID());
                configID = sbg->getConfigID();
                sageBlockGroup *sibal = blockBuf->getCtrlGroup(sageBlockGroup::CONFIG_UPDATE);
                assert(sibal);
                sibal->setFrameID(sbg->getFrameID());
                sibal->setConfigID(configID);
                blockBuf->pushBack(sibal);
              }
              else {
                streamList[i].bGroup = NULL;
              }

              // cube rev 7167
              streamList[i].curFrame = sbg->getFrameID()+1;
              FD_CLR(streamList[i].dataSockFd, &streamFds);
            }
          }

          //
          // received new frame or next frame
          //
          else if (sbg->getFrameID() > curFrame) {
            if (sbg->getFlag() == sageBlockGroup::PIXEL_DATA) {

              //std::cerr << "sagePixelReceiver::readData() : received groupd with next frame id " << sbg->getFrameID() << std::endl;

              streamList[i].bGroup = sbg; // replace to new frame (new group)
              configID = MAX(configID, sbg->getConfigID()); // config may have been changed in new frame
            }
            else {
              // this isn't PIXEL_DATA -> clear screen should occur
              // config is updated and screen is cleared

              //SAGE_PRINTLOG( "[%d,%d] sagePixelReceiver::%s() : new/next frame of CONFIG_UPDATE. configID %d\n", shared->nodeID, instID, __FUNCTION__, sbg->getConfigID());

              reuseBlockGroup = true;
              if (sbg->getConfigID() > configID) {
                configID = sbg->getConfigID();
                streamList[i].bGroup = blockBuf->getCtrlGroup(sageBlockGroup::CONFIG_UPDATE);
                streamList[i].bGroup->setFrameID(sbg->getFrameID());
                streamList[i].bGroup->setConfigID(configID);
              }
              else {
                streamList[i].bGroup = NULL;
              }
            }
            streamList[i].curFrame = sbg->getFrameID();

            // in the case of multisender; receiver should wait for slow ones. without this statement, select() will keep return
            FD_CLR(streamList[i].dataSockFd, &streamFds);
          }

          //
          // something's wrong
          //
          else {
            SAGE_PRINTLOG("sagePixelReceiver::readData() : blocks arrived out of order. frame ID I receved %d, but curFrame is %d", sbg->getFrameID(), curFrame);
          }
        }
        else {
          // nwObj->recvGrp(streamList[i].senderID, sbg) return 0 or less

          //SAGE_PRINTLOG("sagePixelReceiver::readData() : nwObj->recvGrp(stream %d) returned %d\n", i, rcvSize);
          endFlag = true;
          break;
        }
      }

      nextFrame = MIN(nextFrame, streamList[i].curFrame);
    } // end of for each sender




    if (nextFrame < SAGE_INT_MAX && nextFrame > curFrame) {
      // then this is new frame. This is how sage differentiate the next frame
      //SAGE_PRINTLOG("sagePixelReceiver::readData() : Receiving Done ! SDM %d PDL %d, curFrame(%d) will be updated to nextFrame(%d)\n", shared->nodeID, instID, curFrame,nextFrame);

      curFrame = nextFrame;
      int pushNum = 0;
      if (updated) {
        // this means that receiver keeps getting pixel data

        //SAGE_PRINTLOG("sagePixelReceiver::readData() : marking END_FRAME\n");

        blockBuf->finishFrame(); // enqueue END_FRAME flag to the blockBuf
        pushNum++;
        if (blockBuf->isWaitingData())
          shared->eventQueue->sendEvent(EVENT_READ_BLOCK, instID);

        updated = false;
      }

      for (int i=0; i<senderNum; i++) {
        // set it again
        FD_SET(streamList[i].dataSockFd, &streamFds);
        sageBlockGroup *bGrp = streamList[i].bGroup;

        if (bGrp && bGrp->getFrameID() == curFrame) {
          if (bGrp->getFlag() == sageBlockGroup::PIXEL_DATA)
            updated = true;
          blockBuf->pushBack(bGrp); // push temporary group to real queue
          //SAGE_PRINTLOG("push back frame %d", bGrp->getFrameID());
          pushNum++;
        }
      }

      if (blockBuf->isWaitingData())
        shared->eventQueue->sendEvent(EVENT_READ_BLOCK, instID);
    } // end if
  } // end while(endFlag)

  //SAGE_PRINTLOG("sagePixelReceiver::readData() : exit reading thread");

  return 0;
}

sagePixelReceiver::~sagePixelReceiver()
{
  endFlag = true;
  blockBuf->releaseLock();

  for (int i=0; i<senderNum; i++) {
    if (nwObj)
      nwObj->close(streamList[i].senderID);
  }

  pthread_join(thId, NULL);

  delete [] streamList;
  SAGE_PRINTLOG("<sagePixelReceiver shutdown>");
}
