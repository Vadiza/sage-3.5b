/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: streamInfo.h
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

#ifndef _STREAM_INFO_H
#define _STREAM_INFO_H

#include "sageBase.h"

class sageBlock;

class streamInfo : public sageRect {
public:
  sageRect imgCoord;
  int receiverID;  // Display Node ID
  int rcvNum;
  int connectionIdx;

  streamInfo() : receiverID(0), rcvNum(0) {}
  ~streamInfo() {}

  void printImageInfo();
  int generateInitBlock(sageBlock *block, int margin);
};

class streamGroup : public sageRect {
private:
  std::vector<streamInfo> streamList;
  int rcvNum;

public:
  streamGroup();
  ~streamGroup() {}
  void init();
  void operator=(sageRect& rect) { sageRect::operator=(rect); }

  streamInfo* getStream(int i) { return &streamList[i]; }
  std::vector<streamInfo> getStreamList() { return streamList; }
  int streamNum() { return streamList.size(); }
  int getRcvNum() { return rcvNum; }
  void setRcvNum(int num) { rcvNum = num; }

  int addStream(streamInfo &s);
  int addImageInfo(sageRect &imgRect);
  void createMessage(sageMessage &msg, int code);
  void createRcvMsg(int winID, char *msgStr);
  void parseMessage(char *str);
};

class bridgeStreamInfo {
public:
  int firstID, lastID;
  int receiverID;

  bridgeStreamInfo() : firstID(0), lastID(0), receiverID(0) {}
};

class bridgeStreamGroup {
public:
  bridgeStreamInfo *streamList;
  int streamNum;

  bridgeStreamGroup() : streamNum(0), streamList(NULL) {}
  void parseMessage(char *str);
};

#endif
