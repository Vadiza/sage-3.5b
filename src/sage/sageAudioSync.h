/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioSync.h
 * Author : Hyejung Hur,
 * Description : This file contains the class which provides the syncing
 *               mechanism.
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

#ifndef _SAGE_AUDIO_SYNC_H
#define _SAGE_AUDIO_SYNC_H

#include <vector>

#include "sageBase.h"
#include "streamProtocol.h"

class sageSyncInfo {
public:
  int m_sourceId;
  int m_bufferId;
  int m_keyframeNum;
  sageAudioSyncType m_syncType;
  bool m_updateFlag;
  int   m_clientSockFd;
  int m_passNum;
  bool m_lockFlag;
  sageSyncInfo() : m_sourceId(-1), m_bufferId(-1), m_keyframeNum(30), m_syncType(SAGE_SYNC_NONE),
                   m_updateFlag(false), m_clientSockFd(-1), m_passNum(0), m_lockFlag(false) {};
  ~sageSyncInfo() {};
};

class sageAudioSync {
public:
  sageAudioSync(bool audioRecv=false);
  ~sageAudioSync();

public:
  int init(int port);
  int connect(char* ip);

  /** register to buffer information for contol
   */
  void registerBuffer(sageAudioSyncType type, int sourceId, int bufferId, int keyframeNum);

  /** remove buffer from the contorl list.
   */
  int removeBuffer(int bufferId);


  /** check current frame number is keyframe
   */
  bool checkKeyFrame(int bufferId, int frameNum);

  /** check signal
   */
  bool checkSignal(int bufferId);

  int sendSignal(int bufferId);

  int checkConnections();

protected:
  int initListener(int port);
  int initSender(char* ip, int port);

  bool recvSignal(int socketFd, char* msg, int bufferId);
  int closeAll();

  int sendSignal(sageSyncInfo* info);

  void doKeyFrameAction(sageSyncInfo* info);

protected:
  /** buffer list
   */
  std::vector<sageSyncInfo*> m_syncInfoList;

  int         m_serverSockFd;
  int         m_clientSockFd;
  std::vector<int> m_clientSockList;

  sockaddr_in m_localAddr;
  sockaddr_in   m_serverAddr;

  int m_recvPort;
  int m_sendPort;

  // for identify receivers. : if it's aStreamRcv, true
  bool      m_audioReceiver;

  bool      m_connectedDone;
  // timer....

};

#endif
