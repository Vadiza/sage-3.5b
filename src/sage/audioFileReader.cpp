/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  audioFileReader.cpp
 *   Author   :   Hyejung Hur
 *   Description:   Code file for
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
 ********************************************************************************/

#include "audioFileReader.h"
#include "sageAudioModule.h"

audioFileReader::audioFileReader(audioFormatManager* fm)
  : startFlag(false), formatManager(fm), converter(NULL),
    fileID(NULL), buffer(NULL),  fileName(NULL), gapTime(0),
    totalLoadedFrames(0), totalBlocks(0), loopFlag(false)
{
}


audioFileReader::~audioFileReader()
{
  // delete audiolist
}

void audioFileReader::init()
{
  startFlag = false;
  loopFlag = false;
  converter = NULL;
  fileID = NULL;
  buffer = NULL;
  fileName = NULL;
}

sageAudioCircBuf* audioFileReader::load(char* filename, bool loop, int nframes, long totalframes)
{
  if(filename == NULL) return NULL;
  if(formatManager == NULL) return NULL;

  fileName = filename;
  fileID = fopen(fileName, "rb");
  if( fileID == NULL ) {
    std::cerr << "Could not open file :" << fileName << std::endl;
    return NULL;
  }
  startFlag = true;

  converter = formatManager->createConverter(fileName, fileID);
  if(converter == NULL) {
    std::cerr << "Could not support format :" << fileName << std::endl;
    return NULL;
  }

  sageAudioConfig config;
  config.framePerBuffer = nframes;
  totalBlocks = converter->readHeader(config.channels, config.samplingRate, config.framePerBuffer, config.sampleFmt);
  if(totalBlocks <=0) totalBlocks = 1;

  switch(config.sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    bytesPerSample = sizeof(float);
    break;
  case SAGE_SAMPLE_INT16 :
    bytesPerSample = sizeof(short);
    break;
  case SAGE_SAMPLE_INT8 :
    bytesPerSample = sizeof(char);
    break;
  case SAGE_SAMPLE_UINT8 :
    bytesPerSample = sizeof(unsigned char);
    break;
  default :
    bytesPerSample = sizeof(float);
    break;
  }
  //buffer = sageAudioModule::_instance->createBuffer(config, 32);
  buffer = sageAudioModule::_instance->createBuffer(config, 8);
  if(buffer == NULL) {
    std::cerr << "Could not create Buffer" << std::endl;
    return NULL;
  }
  gapTime = 1000000.0/ (config.samplingRate / config.framePerBuffer * config.channels);
  //gapTime *=  0.3;
  std::cout << "----------gaptime "  << gapTime << std::endl;
  if(gapTime < 100) gapTime = 1000;

  totalLoadedFrames = totalframes;
  loopFlag = loop;
  return buffer;

}

int audioFileReader::start()
{
  startFlag = true;
  pthread_t thId;

  if (pthread_create(&thId, 0, readThread, (void*)this) != 0) {
    std::cerr << "audioFileReader : can't create readThread" << std::endl;
    return -1;
  }
  return 0;
}

int audioFileReader::stop()
{
  startFlag = false;
  return 0;
}

void* audioFileReader::readThread(void *args)
{
  audioFileReader *This = (audioFileReader *)args;
  audioBlock* bufferBlock = NULL;
  int result =0;
  int blockIndex =0;

  while(This->startFlag) {
    bufferBlock = This->buffer->getNextWriteBlock();
    if(bufferBlock != NULL) {
      result = This->converter->readFrames(bufferBlock->buff);
      if(result >= 0)  {
        bufferBlock->frameIndex = This->buffer->getWriteIndex();
        if(This->totalLoadedFrames <= 0) {
          bufferBlock->gframeIndex = sageAudioModule::_instance->getgFrameNum();
          //std::cout << "gframeIndex(from current) : " << bufferBlock->gframeIndex << std::endl;
        } else {
          bufferBlock->gframeIndex = (This->totalLoadedFrames * blockIndex) / This->totalBlocks;
          std::cout << "gframeIndex(from calcurate) : " << bufferBlock->gframeIndex << std::endl;
        }
        bufferBlock->reformatted = 1;
        This->buffer->updateWriteIndex();
        blockIndex++;
      } else {
        //std::cout << "result --- finish???" << std::endl;
        if(This->loopFlag) {
          This->converter->begin();
          //blockIndex =0;
        } else {
          This->startFlag = false;
        }
      }
    } else {
      //std::cout << "buffer null" << std::endl;
      //sage::usleep(This->gapTime);
    }
    sage::usleep(This->gapTime);
  }

  std::cout << "audioFileReader :end readThread" << std::endl;
  This->converter->close();
  fclose(This->fileID);
  pthread_exit(NULL);
  return NULL;
}
