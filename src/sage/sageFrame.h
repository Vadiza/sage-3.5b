/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageFrame.h - classes for storing and manipulating image frames
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

#ifndef _SAGE_FRAME_H
#define _SAGE_FRAME_H

#include "sageBlock.h"

class sageFrame;
class sageBlockFrame;
class sageBlockPartition;

class sageSubFrame : public sagePixelData {
public:
  std::vector<int> blockList;
  sageBlockFrame *mainFrame;

  int computeFrameRect();
};

class sageBlockFrame : public sagePixelData {
protected:
  sagePixelBlock *blocks;
  int idx;
  int numCol, numRow, subBlockNum;
  int pixelSize;
  int memWidth;
  sageBlockPartition *partition;

public:
  sageBlockFrame(int w, int h, int bytes, float compX = 1.0, float compY = 1.0);

  //inline void setBlockSize(int w, int h) { blockWidth = w, blockHeight = h; }
  int initFrame(sageBlockPartition *part);
  inline void resetBlockIndex() { idx = 0; }
  bool extractPixelBlock(sagePixelBlock *block, int rowOrder);

  int generateBlocks(int rowOrd);
  int generateSubFrame(sageRect &subRect, sageSubFrame &sFrame);
  bool updateBlockConfig() { return false; }
  int updateBufferHeader() { return 0; }
  ~sageBlockFrame();

  friend class sageSubFrame;
};

#endif
