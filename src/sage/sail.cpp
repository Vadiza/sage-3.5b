/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sail.cpp - the main source of SAGE Application Interface Library
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#include "sail.h"
#include "appUI.h"
#include "envInterface.h"

#if defined(WIN32)
#include <time.h>
#include <GL/gl.h>
#else
#include "unistd.h"  //for getting and writing the PID to a temp file
#endif

#include "sageSync.h"
#include "streamProtocol.h"
#include "sageDoubleBuf.h"
#include "sageStreamer.h"
#include "sageBlock.h"

#ifdef SAGE_AUDIO
#include "sageAudioCircBuf.h"
#include "sageAudioModule.h"
#include "sageAppAudio.h"
#endif

// To detect libsail with automake/autoconf
#ifdef __cplusplus
extern "C" {
  extern int findsage(char *c) { return 1; }
}
#endif


sail::sail() : winID(-1), bufID(0), sailOn(false), audioOn(false), sGroup(NULL), syncServerObj(NULL), _minimizedFrameRate(-1), _skippingCount(0)
{
  envIntf = NULL;
  pixelStreamer = NULL;
  audioStreamer = NULL;
  ui = new appUI(this);
#ifdef SAGE_AUDIO
  audioModule = NULL;
  audioBuffer = NULL;
  audioAppDataHander = NULL;
#endif

#if !defined(WIN32)
  msgThreadID = 0;
#endif
}

sail::~sail()
{
  //shutdown();
#if !defined(WIN32)
  if (msgThreadID)
    pthread_join(msgThreadID, NULL);
#endif
  delete ui;

#ifdef SAGE_AUDIO
  if (audioStreamer) {
    delete audioStreamer;
    audioStreamer = NULL;
  }
  if(audioAppDataHander) {
    delete audioAppDataHander;
    audioAppDataHander = NULL;
  }
  if(audioModule) {
    delete audioModule;
    audioModule = NULL;
  }
#endif
  if (msgMutex)
    free(msgMutex);

  if (notEmpty)
    free(notEmpty);

  if (envIntf) {
    delete envIntf;
    envIntf = NULL;
  }

  if (pixelStreamer) {
    delete pixelStreamer;
    pixelStreamer = NULL;
  }
  if (syncServerObj) {
    delete syncServerObj;
  }
}

/* Standard C Function: Greatest Common Divisor */
static int
sage_gcd ( int a, int b )
{
  int c;
  while ( a != 0 ) {
    c = a; a = b%a;  b = c;
  }
  return b;
}


