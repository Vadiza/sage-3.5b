/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  sageAudioModule.cpp
 *   Author   :   Hyejung Hur
 *   Description:   Code file for managing audio module-create audio, play, capture, save it
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
 ***************************************************************************************************************************/

#include "sageAudioModule.h"
#include "sageAudio.h"
#include "sail.h"

sageAudioModule* sageAudioModule::_instance = NULL;

sageAudioModule::sageAudioModule()
  : fileReader(NULL), fileWriter(NULL), formatManager(NULL), gFrameNum(0)
{
  audioList.clear();
  PaError err = Pa_Initialize();
  if( err != paNoError ) {
    std::cerr << "portaudio could not initializ" << std::endl;
  }
}

sageAudioModule::~sageAudioModule()
{
  if(fileWriter != NULL) {
    fileWriter->stop();
  }

  if(fileReader != NULL) {
    fileReader->stop();
  }

  sage::usleep(1000);

  std::vector<sageAudio*>::iterator iter;
  sageAudio* audio = NULL;
  for(iter = audioList.begin(); iter != audioList.end(); iter++)
  {
    std::cout << "-------------------> 2" << std::endl;
    audio = (sageAudio*) *iter;
    audio->stop();
    audio->closeStream();
  }

  Pa_Terminate();
  sage::usleep(2000);

  for(iter = audioList.begin(); iter != audioList.end(); iter++)
  {
    audio = (sageAudio*) *iter;
    delete audio;
    audio = NULL;
  }
  audioList.clear();

  if(fileReader != NULL) {
    delete fileReader;
    fileReader = NULL;
  }

  if(fileWriter != NULL) {
    delete fileWriter;
    fileWriter = NULL;
  }

  if(formatManager != NULL) {
    delete formatManager;
    formatManager = NULL;
  }

  std::vector<sageAudioCircBuf*>::iterator iterBuffer;
  sageAudioCircBuf* buffer = NULL;
  for(iterBuffer = bufferList.begin(); iterBuffer != bufferList.end(); iterBuffer++)
  {
    buffer = (sageAudioCircBuf*) *iterBuffer;
    delete buffer;
    buffer = NULL;
  }
  bufferList.clear();
}


sageAudioModule* sageAudioModule::instance()
{
  if(_instance == NULL) {
    _instance = new sageAudioModule();
  }
  return _instance;
}

void sageAudioModule::init(sailConfig &conf)
{
  config.deviceNum = conf.audioDeviceNum;
  config.sampleFmt = conf.sampleFmt;
  config.samplingRate = conf.samplingRate;
  config.channels = conf.channels;
  config.framePerBuffer = conf.framePerBuffer;
  config.audioMode = conf.audioMode;

  init();
}

void sageAudioModule::init(sageAudioConfig &conf)
{
  config.deviceNum = conf.deviceNum;
  config.sampleFmt = conf.sampleFmt;
  config.samplingRate = conf.samplingRate;
  config.channels = conf.channels;
  config.framePerBuffer = conf.framePerBuffer;
  config.audioMode = conf.audioMode;

  init();
}

void sageAudioModule::init()
{
  // create audio format manager
  if(formatManager != NULL) {
    delete formatManager;
    formatManager = NULL;
  }
  formatManager = new audioFormatManager();

  // create objects for reading and writing audio files
  if(fileReader != NULL) {
    delete fileReader;
    fileReader = NULL;
  }
  fileReader = new audioFileReader(formatManager);
  fileReader->init();

  if(fileWriter != NULL) {
    delete fileWriter;
    fileWriter = NULL;
  }
  fileWriter = new audioFileWriter(formatManager);
  fileWriter->init();

  std::cout << "sageAudioModule::init: initialized" << std::endl;

}

sageAudioConfig* sageAudioModule::getAudioConfig()
{
  return &config;
}

void sageAudioModule::updateConfig(sageAudioConfig &conf)
{
  config.deviceNum = conf.deviceNum;
  config.sampleFmt = conf.sampleFmt;
  config.samplingRate = conf.samplingRate;
  config.channels = conf.channels;
  config.framePerBuffer = conf.framePerBuffer;
  config.audioMode = conf.audioMode;

  //std::cout << "----" << conf.framePerBuffer << std::endl;
}

sageAudioCircBuf* sageAudioModule::load(char* filename, bool loop, int nframes, long totalframes)
{
  /** todo */
  // give the loading job to fileReader
  if(fileReader != NULL) {
    return (fileReader->load(filename, loop, nframes, totalframes));
  }

  return NULL;
}

int sageAudioModule::save(char* filename)
{
  /** todo */
  // give the loading job to fileReader
  if(fileWriter != NULL) {
    fileWriter->write(filename, config, bufferList[0]);   // need to change...
  }

  return 0;
}

