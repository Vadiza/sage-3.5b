/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageConfig.cpp
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

#include "sageConfig.h"
#include "sageBlock.h"
#include "sageSync.h"

streamerConfig::streamerConfig() : rank(0), resX(0), resY(0), rowOrd(TOP_TO_BOTTOM),
                                   master(true), protocol(SAGE_TCP), asyncUpdate(true), blockX(64), blockY(64), blockSize(0),
                                   compression(NO_COMP), pixFmt(PIXFMT_888), streamType(SAGE_BLOCK_HARD_SYNC),
                                   syncClientObj(NULL), frameRate(30), totalWidth(0), totalHeight(0), groupSize(32767),
  audioOn(false), audioPort(0), audioDeviceNum(0), audioKeyFrame(100), audioProtocol(SAGE_TCP),
  sampleFmt(SAGE_SAMPLE_FLOAT32), samplingRate(44100), channels(2), framePerBuffer(1024),
  syncType(SAGE_SYNC_NONE), totalFrames(0), syncPolicy(SAGE_ASAP_SYNC_HARD),
  autoBlockSize(false), maxBandwidth(1000), maxCheckInterval(1000), flowWindow(5),
  bridgeOn(false), frameDrop(true)
{
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

  swexp = false;
  fromBridgeParallel = 0;
  bzero(mediaFileName, SAGE_NAME_LEN);
}

sailConfig::sailConfig() : nwID(1), fsPort(0), syncPort(0), msgPort(0), appID(0),
                           rendering(true), winX(100), winY(100), winWidth(-1), winHeight(-1),
                           rcvBufSize(0), syncMode(0), bridgePort(0)
{
  memset(appName, 0, SAGE_NAME_LEN);
  memset(masterIP, 0, SAGE_IP_LEN);
  memset(fsIP, 0, SAGE_IP_LEN);
  memset(launcherID, 0, SAGE_NAME_LEN);
  strcpy(launcherID, "none");
  appName[0] = '\0';
  masterIP[0] = '\0';
  bridgeIP[0] = '\0';
  fsIP[0] = '\0';
  sprintf(streamIP, "127.0.0.1");
}

int sailConfig::setAppName(char *name)
{
  int len = strlen(name);
  if (len > SAGE_NAME_LEN) {
    std::cerr << "sailConfig::setAppName : the length of application name exceed SAGE_NAME_LEN" <<
      std::endl;
    return -1;
  }

  strcpy(appName, name);

  return 0;
}

