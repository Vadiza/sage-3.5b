/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlockPool.cpp
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

#include "sageBlockPool.h"
#include "sageBlock.h"

const int sageBlockGroup::PIXEL_DATA    = 1;
const int sageBlockGroup::CONFIG_UPDATE = 2;
const int sageBlockGroup::END_FRAME     = 3;

sageBlockGroup::sageBlockGroup(int blkSize, int grpSize, char opt) : frameSize(0),
                                                                     flag(sageBlockGroup::PIXEL_DATA), blockNum(0), frameID(0), refCnt(0), deRefCnt(0)
{
  blockSize = blkSize;
  int bufLen = grpSize / blkSize; // # of blocks in this group

  if (opt & GRP_CIRCULAR) {
    buf = new sageCircBufSingle(bufLen, true);
  }
  else {
    buf = new sageSerialBuf(bufLen);
  }

  if (opt & GRP_MEM_ALLOC) {
    for (int i=0; i<bufLen; i++) {
      sagePixelBlock *newBlock = new sagePixelBlock(blkSize);
      if (!newBlock) {
        SAGE_PRINTLOG("sageBlockGroup::sageBlockGroup : fail to create a new block");
        break;
      }

      newBlock->setGroup(this);

      if (!buf->pushBack((sageBufEntry)newBlock)) {
        SAGE_PRINTLOG("sageBlockGroup::sageBlockGroup : fail to add a block");
        break;
      }
    }
    memAlloc = true;
  }
  else {
    memAlloc = false;
  }

  if (opt & GRP_USE_IOV) {
#ifdef WIN32
    iovs = new WSABUF[bufLen+1];
#else
    iovs = new struct iovec[bufLen+1];
#endif
    if (memAlloc)
      genIOV();
  }
  else
    iovs = NULL;
}

bool sageBlockGroup::pushBack(sagePixelBlock* block)
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::pushBack : block buffer is NULL");
    return false;
  }

  if (!buf->pushBack((sageBufEntry)block)) {
    //SAGE_PRINTLOG("sageBlockGroup::pushBack : block buffer is full");
    return false;
  }

  return true;
}

sagePixelBlock* sageBlockGroup::front()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::front : block buffer is NULL");
    return NULL;
  }

  return (sagePixelBlock *)buf->front();
}

bool sageBlockGroup::next()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::next : block buffer is NULL");
    return false;
  }

  if (!buf->next())
    SAGE_PRINTLOG("sageBlockGroup::next : block buffer is empty");

  return true;
}

void sageBlockGroup::resetGrp()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::resetGrp : block buffer is NULL");
    return;
  }

  buf->reset();
}

void sageBlockGroup::resetIdx()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::resetIdx : block buffer is NULL");
    return;
  }

  buf->resetIdx();
}

bool sageBlockGroup::isEmpty()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::isEmpty : block buffer is NULL");
    return true;
  }

  return buf->isEmpty();
}

bool sageBlockGroup::isFull()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::isFull : block buffer is NULL");
    return false;
  }

  return buf->isFull();
}

int sageBlockGroup::size()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::size : block buffer is NULL");
    return -1;
  }

  return buf->size();
}

int sageBlockGroup::getBlockNum()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::getBlockNum : block buffer is NULL");
    return -1;
  }

  return buf->getEntryNum();
}

sagePixelBlock* sageBlockGroup::operator[] (int idx)
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::operator[] : block buffer is NULL");
    return NULL;
  }

  return (sagePixelBlock *)(*buf)[idx];
}

int sageBlockGroup::returnBlocks(sageBlockGroup* grp)
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::returnBlocks : block buffer is NULL");
    return -1;
  }

  int retVal = -1;

  for (int i=0; i<grp->buf->getEntryNum(); i++) {
    sagePixelBlock *pBlock = (sagePixelBlock *)(*grp->buf)[i];
    if (pBlock && pBlock->dereference() == 0) {
      if (!buf->pushBack((sageBufEntry)pBlock)) {
        retVal = i;
        break;
      }
    }
  }

  return retVal;
}

bool sageBlockGroup::setRefCnt()
{
  refCnt = 0;
  deRefCnt = 0;

  return true;
}

