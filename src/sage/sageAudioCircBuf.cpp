/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sagesageAudioCircBuf.cpp - the circular buffer to managing audio sample blocks
 * Author : Byungil Jeong, Hyejung Hur
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

#include "sageAudioCircBuf.h"
#include "sageAudioSync.h"
#include "sageSync.h"
#include "sageAudioModule.h"

#define UINT8_TO_FLOAT(x) ( ((float)(x) -128.0)/ 127.0 )
#define INT8_TO_FLOAT(x) ( (float) (x) / 127.0 )
#define INT16_TO_FLOAT(x) ( (float)(x) / 32768.0 )
#define FLOAT_TO_FLOAT(x) ( (float)(x) )

sageAudioCircBuf::sageAudioCircBuf(sageSyncClient *sync, int nID, int keyframe)
  : readIndex(0), writeIndex(0), blocksNum(0), full(false), empty(true), bytesBlock(0),
    sampleFmt(SAGE_SAMPLE_FLOAT32), sampleBuffSize(0), blockArray(NULL), audioId(-1),
    synchronizer(NULL), syncClientObj(sync), locked(false), lastFrameIndex(-1), refRead(0), refMutex(0),
  instID(nID), syncKeyFrame(keyframe), lastgFrameIndex(-0)
{
  queueLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(queueLock, NULL);
  pthread_mutex_unlock(queueLock);
  notFull = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notFull, NULL);
}

void sageAudioCircBuf::setKeyframe(int keyno)
{
  syncKeyFrame = keyno;
}

sageAudioCircBuf::~sageAudioCircBuf()
{
  pthread_cond_signal(notFull);

  //pthread_mutex_unlock(queueLock);

  if(blockArray != NULL) {
    clearBlocks();
  }

  if(queueLock)
    free(queueLock);
  if(notFull)
    free(notFull);
}


int sageAudioCircBuf::init(int id, int blockNum, sageSampleFmt fmt, int size)
{
  audioId = id;

  refRead =0;
  refMutex =0;
  readers.clear();
  readers.push_back(0);

  if(blockArray != NULL) {
    clearBlocks();
  }

  // initialize variables
  writeIndex = readIndex = 0;
  blocksNum = blockNum;
  sampleFmt = fmt;
  sampleBuffSize = size;

  blockArray = new audioBlock[blocksNum];

  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = new float[sampleBuffSize];
        //array = (float*) malloc(sizeof(float) * sampleBuffSize);
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = 0.0;
        }
        blockArray[j].buff = (void*) array;
      }
      bytesBlock = sizeof(float) * sampleBuffSize;
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = new short[sampleBuffSize];
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = 0;
        }
        blockArray[j].buff = (void*) array;
      }
      bytesBlock = sizeof(short) * sampleBuffSize;
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = new char[sampleBuffSize];
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = '0';
        }
        blockArray[j].buff = (void*) array;
      }
      bytesBlock = sizeof(char) * sampleBuffSize;
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = new unsigned char[sampleBuffSize];
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = 128;
        }
        blockArray[j].buff = (void*) array;
      }
      bytesBlock = sizeof(unsigned char) * sampleBuffSize;
    }
    break;
  }

  full = false;
  empty = true;
  locked = false;
  lastFrameIndex = -1;
  synchronizer = NULL;

  return 0;
}

void sageAudioCircBuf::clearBlocks()
{
  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (float*) blockArray[j].buff;
        delete[] array;
        blockArray[j].buff = NULL;
      }
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (short*) blockArray[j].buff;
        delete[] array;
        blockArray[j].buff = NULL;
      }
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (char*) blockArray[j].buff;
        delete[] array;
        blockArray[j].buff = NULL;
      }
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (unsigned char*) blockArray[j].buff;
        delete[] array;
        blockArray[j].buff = NULL;
      }
    }
    break;
  default :
    break;
  }
  delete [] blockArray;
  blockArray = NULL;

}

void sageAudioCircBuf::reset()
{
  readIndex = 0;
  writeIndex = 0;
  full = false;
  empty = true;
  locked = false;
  lastFrameIndex = -1;
  //synchronizer = NULL;

  refMutex =0;
  refRead =0;
  readers.clear();
  readers.push_back(0);

  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (float*) blockArray[j].buff;
        blockArray[j].reformatted = 0;
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = 0.0;
        }
      }
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (short*) blockArray[j].buff;
        blockArray[j].reformatted = 0;
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = 0;
        }
      }
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array = (char*) blockArray[j].buff;
        blockArray[j].reformatted = 0;
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = '0';
        }
      }
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *array;
      for( int j=0; j<blocksNum ; j++ ) {
        array =(unsigned char*) blockArray[j].buff;
        blockArray[j].reformatted = 0;
        for( int k=0; k< sampleBuffSize ; k++ ) {
          array[k] = 128;
        }
      }
    }
    break;
  }

  pthread_cond_signal(notFull);
  //pthread_mutex_unlock(queueLock);

}



