/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sail.h
 * Author : Byungil Jeong
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
 * Direct questions, comments etc about SAGE to sage_users@listserv.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#ifndef SAIL_H_
#define SAIL_H_

#include "sageConfig.h"
//#include "appUI.h"
#include <queue>
#include <string>

#ifndef GL_ABGR_EXT
#  define GL_ABGR_EXT 0x8000
#endif
#ifndef GL_BGR
#  define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#  define GL_BGRA 0x80E1
#endif


#ifndef GL_UNSIGNED_SHORT_5_6_5
#  define GL_UNSIGNED_SHORT_5_6_5      0x8363
#endif

// awf: more (similar) defines for win32...
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#  define GL_UNSIGNED_SHORT_5_5_5_1      0x8034
#endif
#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#  define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8066
#endif

#include "sagePixelType.h"

class appUI;
class appWidget;
class envInterface;
class sageStreamer;
class streamProtocol;
class sageSyncServer;
class sageSyncClient;
class sageDoubleBuf;
class syncGroup;
#ifdef SAGE_AUDIO
class sageAudioCircBuf;
class sageAudioModule;
class sageAppAudio;
#endif

/**
 * \brief SAGE applicatoins communicate with the FSManager and stream pixels to SAGE receivers through SAIL
 */
class sail {
private:
  envInterface    *envIntf;
  sageStreamer    *pixelStreamer;
  sageStreamer    *audioStreamer;
  sailConfig       config;
  sagePixelType    pInfo;

  bool             sailOn;
  bool audioOn;            // audio on/off flag

  int bufSize;
  int winID;

  std::deque<sageMessage> appMsgQueue;
  std::queue<std::string> appWidgetMsgQueue;

  int bufID;
  sageDoubleBuf *doubleBuf;

#ifdef SAGE_AUDIO
  sageAudioCircBuf* audioBuffer;
  sageAudioModule* audioModule;
  sageAppAudio* audioAppDataHander;
#endif

  int waitNodes;
  int connectedNode;
  bool connectionDone;

  int reconfigStep;

  //double reportTime;
  sageTimer perfTimer;
  double frameRate;
  double bandWidth;
  double totalBand;
  int perfWait;
  int reportRate;
  double frameInterval;

  pthread_mutex_t   *msgMutex;
  pthread_cond_t    *notEmpty;
  sageSyncServer *syncServerObj; /**< sageSyncServer object for master node */
  syncGroup *sGroup;

  /**
   * when an app is minimized through ratko's UI
   * UI sends desired framerate
   * -1 to resume
   * @param args
   */
  int _minimizedFrameRate;
  int _skippingCount;

  /**
   * mutex for _minimizedFrameRate
   * @param args
   */
  pthread_mutex_t *_mfrMutex;

  pthread_t msgThreadID;

  /**
   * This thread is created at the end of sail::init()
   * It keeps calling sail::readMessage() which receives message through environInterface
   */
  static void* msgThread(void *args);

  //static void* nwThread(void *args);
  int checkSyncServer();
  int generateSageBlocks();

public:
  sail();
  ~sail();

  /**
   * accepts sailConfig object and sets necessary parameters
   * ready audioBuffer if SAGE_AUDIO
   * creates pthreads mutex and condition variable.
   * if it runs on the master node then creates sageSyncServer object.
   * followed by sync related code
   * followed by creation of sageBlockStreamer object
   * followed by SAGE_AUDIO part
   * finally pthread_create 'msgThread'
   *
   * @return 0 on success, -1 otherwise
   */
  int init(sailConfig &conf);
  int getWinID() {return winID;}

  /**
   * This function is called in msgThread()
   * getting message through envInf
   */
  int readMessage();
  int sendMessage(sageMessage &msg);
  int sendMessage(int code);
  int sendMessage(int code, int data);
  int sendMessage(int code, char *data);
  int parseMessage(sageMessage &msg);
  int swapBuffer(int mode = SAGE_BLOCKING);
#ifdef SAGE_AUDIO
  /**
   * calls audioAppDataHander->swapBuffer(size, buf);
   */
  int pushAudioData(int size, void *buf);
#endif

  void* getBuffer();
  int initPixelBlocks();
  int streamPixelBlocks(int blockFrame);
  int streamFBO();

  //int resizeBuffer(int x, int y, int depth);
  int getRowOrder() { return config.rowOrd; }
  int getBufSize() { return bufSize; }
  sailConfig Config() { return config; }
  int checkMsg(sageMessage &msg, bool blocking);
  int requestNewFrame();
  int shutdown();
  int sendPerformanceInfo();
  void getBufferResolution(int &w, int &h);

  int initFrameBufferObject();  // initailize FBO of SAIL
  int bindFrameBufferObject();  // bind the FBO of SAIL
  int releaseFrameBufferObject();  // back to regular frame buffer
  int transferFBO();      // transfer pixels written into FBO to SAGE

  // ui widgets (if any)
  appUI *ui;
  int addWidget(appWidget *w);
  void removeWidget(appWidget *w);

  inline bool isSailOn() { return sailOn; }

  friend class envInterface;
};

#endif