int sageAudioModule::play(int id)
{
  sageAudio* audio = NULL;
  if(config.audioMode == SAGE_AUDIO_CAPTURE || config.audioMode == SAGE_AUDIO_FWCAPTURE || config.audioMode == SAGE_AUDIO_PLAY) {
    std::vector<sageAudio*>::iterator iter;
    std::cout << "audioList size = " << audioList.size() << std::endl;
    for( iter = audioList.begin(); iter != audioList.end(); iter++)
    {
      audio = (sageAudio*) *iter;
      std::cout << "audio id = " << id << " " << audio->getID() << "?" << std::endl;
      if(audio->getID() == id)
      {
        audio->play();
        return 0;
      }
    }
  }

  if(config.audioMode == SAGE_AUDIO_READ) {
    fileReader->start();
  } else if(config.audioMode == SAGE_AUDIO_WRITE) {
    fileWriter->start();
  }

  // for testing
  /*f(config.audioMode == SAGE_AUDIO_APP) {
    fileWriter->start();
    }*/

  if(config.audioMode == SAGE_AUDIO_PLAY) {
    // find it from buffer
    std::vector<sageAudioCircBuf*>::iterator iterBuffer;
    sageAudioCircBuf* temp = NULL;
    for(iterBuffer = bufferList.begin(); iterBuffer != bufferList.end(); iterBuffer++)
    {
      temp = (sageAudioCircBuf*) *iterBuffer;
      if(temp->getAudioId() == id)
      {
        break;
      }
    }
    // apply it to audio
    if((temp != NULL) && (audioList.size() > 0))
    {
      std::cout << " audio id " << temp->getAudioId() << std::endl;
      audio = audioList[0];
      audio->reset(id, temp);
      audio->openStream();
      audio->play();
    }
  }

  return 0;
}

int sageAudioModule::pause(int id)
{
  if(config.audioMode == SAGE_AUDIO_CAPTURE || config.audioMode == SAGE_AUDIO_PLAY) {
    std::vector<sageAudio*>::iterator iter;
    sageAudio* audio = NULL;
    for( iter = audioList.begin(); iter != audioList.end(); iter++)
    {
      audio = (sageAudio*) *iter;
      if(audio->getID() == id)
      {
        audio->pause();
      }
    }
  }
  else if(config.audioMode == SAGE_AUDIO_READ) {
    //fileReader->();
  }
  else if(config.audioMode == SAGE_AUDIO_WRITE) {
  }

  return 0;
}

int sageAudioModule::stop(int id)
{
  sageAudio* audio = NULL;
  if(config.audioMode == SAGE_AUDIO_CAPTURE || config.audioMode == SAGE_AUDIO_PLAY) {
    std::vector<sageAudio*>::iterator iter;
    for( iter = audioList.begin(); iter != audioList.end(); iter++)
    {
      audio = (sageAudio*) *iter;
      if(audio->getID() == id)
      {
        audio->stop();
        audio->closeStream();
      }
    }
  }
  else if(config.audioMode == SAGE_AUDIO_READ) {
    fileReader->stop();
  }
  else if(config.audioMode == SAGE_AUDIO_WRITE) {
  }

  if(config.audioMode == SAGE_AUDIO_PLAY) {
    // delete buffer
    // audio create for
    if(audio != NULL)
    {
      // find buffer and delete it
      std::vector<sageAudioCircBuf*>::iterator iterBuffer;
      sageAudioCircBuf* temp = NULL;
      std::cout << "bufferlist size = " << bufferList.size() << " id "<< std::endl;
      for(iterBuffer = bufferList.begin(); iterBuffer != bufferList.end(); iterBuffer++)
      {
        temp = (sageAudioCircBuf*) *iterBuffer;
        std::cout << "buffer  : " << temp->getAudioId() << " " << audio->getID() << std::endl;
        //if(temp->getAudioId() == audio->getID())
        if(temp->getAudioId() == id)
        {
          bufferList.erase(iterBuffer);
          break;
        }
      }

      std::cout << "bufferlist size = " << bufferList.size() << std::endl;
      // check buffer size
      // if buffer size is still more then one
      if(bufferList.size() > 0)
      {
        // return next buffer's audio id
        std::cout << "return  : " << bufferList[0]->getAudioId() << std::endl;
        return bufferList[0]->getAudioId();
      } else
      {
        std::cout << "delete audio" << std::endl;
        delete audio;
        audio = NULL;
        audioList.clear();
      }
    }
  }

  return -1;
}