int sail::init(sailConfig &conf)
{
  config = conf;

  if (config.protocol == SAGE_UDP)  {
    config.autoBlockSize = true;
  } else {

    if (config.pixFmt != PIXFMT_DXT && config.pixFmt != PIXFMT_DXT5 && config.pixFmt != PIXFMT_DXT5YCOCG && config.pixFmt != PIXFMT_YUV) {
      // Hack the block sizes
      SAGE_PRINTLOG("BLOCK> Block sizes specified: %d x %d", config.blockX, config.blockY);
      int bx = config.resX;
      int by = config.resY;
      int g = sage_gcd(bx, by);
      int nbx = bx / g;
      int nby = by / g;
      //  1 < number of blocks  < 100 : at least one block, happens when widht==height
      if ( ((nbx * nby) < 100) && (nbx*nby>1) ) {
        // if number of blocks using GCD is under 100, use that
        SAGE_PRINTLOG("BLOCK> using GCD approach: gcd[%d, %d]  = %d -- %d blocks", bx, by, g, nbx*nby);
        config.blockX = g;
        config.blockY = g;
      }
      else {
        // othewise, use sizes to create 50 blocks
        SAGE_PRINTLOG("BLOCK> using 50 blocks approach");
        config.blockX = config.resX / 10;
        config.blockY = config.resY / 5;
      }
      SAGE_PRINTLOG("BLOCK> New block sizes: %d x %d", config.blockX, config.blockY);
    }

    // Check that Pixel block size (i.e. groupSize) can fit a block
    int minGroupSize = BLOCK_HEADER_SIZE + (config.blockX * config.blockY * getPixelSize(config.pixFmt));
    if (config.groupSize < minGroupSize) {
      SAGE_PRINTLOG("Adjusting groupSize from %d to %d\n", config.groupSize, minGroupSize);
      config.groupSize = minGroupSize;
    }
  }

  // check that winWidth and winHeight has been set, if not, it to 500,500
  if (config.winWidth == -1 || config.winHeight == -1)
  {
    config.winWidth = 500;
    config.winHeight = 500;
  }


#ifdef WIN32
  static int WinSockInitialized = 0;
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;

  wVersionRequested = MAKEWORD(1, 1); // It requires Winsock version 2.

  err = WSAStartup(wVersionRequested, &wsaData);
  if (err != 0) {
    SAGE_PRINTLOG("Cannot load the valid winsock 2 dll. Program will be terminated.\n");
    exit(1);
  }
  else {
    SAGE_PRINTLOG("Winsock 2 dll loaded\n");
  }

  WinSockInitialized = 1; // Winsock dll is loaded.
#else
  // write the PID to a temp file named: $SAGE_DIRECTORY/bin/appLauncher/pid/appName_appID.pid
  /*char fileName[100];
    memset(fileName,' ', 100);
    char *sageDir = getenv("SAGE_DIRECTORY");
    sprintf(fileName, "%s/bin/appLauncher/pid/%s_%d.pid", sageDir, config.appName, config.appID);
    FILE *fp1 = fopen(fileName, "w");
    if (!fp1) {
    SAGE_PRINTLOG("failed to create a pid file %s\n", fileName);
    }
    else {
    // change the permission of the file so that anybody can write/read it
    chmod(fileName, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
    fprintf(fp1, "%d", getpid());
    fclose(fp1);
    }*/
  // done writing
#endif

  sailOn = true;
  pInfo.initType(config.pixFmt);

  if (config.rendering) {

    // rounding error may occur...
    if (config.totalWidth > 0 && config.totalHeight > 0) {
      config.resX = (int)floor(config.totalWidth *
                               (config.imageMap.right - config.imageMap.left) + 0.5);
      config.resY = (int)floor(config.totalHeight *
                               (config.imageMap.top - config.imageMap.bottom) + 0.5);
    }
    else {
      config.totalWidth = (int)floor((config.resX/
                                      (config.imageMap.right - config.imageMap.left) + 0.5));
      config.totalHeight = (int)floor((config.resY/
                                       (config.imageMap.top - config.imageMap.bottom) + 0.5));
    }

    config.imageMap.width = config.resX;
    config.imageMap.height = config.resY;

    // rounding error may occur...
    config.imageMap.locate();

    // Calculate the pixel buffer size
    bufSize = config.resX * config.resY * getPixelSize(config.pixFmt);
  }

  connectedNode = 0;
  connectionDone = false;

  frameRate = 0;
  bandWidth = 0;
  totalBand = 0.0;
  perfWait = config.nodeNum;
  reportRate = 0;
  frameInterval = 0.0;

#ifdef SAGE_AUDIO
  int audioBuffSize = 0;
  if(config.audioOn) {
    switch(config.sampleFmt) {
    case SAGE_SAMPLE_FLOAT32 :
      config.bytesPerSample = sizeof(float);
      break;
    case SAGE_SAMPLE_INT16 :
      config.bytesPerSample = sizeof(short);
      break;
    case SAGE_SAMPLE_INT8 :
      config.bytesPerSample = sizeof(int);
      break;
    case SAGE_SAMPLE_UINT8 :
      config.bytesPerSample = sizeof(unsigned int);
      break;
    default :
      break;
    }
    audioBuffSize = config.framePerBuffer * config.channels * config.bytesPerSample;
  }
#endif

  //SAGE_PRINTLOG("SAIL buffer allocated");

  int syncPort = config.syncPort + config.appID;

  msgMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(msgMutex, NULL);
  pthread_mutex_unlock(msgMutex);

  /** mutex for minimized window **/
  _mfrMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(_mfrMutex, NULL);
  pthread_mutex_unlock(_mfrMutex);

  notEmpty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notEmpty, NULL);

  if (config.master && config.nodeNum > 1) {
    syncServerObj = new sageSyncServer;
    if (syncServerObj->init(syncPort) < 0) {
      SAGE_PRINTLOG("SAIL : Error init'ing the sync server object\n");
      return -1;
    }
    SAGE_PRINTLOG("SAIL sync server created\n");
  }
  else
    syncServerObj = NULL;

  envIntf = new envInterface;
  if (config.bridgeOn)
    envIntf->init(config, config.bridgeIP, config.bridgePort);
  else if (config.fsIP[0] != '\0' && config.fsPort > 0)
    envIntf->init(config, config.fsIP, config.fsPort);
  else
    envIntf->init(config);

  if (config.master) {
    char selfIP[SAGE_IP_LEN];
    envIntf->getSelfIP(selfIP);

    waitNodes = config.nodeNum;
    if (config.rendering) {
      waitNodes--;

      if (config.nodeNum > 1) {
        config.syncClientObj = new sageSyncClient;

        if (config.syncClientObj->connectToServer((char *)"127.0.0.1", syncPort) < 0) {
          SAGE_PRINTLOG("Sail::init() : fail to connect to sync master\n");
          return -1;
        }
        else
          SAGE_PRINTLOG("Connected to SAIL sync master\n");
      }
      else
        config.syncClientObj = NULL;

      if (waitNodes < 1) {
        char msgStr[TOKEN_LEN];

        if (config.bridgeOn) {
          int rowNum = (int)ceil((float)config.totalHeight/config.blockY);
          int colNum = (int)ceil((float)config.totalWidth/config.blockX);
          int blockNum = rowNum*colNum;  // total block number of this app
          sprintf(msgStr, "%s %d %d %d %d %d %s %d %d %d %d %d",
                  config.appName, config.winX, config.winY, config.winWidth, config.winHeight,
                  config.protocol, config.fsIP, config.fsPort, config.totalWidth, config.totalHeight,
                  blockNum, (int)config.audioOn);

          SAGE_PRINTLOG("send SAIL register message to sage bridge\n");
          sendMessage(BRIDGE_APP_REG, msgStr);
        }
        else {
          double resX = config.resX/(config.imageMap.right-config.imageMap.left);
          double resY = config.resY/(config.imageMap.top-config.imageMap.bottom);
          double totalBandWidth = config.frameRate*resX*resY*pInfo.bytesPerPixel*8;
          int bandWidthReq = (int)floor(totalBandWidth/1000000.0+0.5);

          sprintf(msgStr, "%s %d %d %d %d %d %s %d %d %d %d %d %d %s %s",
                  config.appName, config.winX, config.winY, config.winWidth, config.winHeight,
                  bandWidthReq, config.streamIP, config.totalWidth, config.totalHeight,
                  (int)config.audioOn, config.protocol, config.frameRate, config.appID, config.launcherID

                  , config.mediaFileName
                  );

          sendMessage(REG_APP, msgStr);
        }

      }
    } // end if (config.rendering)
  } // end if (config.master)
  else {
    if (config.nodeNum > 1) {
      config.syncClientObj = new sageSyncClient;

      if (config.syncClientObj->connectToServer(config.masterIP, syncPort) < 0) {
        SAGE_PRINTLOG("Sail::init() : fail to connect to sync master\n");
        return -1;
      }
      SAGE_PRINTLOG("Connected to SAIL sync master\n");
    }
    else
      config.syncClientObj = NULL;

    char msgStr[TOKEN_LEN];
    sprintf(msgStr, "%d %d", config.totalWidth, config.totalHeight);
    sendMessage(SAIL_SLAVE_REG, msgStr);
  }

  appMsgQueue.clear();

  if (config.rendering) {
    pixelStreamer = new sageBlockStreamer((streamerConfig &)config, pInfo.bytesPerPixel);

    doubleBuf = pixelStreamer->getDoubleBuffer();
  }
  else {
    if (!config.master) {
      SAGE_PRINTLOG("sail::init() : incorrect configuration in rendering/master option\n");
      return -1;
    }

    config.imageMap.left = config.imageMap.right = config.imageMap.top = config.imageMap.bottom = 0;
    pixelStreamer = NULL;
  }

