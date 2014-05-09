/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudio.h
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

#ifndef _SAGE_AUDIO_H
#define _SAGE_AUDIO_H

// portaudio header file
#include <portaudio.h>

#ifdef linux1
#include <libiec61883/iec61883.h>
#include <sys/select.h>
#endif

#include "sageAudioModule.h"
#include "sageAudioCircBuf.h"
#include "sageBase.h"

class sageAudio {
public:
  /** audio mode
   */
  enum {AUDIO_PLAY, AUDIO_PAUSE, AUDIO_STOP};
public:
  sageAudio();
  ~sageAudio();

  /** set parameters for setting audio up and assign pointer of circular buffer
   */
  int init( int id, sageAudioConfig &conf, sageAudioMode mode, sageAudioCircBuf *buf );
  int reset( int id, sageAudioCircBuf *buf );

  /** open audio stream in play mode or capture mode
   */
  virtual int   openStream();

  /** close audio stream
   */
  virtual int   closeStream();

  int play();
  int pause();
  int stop();

  /** test audio input/output device
   */
  void testDevices();

  int getID();

  /** callback function for play audio stream
   */
  static int playCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData );

  /** callback function for capture audio stream
   */
  static int recordCallback( const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData );
#ifdef linux1

  static void* audioThread(void *args);

  /** callback function for capture firewire audio stream
   */
  static int recordFWCallback(iec61883_amdtp_t amdtp, char *data,
                              int nsamples, unsigned int dbc,
                              unsigned int dropped, void *callback_data);
#endif

protected:
  int ID;

  /** circular buffer for saving audio stream block
   */
  sageAudioCircBuf* buffer;

  /** audio sample format : float, short, char, unsigned char
   */
  sageSampleFmt sampleFmt;

  /** numbers of audio channel
   */
  int  channels;

  /** object for audio streaming : portaudio object
   */
  PaStream*   audioStream;

  /** parameters for audio streaming
   */
  PaStreamParameters*   audioParameters;

  /** audio mode : play, capture
   */
  sageAudioMode audioMode;

  /** sound card device number : 0 is default device number
   */
  int deviceNum;

  /** according to audio sample format, byte per sample is different
   */
  int bytesPerSample;

  /** audio sampling rate : default is 44100
   * standard sampling rate : 8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0
   */
  long samplingRate;

  /** frames per buffer : default is 1024
   */
  int  framePerBuffer;

  /** play flag : play, pause, stop
   */
  int   playFlag;

#ifdef linux1
  raw1394handle_t handle;
  iec61883_amdtp_t amdtp;
#endif

  /** latency
   */
  long minLatency;
  long maxLatency;

};

#endif
