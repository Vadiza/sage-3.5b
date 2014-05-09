/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: streamInfo.cpp - operations for the stream information class
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

#include "streamInfo.h"
#include "sageBlock.h"
#include <math.h>

void streamInfo::printImageInfo()
{
  std::cout << "Display Info : " << x << " " << y << " " << width << " " << height << std::endl;
  std::cout << "Image Info : " << imgCoord.x << " " << imgCoord.y << " "
            << imgCoord.width << " " << imgCoord.height << std::endl;
}

streamGroup::streamGroup() : rcvNum(0)
{
  init();
}

void streamGroup::init()
{
  if (streamList.size() > 0)
    streamList.clear();
}

int streamGroup::addStream(streamInfo &s)
{
  streamList.push_back(s);

  return 0;
}

int streamGroup::addImageInfo(sageRect &imgRect)
{
  int streamNum = streamList.size();
  //sageRect dispRect(x, y, width, height);

  for(int i=0; i<streamNum; i++) {
    sageRect normRect = streamList[i];
    normRect.normalize(*this);
    streamList[i].imgCoord = imgRect*normRect;
  }

  return 0;
}

void streamGroup::createRcvMsg(int winID, char *msgStr)
{
  sprintf(msgStr, "%d %d %d %d %d %d %d", winID, x, y, width, height, rcvNum, (int)orientation);
}

void streamGroup::createMessage(sageMessage &msg, int code)
{
  int streamNum = streamList.size();
  int dataSize = (streamNum + 1) * 128;
  msg.init(0, code, 0, dataSize, NULL);

  char *info = (char *)msg.getData();
  sprintf(info, "%d %d %d %d %d ", x, y, width, height, (int)orientation);

  for (int i=0; i<streamNum; i++) {
    char str[TOKEN_LEN];
    sprintf(str, "%d %d %d %d %d ", streamList[i].x, streamList[i].y,
            streamList[i].width, streamList[i].height, streamList[i].receiverID);
    strcat(info, str);
  }

  dataSize = strlen(info)+1;
  msg.setSize(dataSize);
}

void streamGroup::parseMessage(char *str)
{
  init();

  char token[TOKEN_LEN];
  streamInfo info;
  sageToken tokenBuf(str);

  tokenBuf.getToken(token);
  x = atoi(token);
  tokenBuf.getToken(token);
  y = atoi(token);
  tokenBuf.getToken(token);
  width = atoi(token);
  tokenBuf.getToken(token);
  height = atoi(token);
  int bufLen = tokenBuf.getToken(token);
  orientation = (sageRotation)atoi(token);

  if (bufLen < 1) {
    std::cout << "streamGroup::parseMessage() : error while parsing stream info message" <<
      std::endl;
  }

  while(bufLen > 0) {
    bufLen = tokenBuf.getToken(token);
    if (bufLen < 1)
      break;

    info.x = atoi(token);

    tokenBuf.getToken(token);
    info.y = atoi(token);

    tokenBuf.getToken(token);
    info.width = atoi(token);

    tokenBuf.getToken(token);
    info.height = atoi(token);

    tokenBuf.getToken(token);
    info.receiverID = atoi(token);

    streamList.push_back(info);
  }
}

void bridgeStreamGroup::parseMessage(char *str)
{
  char token[TOKEN_LEN];
  bridgeStreamInfo info;
  sageToken tokenBuf(str);

  tokenBuf.getToken(token);
  streamNum = atoi(token);

  if (streamList)
    delete streamList;
  streamList = new bridgeStreamInfo[streamNum];

  for (int i=0; i<streamNum; i++) {
    if (i>=1)
      streamList[i].firstID = streamList[i-1].lastID+1;

    tokenBuf.getToken(token);
    streamList[i].lastID = atoi(token);

    tokenBuf.getToken(token);
    streamList[i].receiverID = atoi(token);
  }
}