void sageAudioModule::testDevices()
{
  std::vector<sageAudio*>::iterator iter;
  sageAudio* audio = NULL;
  iter = audioList.begin();
  audio = (sageAudio*) *iter;
  if(audio != NULL)
  {
    audio->testDevices();
  }
}

std::vector<sageAudioCircBuf*>& sageAudioModule::getBufferList(void)
{
  return bufferList;
}

sageAudioCircBuf* sageAudioModule::createObject(int instID)
{
  return createObject(config, instID);
}

sageAudioCircBuf* sageAudioModule::createObject(sageAudioConfig &conf, int instID)
{
  int id = bufferList.size();
  if(instID >= 0)
    id = instID;

  // create shared circular buffer for audio data
  sageAudioCircBuf *objectBuffer= new sageAudioCircBuf();
  int bufferBlockNum = 16;
  //int bufferBlockNum = 64;
  int newBufSize = conf.framePerBuffer * conf.channels;

  int err = -1;
  // according to mode, create objects and set init values to the objects
  // create audio
  sageAudio* audio = NULL;
  if(conf.audioMode == SAGE_AUDIO_CAPTURE)
  {
    objectBuffer->init(id, bufferBlockNum, conf.sampleFmt, newBufSize);
    audio = new sageAudio();
    err = audio->init(id, conf, SAGE_AUDIO_CAPTURE, objectBuffer);
    if(err <0) {
      delete objectBuffer;
      delete audio;
      objectBuffer = NULL;
      audio = NULL;
      std::cerr << "sageAudioModule::createObject: failed" << std::endl;
      return NULL;
    }
    audio->openStream();
    std::cout << "sageAudioModule::createObject: audio object is created" << std::endl;
  }
  else if(conf.audioMode == SAGE_AUDIO_PLAY)
  {
    conf.sampleFmt = SAGE_SAMPLE_FLOAT32;
    objectBuffer->init(id, bufferBlockNum, conf.sampleFmt, newBufSize);
    if(audioList.size() == 0)
    {
      audio = new sageAudio();
      err = audio->init(id, conf, SAGE_AUDIO_PLAY, objectBuffer);
      if(err <0) {
        delete objectBuffer;
        delete audio;
        objectBuffer = NULL;
        audio = NULL;
        std::cerr << "sageAudioModule::createObject: failed" << std::endl;
        return NULL;
      }
      audio->openStream();
      std::cout << "sageAudioModule::createObject: audio object is created" << std::endl;
    }
    else
    {
      std::cout << "sageAudioModule::createObject: audio buffer is created" << std::endl;
    }
  }
  else if(conf.audioMode == SAGE_AUDIO_FWCAPTURE)
  {
    objectBuffer->init(id, bufferBlockNum, conf.sampleFmt, newBufSize);
    audio = new sageAudio();
    err = audio->init(id, conf, SAGE_AUDIO_FWCAPTURE, objectBuffer);
    if(err <0) {
      delete objectBuffer;
      delete audio;
      objectBuffer = NULL;
      audio = NULL;
      std::cerr << "sageAudioModule::createObject: failed" << std::endl;
      return NULL;
    }
    audio->openStream();
    std::cout << "sageAudioModule::createObject: audio object is created" << std::endl;
  }

  bufferList.push_back(objectBuffer);
  if(audio)
    audioList.push_back(audio);

  return objectBuffer;

}

int sageAudioModule::deleteObject(int id)
{

  stop(id);
  /** todo */
  // delete object
  // delete buffer
  // remove from the list

  return 0;
}

sageAudioCircBuf* sageAudioModule::createBuffer(int instID)
{
  return createBuffer(config);
}

sageAudioCircBuf* sageAudioModule::createBuffer(sageAudioConfig &conf, int size, int instID)
{
  int id = -1;
  if(instID >= 0)
    id = instID;

  // create shared circular buffer for audio data
  sageAudioCircBuf *objectBuffer= new sageAudioCircBuf();
  //int bufferBlockNum = 16;
  int bufferBlockNum = size;
  int newBufSize = conf.framePerBuffer * conf.channels;
  objectBuffer->init(id, bufferBlockNum, conf.sampleFmt, newBufSize);

  bufferList.push_back(objectBuffer);

  std::cout << "sageAudioModule::createBuffer: audio buffer is created" << std::endl;

  config.sampleFmt = conf.sampleFmt;
  config.samplingRate = conf.samplingRate;
  config.channels = conf.channels;
  config.framePerBuffer = conf.framePerBuffer;

  return objectBuffer;

}

void sageAudioModule::setgFrameNum(long frames)
{
  gFrameNum = frames;
}
long sageAudioModule::getgFrameNum()
{
  return gFrameNum;
}