audioBlock* sageAudioCircBuf::readBlock(int Id)
{
  if(refMutex == 1) return NULL;
  if (empty) {
    return NULL;
  }
  else {
    if(readers[Id] == 0) {
      readers[Id] = 1;
      refRead++;
    } else {
      return NULL;
    }
  }
  //std::cout << "readBlock " << std::endl;

  return &blockArray[readIndex];
}

audioBlock* sageAudioCircBuf::readBlock(int Id, int frameNum)
{
  if(refMutex == 1) return NULL;

  if (empty) {
    return NULL;
  }
  else   {
    // ????? - not working correctly
    if(readers[Id] == 0) {
      readers[Id] = 1;
      refRead++;
    } else {
      return NULL;
    }

    //if (frameNum%10000 >= blockArray[readIndex].frameIndex) {
    if (frameNum >= blockArray[readIndex].frameIndex) {
      return &blockArray[readIndex];
    }
  }

  return NULL;
}
/*
  audioBlock* sageAudioCircBuf::readBlock()
  {
  if (empty) {
  return NULL;
  }
  else
  return &blockArray[readIndex];
  }

  audioBlock* sageAudioCircBuf::readBlock(int frameNum)
  {
  if (empty) {
  return NULL;
  }
  else   {
  //if (frameNum%10000 >= blockArray[readIndex].frameIndex) {
  if (frameNum >= blockArray[readIndex].frameIndex) {
  return &blockArray[readIndex];
  }
  }

  return NULL;
  }
*/

audioBlock* sageAudioCircBuf::getNextWriteBlock()
{
  if(synchronizer) {
    if(locked == true) {
      // waiting for signal from graphic stream receiver
      locked = !synchronizer->checkSignal(audioId);
      if(locked == true) {
        //std::cout << "checkSignal : locked " << std::endl;
      } else
      {
        std::cout << "checkSignal : free " << std::endl;
      }
    }
  }

  // if buffer is full or if buffer is locked for writing
  pthread_mutex_lock(queueLock);
  while (full) {
    pthread_cond_wait(notFull, queueLock);
    //return NULL;
  }
  pthread_mutex_unlock(queueLock);
  if (locked) {
    return NULL;
  }
  audioBlock *block = &blockArray[writeIndex];
  clearBlock(writeIndex);

  //std::cout << "wrote " << std::endl;

  return block;
}


int sageAudioCircBuf::updateReadIndex()
{
  refMutex = 1;
  int gframeIndex=0;
  int size = readers.size();
  if(refRead == size) {
    gframeIndex = blockArray[readIndex].frameIndex;
    clearBlock(readIndex);
    readIndex = (readIndex+1) % blocksNum;

    full = false;
    //pthread_mutex_unlock(queueLock);
    pthread_cond_signal(notFull);

    if (readIndex == writeIndex) {
      empty = true;
    }
    refRead = 0;
    for(int i=0; i <size; i++)
    {
      readers[i] =0;
    }
  }

  if(syncClientObj)
  {
    int tempIndex = gframeIndex % syncKeyFrame;
    int diff = lastgFrameIndex - tempIndex;

    if(diff > 0)
    {
      tempIndex = gframeIndex -tempIndex;
      //std::cout << "-------> sync signal -- audio update" << tempIndex << instID << std::endl;
      syncClientObj->sendSlaveUpdate(tempIndex, instID, syncKeyFrame, SAGE_UPDATE_AUDIO);

      //syncClientObj->sendSlaveUpdate(gframeIndex + 1, instID,1, SAGE_UPDATE_AUDIO);
    }
    lastgFrameIndex = gframeIndex % syncKeyFrame;
  }
  refMutex =0;
  return readIndex;
}


/*
  int sageAudioCircBuf::updateReadIndex()
  {
  clearBlock(readIndex);
  readIndex = (readIndex+1) % blocksNum;
  full = false;
  if (readIndex == writeIndex)
  empty = true;

  return 0;
  }
*/

int sageAudioCircBuf::updateWriteIndex()
{
  audioBlock *block = &blockArray[writeIndex];
  lastFrameIndex = block->frameIndex;

  writeIndex = (writeIndex+1) % blocksNum;
  int nextIndex = (writeIndex+1) % blocksNum;
  if (nextIndex == readIndex) {
    full = true;
    //pthread_mutex_lock(queueLock);
  }

  empty = false;

  // check keyframe
  if(synchronizer) {
    locked = synchronizer->checkKeyFrame(audioId, lastFrameIndex);
  }
  //std::cout << "checkKeyFrame : " << lastFrameIndex << std::endl;
  if(locked) {
    std::cout << "checkKeyFrame : keyframe -> locked : " << lastFrameIndex << std::endl;
  }
  return 0;
}

