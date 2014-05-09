/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageAudioReceiver.h
 * Author : Byungil Jeong, Rajvikram Singh
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

#ifndef SAGEAUDIORECEIVER_H_
#define SAGEAUDIORECEIVER_H_

#include "sageReceiver.h"
#include "sageBlock.h"

class sageAudioCircBuf;
class sageEventQueue;

/**
 * \brief instantiated by initStream message in sageAudioManager
 */
class sageAudioReceiver : public sageReceiver {
protected:

  int bytesPerSample;
  sageAudioBlock audioNBlock;
  sageAudioCircBuf* buffer; /**< sageAudioCircBuf */

  int syncFrame; /**< sync frame number */

  sageEventQueue *eventQueue;
  sageSampleFmt sampleFmt;
  streamData *streamList; /**< list of streams */
  int streamIdx;

  int oldFrame;
  int updateFrame;
  int reportRate;
  sageTimer perfTimer;
  unsigned long bandWidth;
  unsigned packetLoss;

  bool activeRecv;
  bool masterFlag;
  bool resetFlag;
  int  m_senderID;

public:
  /**
   * starts sageReceiver::nwReadThread()<BR>
   * sageReceiver::readData() is pure virtual, so it should be overriden here.<BR>
   * sageReceiver::nwReadThread() is just calling sageReceiver::readData().
   */
  virtual int addStream(int senderID);

  sageAudioReceiver(char *msg, sageEventQueue *queue, streamProtocol *obj, sageAudioCircBuf *buff);
  //virtual int readAudioData(int senderID);
  //virtual int readData(int senderID, sageBlockQueue *queue);

  /**
   * It defines function body of sageReceiver::readData() which is pure virtual
   */
  virtual int readData();
  virtual ~sageAudioReceiver() {}

  /**
   * just sets sageAudioReceiver::syncFrame = frame
   */
  virtual void processSync(int frame);

public:
  int getInstID();
  int getSenderID();
  bool isActive();
  int getAudioId();
  void setMaster(bool flag);
  bool isMaster(void);

  void setResetFlag(void);
  int evalPerformance(char **frameStr, char **bandStr);
  inline void setReportRate(int rate) { reportRate = rate; }
  inline void resetTimer() { perfTimer.reset(); }
  inline void resetBandWidth() { bandWidth = 0; }
  inline void resetFrame() { oldFrame = updateFrame; }

};

inline int sageAudioReceiver::getInstID()
{
  return instID;
}

inline int sageAudioReceiver::getSenderID()
{
  return m_senderID;
}

#endif
