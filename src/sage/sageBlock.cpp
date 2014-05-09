/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBlock.cpp - a few geometrical operations for rectangles used in SAGE
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

#include "sageBlock.h"

void sagePixelData::operator=(sageRect &rect)
{
  sageRect::operator=(rect);
}

int sagePixelData::initBuffer()
{
  int size = (int)ceil(width*height*bytesPerPixel/(compressX*compressY)) + BLOCK_HEADER_SIZE;

  if (size <= BLOCK_HEADER_SIZE) {
    SAGE_PRINTLOG("sagePixelBlock::initBuffer : incorrect block size");
    return -1;
  }

  releaseBuffer();

  if (allocateBuffer(size) < 0)
    return -1;

  //std::cout << "allocate " << size << " bytes" << std::endl;
  pixelData = buffer + BLOCK_HEADER_SIZE;

  return 0;
}

int sagePixelData::releaseBuffer()
{
  //std::cout << "release buffer" << std::endl;
  if (buffer)
    free((void *)buffer);

  return 0;
}

int sagePixelData::allocateBuffer(int size)
{
  buffer = (char *)malloc(size);
  if (!buffer) {
    SAGE_PRINTLOG("sageBlock::allocateBuffer : fail to allocate %d bytes", size);
    return -1;
  }
  bufSize = size;

  return 0;
}

sagePixelBlock::sagePixelBlock(int size) : valid(false)
{
  flag = SAGE_PIXEL_BLOCK;
  allocateBuffer(size);

  //std::cout << "allocate " << size << " bytes" << std::endl;
  pixelData = buffer + BLOCK_HEADER_SIZE;
}

sagePixelBlock::sagePixelBlock(sagePixelBlock &block)
{
  allocateBuffer(block.bufSize);
  pixelData = buffer + BLOCK_HEADER_SIZE;

  memcpy(buffer, block.buffer, bufSize);
  updateBlockConfig();
}

int sagePixelBlock::updateBufferHeader()
{
  if (!buffer) {
    SAGE_PRINTLOG("sagePixelBlock::updateBufferHeader : buffer is null");
    return -1;
  }

  memset(buffer, 0, BLOCK_HEADER_SIZE);
  int headerSize = 0;

#if defined(WIN32)
  headerSize = _snprintf(buffer, BLOCK_HEADER_SIZE, "%d %d %d %d %d %d %d %d",
                         bufSize, flag, x, y, width, height, frameID, blockID);
#else
  headerSize = snprintf(buffer, BLOCK_HEADER_SIZE, "%d %d %d %d %d %d %d %d",
                        bufSize, flag, x, y, width, height, frameID, blockID);
#endif

  if (headerSize >= BLOCK_HEADER_SIZE) {
    SAGE_PRINTLOG("sagePixelBlock::updateBufferHeader : block header has been truncated.");
    return -1;
  }

  return 0;
}

bool sagePixelBlock::updateBlockConfig()
{
  if (!buffer) {
    SAGE_PRINTLOG("sagePixelBlock::updateBlockConfig : buffer is null");
    return false;
  }

  //std::cout << "buf : " << buffer << std::endl;

  sscanf(buffer, "%d %d %d %d %d %d %d %d", &bufSize, &flag, &x, &y, &width, &height,
         &frameID, &blockID);

  return true;
}

sagePixelBlock::~sagePixelBlock()
{
  releaseBuffer();
}


sageAudioBlock::sageAudioBlock() : frameID(0), gframeID(0),
                                   bytesPerSample(4), sampleFmt(SAGE_SAMPLE_FLOAT32),
                                   sampleRate(0), channels(0), framePerBuffer(0),
                                   extraInfo(NULL), tileID(0), nodeID(0)
{
  flag = SAGE_AUDIO_BLOCK;
}

sageAudioBlock::sageAudioBlock(int frame, sageSampleFmt type, int byte, int rate, int chan, int framesperbuffer)
  :                       frameID(frame), gframeID(0),
                          bytesPerSample(byte), sampleFmt(type),
                          sampleRate(rate), channels(chan), framePerBuffer(framesperbuffer),
                          extraInfo(NULL), tileID(0), nodeID(0)
{
  flag = SAGE_AUDIO_BLOCK;
  initBuffer();
}