#ifdef SAGE_AUDIO
  if(audioStreamer != NULL) {
    delete audioStreamer;
    audioStreamer = NULL;
  }
  if (config.audioOn) {

    // create audio module
    sageAudioModule::instance();
    audioModule = sageAudioModule::_instance;
    audioModule->init(config);
    sageAudioCircBuf* buffer = NULL;
    if(config.audioMode == SAGE_AUDIO_CAPTURE || config.audioMode == SAGE_AUDIO_FWCAPTURE || config.audioMode == SAGE_AUDIO_PLAY) {
      buffer = audioModule->createObject();
    }
    else if(config.audioMode == SAGE_AUDIO_APP) {
      buffer = audioModule->createBuffer();
      audioAppDataHander = new sageAppAudio(buffer, config.audioBuffSize);
      //audioObj->save("c.wav");   // testing...
    }
    else if(config.audioMode == SAGE_AUDIO_READ) {
      buffer = audioModule->load(config.audioFileName, true, config.framePerBuffer, config.totalFrames);   // for the test
      sageAudioConfig* audioConfig = audioModule->getAudioConfig();
      config.sampleFmt = audioConfig->sampleFmt;
      config.samplingRate = audioConfig->samplingRate;
      config.channels = audioConfig->channels;
      config.framePerBuffer = audioConfig->framePerBuffer;

      //buffer = audioObj->load(config.audioFileName, true);   // for the test
      if(buffer != NULL) {
        audioModule->play(0);
      }
    }

    if(buffer != NULL) {
      SAGE_PRINTLOG("audio module is created and connected to audio streamer");
      audioBuffer = buffer;

      // if audio == true,
      // create audio streamer
      audioStreamer = new sageAudioStreamer(config, audioBuffSize, buffer);
      SAGE_PRINTLOG("audio streamer initialized");
    }
  }
