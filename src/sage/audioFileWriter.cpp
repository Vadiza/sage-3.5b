/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  audioFileWriter.cpp
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

#include "audioFileWriter.h"
#include "sageAudioModule.h"

audioFileWriter::audioFileWriter(audioFormatManager* fm)
  : buffer(NULL),  formatManager(fm), converter(NULL), fileName(NULL),
    fileID(NULL), startFlag(false)
{
}


audioFileWriter::~audioFileWriter()
{
  // delete audiolist
}

void audioFileWriter::init()
{
  startFlag = false;
  converter = NULL;
  fileID = NULL;
  buffer = NULL;
  fileName = NULL;
}

int audioFileWriter::write(char* filename, sageAudioConfig &config, sageAudioCircBuf* buf)
{
  if(filename == NULL) return -1;
  if(formatManager == NULL) return -1;
  if(buf == NULL) return -1;

  buffer = buf;

  framePerBuffer = config.framePerBuffer;
  channels = config.channels;
  samplingRate = config.samplingRate;

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

  fileName = filename;
  fileID = NULL;
  fileID = fopen(fileName, "wb");
  if( fileID == NULL )
  {
    std::cerr << "Could not open file :" << fileName << std::endl;
    return -1;
  }
  startFlag = true;

  converter = formatManager->createConverter(fileName, fileID);
  if(converter == NULL) {
    std::cerr << "Could not support format :" << fileName << std::endl;
    return NULL;
  }
  converter->writeHeader(channels, samplingRate, framePerBuffer, bytesPerSample*8);

  /*int length = strlen(headerInfo);
    std::cout << headerInfo;
    fwrite( headerInfo, length * sizeof(char), length, fileID );*/

  return 0;

}

int audioFileWriter::start()
{
  startFlag = true;
  pthread_t thId;

  if (pthread_create(&thId, 0, writeThread, (void*)this) != 0) {
    std::cerr << "audioFileWriter : can't create writerThread" << std::endl;
    return -1;
  }

  return 0;
}

int audioFileWriter::stop()
{
  startFlag = false;
  return 0;
}

void* audioFileWriter::writeThread(void *args)
{
  audioFileWriter *This = (audioFileWriter *)args;

  if(This == NULL) {
    fclose(This->fileID);
    pthread_exit(NULL);
    return NULL;
  }

  int Id = This->buffer->addReader();

  // write data
  // #data
  audioBlock* bufferBlock = NULL;
  while(This->startFlag) {

    bufferBlock = This->buffer->readBlock(Id);
    if(bufferBlock != NULL) {
      This->converter->writeFrames(bufferBlock->buff);
      This->buffer->updateReadIndex();
      sage::usleep(100);
    }
    else {
      sage::usleep(1000);
    }

  }

  std::cout << "audioFileWriter :end writeThread" << std::endl;
  This->buffer->deleteReader(Id);

  This->converter->close();
  fclose(This->fileID);
  pthread_exit(NULL);
  return NULL;

}