int sailConfig::init(char *fname)
{
  sage::initUtil();

  char *sageDir = getenv("SAGE_DIRECTORY");
  if (!sageDir) {
    SAGE_PRINTLOG("sailConfig: cannot find the environment variable SAGE_DIRECTORY");
    SAGE_PRINTLOG("sailConfig:      using current and home directories as default");
    //return -1;
  }

  // Fist looking in "/application/conf"
  data_path path ("/applications/conf");
  std::string found = path.get_file(fname);
  if (found.empty()) {
    // Then looking in "/application/"
    data_path path2 ("/applications");
    found = path2.get_file(fname);
    if (found.empty()) {
      SAGE_PRINTLOG("sailConfig: cannot find the file [%s]", fname);
      return -1;
    }
  }
  const char *configName = found.c_str();
  SAGE_PRINTLOG("sailConfig: using [%s] configuration file", configName);

  FILE *fp = fopen(configName, "r");
  if (!fp) {
    SAGE_PRINTLOG("sailConfig: fail to open sail config file [%s]\n", configName);
    return -1;
  }

  char token[TOKEN_LEN];
  while(getToken(fp, token) != EOF) {
    sage::toupper(token);
    if (strcmp(token, "APPNAME") == 0) {
      getToken(fp, appName);
    }
    else if (strcmp(token, "FSIP") == 0) {
      getToken(fp, fsIP);
    }
    else if (strcmp(token, "FSPORT") == 0) {
      getToken(fp, token);
      fsPort = atoi(token);
    }
    else if (strcmp(token, "BRIDGEON") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      bridgeOn = (strcmp(token, "true") == 0);
    }
    else if (strcmp(token, "BRIDGEIP") == 0) {
      getToken(fp, bridgeIP);
    }
    else if (strcmp(token, "BRIDGEPORT") == 0) {
      getToken(fp, token);
      bridgePort = atoi(token);
    }
    else if (strcmp(token, "MASTERIP") == 0) {
      getToken(fp, masterIP);
    }
    else if (strcmp(token, "NWID") == 0) {
      getToken(fp, token);
      nwID = atoi(token);
    }
    else if (strcmp(token, "MSGPORT") == 0) {
      getToken(fp, token);
      msgPort = atoi(token);
    }
    else if (strcmp(token, "SYNCPORT") == 0) {
      getToken(fp, token);
      syncPort = atoi(token);
    }
    else if (strcmp(token, "NODENUM") == 0) {
      getToken(fp, token);
      nodeNum = atoi(token);
    }
    else if (strcmp(token, "APPID") == 0) {
      getToken(fp, token);
      appID = atoi(token);
    }
    else if (strcmp(token, "LAUNCHERID") == 0) {
      getToken(fp, launcherID);
    }
    else if (strcmp(token, "PIXELBLOCKSIZE") == 0) {
      getToken(fp, token);
      blockX = atoi(token);
      getToken(fp, token);
      blockY = atoi(token);
    }
    else if (strcmp(token, "BLOCKTHRESHOLD") == 0) {
      getToken(fp, token);
      blockSize = atoi(token);
    }
    else if (strcmp(token, "WINX") == 0) {
      getToken(fp, token);
      winX = atoi(token);
    }
    else if (strcmp(token, "WINY") == 0) {
      getToken(fp, token);
      winY = atoi(token);
    }
    else if (strcmp(token, "WINWIDTH") == 0) {
      getToken(fp, token);
      winWidth = atoi(token);
    }
    else if (strcmp(token, "WINHEIGHT") == 0) {
      getToken(fp, token);
      winHeight = atoi(token);
    }
    else if (strcmp(token, "NWPROTOCOL") == 0) {
      getToken(fp, token);
      sage::toupper(token);
      if (strcmp(token, "TCP") == 0)
        protocol = SAGE_TCP;
      else if (strcmp(token, "UDP") == 0)
        protocol = SAGE_UDP;
    }
    else if (strcmp(token, "ASYNCUPDATE") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      asyncUpdate = (strcmp(token, "true") == 0);
    }
    else if (strcmp(token, "SYNCMODE") == 0) {
      getToken(fp, token);
      syncMode = atoi(token);
    }
    else if (strcmp(token, "STREAMIP") == 0) {
      getToken(fp, streamIP);
    }
    else if (strcmp(token, "FRAMERATE") == 0) {
      getToken(fp, token);
      frameRate = atoi(token);
    }
    else if (strcmp(token, "COMPRESSION") == 0) {
      getToken(fp, token);
      if (strcmp(token, "RLE") == 0)
        compression = RLE_COMP;
      else if (strcmp(token, "LUV") == 0)
        compression = LUV_COMP;
      else if (strcmp(token, "DXT") == 0)
        compression = DXT_COMP;
      else
        compression = NO_COMP;
    }
    else if (strcmp(token, "STREAMTYPE") == 0) {
      getToken(fp, token);
      if (strcmp(token, "SAGE_BLOCK_NO_SYNC") == 0) {
        streamType = SAGE_BLOCK_NO_SYNC;
      }
      /*
        else if (strcmp(token, "SAGE_BLOCK_CONST_SYNC") == 0) {
        streamType = SAGE_BLOCK_CONST_SYNC;
        }
        else if (strcmp(token, "SAGE_BLOCK_SOFT_SYNC") == 0) {
        streamType = SAGE_BLOCK_SOFT_SYNC;
        }
      */
      else if (strcmp(token, "SAGE_BLOCK_HARD_SYNC") == 0) {
        streamType = SAGE_BLOCK_HARD_SYNC;
      }
      else {
        streamType = SAGE_BLOCK_NO_SYNC;
      }
    }
    else if (strcmp(token, "GROUPSIZE") == 0) {
      getToken(fp, token);
      groupSize = getnumber(token); //atoi(token);
    }
    else if (strcmp(token, "AUTOBLOCKSIZE") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      autoBlockSize = (strcmp(token, "true") == 0);
    }
    else if (strcmp(token, "MAXBANDWIDTH") == 0) {
      getToken(fp, token);
      maxBandwidth = atoi(token);
    }
    else if (strcmp(token, "MAXCHECKINTERVAL") == 0) {
      getToken(fp, token);
      maxCheckInterval = atoi(token);
    }
    else if (strcmp(token, "FLOWWINDOW") == 0) {
      getToken(fp, token);
      flowWindow = atoi(token);
    }
    else if (strcmp(token, "AUDIOON") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      audioOn = (strcmp(token, "true") == 0);
    }
    else if (strcmp(token, "AUDIO") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      audioOn = (strcmp(token, "true") == 0);
    }
    else if (strcmp(token, "AUDIOKEYFRAME") == 0) {
      getToken(fp, token);
      audioKeyFrame = atoi(token);
    }
    else if(strcmp(token, "AUDIOTYPE") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      if(strcmp(token, "capture") == 0) {
        audioMode = SAGE_AUDIO_CAPTURE;
      }
      else if(strcmp(token, "fwcapture") == 0) {
        audioMode = SAGE_AUDIO_FWCAPTURE;
      }
      else if(strcmp(token, "play") == 0) {
        audioMode = SAGE_AUDIO_PLAY;
      }
      else if(strcmp(token, "read") == 0) {
        audioMode = SAGE_AUDIO_READ;
      }
      else if(strcmp(token, "appData") == 0) {
        audioMode = SAGE_AUDIO_APP;
      }
    }
    else if(strcmp(token, "AUDIOFILE") == 0) {
      getToken(fp, audioFileName);
    }
    else if (strcmp(token, "AUDIOPORT") == 0) {
      getToken(fp, token);
      audioPort = atoi(token);
    }
    else if (strcmp(token, "DEVICENUM") == 0) {
      getToken(fp, token);
      audioDeviceNum = atoi(token);
    }
    else if (strcmp(token, "SAMPLINGRATE") == 0) {
      getToken(fp, token);
      samplingRate = atoi(token);
    }
    else if (strcmp(token, "SAMPLEFORMAT") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      if (strcmp(token, "float32") == 0) {
        sampleFmt = SAGE_SAMPLE_FLOAT32;
      }
      else if (strcmp(token, "int16") == 0) {
        sampleFmt = SAGE_SAMPLE_INT16;
      }
      else if (strcmp(token, "int8") == 0) {
        sampleFmt = SAGE_SAMPLE_INT8;
      }
      else if (strcmp(token, "uint8") == 0) {
        sampleFmt = SAGE_SAMPLE_UINT8;
      }
    }
    else if (strcmp(token, "CHANNELS") == 0) {
      getToken(fp, token);
      channels = atoi(token);
    }
    else if (strcmp(token, "FRAMEPERBUFFER") == 0) {
      getToken(fp, token);
      framePerBuffer = atoi(token);
    }
    else if (strcmp(token, "SYNCTYPE") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      if (strcmp(token, "none") == 0) {
        syncType = SAGE_SYNC_NONE;
      }
      else if (strcmp(token, "audiodriven") == 0) {
        syncType = SAGE_SYNC_AUDIO_DRIVEN;
      }
      else if (strcmp(token, "graphicdriven") == 0) {
        syncType = SAGE_SYNC_GRAPHIC_DRIVEN;
      }
    }
    else if (strcmp(token, "AUDIONWPROTOCOL") == 0) {
      getToken(fp, token);
      sage::toupper(token);
      if (strcmp(token, "TCP") == 0)
        audioProtocol = SAGE_TCP;
      else if (strcmp(token, "UDP") == 0)
        audioProtocol = SAGE_UDP;
    }
    else if (strcmp(token, "RENDERBUFX") == 0) {
      getToken(fp, token);
      resX = atoi(token);
    }
    else if (strcmp(token, "RENDERBUFY") == 0) {
      getToken(fp, token);
      resY = atoi(token);
    }
    else if (strcmp(token, "SWEXP") == 0 ) {
      getToken(fp, token);
      if ( atoi(token) == 1 ) {
        swexp = true;
      }
    }
  }

  fclose(fp);

  return 0;
}