bool sageBlockGroup::genIOV()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::genIOV : block buffer is NULL");
    return false;
  }

  if (!iovs) {
    SAGE_PRINTLOG("sageBlockGroup::genIOV : iovec is not enabled");
    return false;
  }

#ifdef WIN32
  iovs[0].buf = header;
  iovs[0].len = GROUP_HEADER_SIZE;
#else
  iovs[0].iov_base = header;
  iovs[0].iov_len = GROUP_HEADER_SIZE;
#endif
  blockNum = buf->getEntryNum();
  for (int i=0; i<blockNum; i++) {
    sagePixelBlock *block = (sagePixelBlock *)(*buf)[i];
#ifdef WIN32
    iovs[i+1].buf = block->getBuffer();
    iovs[i+1].len = blockSize;
#else
    iovs[i+1].iov_base = block->getBuffer();
    iovs[i+1].iov_len = blockSize;
#endif
  }

  return true;
}

int sageBlockGroup::sendData(int sockFd)
{
  if (!iovs) {
    SAGE_PRINTLOG("sageBlockGroup::sendData : iovec is not enabled");
    return -1;
  }

  int sendSize = 0;
  sprintf(header, "%d %d %d", blockNum, frameID, configID);

  //SAGE_PRINTLOG("send %s", header);
  //for (int i=1; i<=blockNum; i++)
  //   SAGE_PRINTLOG("header %s", (char *)iovs[i].iov_base);

  int writtenBlocks = 0, bOffset = 0;
  int iovNum = blockNum+1;

  while (writtenBlocks < iovNum) {
    int writtenSize = 0;

#ifdef WIN32
    if (bOffset > 0) {
      iovs[writtenBlocks].buf = (char *)(iovs[writtenBlocks].buf) + bOffset;
      iovs[writtenBlocks].len -= bOffset;
    }

    DWORD WSAFlags = 0;
    ::WSASend(sockFd, iovs+writtenBlocks, iovNum-writtenBlocks, (DWORD*)&writtenSize,
              WSAFlags, NULL, NULL);

    if (bOffset > 0) {
      iovs[writtenBlocks].buf = (char *)(iovs[writtenBlocks].buf) - bOffset;
      iovs[writtenBlocks].len += bOffset;
    }
#else
    if (bOffset > 0) {
      iovs[writtenBlocks].iov_base = (char *)(iovs[writtenBlocks].iov_base) + bOffset;
      iovs[writtenBlocks].iov_len -= bOffset;
    }

    writtenSize = ::writev(sockFd, iovs+writtenBlocks, iovNum-writtenBlocks);

    if (bOffset > 0) {
      iovs[writtenBlocks].iov_base = (char *)(iovs[writtenBlocks].iov_base) - bOffset;
      iovs[writtenBlocks].iov_len += bOffset;
    }
#endif

    if (writtenSize <= 0)
      return -1;

    sendSize += writtenSize;
    bOffset = (sendSize-GROUP_HEADER_SIZE)%blockSize;
    if (sendSize < GROUP_HEADER_SIZE) {
      writtenBlocks = 0;
      bOffset = sendSize;
    }
    else
      writtenBlocks = 1 + (sendSize-GROUP_HEADER_SIZE)/blockSize;

    //SAGE_PRINTLOG("written blocks %d", writtenBlocks);
  }

  return sendSize;
}

