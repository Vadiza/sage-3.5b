/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlockPartition.cpp - handling block partition info
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

#include "sageBlockPartition.h"

sageBlockPartition::sageBlockPartition(int bw, int bh, int iw, int ih) : blockWidth(bw),
                                                                         blockHeight(bh), entryNum(0)
{
  x = y = 0;
  width = iw;
  height = ih;
  rowNum = (int)ceil((float)height/blockHeight);
  colNum = (int)ceil((float)width/blockWidth);
  totalBlockNum = rowNum*colNum;
  blockTable = NULL;
}

void sageBlockPartition::initBlockTable()
{
  blockTable = new pixelBlockMap*[totalBlockNum];
  for (int i=0; i<totalBlockNum; i++)
    blockTable[i] = NULL;
}

void sageBlockPartition::getBlock(int id, sagePixelBlock &block)
{
  if (id > totalBlockNum)
    SAGE_PRINTLOG("sageBlockPartition::getBlock : block ID %d is out of scope", id);

  int row = id/colNum;
  int col = id%colNum;
  block.x = col*blockWidth;
  block.y = row*blockHeight;
  block.width = blockWidth;
  block.height = blockHeight;
  block.setID(id);
}

void sageBlockPartition::setViewPort(sageRect &rect)
{
  viewPort = rect;
  vOffsetX = viewPort.x/blockWidth;
  vOffsetY = viewPort.y/blockHeight;
  blockLayout.x = vOffsetX*blockWidth;
  blockLayout.y = vOffsetY*blockHeight;
  int rightBound = (int)ceil((float)(viewPort.x + viewPort.width)/blockWidth)*blockWidth;
  int topBound = (int)ceil((float)(viewPort.y + viewPort.height)/blockHeight)*blockHeight;
  blockLayout.width = rightBound - blockLayout.x;
  blockLayout.height = topBound - blockLayout.y;
  vRowNum = blockLayout.height/blockHeight;
  vColNum = blockLayout.width/blockWidth;
  blockNum = vRowNum * vColNum;
}

int sageBlockPartition::getVisibleBlockID(int idx)
{
  if (idx >= blockNum) {
    SAGE_PRINTLOG("sageBlockPartition::getVisbleBlockID : index is out of scope");
    return -1;
  }

  int row = idx/vColNum;
  int col = idx%vColNum;
  int blockID = (vOffsetY+row)*colNum + vOffsetX+col;

  return blockID;
}

// get blocks within or on the edges of the viewport
// blocks on the edges are cropped
void sageBlockPartition::getVisibleBlock(int idx, sagePixelBlock &block)
{
  int blockID = getVisibleBlockID(idx);
  if (blockID >= 0) {
    getBlock(blockID, block);
    block.crop(viewPort);
  }
}

void sageBlockPartition::adjustBlockCoord(sagePixelBlock &block)
{
  sagePixelBlock stdBlock;
  getBlock(block.getID(), stdBlock);
  block.moveOrigin((sageRect &)stdBlock);
}

void sageBlockPartition::genBlockTable(int monIdx)
{
  for (int i=0; i<blockNum; i++) {
    blockMontageMap *map = new blockMontageMap;

    map->blockID = getVisibleBlockID(i);
    map->infoID = monIdx;

    sagePixelBlock stdBlock;
    getBlock(map->blockID, stdBlock);
    stdBlock.moveOrigin(blockLayout);
    map->x = stdBlock.x;
    map->y = stdBlock.y;

    insertBlockMap(map);
  }
}

int sageBlockPartition::setStreamInfo(int infoID, sageRect &window)
{
  setViewPort(window);
  int mapNum = 0;

  for (int i=0; i<blockNum; i++) {
    pixelBlockMap *map = new pixelBlockMap;
    map->blockID = getVisibleBlockID(i);
    map->infoID = infoID;
    if (insertBlockMap(map))
      mapNum++;
  }

  return mapNum;
}

int sageBlockPartition::setStreamInfo(int infoID, int begin, int end)
{
  if (begin > end) {
    SAGE_PRINTLOG("sageBlockPartition::setStreamInfo : invalid block ID scope");
    return -1;
  }

  begin = MIN(totalBlockNum, begin);
  end = MIN(totalBlockNum, end);

  int mapNum = 0;
  for (int i=begin; i<=end; i++) {
    pixelBlockMap *map = new pixelBlockMap;
    map->blockID = i;
    map->infoID = infoID;
    if (insertBlockMap(map))
      mapNum++;
  }

  return mapNum;
}

bool sageBlockPartition::insertBlockMap(pixelBlockMap *map)
{
  if (!map) {
    SAGE_PRINTLOG("sageBlockPartition::insertBlockMap : block map is NULL");
    return false;
  }

  int index = map->blockID;
  if (index >= totalBlockNum) {
    SAGE_PRINTLOG("sageBlockPartition::insertBlockMap : blockID is out of scope");
    return false;
  }

  if (!blockTable) {
    SAGE_PRINTLOG("blockTable is not initialized");
    return false;
  }

  if (blockTable[index] == NULL) {
    blockTable[index] = map;
    blockTable[index]->count = 1;
    entryNum++;
  }
  else {
    pixelBlockMap *iter = blockTable[index];
    if (iter->infoID == map->infoID) {
      delete map;
      return false;
    }

    while(iter->next) {
      iter = iter->next;
      if (iter->infoID == map->infoID) {
        delete map;
        return false;
      }
    }
    iter->next = map;
    blockTable[index]->count++;
  }

  return true;
}

pixelBlockMap* sageBlockPartition::getBlockMap(int blockID)
{
  if (blockID >= totalBlockNum) {
    SAGE_PRINTLOG("sageBlockPartition::getBlockMap : blockID %d is out of scope", blockID);
    return NULL;
  }

  if (!blockTable) {
    SAGE_PRINTLOG("blockTable is not initialized");
    return NULL;
  }

  return blockTable[blockID];
}

void sageBlockPartition::clearBlockTable()
{
  if (!blockTable) {
    SAGE_PRINTLOG("blockTable is not initialized");
    return;
  }

  for (int i=0; i<totalBlockNum; i++) {
    pixelBlockMap *iter = blockTable[i];
    while(iter) {
      pixelBlockMap *map = iter;
      iter = iter->next;
      delete map;
    }
    blockTable[i] = NULL;
  }

  entryNum = 0;
}

void sageBlockPartition::setTileLayout(sageRect tileRect)
{
  tileRect.normalize(displayLayout);
  sageRect renderingLayout = (*(sageRect*)this);
  sageRect imgRect = renderingLayout*tileRect;
  setViewPort(imgRect);
}

// get block position in block layout
void sageBlockPartition::getBlockPosition(sagePixelBlock &block)
{
  sagePixelBlock stdBlock;
  getBlock(block.getID(), stdBlock);
  stdBlock.moveOrigin(blockLayout);
  block.translate(stdBlock);
}
