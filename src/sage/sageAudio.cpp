/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  sageAudio.cpp
 *   Author   :   Hyejung Hur
 *   Description:   Code file for audio capture and play
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

#include "sageAudio.h"
#include "sageAudioModule.h"

sageAudio::sageAudio()
  : audioStream(NULL), audioParameters(NULL), audioMode(SAGE_AUDIO_CAPTURE), deviceNum(-1),
    sampleFmt(SAGE_SAMPLE_FLOAT32), samplingRate(44100), channels(2), framePerBuffer(1024),
    playFlag(AUDIO_STOP), buffer(NULL), minLatency(1000), maxLatency(0), ID(-1)
{
}

sageAudio::~sageAudio()
{
  closeStream();

  if( audioParameters != NULL) {
    delete audioParameters;
    audioParameters = NULL;
  }
}

int sageAudio::init(int id, sageAudioConfig &config, sageAudioMode mode, sageAudioCircBuf *buf)
{
  ID = id;

  // initialize audio setting variables
  sampleFmt = config.sampleFmt;
  samplingRate = config.samplingRate;
  channels = config.channels;
  framePerBuffer = config.framePerBuffer;
  deviceNum = config.deviceNum;

  std::cout << "sageAudio:: samplingRate: " <<  samplingRate << " channels: " << channels << " framePerBuffer: " << framePerBuffer << std::endl;
  std::cout << "sageAudio:: sampleFmt: ";

  switch(sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    bytesPerSample = sizeof(float);
    std::cout << "SAGE_SAMPLE_FLOAT32" ;
    break;
  case SAGE_SAMPLE_INT16 :
    bytesPerSample = sizeof(short);
    std::cout << "SAGE_SAMPLE_INT16" ;
    break;
  case SAGE_SAMPLE_INT8 :
    bytesPerSample = sizeof(char);
    std::cout << "SAGE_SAMPLE_INT8" ;
    break;
  case SAGE_SAMPLE_UINT8 :
    bytesPerSample = sizeof(unsigned char);
    std::cout << "SAGE_SAMPLE_UINT8" ;
    break;
  default :
    bytesPerSample = sizeof(float);
    sampleFmt = SAGE_SAMPLE_FLOAT32;
    break;
  }

  audioMode = mode;
  buffer = buf;

  std::cout << " mode: ";

  if(audioMode == SAGE_AUDIO_FWCAPTURE) {

#ifdef linux1
    handle = raw1394_new_handle_on_port (0);
    if(!handle)
    {
      std::cout << "sageAudio::init: failed to get libraw1394 handle" << std::endl;
      return -1;
    }
    // to capture
    // All audio data must be signed 16bit 44.1KHz stereo PCM
    sampleFmt = SAGE_SAMPLE_UINT8;
    samplingRate = 44100;
    channels = 2;
#endif

    std::cout << "SAGE_AUDIO_FWCAPTURE" << std::endl;
  }
  else {
    // set audio parameters
    audioParameters = new PaStreamParameters();
    switch(audioMode) {
    case SAGE_AUDIO_CAPTURE :
      /** todo */
      // device id...........
      audioParameters->device =  Pa_GetDefaultInputDevice();
      std::cout << "SAGE_AUDIO_CAPTURE" ;
      break;
    case SAGE_AUDIO_PLAY :
      if(deviceNum == -1)
      {
        audioParameters->device = Pa_GetDefaultOutputDevice();
      } else
      {
        audioParameters->device =  deviceNum;
      }
      std::cout << "SAGE_AUDIO_PLAY : " << audioParameters->device  ;
      break;
    default :
      break;
    }
    std::cout << std::endl;
    if(audioParameters->device < 0) {
      std::cerr << "sageAudio::init: could not find default device" << std::endl;
      return -1;
    }

    audioParameters->channelCount = channels;
    switch(sampleFmt)
    {
    case SAGE_SAMPLE_FLOAT32 :
      audioParameters->sampleFormat = paFloat32;
      break;
    case SAGE_SAMPLE_INT16 :
      audioParameters->sampleFormat = paInt16;
      break;
    case SAGE_SAMPLE_INT8 :
      audioParameters->sampleFormat = paInt8;
      break;
    case SAGE_SAMPLE_UINT8 :
      audioParameters->sampleFormat = paUInt8;
      break;
    default :
      break;
    }
    audioParameters->suggestedLatency = Pa_GetDeviceInfo( audioParameters->device )->defaultLowOutputLatency;
    audioParameters->hostApiSpecificStreamInfo = NULL;
  }
  //std::cout << "sageAudio::init: initialized" << std::endl;

  return 0;
}

