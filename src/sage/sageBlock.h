/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlock.h
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

#ifndef _SAGE_BLOCK_H
#define _SAGE_BLOCK_H

#include "sageRect.h"

#define BLOCK_HEADER_SIZE 128

#define SAGE_PIXEL_BLOCK   1
#define SAGE_UPDATE_BLOCK  2

//#define SAGE_BUFFER_BLOCK   2
//#define SAGE_IOVEC_FRAME   3
//#define SAGE_RLE_BLOCK      4
#define SAGE_AUDIO_BLOCK  5

#define SAGE_FRAME_BLOCK   100
#define SAGE_STOP_BLOCK 101
#define SAGE_TIME_BLOCK   102
#define SAGE_ACK_BLOCK   103
#define SAGE_SKIP_BLOCK 105
#define SAGE_CLEAR_BLOCK 106

/**
 * sageBLock
 */
class sageBlock {
protected:
  char *buffer;
  int bufSize;
  int flag;      //   specify the function of block
  int rcnt;
  int blockID;

public:
  sageBlock() : flag(SAGE_PIXEL_BLOCK), buffer(NULL), bufSize(0), rcnt(0) {}

  inline int getFlag() { return flag; }
  inline void setFlag(int f) { flag = f; }
  inline char *getBuffer() { return buffer; }
  inline int getBufSize() { return bufSize; }
  inline void setRefCnt(int cnt = 0) { rcnt = cnt; }
  inline int getRefCnt() { return rcnt; }
  inline int reference(int cnt = 1) { rcnt += cnt; return rcnt; }
  inline int dereference(int cnt = 1) { rcnt -= cnt; return rcnt; }
  inline void setID(int id) { blockID = id; }
  inline int getID() { return blockID; }

  virtual int updateBufferHeader() = 0;
  virtual bool updateBlockConfig() = 0;
  virtual int getFrameID() = 0;
  virtual ~sageBlock() {}
};

/**
 * sagePixelData
 */
class sagePixelData : public sageRect, public sageBlock {
protected:
  char *pixelData; // starting address of image buffer
  int frameID;  // frame to which this block belongs
  sagePixFmt pixelType;   // pixel format of block
  int bytesPerPixel;
  float compressX, compressY;

  int initBuffer();
  int releaseBuffer();
  int allocateBuffer(int size);

public:
  sagePixelData() : pixelData(NULL), frameID(0), pixelType(PIXFMT_888),
                    bytesPerPixel(3), compressX(1.0), compressY(1.0) {}

  virtual int getFrameID() { return frameID; }
  void operator=(sageRect &rect);
  inline void setFrameID(int id) { frameID = id; }
  inline sagePixFmt getPixelType() { return pixelType; }
  inline void setPixelType(sagePixFmt type) { pixelType = type; }
  inline int getBytesPerPixel() { return bytesPerPixel; }
  inline void setBytesPerPixel(int byte) { bytesPerPixel = byte; }
  inline char *getPixelBuffer() { return pixelData; }
};

class sageBlockGroup;

/**
 * sagePixelBlock
 *
 * sageBlockGroup object has (grpSize/blockSize) of this sagePixelBlock object in its sageCircBufSingle
 */
class sagePixelBlock : public sagePixelData {
protected:
  bool valid;
  sageBlockGroup *grp;

public:
  sagePixelBlock() : valid(false), grp(NULL) {}
  sagePixelBlock(int size);
  sagePixelBlock(sagePixelBlock& block);
  //sagePixelBlock(int w, int h, int bytes, float compX, float compY,
  //   int frame, int confID);

  inline sageBlockGroup* getGroup() { return grp; }
  inline void setGroup(sageBlockGroup *parent) { grp = parent; }
  inline void clearHeader() { memset(buffer, 0, BLOCK_HEADER_SIZE); }
  inline void clearBuffer() { memset(buffer, 0, bufSize); }
  int updateBufferHeader();
  bool updateBlockConfig();

  inline bool isValid()    { return valid; }

  //void recalcBufSize();

  ~sagePixelBlock();
};

/**
 * sageAudioBlock
 */
class sageAudioBlock : public sageBlock {
protected:
  int frameID;    // audio frame to which this block belongs
  int gframeID;   // video frame to which this block belong

  sageSampleFmt sampleFmt;   // sample format of block
  int bytesPerSample;
  int channels;
  int framePerBuffer;
  long sampleRate;
  char* audioData;

  char *extraInfo;
  int tileID;
  int nodeID;

  int releaseBuffer();
  int allocateBuffer(int size);

public:
  sageAudioBlock();
  sageAudioBlock(int frame, sageSampleFmt type, int byte, int rate, int chan, int framesperbuffer);
  ~sageAudioBlock();

  int initBuffer();
  int initBuffer(int size);

public:
  int updateBufferHeader();
  virtual bool updateBlockConfig();
  virtual int getTileID() { return tileID; }

  inline void setTileID(int id) { tileID = id; }
  inline int getNodeID() { return nodeID; }
  inline void setNodeID(int id) { nodeID = id; }

  virtual int getFrameID() { return frameID; }
  void operator=(sageAudioBlock &rect);
  inline void setFrameID(int id) { frameID = id; }

  virtual int getgFrameID() { return gframeID; }
  inline void setgFrameID(int id) { gframeID = id; }

  inline sageSampleFmt getSampleFormat() { return sampleFmt; }
  inline void setSampleFormat(sageSampleFmt type) { sampleFmt = type; }

  inline int getBytesPerSample() { return bytesPerSample; }
  inline void setBytesPerSample(int byte) { bytesPerSample = byte; }

  inline int getChannel() { return channels; }
  inline void setChannel(int num) { channels = num; }

  inline int getFramePerBuffer() { return framePerBuffer; }
  inline void setFramePerBuffer(int num) { framePerBuffer = num; }

  inline int getSampleRate() { return sampleRate; }
  inline void setSampleRate(int rate) { sampleRate = rate; }

  char *getExtraInfo() { return extraInfo; }

  inline char *getAudioBuffer() { return audioData; }
};


/*
  class sageControlBlock : public sageBlock {
  protected:
  int frameID;
  char ctrlInfo[TOKEN_LEN];

  public:
  sageControlBlock(int f, int frame, int size = 0);
  virtual ~sageControlBlock();

  int updateBufferHeader(char *info);
  virtual int updateBlockConfig();
  virtual int getFrameID() { return frameID; }
  char *getControlInfo() { return ctrlInfo; }
  };
*/

#endif
