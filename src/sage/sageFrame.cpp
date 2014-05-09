/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageFrame.cpp - functions manipulate image frames in SAGE
 * Author : Byungil Jeong, Luc Renambot
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
#include "sageFrame.h"
#include "streamInfo.h"
#include "sageBlockPartition.h"

sageBlockFrame::sageBlockFrame(int w, int h, int bytes, float compX, float compY)
  : blocks(NULL)
{
  width = w;
  height = h;
  bytesPerPixel = bytes;

  compressX = compX;
  compressY = compY;

  initBuffer();
}

int sageBlockFrame::initFrame(sageBlockPartition *part)
{
  partition = new sageBlockPartition(*part);
  partition->setViewPort(*this);
  pixelSize = (int)ceil(bytesPerPixel/compressX);
  memWidth = width*pixelSize;

  return 0;
}

bool sageBlockFrame::extractPixelBlock(sagePixelBlock *block, int rowOrder)
{
  if (!block) {
    SAGE_PRINTLOG("sageBlockFrame::extractPixelBlock : block is NULL");
    //return false;
  }

  partition->getVisibleBlock(idx, *block);
  sageRect blockRect = *block;
  blockRect.moveOrigin(*this);

  char *blockAddr;
  int yPos = 0;
  //std::cerr << "block addr " << blockRect.y*memWidth + blockRect.x*pixelSize << std::endl;
  if (rowOrder == BOTTOM_TO_TOP)
    yPos = (int)ceil(blockRect.y/compressY);
  else
    yPos = (int)ceil((height-1-blockRect.y)/compressY);

  blockAddr = pixelData + yPos*memWidth + blockRect.x*pixelSize;

  char *blockBuf = block->getPixelBuffer();

  int srcHeight = (int)ceil(block->height/compressY);
  int srcWidth = block->width*pixelSize;

  for (int i=0; i<srcHeight; i++) {
    memcpy(blockBuf, blockAddr, srcWidth);
    blockBuf += srcWidth;
    if (rowOrder == BOTTOM_TO_TOP) {
      blockAddr += memWidth;
    }
    else {
      blockAddr -= memWidth;
    }
  }

  partition->adjustBlockCoord(*block);

  idx++;

  if (idx == partition->getBlockNum()) {
    resetBlockIndex();
    return false;  // finish extraction of a frame
  }

  return true;  // continue extraction
}

int sageBlockFrame::generateBlocks(int rowOrd)
{
  return 0;
}

int sageBlockFrame::generateSubFrame(sageRect &rect, sageSubFrame &s)
{
  return 0;
}

sageBlockFrame::~sageBlockFrame()
{
  if (blocks) {
    delete [] blocks;
  }

  releaseBuffer();
}

/*
  int sageUdpFrame::computeBlockSize(int packetSize, )
  {
  double payLoadSize = packetSize - BLOCK_HEADER_SIZE;
  if (config.protocol == SAGE_UDP) {
  config.blockX = (int)floor(sqrt(payLoadSize/bytesPerPixel));
  config.blockY = config.blockX;
  }
  }
*/