int sageAudio::reset( int id, sageAudioCircBuf *buf )
{
  ID = id;
  buffer = buf;
  std::cout << "sageAudio::reset " << ID << std::endl;
  return 0;
}

int   sageAudio::openStream()
{
  if( buffer == NULL ) {
    std::cerr << "audio stream could not open because buffer is NULL" << std::endl;
    return -1;
  }

  if(audioMode == SAGE_AUDIO_FWCAPTURE) {

#ifdef linux1
    /*amdtp = iec61883_amdtp_recv_init (handle, sageAudio::recordFWCallback, (void *)this );
      if( amdtp == NULL)
      {
      std::cerr << "audio firewire stream could not be initiated" << std::endl;
      return -1;
      }*/
#endif

  }
  else {

    PaError err;
    if( audioStream != NULL ) {
      std::cerr << "audio stream is already opened" << std::endl;
      return -1;
    }

    if( audioMode == SAGE_AUDIO_CAPTURE )
    {
      err = Pa_OpenStream(
                          &audioStream,
                          audioParameters,
                          NULL,                  // no outputParameters
                          samplingRate,
                          framePerBuffer,
                          paClipOff,         // we won't output out of range samples so don't bother clipping them
                          sageAudio::recordCallback,
                          this );

      if( err != paNoError ) {
        std::cerr << "audio stream could not open" << std::endl;
        return err;
      }

    } else if ( audioMode == SAGE_AUDIO_PLAY )
    {
      err = Pa_OpenStream(
                          &audioStream,
                          NULL,                  // no inputParameters
                          audioParameters,
                          samplingRate,
                          framePerBuffer,
                          paClipOff,            // we won't output out of range samples so don't bother clipping them
                          sageAudio::playCallback,
                          this );

      if( err != paNoError ) {
        std::cerr << "audio stream could not open" << std::endl;
        return err;
      }
      std::cout << "audio stream is opened" << std::endl;
    }
    /** todo */
    //startTime = Pa_GetStreamTime( audioStream );
  }

  return 0;
}

int   sageAudio::closeStream()
{
  if(audioMode == SAGE_AUDIO_FWCAPTURE) {

#ifdef linux1
    iec61883_amdtp_close (amdtp);
#endif

  }
  else {
    if(audioStream == NULL) {
      return -1;
    }

    buffer = NULL;
    PaError err;
    err = Pa_CloseStream( audioStream );
    if( err != paNoError ) {
      std::cerr << "audio stream could not close" << std::endl;
      return err;
    }
    std::cerr << "audio stream is closed" << std::endl;
    audioStream = NULL;
  }

  return 0;
}

#ifdef linux1
void* sageAudio::audioThread(void *args)
{
  sageAudio* This = (sageAudio*) args;

  This->amdtp = iec61883_amdtp_recv_init (This->handle, recordFWCallback, (void *)This );
  if( !This->amdtp )
  {
    std::cerr << "audio firewire stream could not be initiated" << std::endl;
    return NULL;
  }
  if( iec61883_amdtp_recv_start (This->amdtp, 63) < 0 )
  {
    std::cerr << "audio firewire stream could not be started" << std::endl;
    return NULL;
  }

  int fd = raw1394_get_fd(This->handle);
  struct timeval tv;
  fd_set rfds;
  int result = 0;

  std::cerr << "sageAudio::audioThread:: starting to receive : fd: " << fd << std::endl;

  do {
    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 20000;

    if (select (fd + 1, &rfds, NULL, NULL, &tv) > 0) {
      if(FD_ISSET(fd, &rfds)) {
        result = raw1394_loop_iterate (This->handle);
        //std::cerr << "sageAudio::audioThread:: 1394 loop : " << result << std::endl;
      }
    }

  } while ((This->playFlag != AUDIO_STOP) && (result == 0));

  std::cerr << "sageAudio::audioThread:: done" << std::endl;

}
#endif