sageAudioBlock::~sageAudioBlock()
{
  releaseBuffer();
}

int sageAudioBlock::initBuffer()
{
  int newBufSize = framePerBuffer * channels * bytesPerSample;
  initBuffer(newBufSize);
  return 0;
}

int sageAudioBlock::initBuffer(int size)
{
  if (buffer)
  {
    releaseBuffer();
  }

  if (allocateBuffer(size) < 0)
    return -1;

  bufSize = size;
  audioData = buffer + BLOCK_HEADER_SIZE;
  return 0;
}

int sageAudioBlock::releaseBuffer()
{
  //std::cout << "release buffer" << std::endl;
  if (buffer) {
    free(buffer);
    buffer = NULL;
    //if (buffer)
    //delete [] buffer;
  }

  return 0;
}

int sageAudioBlock::allocateBuffer(int size)
{

  buffer = (char*)malloc(size);
  bufSize = size;

  return 0;
}

int sageAudioBlock::updateBufferHeader()
{
  if (!buffer) {
    SAGE_PRINTLOG("sageAudioBlock::updateBufferHeader : buffer is null");
    return -1;
  }

  memset(buffer, 0, BLOCK_HEADER_SIZE);
  int headerSize = 0;

  /*
    if (frameID >= 0)
    frameID = frameID % 10000;      // 4 digit frame number

    if (gframeID >= 0)
    gframeID = gframeID % 10000; // 4 digit frame number
  */


#if defined(WIN32)
  headerSize = _snprintf(buffer, BLOCK_HEADER_SIZE, "%d %d %d %d %d %d %d %d %d %d",
                         bufSize, flag,(int)sampleFmt, sampleRate, channels, framePerBuffer, frameID, gframeID, tileID, nodeID);
#else
  headerSize = snprintf(buffer, BLOCK_HEADER_SIZE, "%d %d %d %d %d %d %d %d %d %d",
                        bufSize, flag, (int)sampleFmt, sampleRate, channels, framePerBuffer, frameID, gframeID, tileID, nodeID);
#endif

  if (headerSize >= BLOCK_HEADER_SIZE) {
    SAGE_PRINTLOG("sageAudioBlock::updateBufferHeader : block header has been truncated.");
    return -1;
  }

  return 0;
}

bool sageAudioBlock::updateBlockConfig()
{
  /*
    if (frameID >= 0)
    frameID = frameID % 10000;      // 4 digit frame number

    if (gframeID >= 0)
    gframeID = gframeID % 10000; // 4 digit frame number
  */


  if (!buffer) {
    SAGE_PRINTLOG("sageAudioBlock::updateBlockConfig : buffer is null");
    return false;
  }

  sscanf(buffer, "%d %d %d %d %d %d %d %d %d %d", &bufSize, &flag, &sampleFmt, &sampleRate,
         &channels, &framePerBuffer, &frameID,  &gframeID, &tileID, &nodeID);

  //std::cout << buffer << std::endl;
  extraInfo = sage::tokenSeek(buffer, 10);
  if (!extraInfo && flag == SAGE_AUDIO_BLOCK) {
    // SAGE_PRINTLOG("sageAudioBlock::updateBlockConfig : extraInfo is NULL");
  }

  return true;
}


/*
  sageControlBlock::sageControlBlock(int f, int frame, int size) : frameID(frame)
  {
  flag = f;
  bufSize = size;

  if (size > 0)
  buffer = new char[size];
  }

  int sageControlBlock::updateBufferHeader(char *info)
  {
  if (!buffer) {
  SAGE_PRINTLOG("sageControlBlock::updateBufferHeader : buffer is null");
  return -1;
  }

  memset(buffer, 0, BLOCK_HEADER_SIZE);
  sprintf(buffer, "%d %d %d %s", bufSize, flag, frameID, info);

  return 0;
  }

  int sageControlBlock::updateBlockConfig()
  {
  if (!buffer) {
  SAGE_PRINTLOG("sageControlBlock::updateBlockConfig : buffer is null");
  return -1;
  }

  sscanf(buffer, "%d %d %d %s", &bufSize, &flag, &frameID, ctrlInfo);

  return 0;
  }

  sageControlBlock::~sageControlBlock()
  {
  if (buffer)
  delete [] buffer;
  }
*/
