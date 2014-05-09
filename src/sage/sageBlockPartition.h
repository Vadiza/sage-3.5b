/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlockPartition.h
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

#ifndef _SAGE_BLOCK_PARTITION_H
#define _SAGE_BLOCK_PARTITION_H

#include "sageBase.h"
#include "sageBlock.h"

class pixelBlockMap {
public:
  int blockID;
  int infoID;
  int count;
  pixelBlockMap *next;

  pixelBlockMap() : blockID(0), infoID(0), next(NULL) {}
};

class blockMontageMap : public pixelBlockMap {
public:
  int x, y;

  blockMontageMap() : x(0), y(0) { next = NULL; }
};

class sageBlockPartition : public sageRect {
private:
  int blockWidth, blockHeight;
  int rowNum, colNum;
  int blockNum, totalBlockNum;
  sageRect viewPort;
  sageRect blockLayout;
  sageRect displayLayout;
  int vRowNum, vColNum, vOffsetX, vOffsetY;
  pixelBlockMap **blockTable;
  int entryNum;

public:
  sageBlockPartition(int bw, int bh, int iw, int ih);
  void getBlock(int id, sagePixelBlock &rect);
  void setViewPort(sageRect &rect);
  int getVisibleBlockID(int idx);
  void getVisibleBlock(int idx, sagePixelBlock &block);
  void adjustBlockCoord(sagePixelBlock &block);
  int setStreamInfo(int infoID, sageRect &window);
  int setStreamInfo(int infoID, int begin, int end);
  bool insertBlockMap(pixelBlockMap *map);
  pixelBlockMap* getBlockMap(int blockID);
  void clearBlockTable();
  void initBlockTable();
  void genBlockTable(int monIdx);
  inline int getBlockNum() { return blockNum; }
  inline int getTotalBlockNum() const {return totalBlockNum;}
  inline int tableEntryNum() { return entryNum; }
  inline void setDisplayLayout(sageRect &layout) { displayLayout = layout; }
  inline sageRect& getViewPort() { return viewPort; }
  inline sageRect& getBlockLayout() { return blockLayout; }
  void setTileLayout(sageRect tileRect);
  void getBlockPosition(sagePixelBlock &block);
};

#endif