/** todo */
// more ......
int sageAudio::play()
{
  if(playFlag != AUDIO_PLAY)
  {
    if(audioMode == SAGE_AUDIO_FWCAPTURE) {

#ifdef linux1
      /*if( iec61883_amdtp_recv_start (amdtp, 63) < 0 )
        {
        std::cerr << "audio firewire stream could not be started" << std::endl;
        return -1;
        }*/
      pthread_t thId;
      if (pthread_create(&thId, 0, audioThread, (void*)this) != 0) {
        std::cerr << "sageAudio::play : can't create audioThread" << std::endl;
        return -1;
      }
#endif

    }
    else {
      PaError err;
      err = Pa_StartStream( audioStream );
      if( err != paNoError ) {
        std::cerr << "audio stream could not open" << std::endl;
        return err;
      }
    }
  }
  playFlag = AUDIO_PLAY;
  return 0;
}

int sageAudio::pause()
{
  if(playFlag == AUDIO_PLAY)
  {
    if(audioMode == SAGE_AUDIO_FWCAPTURE) {

#ifdef linux1
      iec61883_amdtp_recv_stop (amdtp);
#endif

    }
    else {
      PaError err;
      err = Pa_StopStream( audioStream );
      if( err != paNoError ) {
        std::cerr << "audio stream could not stop" << std::endl;
        return err;
      }
    }
  }

  playFlag = AUDIO_PAUSE;
  return 0;
}

int sageAudio::stop()
{
  if(playFlag == AUDIO_PLAY)
  {
    if(audioMode == SAGE_AUDIO_FWCAPTURE) {

#ifdef linux1
      iec61883_amdtp_recv_stop (amdtp);
#endif

    }
    else {
      PaError err;
      err = Pa_StopStream( audioStream );
      std::cout << "audio stream stop" << std::endl;
      if( err != paNoError ) {
        std::cerr << "audio stream could not stop" << std::endl;
        return err;
      }
    }
  }
  playFlag = AUDIO_STOP;
  return 0;
}

