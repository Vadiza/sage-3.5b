/***************************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module:  sageAudioCircBuf.h
 * Author : Byungil Jeong, Hyejung Hur
 *
 *   Description: This is the header file for the circular buffer of sage pixel blocks
 *
 *   Notes   :   This class is supposed to provide the common buffers for holding
 *         the pixel blocks. It also takes care of the reading and writing of pixel blocks
 *         in a circular fashion. The member functions will be able to provide a complete
 *         interface to handling these buffers.
 *
 *         The algo is that the buffers are writen to and read from in the same direction
 *         [0 to n and then back to 0]
 *
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
 *****************************************************************************************/


#ifndef _SAGE_AUDIO_CIRCBUF_H
#define _SAGE_AUDIO_CIRCBUF_H

#include "sageBase.h"

class sageAudioSync;
class sageSyncClient;

class audioBlock {
public:
  long frameIndex;
  long gframeIndex;
  int reformatted;
  void* buff;
  double timestamp;

  audioBlock(): frameIndex(-1), gframeIndex(-1), reformatted(0), buff(NULL), timestamp(0) {};
  ~audioBlock() {};
};

/** todo */
// need to extend for several readers and writers....

/**
 * This class is supposed to provide the common buffers for holding the pixel blocks.
 * It also takes care of the reading and writing of pixel blocks in a circular fashion.
 */
class sageAudioCircBuf {
private:
  int audioId;
  int instID;

  int readIndex;
  int writeIndex;

  pthread_mutex_t *queueLock;
  pthread_cond_t *notFull;

  /** numbers of audio block
   */
  int blocksNum;

  /** audio format : float, short, char, unsigned char
   */
  sageSampleFmt sampleFmt;

  /** buffer size in an audio block
   */
  int sampleBuffSize;

  /** array of audio block
   */
  audioBlock* blockArray;

  /** bytes in an audio block
   */
  int bytesBlock;

  bool full;
  bool empty;

  /** synchronizer for audio and video streaming
   */
  sageAudioSync* synchronizer;
  sageSyncClient *syncClientObj; /**< sageSyncClient object */

  /** make buffer lock- can not write
   */
  bool locked;

  /** frame index of last block in writing
   */
  int lastFrameIndex;
  int lastgFrameIndex;
  int syncKeyFrame; /**< synchronization key frame */

  int refRead;
  int refMutex;
  std::vector<int> readers;

protected:
  void clearBlocks();

public:
  /**
   * keyframe is set to 100 by default<BR>
   * every 100 frame, sendslaveupdate will be called
   */
  sageAudioCircBuf(sageSyncClient *sync= NULL, int nID=0, int keyframe = 100);
  ~sageAudioCircBuf();

  /** create aduio blocks and create buffers in each audio block.
   * @param id integer
   * @param blockNum integer
   * @param fmt sageSampleFmt
   * @param size integer
   * @return integer
   */
  int init(int id, int blockNum, sageSampleFmt fmt, int size);

  /** get audio block pointer for writing
   * @param void
   * @return audioBlock *
   */
  audioBlock* getNextWriteBlock();


  /** get audio block pointer for read
   * not good idea to give pointer for reading
   */
  audioBlock* readBlock(int Id = 0);
  audioBlock* readBlock(int Id, int frameNum);

  /** update read index
   * sendSlaveUpdate() is called here.
   */
  int updateReadIndex();

  /** update write index
   */
  int updateWriteIndex();

  int getReadIndex();
  int getWriteIndex();

  /** get numbers of audio blocks
   */
  int getSize();
  int getAudioId();
  int getBytesBlock();

  int merge(audioBlock* block);
  int convertToFloat(sageSampleFmt fmt, void* rawdata, audioBlock* block);


  void clearBlock(int frameNum);
  void reset();

  void connectSync(sageAudioSync* synch);

  /**
   * sends update to syncMaster with the event SAGE_UPDATE_AUDIO
   */
  void connectSyncClient(sageSyncClient* synch);

  void setInstID(int ID);

  /** use for synchronization
   */
  void setLock();
  void setFree();

  void setKeyframe(int keyno);

  /** get frame number of recently recored block
   */
  int getLastFrameIdx();

  // add
  int addReader();
  void deleteReader(int Id);
};

inline void sageAudioCircBuf::setInstID(int ID) { instID = ID; };

#endif