int sageAudioCircBuf::getReadIndex()
{
  return readIndex;
}
int sageAudioCircBuf::getWriteIndex()
{
  return writeIndex;
}

int sageAudioCircBuf::convertToFloat(sageSampleFmt fmt, void* rawdata, audioBlock* block)
{
  if(sampleFmt != SAGE_SAMPLE_FLOAT32) return -1;

  float *wptr = (float*) block->buff;

  switch(fmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *rptr = (float*) rawdata;
      for( int i=0; i< sampleBuffSize; i++ )
      {
        *wptr++ = FLOAT_TO_FLOAT(*rptr);
        rptr++;
      }
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *rptr = (short*) rawdata;
      for( int i=0; i< sampleBuffSize; i++ )
      {
        *wptr++ = INT16_TO_FLOAT(*rptr);
        rptr++;
      }
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *rptr = (char*) rawdata;
      for( int i=0; i< sampleBuffSize; i++ )
      {
        *wptr++ = INT8_TO_FLOAT(*rptr);
        rptr++;
      }
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *rptr = (unsigned char*) rawdata;

      for( int i=0; i< sampleBuffSize; i++ )
      {
        *wptr++ = UINT8_TO_FLOAT(*rptr);
        rptr++;
      }
    }
    break;
  }

  return 1;
}

int sageAudioCircBuf::merge(audioBlock* block)
{
  std::vector<sageAudioCircBuf*> bufferList = sageAudioModule::_instance->getBufferList();

  float size = 1.0 / bufferList.size();

  float *wptr = (float*) block->buff;
  /*for( int i=0; i< sampleBuffSize; i++ )
    {
    *wptr = *wptr * size ;
    //*wptr = *wptr * 0.0 ;
    wptr++;
    }*/
  float *rptr = NULL;

  std::vector<sageAudioCircBuf*>::iterator iterBuffer;
  sageAudioCircBuf* temp = NULL;
  audioBlock *secondblock = NULL;
  for(iterBuffer = bufferList.begin(); iterBuffer != bufferList.end(); iterBuffer++)
  {
    temp = (sageAudioCircBuf*) *iterBuffer;
    if(temp->getAudioId() != audioId)
    {
      secondblock = temp->readBlock();
      if(secondblock == NULL) continue;
      if(secondblock->reformatted != 1) continue;

      wptr = (float*) block->buff;
      rptr = (float*) secondblock->buff;
      for( int i=0; i< sampleBuffSize; i++ )
      {
        //*wptr = *rptr;
        //*wptr = *wptr + size * (*rptr);
        *wptr = *wptr + (*rptr);
        //std::cout << *wptr << std::endl;
        wptr++;
        rptr++;
      }
      //std::cout << "merging " << temp->getAudioId() << ", mine: " << audioId << " size : " << size << std::endl;

      temp->updateReadIndex();
    }
  }

  return 1;
}

int sageAudioCircBuf::getSize()
{
  return blocksNum;
}

int sageAudioCircBuf::getAudioId()
{
  return audioId;
}

void sageAudioCircBuf::clearBlock(int frameNum)
{
  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *array = (float*) blockArray[frameNum].buff;
      for( int j=0; j< sampleBuffSize ; j++ ) {
        array[j] = 0;
      }
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *array = (short*) blockArray[frameNum].buff;
      for( int j=0; j< sampleBuffSize ; j++ ) {
        array[j] = 0;
      }
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *array = (char*) blockArray[frameNum].buff;
      for( int j=0; j< sampleBuffSize ; j++ ) {
        array[j] = '0';
      }
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *array = (unsigned char*) blockArray[frameNum].buff;
      for( int j=0; j< sampleBuffSize ; j++ ) {
        array[j] = '0';
      }
    }
    break;
  default :
    break;
  }

}

int sageAudioCircBuf::getBytesBlock()
{
  return bytesBlock;
}

void sageAudioCircBuf::connectSync(sageAudioSync* synch)
{
  synchronizer = synch;

}

void sageAudioCircBuf::connectSyncClient(sageSyncClient* synch)
{
  syncClientObj = synch;
  if(syncClientObj != NULL)
  {
    syncClientObj->sendSlaveUpdate(1, instID, syncKeyFrame, SAGE_UPDATE_AUDIO);
  }
}

void sageAudioCircBuf::setLock()
{
  //locked = true;
}

void sageAudioCircBuf::setFree()
{
  //locked = false;
}

int sageAudioCircBuf::getLastFrameIdx()
{
  return lastFrameIndex;
}

int sageAudioCircBuf::addReader()
{
  int index = readers.size();
  readers.push_back(0);
  return index;
}

void sageAudioCircBuf::deleteReader(int Id)
{
  readers[Id] = -1;
}