int sageBlockGroup::readData(int sockFd)
{
  if (!iovs) {
    SAGE_PRINTLOG("sageBlockGroup::readData : iovec is not enabled");
    return -1;
  }

  int headerSize = 0, recvSize = 0;
  headerSize = sage::recv(sockFd, (void *)header, GROUP_HEADER_SIZE, MSG_PEEK);
  if (headerSize > 0)
    sscanf(header, "%d %d %d", &blockNum, &frameID, &configID);
  else
    return -1;

  //SAGE_PRINTLOG("iov num %d", blockNum+1);
  //for (int i=0; i<blockNum+1; i++)
  //  SAGE_PRINTLOG("iov size %s", iovs[i].iov_len);

  int readBlocks = 0, bOffset = 0;
  int iovNum = blockNum+1;

  while (readBlocks < iovNum) {
    int readSize = 0;

#ifdef WIN32
    if (bOffset > 0) {
      iovs[readBlocks].buf = (char *)(iovs[readBlocks].buf) + bOffset;
      iovs[readBlocks].len -= bOffset;
    }

    DWORD WSAFlags = 0;
    ::WSARecv(sockFd, iovs+readBlocks, iovNum - readBlocks, (DWORD*)&readSize, &WSAFlags, NULL, NULL);

    if (bOffset > 0) {
      iovs[readBlocks].buf = (char *)(iovs[readBlocks].buf) - bOffset;
      iovs[readBlocks].len += bOffset;
    }
#else
    if (bOffset > 0) {
      iovs[readBlocks].iov_base = (char *)(iovs[readBlocks].iov_base) + bOffset;
      iovs[readBlocks].iov_len -= bOffset;
    }

    readSize = ::readv(sockFd, iovs+readBlocks, iovNum - readBlocks);

    if (bOffset > 0) {
      iovs[readBlocks].iov_base = (char *)(iovs[readBlocks].iov_base) - bOffset;
      iovs[readBlocks].iov_len += bOffset;
    }
#endif

    if (readSize <= 0) {
      SAGE_PRINTLOG("sageBlockGroup::readData : error in reading io_vec");
      return -1;
    }

    recvSize += readSize;
    bOffset = (recvSize-GROUP_HEADER_SIZE)%blockSize;
    if (recvSize < GROUP_HEADER_SIZE) {
      readBlocks = 0;
      bOffset = recvSize;
    }
    else
      readBlocks = 1 + (recvSize-GROUP_HEADER_SIZE)/blockSize;

  } // end of while

  //SAGE_PRINTLOG("group %s data size %d", header, recvSize);
  //for (int i=1; i<=blockNum; i++)
  //SAGE_PRINTLOG("block header %s", (char *)iovs[i].iov_base);

  return recvSize;
}

int sageBlockGroup::sendDatagram(int sockFd)
{
  if (!iovs) {
    SAGE_PRINTLOG("sageBlockGroup::sendData : iovec is not enabled");
    return -1;
  }

  int sendSize = 0;
  sprintf(header, "%d %d %d", blockNum, frameID, configID);

  int iovNum = blockNum+1;

#ifdef WIN32
  DWORD WSAFlags = 0;
  ::WSASend(sockFd, iovs, iovNum, (DWORD*)&sendSize,
            WSAFlags, NULL, NULL);
#else
  sendSize = ::writev(sockFd, iovs, iovNum);
#endif

  return sendSize;
}

int sageBlockGroup::readDatagram(int sockFd)
{
  if (!iovs) {
    SAGE_PRINTLOG("sageBlockGroup::readDatagram : iovec is not enabled");
    return -1;
  }

  int recvSize = 0;
  blockNum = buf->getEntryNum();
  int dataSize = blockNum*blockSize + GROUP_HEADER_SIZE;

  while(recvSize < dataSize) {
    int headerSize = sage::recv(sockFd, (void *)header, GROUP_HEADER_SIZE, MSG_PEEK);
    if (headerSize > 0)
      sscanf(header, "%d %d %d", &blockNum, &frameID, &configID);
    else
      return -1;

    int iovNum = blockNum+1;
    dataSize = blockNum*blockSize + GROUP_HEADER_SIZE;

#ifdef WIN32
    DWORD WSAFlags = 0;
    ::WSARecv(sockFd, iovs, iovNum, (DWORD*)&recvSize, &WSAFlags, NULL, NULL);
#else
    recvSize = ::readv(sockFd, iovs, iovNum);
#endif

    if (recvSize <= 0) {
      SAGE_PRINTLOG("sageBlockGroup::readDatagram : error in reading io_vec");
      SAGE_PRINTLOG("iov_num %d frameID %d", iovNum, frameID);
      return -1;
    }
  }

  return recvSize;
}


/*
  void sageBlockGroup::clearHeaders()
  {
  if (!buf) {
  SAGE_PRINTLOG("sageBlockGroup : block buffer is NULL");
  return;
  }

  for (int i=0; i<blockNum; i++) {
  sagePixelBlock *block = (sagePixelBlock *)(*buf)[i];
  block->clearHeader();
  }
  }
*/

