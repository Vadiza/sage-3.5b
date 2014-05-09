/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAppAudio.cpp -
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

#include "sageAppAudio.h"
#include "sageAudioCircBuf.h"
#include "sageAudioModule.h"

sageAppAudio::sageAppAudio(sageAudioCircBuf* audioBuffer, int maxsize) : buffer(audioBuffer), initialized(false)
{
  maxAudioBuffSize = maxsize+512;
  queueLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(queueLock, NULL);
  pthread_mutex_unlock(queueLock);
  notFull = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notFull, NULL);
  notEmpty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notEmpty, NULL);

  audioAppRawBuffer[0] = malloc(maxAudioBuffSize);
  audioAppRawBuffer[1] = malloc(maxAudioBuffSize);
  audiobufSize[0] = maxAudioBuffSize;
  audiobufSize[1] = maxAudioBuffSize;

  remainBufSize =0;
  remainBuf = NULL;


  full = false;
  empty = true;
  writeIndex = 0;
  readIndex = 0;

  /*if (pthread_create(&thAppId, 0, audioThread, (void*)this) != 0) {
    std::cerr << "sageAppAudio : can't create audioThread" << std::endl;
    }*/

}

sageAppAudio::~sageAppAudio()
{
  if(queueLock)
    free(queueLock);

  if(notFull)
    free(notFull);

  pthread_join(thAppId, NULL);

  if(audioAppRawBuffer[0])
  {
    free(audioAppRawBuffer[0]);
    audioAppRawBuffer[0] = NULL;
  }
  if(audioAppRawBuffer[1])
  {
    free(audioAppRawBuffer[1]);
    audioAppRawBuffer[1] = NULL;
  }
}

int sageAppAudio::start(void)
{
  if (pthread_create(&thAppId, 0, audioThread, (void*)this) != 0) {
    std::cerr << "sageAppAudio : can't create audioThread" << std::endl;
  }
  initialized = true;
  return 1;
}

void* sageAppAudio::audioThread(void *args)
{
  sageAppAudio *This = (sageAppAudio *)args;

  bool loop = true;
  while(loop) {
    This->processData();
  }

  SAGE_PRINTLOG("sail::audioThread : exit the application audio thread");
  pthread_exit(NULL);
  return NULL;
}


int sageAppAudio::processData()
{
  pthread_mutex_lock(queueLock);

  while (empty) {
    pthread_cond_wait(notEmpty, queueLock);
  }

  char * buf = (char *)audioAppRawBuffer[readIndex];

  int byteBlock = buffer->getBytesBlock();

  int numBlock = audiobufSize[readIndex] / byteBlock;
  remainBufSize = audiobufSize[readIndex] % byteBlock;
  //remainBufSize = 0;

  audioBlock *block;
  for(int i=0; i < numBlock ; i++)
  {
    //SAGE_PRINTLOG("sageAppAudio::processData : size %d %d \n", i, numBlock);

    block = buffer->getNextWriteBlock();
    while(block == NULL) {
      sage::usleep(10);
      block = buffer->getNextWriteBlock();
      SAGE_PRINTLOG("sageAppAudio::processData : trying to get buffer : %d %d\n", i, numBlock);
      //std::cout << "trying to get buffer..." << std::endl;
    }

    if(block != NULL)
    {
      memcpy(block->buff, buf, byteBlock);
      block->frameIndex = buffer->getWriteIndex();
      block->gframeIndex = sageAudioModule::_instance->getgFrameNum();
      block->reformatted = 1;
      buffer->updateWriteIndex();
      buf += byteBlock;
    }
  }

  //SAGE_PRINTLOG("%d - %d\n\n", byteBlock, remainBufSize);

  if(remainBufSize > 0) {
    remainBuf = buf;
  }

  readIndex = (readIndex +1) % 2;
  if (writeIndex == readIndex) {
    empty = true;
    pthread_cond_signal(notFull);
  }
  full = false;

  pthread_mutex_unlock(queueLock);

  return 0;
}

int sageAppAudio::swapBuffer(int size, void *buf)
{
  if(initialized == false) return 0;
  if(buf == NULL) return 0;

  pthread_mutex_lock(queueLock);

  //SAGE_PRINTLOG("sageAppAudio::swapBuffer : size %d %d\n", size, remainBufSize);

  while (full) {
    pthread_cond_wait(notFull, queueLock);
  }

  char * bufaudio = (char *) audioAppRawBuffer[writeIndex];
  memset(bufaudio, 0, maxAudioBuffSize);
  if(remainBufSize > 0) {
    memcpy(bufaudio, remainBuf, remainBufSize);
    bufaudio += remainBufSize;
    remainBufSize=0;
  }

  memcpy(bufaudio, buf, size);
  audiobufSize[writeIndex] = size + remainBufSize;


  writeIndex = (writeIndex +1) % 2;
  //int nextIdx = (writeIdx+1) % 2;
  if (writeIndex == readIndex) {
    full = true;
  }
  empty = false;

  pthread_cond_signal(notEmpty);

  pthread_mutex_unlock(queueLock);

  return 0;
}
