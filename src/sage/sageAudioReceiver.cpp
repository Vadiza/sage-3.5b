/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioReceiver.cpp
 * Author : Hyejung Hur, Byungil Jeong
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

#include "sageAudioReceiver.h"
#include "streamProtocol.h"
#include "sageAudioCircBuf.h"
#include "sageEvent.h"

sageAudioReceiver::sageAudioReceiver(char *msg, sageEventQueue *queue, streamProtocol *obj, sageAudioCircBuf *buff)
  : sageReceiver(), activeRecv(true), masterFlag(false), m_senderID(-1)
{
  buffer = buff;
  nwObj = obj;
  eventQueue = queue;

  char *msgPt = sage::tokenSeek(msg, 2);

  int syncMode, samplingRate, channels, framePerBuffer, keyFrame, frameRate;

  //6881394 1 0
  // 0
  // 1 44100 2 512 1
  //std::cout << "--------> receiver : " << msgPt << std::endl;

  sscanf(msgPt, "%d %d %d %d %d %d %d %d %d %d", &instID, &senderNum, &blockSize, &syncMode, (int*) &sampleFmt,
         &samplingRate, &channels, &framePerBuffer, &frameRate, &keyFrame);
  buffer->setKeyframe(keyFrame);
  std::cout << "------------------>audioReceiver : " << instID << " keyFrame : " << keyFrame << std::endl;
  streamList = new streamData[senderNum];
  streamIdx = 0;

  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    bytesPerSample = sizeof(float);
    break;
  case SAGE_SAMPLE_INT16 :
    bytesPerSample = sizeof(short);
    break;
  case SAGE_SAMPLE_INT8 :
    bytesPerSample = sizeof(int);
    break;
  case SAGE_SAMPLE_UINT8 :
    bytesPerSample = sizeof(unsigned int);
    break;
  default :
    break;
  }
  audioNBlock.setSampleFormat(sampleFmt);
  audioNBlock.setSampleRate(samplingRate);
  audioNBlock.setChannel(channels);
  audioNBlock.setFramePerBuffer(framePerBuffer);
  audioNBlock.setBytesPerSample(bytesPerSample);
  audioNBlock.initBuffer(blockSize);

  audioNBlock.updateBufferHeader();

}

void sageAudioReceiver::processSync(int frame)
{
  syncFrame = frame;

  //std::cout << "   receive sync " << syncFrame << std::endl;

  //if (updateFrame == syncFrame-1) {
  //swapMontages();
  //}
}

void sageAudioReceiver::setMaster(bool flag)
{
  masterFlag = flag;
}

bool sageAudioReceiver::isMaster(void)
{
  return masterFlag;
}

void sageAudioReceiver::setResetFlag(void)
{
  resetFlag = true;
}

int sageAudioReceiver::addStream(int senderID)
{
  //readThreadParam *param = new readThreadParam;
  //param->This = this;
  //param->senderID = senderID;

  m_senderID = senderID;
  streamList[streamIdx].senderID = senderID;
  streamList[streamIdx].dataSockFd = nwObj->getRcvSockFd(senderID);
  streamIdx++;

  // buffer reset
  buffer->reset();

  //if (pthread_create(&param->thId, 0, nwReadThread, (void*)param) != 0)
  if (pthread_create(&thId, 0, nwReadThread, (void*)this) != 0)
  {
    SAGE_PRINTLOG("sageReceiver : can't create network reading thread");
    return -1;
  }

  //threadList.push_back(param);
  //return threadList.size();

  return streamIdx;
}


//int sageAudioReceiver::readData(int senderID, sageBlockQueue *queue)
int sageAudioReceiver::readData()
{
  if (!eventQueue) {
    SAGE_PRINTLOG("sageAudioReceiver::readSampleData : event queue is null");
    return -1;
  }

  if (!buffer) {
    SAGE_PRINTLOG("sageAudioReceiver::readSampleData : buffer is null");
    return -1;
  }

  if (!nwObj)  {
    SAGE_PRINTLOG("sageAudioReceiver::readSampleData : network object is null");
    return -1;
  }

  audioBlock* block;
  while(!endFlag) {

    if(resetFlag == true) {
      buffer->reset();
      std::cout << "buffer is reset" << std::endl;
      resetFlag = false;
    }

    block = buffer->getNextWriteBlock();
    if(block == NULL) {
      std::cout << "could not get buffer for writing" << std::endl;
      return -1;
    }

    int rcvSize = nwObj->recv(streamList[0].senderID, &audioNBlock, SAGE_BLOCKING);
    if (rcvSize <= 0) {
      activeRecv = false;
      SAGE_PRINTLOG("sageAudioReceiver::readSampleData : exit loop");
      endFlag = true;
    }
    else {

      bandWidth += rcvSize;
      //std::cout << "enqueued a block : " << (char *)freeBlock->getBuffer() << std::endl;
      if(audioNBlock.getFlag() == SAGE_AUDIO_BLOCK)  {
        //std::cout << "---------->audioInstance::rcvStream " << buffer->getAudioId() << " " <<  audioNBlock.getFrameID() << std::endl;
        char* tempbuff = audioNBlock.getAudioBuffer();
        buffer->convertToFloat(sampleFmt, tempbuff, block);

        block->frameIndex = audioNBlock.getgFrameID();
        if(masterFlag == true)
        {
          // merge current available version
          buffer->merge(block);
        }

        block->reformatted = 1;
        buffer->updateWriteIndex();
      }

    }
  }

  // delete buffer
  if(buffer) {
    delete buffer;
    buffer = NULL;
  }

  return 0;
}

bool sageAudioReceiver::isActive()
{
  return activeRecv;
}

int sageAudioReceiver::getAudioId()
{
  if(buffer)
  {
    return buffer->getAudioId();
  }
  return -1;
}


int sageAudioReceiver::evalPerformance(char **frameStr, char **bandStr)
{
  //Calculate performance here
  double elapsedTime = perfTimer.getTimeUS();

  if (elapsedTime > 1000000.0*reportRate && reportRate > 0) {
    *bandStr = new char[TOKEN_LEN];

    float obsBandWidth = (float) (bandWidth * 8.0 / (elapsedTime));
    float obsLoss = (float) (packetLoss * 8.0 / (elapsedTime));
    bandWidth = 0;
    packetLoss = 0;
    // HYEJUNG
    /*
      sprintf(*bandStr, "%d %d %7.2f %7.2f", instID, shared->nodeID, obsBandWidth, obsLoss);

      bool active = false;
      for (int i=0; i<streamNum; i++) {
      if (queueList[i]->isActive()) {
      active = true;
      break;
      }
      }

      if (active) {
      *frameStr = new char[TOKEN_LEN];
      float frameRate = (float) ((updateFrame - oldFrame)*1000000.0/elapsedTime);
      oldFrame = updateFrame;
      sprintf(*frameStr, "%d %f", instID, frameRate);
      }*/
    // HYEJUNG

    perfTimer.reset();
  }

  return 0;
}
