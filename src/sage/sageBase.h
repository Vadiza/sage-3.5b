/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageBase.h
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

#ifndef _SAGE_BASE_H
#define _SAGE_BASE_H

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <numeric>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef WIN32
#include <io.h>
#include <process.h>

#define _WIN32_WINNT    0x0501  // Macro for getting the switchToThread() to work
#ifdef N_PLAT_NLM
#include <ws2nlm.h>
#else
#include <winsock2.h>
#endif
#include <ws2tcpip.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <fcntl.h>
#endif

#define SAGE_IP_LEN 128
#define SAGE_MAX_DOMAIN 10
#define SAGE_CMD_LEN 1024
#define SAGE_NAME_LEN 256
#define MAX_TILES_PER_NODE 100
#define TRACKING_DATA_SIZE 256
#define SAGE_PERF_START 10
#define CMAN_MSG_SIZE 2048
#define DISPLAY_BUF_NUM 2
#define BRIDGE_BUF_NUM  5
#define MAX_ENDPOINT_NUM 100
#define MAX_INST_NUM   100
#define MAX_FRAME_RATE 120
#define DEFAULT_IMAGE_SIZE 3145728

#define MESSAGE_HEADER_SIZE 36
#define MESSAGE_FIELD_SIZE 9

#define TOP_TO_BOTTOM 0
#define BOTTOM_TO_TOP 1

typedef unsigned sageApiOption;
#define SAGE_BLOCKING     1
#define SAGE_NON_BLOCKING 2
#define SAGE_BACKWARD     4
#define SAGE_CONTROL      8

#define SAGE_TIMEOUT 1
#define SAGE_INT_MAX 2147483647

enum nwProtocol {SAGE_TCP, SAGE_UDP, LAMBDA_STREAM};
enum sageStreamMode {SAGE_RCV, SAGE_ARCV, SAGE_SEND, SAGE_BRIDGE};
enum sagePixFmt {PIXFMT_NULL, PIXFMT_555, PIXFMT_555_INV, PIXFMT_565, PIXFMT_565_INV,
                 PIXFMT_888, PIXFMT_888_INV, PIXFMT_8888, PIXFMT_8888_INV, PIXFMT_RLE, PIXFMT_LUV,
                 PIXFMT_DXT, PIXFMT_YUV, PIXFMT_RGBS3D, PIXFMT_DXT5, PIXFMT_DXT5YCOCG};
enum sageCompressType {NO_COMP, RLE_COMP, LUV_COMP, DXT_COMP};

enum sageSampleFmt {SAGE_SAMPLE_FLOAT32, SAGE_SAMPLE_INT16, SAGE_SAMPLE_INT8, SAGE_SAMPLE_UINT8};
enum sageAudioMode {SAGE_AUDIO_CAPTURE = 1, SAGE_AUDIO_PLAY = 2, SAGE_AUDIO_READ = 4, SAGE_AUDIO_WRITE = 8, SAGE_AUDIO_APP = 16, SAGE_AUDIO_FWCAPTURE = 32};
enum sageAudioSyncType {SAGE_SYNC_NONE, SAGE_SYNC_AUDIO_DRIVEN, SAGE_SYNC_GRAPHIC_DRIVEN};

class sageMessage {
private:
  char *dest;
  char *code;
  char *appCode;
  char *size;

  void *data;
  char *buffer;
  int bufSize;
  int clientID;

public:
  sageMessage();
  ~sageMessage();
  // init by message contents
  int init(int dst, int co, int app, int sz, const void *dat);
  // init by message buffer size
  //void init();
  int init(int);
  int destroy();
  int set(int dst, int co, int app, int sz, const void *dat);
  int getDest();
  int getCode();
  int getAppCode();
  int getSize();
  int getBufSize() { return bufSize; }
  void* getData();
  char* getBuffer();
  int getClientID() { return clientID; }

  int setDest(int);
  int setCode(int);
  int setAppCode(int);
  int setSize(int s);
  int setData(int, void*);
  void setClientID(int id) { clientID = id; }
};

#include "misc.h"
#include "sageRect.h"

#endif
