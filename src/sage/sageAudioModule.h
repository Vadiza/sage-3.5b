/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioModule.h
 * Author : Hyejung Hur
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

#ifndef _SAGE_AUDIO_MODULE_H
#define _SAGE_AUDIO_MODULE_H

#include <vector>

#include "sageBase.h"
#include "sageAudioCircBuf.h"
#include "audioFileReader.h"
#include "audioFileWriter.h"
#include "audioFormatManager.h"

class sailConfig;
class sageAudio;

/**
 * class sageAudioConfig
 */
class sageAudioConfig {
public:
  int  deviceNum;
  sageSampleFmt sampleFmt;
  long samplingRate;
  int  channels;
  int  framePerBuffer;
  sageAudioMode audioMode;

  sageAudioConfig() : deviceNum(-1), sampleFmt(SAGE_SAMPLE_FLOAT32), samplingRate(44100), channels(2), framePerBuffer(1024), audioMode(SAGE_AUDIO_CAPTURE) {}
  ~sageAudioConfig() { }
};


/**
 * \brief one instance per sageAudioManager
 */
class sageAudioModule {
public:
  /** this class is singleton
   */
  static sageAudioModule* _instance;

  /** create instance and assign it to _instance
   */
  static sageAudioModule* instance();

  /** sets parameters for audio configuration
   * will call init() after setting parameters
   */
  void init(sailConfig &conf);
  virtual void init(sageAudioConfig &conf);
  void updateConfig(sageAudioConfig &conf);

  sageAudioCircBuf* load(char* filename, bool loop, int nframes, long totalframes=0);
  int save(char* filename);

  /** open audio stream and play
   */
  int play(int id);

  int pause(int id);
  int stop(int id);

  /**
   * sageAudio is instantiated in here
   */
  sageAudioCircBuf* createObject(sageAudioConfig &conf, int instID =-1);
  // temporaly....
  sageAudioCircBuf* createObject(int instID =-1);
  int deleteObject(int id);

  sageAudioCircBuf* createBuffer(int instID=-1);
  sageAudioCircBuf* createBuffer(sageAudioConfig &conf, int size=64, int instID=-1);

  /** test audio input/output device
   */
  void testDevices();

  ~sageAudioModule();

  void setgFrameNum(long frames);
  long getgFrameNum();

  std::vector<sageAudioCircBuf*>& getBufferList(void);
  sageAudioConfig* getAudioConfig();

protected:
  /**
   * initialize() of PortAudio lib is called
   */
  sageAudioModule();

  /** create audio and create circular buffer for saving audio block.
   * New audioFormatManager, audioFileReader, and audioFileWriter objects are generated.
   */
  void init();

protected:
  /** audio pointer
   */
  //sageAudio* audio;
  std::vector<sageAudio*> audioList;

  /** audio circular buffer list
   */
  std::vector<sageAudioCircBuf*> bufferList;

  /** audio configuration
   */
  sageAudioConfig config;

  audioFileReader* fileReader;
  audioFileWriter* fileWriter;
  audioFormatManager* formatManager;

  long gFrameNum;

};

#endif