bool sageBlockGroup::updateConfig()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::updateConfig() : block buffer is NULL");
    return false;
  }

  if (blockNum > 0)
    flag = sageBlockGroup::PIXEL_DATA;
  else
    flag = sageBlockGroup::CONFIG_UPDATE;

  for (int i=0; i<blockNum; i++) {
    sagePixelBlock *block = (sagePixelBlock *)(*buf)[i];
    if (block)
      block->updateBlockConfig();
  }

  buf->setEntryNum(blockNum);

  return true;
}

void sageBlockGroup::clearBuffers()
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockGroup::clearBuffers() : block buffer is NULL");
    return;
  }

  for (int i=0; i<buf->size(); i++) {
    sagePixelBlock *block = (sagePixelBlock *)(*buf)[i];
    block->clearBuffer();
  }
}

void sageBlockGroup::clear()
{
  buf->setBlockingFlag(false);
  sagePixelBlock *pBlock;
  while (pBlock = front()) {
    delete pBlock;
    next();
  }
}

sageBlockGroup::~sageBlockGroup()
{
  if (buf) {
    if (memAlloc)
      clear();

    delete buf;
  }

  if (iovs)
    delete [] iovs;
}

sageBlockBuf::sageBlockBuf(int bufSize, int grpSize, int blkSize, char opt) :
  waitingData(false), frameInterval(100000), firstGroup(true)
{
  if (bufSize <= 0 || grpSize <= 0 || blkSize <= 0) {
    SAGE_PRINTLOG("sageBlockBuf::sageBlockBuf : error in buffer parameter");
    SAGE_PRINTLOG("buffer size : %d   group size : %d  block size : %d", bufSize,
                  grpSize, blkSize);
    return;
  }

  int blockNum = grpSize / blkSize; // number of blocks in a blockGroup

  int bufLen = bufSize / (blockNum * blkSize); // number of blockGroups in a bufSize

  int bScale = 1;

  //SAGE_PRINTLOG("buf size %d group size %d block size %d bufLen %d", bufSize, grpSize, blkSize, bufLen);

  if (opt & BUF_CTRL_GROUP) {
    ctrlPool = new sageCircBufSingle(bufLen, true);
    bScale = 2;
  }
  else
    ctrlPool = NULL;

  if (opt & BUF_MULTI_READER) {
    multiReader = true;
    dataPool = new sageCircBufSingle(bufLen, true);
    vacantPool = new sageRAB(bufLen);
    buf = new sageCircBufMulti(bufLen*bScale, true);
  }
  else {
    multiReader = false;
    dataPool = new sageCircBufSingle(bufLen, true);
    vacantPool = NULL;
    if (opt & BUF_BLOCKING_READ) {
      buf = new sageCircBufSingle(bufLen*bScale, true);
    }
    else {
      buf = new sageCircBufSingle(bufLen*bScale, false);
      waitingData = true;
    }
  }

  if (!buf)
    SAGE_PRINTLOG("sageBlockBuf::sageBlockBuf : fail to allocate block buffer");

  for (int i=0; i<bufLen; i++) {
    sageBlockGroup *newGrp;
    if (opt & BUF_MEM_ALLOC) {
      newGrp = new sageBlockGroup(blkSize, grpSize, GRP_MEM_ALLOC | GRP_USE_IOV);
      memAlloc = true;
    }
    else {
      newGrp = new sageBlockGroup(blkSize, grpSize, GRP_USE_IOV);
      memAlloc = false;
    }

    if (!newGrp) {
      SAGE_PRINTLOG("sageBlockBuf::sageBlockBuf : fail to create a new block group");
      break;
    }

    if (!dataPool->pushBack((sageBufEntry)newGrp)) {
      SAGE_PRINTLOG("sageBlockBuf::sageBlockBuf : fail to add a block group");
      break;
    }

    if (ctrlPool) {
      newGrp = new sageBlockGroup;
      if (!newGrp) {
        SAGE_PRINTLOG("sageBlockBuf::sageBlockBuf : fail to create a new control group");
        break;
      }

      if (!ctrlPool->pushBack((sageBufEntry)newGrp)) {
        SAGE_PRINTLOG("sageBlockBuf::sageBlockBuf : fail to add a ctrl group");
        break;
      }
    }
  }
}