#endif

  //pthread_t thId;

  if (pthread_create(&msgThreadID, 0, msgThread, (void*)this) != 0) {
    SAGE_PRINTLOG("SAIL : can't create msgThread\n");
    return -1;
  }

  return 0;
}

void sail::getBufferResolution(int &w, int &h)
{
  w = config.resX;
  h = config.resY;
}

int sail::addWidget(appWidget *w)
{
  ui->addWidget(w);
  return 0;
}

void sail::removeWidget(appWidget *w)
{
  ui->removeWidget(w);
}



int sail::swapBuffer(int mode)
{
  if (!config.rendering) {
    SAGE_PRINTLOG("sail::swapBuffer() : this node is not configured to stream pixels\n");
    return -1;
  }

  if (mode == SAGE_NON_BLOCKING && !doubleBuf->isEmpty()) {
    return 1;
  }

  /**
   * if the streaming thread is finished. sail::swapBuffer() will block permanently.
   * (because of semaphore operation on the double buffer b/w sail::swapBuffer() and SBS::streamLoop() )
   * If it blocks permanently, app could never get sageMessage which causes the killing app instance issue.
   * So, if the streaming thread is finished, then swapBuffer() should return with error.

   if ( ! pixelStreamer->isStreamerOn() ) {
   SAGE_PRINTLOG("sail::%s() : streamerOn is false\n", __FUNCTION__);
   sageMessage msg;
   msg.init(0, APP_QUIT, 0, 0, NULL);
   pthread_mutex_lock(msgMutex);
   appMsgQueue.push_front(msg);
   pthread_mutex_unlock(msgMutex);
   return -1;
   }
  */

  //if ( ! config.asyncUpdate ) {
  pthread_mutex_lock(_mfrMutex);
  if ( _minimizedFrameRate > 0 ) {
    // if -1 then it's normal
    // for now we're gonna skip 20 frames.
    // later this should be more intelligent by measuring app's framerate
    _skippingCount++;
  }
  else if ( _minimizedFrameRate == -1 ) {
    //SAGE_PRINTLOG("sail::swapBuffer() : App is now back to normal state\n");
    _skippingCount = 0;
  }
  pthread_mutex_unlock(_mfrMutex);

  if ( _skippingCount != 0 ) {
    if ( _skippingCount < (config.frameRate - _minimizedFrameRate) ) {
      // for now this is constant
      //SAGE_PRINTLOG("sail::swapBuffer() : just return for now. skippingCount %d\n", _skippingCount);
      //usleep(50 * 1000); // 50 msec for now
      return 0;
      // no pixel streaming will occur
      // because doubleBuffer queueLength can't be increased if doubleBuffer->swapBuffer() isn't called.
      // doubleBuffer->getBackBuffer() wait on condition (queuelen > 0)
      // which will make sageBlockStreamer block
      // However an app will keep proceeding because doubleBuffer->getFrontBuffer (which is called by sail::getBuffer() won't wait for any condition
    }
    else {
      _skippingCount = 0; // reset
      //  SAGE_PRINTLOG("sail::%s() : config.frameRate %d, _minimizedFrameRate %d\n", __FUNCTION__, config.frameRate, _minimizedFrameRate);
    }
  }
  //}

  doubleBuf->swapBuffer();

#ifdef SAGE_AUDIO
  if(audioModule)
  {
    audioModule->setgFrameNum(pixelStreamer->getFrameID());
  }
#endif

  return 0;
}

