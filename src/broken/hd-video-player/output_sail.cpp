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

#include "stdafx.h"
#include "output_sail.h"


C_Output_sail::C_Output_sail() {
  SetZero();
  sageInf = NULL;
}

void C_Output_sail::SetZero() {
  m_width = 0;
  m_height = 0;
  m_byte_pixel = 0;
  m_color_mode = 0;
  m_flag_empty = 0;
  m_frame_buf = 0;

  m_execConfigName = NULL;
  m_nwID = 0;

  m_flag_notinit = 1;
  m_output_addr = 0;
  m_output_port = 0;

  //m_audioOn =true;
  m_sailstarted =false;
}

//-------------------------------------------------------------------------------------
void C_Output_sail::setSailEnv(char* execConfigName, int nwID) {
  m_execConfigName = execConfigName;
  m_nwID = nwID;

  fprintf(stderr, "\r\n m_exec : %s , m_nwID : %d", execConfigName, nwID);
}


//-------------------------------------------------------------------------------------
int C_Output_sail::sailInit()
{
  sailConfig scfg;
  sageRect glImageMap;

  glImageMap.left = 0.0;
  glImageMap.right = 1.0;
  glImageMap.bottom = 0.0;
  glImageMap.top = 1.0;


  if(sageInf != NULL) {
    sageInf->shutdown();
    delete sageInf;
    sageInf = NULL;
  }
  sageInf = new sail();

#if defined(SAGE_VERSION1)
  scfg.cfgFile = strdup("sage.conf");
  scfg.appName = strdup("svt");
  scfg.rank = 0;
  scfg.ip = NULL; // strdup("192.168.81.120");
  scfg.execConfig = NULL;
  scfg.nwID = 2;

  if(m_color_mode == RGB24) {
    m_byte_pixel = 3;
    scfg.colorDepth = 24;
    scfg.pixFmt = TVPIXFMT_888_INV;
  } else if(m_color_mode == RGB16) {
    m_byte_pixel = 2;
    scfg.colorDepth = 16;
    scfg.pixFmt = TVPIXFMT_565;
  } else return -1;

  scfg.resX = m_width;
  scfg.resY = m_height;
  scfg.imageMap = glImageMap;
  scfg.rowOrd = BOTTOM_TO_TOP;

  //m_frame_buf = (unsigned char *)malloc(m_width * m_height * m_byte_pixel);
  //memset(m_frame_buf, 0, scfg.resX *scfg.resY*m_byte_pixel);

#else
  sage::win32Init();

  if(m_audioOn) {
#if defined (SAGE_AUDIO)
    scfg.framePerBuffer = m_framePerBuffer;
#endif
  }

  scfg.init("svt.conf");
  scfg.setAppName("svt");
  scfg.rank = 0;
  scfg.master = true;
  scfg.nwID = 1;
  scfg.syncPort = 13000;
  scfg.nodeNum = 1;
  scfg.blockX= 64; // 128; // 64;
  scfg.blockY= 64; // 90;  // 64;
  sprintf(scfg.fsIP, "%s", m_output_addr);

  if(m_color_mode == RGB24) {
    m_byte_pixel = 3;
    scfg.pixFmt = PIXFMT_888_INV; //PIXFMT_888;
    scfg.rowOrd = BOTTOM_TO_TOP;
    SAGE_PRINTLOG("SAGE> color mode 24 888 bits\n");
  } else if(m_color_mode == RGB16) {
    m_byte_pixel = 2;
    scfg.pixFmt = PIXFMT_565;
    scfg.rowOrd = BOTTOM_TO_TOP;
    SAGE_PRINTLOG("SAGE> color mode 16 565 bits\n");
  } else if(m_color_mode == YUV) {
    m_byte_pixel = 2;
    scfg.pixFmt = PIXFMT_YUV;
    scfg.rowOrd = TOP_TO_BOTTOM;
    SAGE_PRINTLOG("SAGE> color mode YUV bits\n");
  } else return -1;

  scfg.resX = m_width;
  scfg.resY = m_height;
  scfg.imageMap = glImageMap;

  // Position and size on the display side
  scfg.winX = 0;
  scfg.winY = 0;
  scfg.winWidth = m_width*1;
  scfg.winHeight = m_height*1;

  SAGE_PRINTLOG("SAGE> Init: w %d h %d\n\n", m_width, m_height);
  // for setting up audio configuration
  if(m_audioOn) {
#if defined (SAGE_AUDIO)
    scfg.audioMode = m_audioMode;
    scfg.audioPort = m_audioPort;
    scfg.audioDeviceNum = m_audioDeviceNum;
    scfg.sampleFmt = m_sampleFmt;
    scfg.samplingRate = m_samplingRate;
    scfg.channels = m_channels;
    //scfg.framePerBuffer = m_framePerBuffer;
    scfg.audioOn = true;
    if(m_audioMode == SAGE_AUDIO_APP) {
      scfg.audioBuffSize = m_abuffsize;
    }
#endif
  }

#endif

  m_sailstarted  = true;
  sageInf->init(scfg);

  return 0;
}