bool sageBlockBuf::pushBack(sageBlockGroup* grp)
{
  if (buf) {
    if (multiReader)
      grp->setRefCnt();

    if (!buf->pushBack((sageBufEntry)grp)) {
      SAGE_PRINTLOG("sageBlockBuf::pushBack : error in pushing back to buffer");
      return false;
    }

    if (firstGroup) {
      frameTimer.reset();
      firstGroup = false;
    }

    return true;
  }

  SAGE_PRINTLOG("sageBlockBuf::pushBack : buf is NULL");
  return false;
}

int sageBlockBuf::findNextFrame(int id)
{
  if (multiReader) {
    sageCircBufMulti *mBuf = (sageCircBufMulti *)buf;
    if (!mBuf->isActive(id))
      return -1;
  }

  bool loop = true;
  sageBlockGroup *sbg = NULL;

  while (loop) {
    sbg = front(id);

    while(!sbg) {
      next(id);
      sbg = front(id);
    }

    if (sbg->getFlag() == sageBlockGroup::END_FRAME )
      loop = false;

    next(id);
  }

  sbg = front(id);
  int nextFrame = 1;
  if (sbg)
    nextFrame = sbg->getFrameID();

  return nextFrame;
}

int sageBlockBuf::addReader(streamerConfig &config, int frameID)
{
  if (!buf) {
    SAGE_PRINTLOG("sageBlockBuf::addReader : buffer is NULL");
    return -1;
  }

  if (!multiReader) {
    SAGE_PRINTLOG("sageBlockBuf::addReader : can't add reader to a single reader buffer");
    return -1;
  }

  int id = config.streamerID;

  sageCircBufMulti *mBuf = (sageCircBufMulti *)buf;

  mBuf->addReader(id);

  //SAGE_PRINTLOG("sageBlockBuf::%s() : mBuf->addReader() returned. id %d, tbn %d, current frameID %d\n", __FUNCTION__, id, totalBlockNum);

  int startFrame = 1;
  if (id > 0) {
    if ( config.asyncUpdate ) {
      startFrame = frameID;
    }
    else {
      startFrame = findNextFrame(id);
    }
  }

  //SAGE_PRINTLOG("sageBlockBuf::%s() returning startFrame %d\n", __FUNCTION__, startFrame);
  return startFrame;
}

sageBlockGroup* sageBlockBuf::front(int id, int minFrame)
{
  sageBlockGroup *sbg = NULL;

  if (multiReader) {
    sageCircBufMulti *mBuf = (sageCircBufMulti *)buf;
    sbg = (sageBlockGroup *)mBuf->front(id);
    if (!mBuf->isActive(id))
      return NULL;

    if (sbg && sbg->getFlag() == sageBlockGroup::PIXEL_DATA && sbg->getFrameID() < minFrame) {
      while(findNextFrame(id) < minFrame);
      sbg = (sageBlockGroup *)mBuf->front(id);
    }
  }
  else {
    sbg = (sageBlockGroup *)buf->front();

    if (!sbg)
      waitingData = true;
    else
      waitingData = false;
  }

  return sbg;
}

bool sageBlockBuf::next(int id)
{
  if (multiReader) {
    sageCircBufMulti *mBuf = (sageCircBufMulti *)buf;
    if (!mBuf->isActive(id))
      return false;

    sageBlockGroup *sbg = (sageBlockGroup *)mBuf->next(id);
    if (sbg) {
      returnBG(sbg);
    }

    return true;
  }

  return buf->next();
}

sageBlockGroup* sageBlockBuf::getFreeBlocks()
{
  // blocked if no block group is available
  sageBlockGroup* freeGroup = (sageBlockGroup *)dataPool->front();
  dataPool->next();
  return freeGroup;
}

sageBlockGroup* sageBlockBuf::getCtrlGroup(int flag)
{
  if (ctrlPool) {
    sageBlockGroup* ctrlGroup = (sageBlockGroup *)ctrlPool->front();
    ctrlPool->next();
    ctrlGroup->setFlag(flag);
    return ctrlGroup;
  }

  SAGE_PRINTLOG("sageBlockBuf::getCtrlGroup : control group pool is disabled");
  return NULL;
}