void* sail::getBuffer()
{
  //SAGE_PRINTLOG("buffer address %x\n" , doubleBuf->getFrontBuffer()->getPixelBuffer());

  return (void *)(doubleBuf->getFrontBuffer()->getPixelBuffer());
}

int sail::checkSyncServer()
{
  /*
    if (connectionDone) {
    if (syncServerObj->checkUpdates() == 0) {

    if (!pauseSync) {
    if (syncMsgQueue.size() > 0) {
    //SAGE_PRINTLOG("==== distribute stream info" << std::endl;
    char *syncMsg = syncMsgQueue.front();
    syncMsgQueue.pop_front();
    masterFrameNum = syncServerObj->distributeSync(syncMsg);
    delete [] syncMsg;
    }
    else
    masterFrameNum = syncServerObj->distributeSync(NULL);
    //SAGE_PRINTLOG("master frame %d", masterFrameNum);
    }

    }
    }
  */
  return 0;
}

int sail::sendPerformanceInfo()
{
  double elapsedTime = perfTimer.getTimeUS();

  if (elapsedTime > 1000000.0 * reportRate && reportRate > 0) {
    if (config.rendering) {
      bandWidth = pixelStreamer->getBandWidth()*8.0f / elapsedTime;
      pixelStreamer->resetBandWidth();
    }

    if (config.master) {
      if (config.rendering) {
        frameRate = (float) (pixelStreamer->getFrameCount() * 1000000.0 / elapsedTime);
        pixelStreamer->resetFrameCounter();

        perfWait--;
        totalBand += bandWidth;

        if (perfWait < 1) {
          char msgStr[TOKEN_LEN];
          sprintf(msgStr, "%7.2f %7.2f %7.2f", totalBand, frameRate, 0.0);
          sendMessage(DISP_SAIL_PERF_RPT, msgStr);
          totalBand = 0.0;
          perfWait = config.nodeNum;
        }
      }
    }
    else {
      char bandStr[TOKEN_LEN];
      sprintf(bandStr, "%7.2f", bandWidth);
      sendMessage(SAIL_PERF_INFO, bandStr);
    }

    perfTimer.reset();
  }

  return 0;
}

void* sail::msgThread(void *args)
{
  sail *This = (sail *)args;

  while(This->sailOn) {
    if (This->readMessage() < 0) {
      This->shutdown();
      break;
    }


    This->sendPerformanceInfo();

    if (This->config.master && This->config.nodeNum > 1) {
      This->envIntf->checkClients();

      sageMessage msg;
      for (int i = 0; i < This->envIntf->getNumClients(); i++) {
        if (This->envIntf->readClientMsg(msg, i) > 0) {
          if (This->parseMessage(msg) < 0) {
            This->shutdown();
            msg.destroy();
            break;
          }
          msg.destroy();
        }
      }
    }
    sage::usleep(1000);
  }

  SAGE_PRINTLOG("sail::msgThread : exit the message thread\n");
  pthread_exit(NULL);
  return NULL;
}

int sail::readMessage()
{
  sageMessage msg;

  int status = envIntf->rcvMessage(msg);

  if (status > 0) {
    if (msg.getCode() < SAIL_MESSAGE   + 100) {
      if (parseMessage(msg) < 0)
        return -1;
      msg.destroy();
    }
    else if (msg.getCode() < SAIL_MESSAGE + 200) {
      if (config.master) {
        envIntf->distributeMessage(msg);
      }
      if (parseMessage(msg) < 0)
        return -1;
      msg.destroy();
    }
    else if (msg.getCode() >= APP_MESSAGE) {
      // store app widget events separately
      pthread_mutex_lock(msgMutex);
      if (msg.getCode() == EVT_APP_WIDGET)
        appWidgetMsgQueue.push(string((char *)msg.getData()));
      else if ( msg.getCode() == APP_QUIT) {
        shutdown();
        appMsgQueue.push_back(msg);
      }
      else {
        appMsgQueue.push_back(msg);
      }

      pthread_mutex_unlock(msgMutex);
      msg.destroy();
    }
  }

  return status;
}