/** This routine will be called by the PortAudio engine when audio is needed.
    It may be called at interrupt level on some machines so don't do anything
    that could mess up the system like calling malloc() or free().
*/
int sageAudio::playCallback( const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData )
{
  if( outputBuffer == NULL ) {
    //std::cerr << "playCallback is return 1" << std::endl;
    return 1;
  }

  sageAudio *This = (sageAudio*)userData;
  //if (This ==  NULL) return 0;      // for safety.... it needs, but it's not possible to get NULL
  //if(This->buffer == NULL) return 0;
  audioBlock *block = This->buffer->readBlock();

  /** todo */
  // check later... it can make strange noisy
  if(block == NULL) {
    //std::cerr << "playCallback is return 0" << std::endl;
    return 0;
  }
  if(block->reformatted != 1) {
    //std::cerr << "playCallback is return 0" << std::endl;
    return 0;
  }
  //if(This->playFlag != AUDIO_PLAY) return 0;

  //std::cout << "playback : ";

  /*if( block->timestamp > 0) {   // neet to modify
    PaTime time = Pa_GetStreamTime(This->audioStream) - block->timestamp;
    double latency = time / framesPerBuffer;

    if( This->minLatency >= latency ) {
    This->minLatency = latency;
    }
    if( This->maxLatency <= latency ) {
    This->maxLatency = latency;
    }
    }*/


  switch(This->sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *rptr = (float*) block->buff;
      float *wptr = (float*) outputBuffer;

      if( block->reformatted != 1 ) {
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = 0;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = 0.0;
          }
        }
      }
      else {
        // update data for play
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = *rptr;
          *rptr=0;
          rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr;
            *rptr=0;
            rptr++;
          }
        }
      }
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *rptr = (short*) block->buff;
      short *wptr = (short*) outputBuffer;

      if( block->reformatted != 1 ) {
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = 0;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = 0;
          }
        }
      }
      else {
        // update data for play
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = *rptr;
          *rptr=0;
          rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr;
            *rptr=0;
            rptr++;
          }
        }
      }
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *rptr = (char*) block->buff;
      char *wptr = (char*) outputBuffer;

      if( block->reformatted != 1 ) {
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = '0';         // not sure....  it's right or not
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = '0';
          }
        }
      }
      else {
        // update data for play
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = *rptr;
          *rptr='0';
          rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr;
            *rptr='0';
            rptr++;
          }
        }
      }
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *rptr = (unsigned char*) block->buff;
      unsigned char *wptr = (unsigned char*) outputBuffer;

      if( block->reformatted != 1 ) {
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = '0';
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = '0';
          }
        }
      }
      else {
        // update data for play
        for( int i=0; i< framesPerBuffer; i++ ) {
          *wptr++ = *rptr;
          *rptr='0';
          rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr;
            *rptr='0';
            rptr++;
          }
        }
      }
    }
    break;
  default :
    break;
  }

  // reset
  block->reformatted = 0;
  //std::cout <<  block->frameIndex << ": " << This->buffer->getReadIndex();
  // update index
  This->buffer->updateReadIndex();

  //std::cout << ": " << This->buffer->getReadIndex() << std::endl;


  return 0;
}

#ifdef linux1
int sageAudio::recordFWCallback(iec61883_amdtp_t amdtp, char *data, int nsamples,
                                unsigned int dbc, unsigned int dropped, void *callback_data)
{
  sageAudio *This = (sageAudio *) callback_data;
  static int total_packets = 0;

  std::cout << "read_packet...." << std::endl;
  if (total_packets == 0) {
    std::cout << "format=0x" << iec61883_amdtp_get_format (amdtp) << " sample_format=0x" <<  iec61883_amdtp_get_sample_format (amdtp) <<
      " channels=" <<  iec61883_amdtp_get_dimension (amdtp) << " rate=" << iec61883_amdtp_get_rate (amdtp) << std::endl;
    // later can change....
    // have to check nsamples & framesPerBuffer
  }

  // TODO: convert to additional formats (20 or 24-bits).
  if (iec61883_amdtp_get_format (amdtp) == IEC61883_AMDTP_FORMAT_RAW && iec61883_amdtp_get_sample_format (amdtp) == IEC61883_AMDTP_INPUT_LE16 &&
      iec61883_amdtp_get_dimension (amdtp) == 2)
  {
    audioBlock *block = This->buffer->getNextWriteBlock();
    if(block == NULL) {
      //std::cout << "no block " << std::endl;
      return 0;
    }
    unsigned char *rptr = (unsigned char*) data;
    //quadlet_t *rptr = (quadlet_t *) data;
    unsigned char *wptr = (unsigned char*) block->buff;

    // update audio data
    if( data == NULL ) {
      for( int i=0; i < This->framePerBuffer; i++ ) {
        *wptr++ = 0;
        for( int j= 1; j < This->channels; j++) {
          *wptr++ = 0;
        }
      }
    }
    else {
      for( int i=0; i < This->framePerBuffer; i++ )
      {
        *wptr++ = *rptr++;
        for( int j= 1; j < This->channels; j++) {
          *wptr++ = *rptr++;
        }
      }
    }
    /*for (int i = 0; i < nsamples; i++) {
    // the first byte contains a label - skip it
    *wptr++ = (sample[i] >> 8);
    *wptr++ = (sample[i] >> 16);
    }*/
  }
  return 0;
}
#endif