//-------------------------------------------------------------------------------------
int C_Output_sail::init(int width, int height, int color_mode, char* output_addr, int output_port)
{
  SetZero();
  m_width = width;
  m_height = height;

  if(output_addr == NULL)
  {
    fprintf(stderr, "\r\n Output Address is needed");
    //            g_done_control = 1;
    return -1;
  } else
    m_output_addr = strdup(output_addr);

  m_output_port = output_port;
  m_color_mode = color_mode;

#if defined (SAGE_AUDIO)
  if(m_audioOn == false) {
    if(m_flag_notinit) {
      return sailInit();
      //m_flag_notinit = 0;
    }
  }
#endif

  return 0;
}



//-------------------------------------------------------------------------------------
int C_Output_sail::push_data(unsigned char* buf , int buf_len)
{
  static int framenum = 0;

  // check some messages
  sageMessage msg;
  if (sageInf->checkMsg(msg, false) > 0) {
    switch (msg.getCode()) {
    case APP_QUIT : {
      //g_done_control = 1;
      return -1;
    }

    }
  }

#if defined(SAGE_VERSION1)
  //memcpy(m_frame_buf, buf, m_width *m_height*m_byte_pixel);
  if(buf == NULL || sageInf->swapBuffer(buf) == -1) {
    //g_done_control = 1;
    return -1;
  }
#else

#endif
  if(buf == NULL)
  {
    //g_done_control = 1;
    return -1;
  }
  else
  {
    switch (FrameRate) {
    case FR10:
      if ( (framenum % 3) != 0 ) {
        framenum = (framenum + 1) % 30;
        return 0;
      }
      break;
    case FR15:
      if ( (framenum % 2) == 0 ) {
        framenum = (framenum + 1) % 30;
        return 0;
      }
      break;
    case FR20:
      if ( (framenum % 3) == 0 ) {
        framenum = (framenum + 1) % 30;
        return 0;
      }
      break;
    case FR25:
      if ( (framenum == 1) || (framenum == 7) || (framenum == 13) || (framenum == 19) || (framenum == 25) ) {
        framenum = (framenum + 1) % 30;
        return 0;
      }
      break;
    case FR30:
      break;
    default:
      break;
    }

    void *m_frame = sageInf->getBuffer();

    memcpy(m_frame, buf, m_width *m_height*m_byte_pixel);

    sageInf->swapBuffer();

    framenum = (framenum + 1) % 30;
  }

  return 0;
}

//-------------------------------------------------------------------------------------
int C_Output_sail::push_audiodata(unsigned char* buf, int buf_len)
{
#if defined(SAGE_AUDIO)
  sageInf->pushAudioData(buf_len, (void*)buf);
#endif
  return 0;
}

void C_Output_sail::setAudioOn(bool on)
{
  m_audioOn= on;
}


int C_Output_sail::init(int size)
{
  // for the test
#if defined (SAGE_AUDIO)
  m_audioMode = SAGE_AUDIO_APP;
  m_sampleFmt = SAGE_SAMPLE_INT16;
#endif
  m_audioPort = 26000;
  m_audioDeviceNum = -1;
  m_samplingRate = 48000; //48000;      // audio sampling rate : default-44100
  m_channels =2;        // number of audio channel
  m_framePerBuffer = 512;//1024; // 1536;   // audio frames per buffer 1536 /  2 = 768
  m_abuffsize = size;

  if(m_flag_notinit) {
    return sailInit();
    //m_flag_notinit = 0;
  }
  return -1;
}

int C_Output_sail::initAudioCaptureMode()
{
#if defined (SAGE_AUDIO)
  m_audioMode = SAGE_AUDIO_CAPTURE;
  m_sampleFmt = SAGE_SAMPLE_INT16;
#endif
  m_audioPort = 26000;
  m_audioDeviceNum = 0;
  m_samplingRate = 44100;     // audio sampling rate : default-44100
  m_channels =2;          // number of audio channel
  m_framePerBuffer = 512; // 1536;    // audio frames per buffer 1024 /  2 = 512

  if(m_flag_notinit) {
    return sailInit();
    //m_flag_notinit = 0;
  }
  return -1;
}

void C_Output_sail::shutdown()
{
  if(m_sailstarted == true) {
#if defined (SAGE_AUDIO)
    // Question ????
    //sageInf->stop();
#endif
    sageInf->shutdown();
    m_sailstarted = false;
    Sleep(100);
    if(sageInf) {
      delete sageInf;
      sageInf = NULL;
    }
  }
}

C_Output_sail::~C_Output_sail ()
{
  if(sageInf) {
    sageInf->shutdown();
    delete sageInf;
    sageInf = NULL;
  }
}