int sail::parseMessage(sageMessage &msg)
{
  char *msgData = (char *)msg.getData();

  switch (msg.getCode()) {
  case SAIL_INIT_MSG : {
    if (msgData) {
      sageNwConfig nwCfg;
      sscanf(msgData, "%d %d %d %d", &winID, &nwCfg.rcvBufSize, &nwCfg.sendBufSize,
             &nwCfg.mtuSize);

      if (config.rendering) {
        pixelStreamer->setWinID(winID);
        pixelStreamer->setNwConfig(nwCfg);
      }

      if (config.audioOn) {
        if(audioStreamer)
        {
          audioStreamer->setWinID(winID);
          audioStreamer->setNwConfig(nwCfg);
        }
      }

      // send widget information to fsManager to distribute to
      // the display nodes and DIM
      ui->sendInitialWidgetInfo();
    }
    else {
      SAGE_PRINTLOG("sail::parseMessage : SAGE_INIT_MSG is NULL\n");
      return -1;
    }

    break;
  }

  case SAIL_SLAVE_REG : {
    waitNodes--;

    if (msgData && (config.totalWidth == 0 || config.totalHeight == 0))
      sscanf(msgData, "%d %d", &config.totalWidth, &config.totalHeight);

    if (waitNodes < 1) {
      if (config.nodeNum > 1) {
        sGroup = new syncGroup;
        //SAGE_PRINTLOG("frame rate = %d", config.frameRate);
        sGroup->init(0, SAGE_ASAP_SYNC_HARD, 0, config.frameRate, config.nodeNum);
        sGroup->blockSync();
        syncServerObj->addSyncGroup(sGroup);
      }

      SAGE_PRINTLOG("send SAIL register message to fsManager\n");
      char msgStr[TOKEN_LEN];

      if (config.bridgeOn) {
        int rowNum = (int)ceil((float)config.totalHeight/config.blockY);
        int colNum = (int)ceil((float)config.totalWidth/config.blockX);
        int blockNum = rowNum*colNum; // total block number of this app
        sprintf(msgStr, "%s %d %d %d %d %d %s %d %d %d %d %d",
                config.appName, config.winX, config.winY, config.winWidth, config.winHeight,
                config.protocol, config.fsIP, config.fsPort, config.totalWidth,
                config.totalHeight, blockNum, (int)config.audioOn);

        sendMessage(BRIDGE_APP_REG, msgStr);
      }
      else {
        // calculate total app image resolution
        // for NRM
        double resX = config.resX/(config.imageMap.right-config.imageMap.left);
        double resY = config.resY/(config.imageMap.top-config.imageMap.bottom);
        double totalBandWidth = config.frameRate*resX*resY*pInfo.bytesPerPixel*8;
        int bandWidthReq = (int)floor(totalBandWidth/1000000.0+0.5);
        sprintf(msgStr, "%s %d %d %d %d %d %s %d %d %d %d %d %d %s",
                config.appName, config.winX, config.winY, config.winWidth, config.winHeight,
                bandWidthReq, config.streamIP, config.totalWidth, config.totalHeight,
                (int)config.audioOn, config.protocol, config.frameRate, config.appID, config.launcherID);

        sendMessage(REG_APP, msgStr);
      }
    }
    break;
  }

  case SAIL_CONNECT_TO_RCV : {
    if (config.rendering) {
      if (!pixelStreamer)
        SAGE_PRINTLOG("sail::parseMessage : No pixelStreamer object\n");

      SAGE_PRINTLOG("sail::%s() : received SAIL_CONNECT_TO_RCV : establishing streaming channel\n", __FUNCTION__);
      if (pixelStreamer->initNetworks(msgData) < 0)
        return -1;

      if (config.master)
        connectedNode++;
    }

    if (config.master) {
      if (connectedNode == config.nodeNum) {
        connectionDone = true;
        SAGE_PRINTLOG("SAIL: Network connection Done\n");
      }
    }
    else {
      sendMessage(SAIL_CONNECTED_TO_RCV);
    }

    break;
  }

  case SAIL_CONNECT_TO_RCV_PORT : {
    if (config.rendering) {
      if (!pixelStreamer)
        SAGE_PRINTLOG("sail::parseMessage : No pixelStreamer object\n");

      SAGE_PRINTLOG("SAIL is initializing network connections for streaming\n");
      if (pixelStreamer->initNetworks(msgData, true) < 0)
        return -1;

      if (config.master)
        connectedNode++;
    }

    if (config.master) {
      if (connectedNode == config.nodeNum) {
        connectionDone = true;
        SAGE_PRINTLOG("SAIL: Network connection Done\n");
      }
    }
    else {
      sendMessage(SAIL_CONNECTED_TO_RCV);
    }

    break;
  }

  case SAIL_CONNECT_TO_ARCV : {
#ifdef SAGE_AUDIO
    if (config.audioOn) {
      if (!audioStreamer)
        SAGE_PRINTLOG("sail::parseMessage : No audioStreamer object\n");
      else {
        SAGE_PRINTLOG("SAIL is initializing network connections for audio streaming\n");

        audioStreamer->initNetworks(msgData);

        audioStreamer->enqueMsg(msgData);
        if(config.audioMode == SAGE_AUDIO_CAPTURE || config.audioMode == SAGE_AUDIO_PLAY) {
          audioModule->play(0);
        } else if(config.audioMode ==  SAGE_AUDIO_APP && audioAppDataHander){
          audioAppDataHander->start();
        }
      }
    }
#endif
    break;
  }
  case SAIL_CONNECTED_TO_RCV : {
    connectedNode++;
    if (connectedNode == config.nodeNum) {
      connectionDone = true;
      SAGE_PRINTLOG("SAIL: Network connection Done\n");
    }
    break;
  }

  case SAIL_FLIP_WINDOW : {
    if (config.rendering) {
      if (config.rowOrd == BOTTOM_TO_TOP)
        config.rowOrd = TOP_TO_BOTTOM;
      else
        config.rowOrd = BOTTOM_TO_TOP;

      pixelStreamer->regeneratePixelBlocks();

      if (doubleBuf->isFirstFrameReady() && config.asyncUpdate)
        doubleBuf->resendBuffer(1);
    }

    break;
  }

  case SAIL_INIT_STREAM : {
    //SAGE_PRINTLOG("SAIL::readMessage() : SAIL_INIT_STREAM, initialize SAGE streams");

    if (config.nodeNum > 1) {
      sGroup->enqueSyncMsg(msgData);
      sGroup->unblockSync();
      if (config.asyncUpdate)
        envIntf->distributeMessage(SAIL_RESEND_FRAME);
    }
    else if (config.rendering) {
      pixelStreamer->enqueMsg(msgData);
      if (doubleBuf->isFirstFrameReady() && config.asyncUpdate) {
        //doubleBuf->resendBuffer(2);

        // with change in PDL, staticApp doesn't need to send 2 frames
        doubleBuf->resendBuffer(1);
      }
    }

    break;
  }

  case SAIL_RESEND_FRAME : {
    //SAGE_PRINTLOG("SAIL::readMessage() : SAIL_RESEND_FRAME");

    if (config.rendering && doubleBuf->isFirstFrameReady() && config.asyncUpdate)
      doubleBuf->resendBuffer(1);
    break;
  }

  case SAIL_SHUTDOWN : {
    SAGE_PRINTLOG("sail::%s() : SAIL_SHUTDOWN\n", __FUNCTION__);
    shutdown();
    break;
  }

  case SAIL_FRAME_RATE : {
    /*
      float frate = atof(msgData);
      if (config.master && config.nodeNum > 1)
      sGroup->setFrameRate(frate);
      else if (config.rendering)
      pixelStreamer->setFrameRate(frate);
      break;
    */
    // -1 : resume whatever it was
    // otherwise : desired frame rate
    // this is not thread-safe. Must be used with mutex

    pthread_mutex_lock(_mfrMutex);
    sscanf(msgData, "%d", &_minimizedFrameRate);
    pthread_mutex_unlock(_mfrMutex);
    break;
  }

  case SAIL_PERF_INFO : {
    double bandData = atof(msgData);
    totalBand += bandData;
    perfWait--;

    if (perfWait < 1) {
      char msgStr[TOKEN_LEN];
      sprintf(msgStr, "%7.2f %5.2f %d", totalBand, frameRate, config.nodeNum);
      sendMessage(DISP_SAIL_PERF_RPT, msgStr);
      perfWait = config.nodeNum;
      totalBand = 0.0;
    }
    break;
  }

  case SAIL_PERF_INFO_REQ : {
    perfTimer.reset();
    reportRate = atoi(msgData);
    break;
  }

  case SAIL_PERF_INFO_STOP : {
    reportRate = 0;
    break;
  }

  case SAIL_SEND_TIME_BLOCK : {
    if (config.rendering) {
      //gStreamer->timeBlockFlag = true;
    }
    else {
      if (config.master)
        envIntf->msgToClient(0, SAIL_SEND_TIME_BLOCK);
      else
        SAGE_PRINTLOG("sail::parseMessage : invalid sail slave configuration\n");
    }
    break;
  }
  default :
    SAGE_PRINTLOG("sail::parseMessage() : sail received an invalid message \n");
  }

  return 0;
}