bool sageBlockBuf::finishFrame()
{
  if (ctrlPool) {
    sageBlockGroup* ctrlGroup = (sageBlockGroup *)ctrlPool->front();
    ctrlPool->next();
    ctrlGroup->setFlag(sageBlockGroup::END_FRAME);
    if (!buf->pushBack((sageBufEntry)ctrlGroup))
      SAGE_PRINTLOG("sageBlockBuf::finishFrame : error in pushing back to buffer");
    //SAGE_PRINTLOG("finish frame");

    frameCounter++;
    if (frameCounter.getValue() == INTERVAL_EVAL_COUNT) {
      frameInterval = frameTimer.getTimeUS() / 100;
      frameTimer.reset();
      frameCounter.reset();
    }

    return true;
  }

  SAGE_PRINTLOG("sageBlockBuf::finishFrame : control group pool is disabled");
  return false;
}

int sageBlockBuf::returnBG(sageBlockGroup* grp)
{
  if (!grp) {
    SAGE_PRINTLOG("sageBlockBuf::returnBG : returned group is NULL");
    return -1;
  }

  if (grp->getFlag() == sageBlockGroup::PIXEL_DATA) {
    if (multiReader && grp->getRefCnt() > 0) {
      if (!vacantPool->insert((sageBufEntry)grp)) {
        SAGE_PRINTLOG("sageBlockBuf::returnBG : error in pushing back to vacant pool");
        char info[TOKEN_LEN];
        getBufInfo(info);
        SAGE_PRINTLOG("%s", info);
      }
    }
    else {
      if (!dataPool->pushBack((sageBufEntry)grp))
        SAGE_PRINTLOG("sageBlockBuf::returnBG : error in pushing back to data pool");
    }
  }
  else {
    if (!ctrlPool->pushBack((sageBufEntry)grp))
      SAGE_PRINTLOG("sageBlockBuf::returnBG : error in pushing back to control pool");
  }

  return 0;
}

int sageBlockBuf::returnBlocks(sageBlockGroup* grp)
{
  if (!grp) {
    SAGE_PRINTLOG("sageBlockBuf : group pointer is NULL");
    return -1;
  }

  if (!vacantPool) {
    SAGE_PRINTLOG("sageBlockBuf : vacant block group pool is NULL");
    return -1;
  }

  for (int i=0; i<grp->getBlockNum(); i++) {
    sagePixelBlock *block = (*grp)[i];

    if (block) {
      //SAGE_PRINTLOG("return block");
      sageBlockGroup *parent = block->getGroup();
      if (parent && (parent->dereference() <= 0)) {
        //SAGE_PRINTLOG("return group");
        int prev = -1;
        for (int j=vacantPool->start(); j>=0; vacantPool->next(j)) {
          sageBlockGroup *sbg = (sageBlockGroup *)(*vacantPool)[j];
          if (sbg && (sbg == parent || sbg->getRefCnt() <= 0)) {
            //SAGE_PRINTLOG("move group");
            vacantPool->remove(j, prev);
            if (!dataPool->pushBack((sageBufEntry)sbg))
              SAGE_PRINTLOG("sageBlockBuf::garbageCollection : error in pushing back to data pool");
            break;
          }
          prev = j;
        }
      }
    }
  }

  return 0;
}

void sageBlockBuf::releaseLock()
{
  buf->releaseLock();
  if (dataPool)
    dataPool->releaseLock();
  if (vacantPool)
    vacantPool->releaseLock();
  if (ctrlPool)
    ctrlPool->releaseLock();
}

bool sageBlockBuf::clear(sageBuf *groupBuf)
{
  if (groupBuf) {
    groupBuf->setBlockingFlag(false);
    sageBlockGroup *sbg;
    while (sbg = (sageBlockGroup *)groupBuf->front()) {
      delete sbg;
      groupBuf->next();
    }

    return true;
  }

  return false;
}

void sageBlockBuf::getBufInfo(char *bufStatus)
{
  int ctrlStat = 0;
  int vacantStat = 0;

  if (ctrlPool)
    ctrlStat = ctrlPool->getStatus();

  if (vacantPool)
    vacantStat = vacantPool->getStatus();

  sprintf(bufStatus, "data buffer : %d% , data pool : %d% , ctrl pool : %d% , vacant pool : %d%",
          buf->getStatus(), dataPool->getStatus(), ctrlStat, vacantStat);
}

sageBlockBuf::~sageBlockBuf()
{
  if (clear(buf))
    delete buf;

  if (clear(dataPool))
    delete dataPool;

  if (clear(vacantPool))
    delete vacantPool;

  if (clear(ctrlPool))
    delete ctrlPool;
}
