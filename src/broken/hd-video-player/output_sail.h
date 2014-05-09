/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
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
 * Direct questions, comments etc about SAGE to http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

/* Output Pixel Format Define */
#ifndef _OUTPUT_SAIL_
#define _OUTPUT_SAIL_

//#include <stdio.h>

#include "sail.h"
#include "misc.h"

#define RGB16     90
#define RGB24   91
#define RGB32   92
#define YUV     93

#define FR10      100
#define FR15      115
#define FR20      120
#define FR25      125
#define FR30      130

extern int FrameRate;

class C_Output_sail {
  int m_width;
  int m_height;
  int m_byte_pixel;
  int m_color_mode;
  int m_flag_empty;
  unsigned char* m_frame_buf;
  bool m_sailstarted;

  char* m_execConfigName;
  int m_nwID;

  sail *sageInf;

  int m_flag_notinit;
  char* m_output_addr;
  int m_output_port;

  bool m_audioOn;       // audio on/off flag
#if defined (SAGE_AUDIO)
  sageAudioMode m_audioMode;  // audio mode : capture, play, read, write
  sageSampleFmt m_sampleFmt;  // audio sampling format : float, short, char, unsigned char
#endif
  int  m_audioPort;       // audio port number
  int  m_audioDeviceNum;    // device number
  long m_samplingRate;      // audio sampling rate : default-44100
  int  m_channels;        // number of audio channel
  int  m_framePerBuffer;    // audio frames per buffer
  int m_abuffsize;

  void SetZero(void);
  int sailInit(void);

public :

  C_Output_sail ();
  virtual ~C_Output_sail ();

  int init(int width, int height, int color_mode,char* output_addr, int output_port);
  void setSailEnv(char* execConfigName, int nwID);
  int push_data(unsigned char* buf , int buf_len);
  int push_audiodata(unsigned char* buf, int buf_len);
  void setAudioOn(bool on);
  int init(int size);
  int initAudioCaptureMode();
  void shutdown();

};



#endif