int sail::sendMessage(sageMessage &msg)
{
  return envIntf->sendMessage(msg);
}

int sail::sendMessage(int code)
{
  return envIntf->sendMessage(code);
}

int sail::sendMessage(int code, int data)
{
  return envIntf->sendMessage(code, data);
}

int sail::sendMessage(int code, char *data)
{
  return envIntf->sendMessage(code, data);
}


int sail::checkMsg(sageMessage &msg, bool blocking)
{
  int retVal = 0;
  pthread_mutex_lock(msgMutex);
  while (blocking && appMsgQueue.size() == 0 && appWidgetMsgQueue.size() == 0)
    pthread_cond_wait(notEmpty, msgMutex);

  // process all widget events first
  while(!appWidgetMsgQueue.empty()) {
    string m = appWidgetMsgQueue.front();
    ui->processWidgetEvent(m.c_str());
    appWidgetMsgQueue.pop();
  }

  // now process one regular app message
  if (appMsgQueue.size() > 0) {
    msg = appMsgQueue.front();
    appMsgQueue.pop_front();
    retVal = 1;
  }

  pthread_mutex_unlock(msgMutex);

  return retVal;
}

int sail::shutdown()
{
  if (!sailOn)
    return 0;

  if (config.master) {
    if(envIntf)
    {
      envIntf->distributeMessage(SAIL_SHUTDOWN);
    }
    // inform fsManager that this app is shutting down
    //SAGE_PRINTLOG("sail::shutdown() : Quit app\n");
    SAGE_PRINTLOG("sail::%s() : sendMsg NOTIFY_APP_SHUTDOWN %d\n", __FUNCTION__, winID);
    sendMessage(NOTIFY_APP_SHUTDOWN, winID);
    sage::sleep(1);
  }

  sailOn = false;

  if (pixelStreamer)
    pixelStreamer->shutdown();

  return 0;
}


/////////////////////////////////////////

/*
  int sail::generateSageBlocks()
  {
  for (int i=0; i<2; i++) {
  (sageRect &)sailBlock[i] = config.imageMap;
  sailBlock[i].buffer = sailBuffer[i];
  sailBlock[i].subWidth = config.blockX;
  sailBlock[i].subHeight = config.blockY;
  sailBlock[i].bytesPerPixel = bytesPerPixel;

  if (config.bridgeOn && config.streamMode == 1) {
  sailBlock[i].flag = SAGE_BUFFER_BLOCK;
  sailBlock[i].rowOrd = config.rowOrd;
  }
  else {
  sailBlock[i].flag = SAGE_PIXEL_BLOCK;
  sailBlock[i].generatePixelBlocks(config.rowOrd);
  }
  }

  return 0;
  }
*/


#ifdef SAGE_AUDIO
int sail::pushAudioData(int size, void *buf)
{
  if(audioAppDataHander != NULL)
    audioAppDataHander->swapBuffer(size, buf);
  else
    return -1;

  return 0;
}
#endif