/** This routine will be called by the PortAudio engine when audio is needed.
    It may be called at interrupt level on some machines so don't do anything
    that could mess up the system like calling malloc() or free().
*/
int sageAudio::recordCallback( const void *inputBuffer, void *outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData )
{
  if(inputBuffer == NULL) return 1;

  sageAudio *This = (sageAudio*)userData;
  //if (This ==  NULL) return 0;      // for safety.... it needs, but it's not possible to get NULL
  //if(This->buffer == NULL) return 0;
  audioBlock *block = This->buffer->getNextWriteBlock();

  while(block == NULL) {
    sage::usleep(10);
    block = This->buffer->getNextWriteBlock();
    //std::cout << "trying to get buffer..." << std::endl;
  }


  /** todo */
  // check later... it can make strange noisy
  if(block == NULL) {
    //std::cout << "no block " << std::endl;
    return 0;
  }
  //if(block->reformatted != 1) return 0;
  //if(This->playFlag != AUDIO_PLAY) return -1;

  switch(This->sampleFmt) {
  case SAGE_SAMPLE_FLOAT32 :
    {
      float *rptr = (float*) inputBuffer;
      float *wptr = (float*) block->buff;

      // update audio data
      if( inputBuffer == NULL ) {
        for( int i=0; i < framesPerBuffer; i++ ) {
          *wptr++ = 0;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = 0;
          }
        }

      }
      else {
        for( int i=0; i < framesPerBuffer; i++ )
        {
          *wptr++ = *rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr++;
          }
        }
      }
    }
    break;
  case SAGE_SAMPLE_INT16 :
    {
      short *rptr = (short*) inputBuffer;
      short *wptr = (short*) block->buff;

      // update audio data
      if( inputBuffer == NULL ) {
        for( int i=0; i < framesPerBuffer; i++ ) {
          *wptr++ = 0;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = 0;
          }
        }

      }
      else {
        for( int i=0; i < framesPerBuffer; i++ )
        {
          *wptr++ = *rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr++;
          }
        }
      }
    }
    break;
  case SAGE_SAMPLE_INT8 :
    {
      char *rptr = (char*) inputBuffer;
      char *wptr = (char*) block->buff;

      // update audio data
      if( inputBuffer == NULL ) {
        for( int i=0; i < framesPerBuffer; i++ ) {
          *wptr++ = 0;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = 0;
          }
        }

      }
      else {
        for( int i=0; i < framesPerBuffer; i++ )
        {
          *wptr++ = *rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr++;
          }
        }
      }
    }
    break;
  case SAGE_SAMPLE_UINT8 :
    {
      unsigned char *rptr = (unsigned char*) inputBuffer;
      unsigned char *wptr = (unsigned char*) block->buff;

      // update audio data
      if( inputBuffer == NULL ) {
        for( int i=0; i < framesPerBuffer; i++ ) {
          *wptr++ = 0;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = 0;
          }
        }

      }
      else {
        for( int i=0; i < framesPerBuffer; i++ )
        {
          *wptr++ = *rptr++;
          for( int j= 1; j < This->channels; j++) {
            *wptr++ = *rptr++;
          }
        }
      }
    }
    break;
  default :
    break;
  }

  //block->frameIndex = This->frameIndex;
  block->frameIndex = This->buffer->getWriteIndex();
  block->gframeIndex = sageAudioModule::_instance->getgFrameNum();
  block->reformatted = 1;
  block->timestamp = Pa_GetStreamTime(This->audioStream);

  // update writeIndex
  This->buffer->updateWriteIndex();

  //std::cout << "recordCallback : " << block->frameIndex << ": " << This->buffer->getWriteIndex() << std::endl;

  return 0;
}

/** test each output device, through its full range of capabilities.
 */
void sageAudio::testDevices()
{
  int     i, numDevices, defaultDisplayed;
  const   PaDeviceInfo *deviceInfo;
  PaStreamParameters inputParameters, outputParameters;
  PaError err;

  numDevices = Pa_GetDeviceCount();
  if( numDevices < 0 )
  {
    std::cerr << "Pa_CountDevices returned 0x" <<  numDevices << std::endl;
    err = numDevices;
    return;
  }

  std::cout << "number of devices = " << numDevices << std::endl;
  for( i=0; i<numDevices; i++ )
  {
    deviceInfo = Pa_GetDeviceInfo( i );
    std::cout << "--------------------------------------- device #" << i << std::endl;

    /* Mark global and API specific default devices */
    defaultDisplayed = 0;
    if( i == Pa_GetDefaultInputDevice() )
    {
      std::cout << "[ Default Input";
      defaultDisplayed = 1;
    }
    else if( i == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultInputDevice )
    {
      const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
      std::cout << "[ Default " << hostInfo->name  << " Input" ;
      defaultDisplayed = 1;
    }

    if( i == Pa_GetDefaultOutputDevice() )
    {
      std::cout << (defaultDisplayed ? "," : "[");
      std::cout << " Default Output";
      defaultDisplayed = 1;
    }
    else if( i == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultOutputDevice )
    {
      const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
      std::cout << (defaultDisplayed ? "," : "[") ;
      std::cout << " Default " << hostInfo->name << " Output";
      defaultDisplayed = 1;
    }

    if( defaultDisplayed )
      std::cout << " ]" << std::endl;

    /* print device info fields */
    std::cout << "Name                        = " << deviceInfo->name << std::endl;
    std::cout << "Host API                    = " << Pa_GetHostApiInfo( deviceInfo->hostApi )->name << std::endl;
    std::cout << "Max inputs = " << deviceInfo->maxInputChannels;
    std::cout << ", Max outputs = " << deviceInfo->maxOutputChannels  << std::endl;

    std::cout << "Default low input latency   = " << deviceInfo->defaultLowInputLatency  << std::endl;
    std::cout << "Default low output latency  = " << deviceInfo->defaultLowOutputLatency  << std::endl;
    std::cout << "Default high input latency  = " << deviceInfo->defaultHighInputLatency  << std::endl;
    std::cout << "Default high output latency = " << deviceInfo->defaultHighOutputLatency << std::endl;

    std::cout << "Default sample rate         = " << deviceInfo->defaultSampleRate << std::endl;

    /* poll for standard sample rates */
    inputParameters.device = i;
    inputParameters.channelCount = deviceInfo->maxInputChannels;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = i;
    outputParameters.channelCount = deviceInfo->maxOutputChannels;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if( inputParameters.channelCount > 0 )
    {
      std::cout << "Supported standard sample rates\n for half-duplex 16 bit" << inputParameters.channelCount << "  channel input = " << std::endl;
      std::cout << "8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 2250.0, 24000.0, 32000.0, " << std::endl;
      std::cout << "44100.0, 48000.0, 88200.0, 96000.0, 192000.0" << std::endl;
    }
    if( outputParameters.channelCount > 0 )
    {
      std::cout << "Supported standard sample rates\n for half-duplex 16 bit" << outputParameters.channelCount << "  channel output = " << std::endl;
      std::cout << "8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 2250.0, 24000.0, 32000.0, " << std::endl;
      std::cout << "44100.0, 48000.0, 88200.0, 96000.0, 192000.0" << std::endl;
    }
    if( inputParameters.channelCount > 0 && outputParameters.channelCount > 0 )
    {
      std::cout << "Supported standard sample rates\n for half-duplex 16 bit" << inputParameters.channelCount << "  channel input, " << outputParameters.channelCount << "  channel output = " << std::endl;
      std::cout << "8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 2250.0, 24000.0, 32000.0, " << std::endl;
      std::cout << "44100.0, 48000.0, 88200.0, 96000.0, 192000.0" << std::endl;
    }
  }
}


int sageAudio::getID()
{
  return ID;
}
