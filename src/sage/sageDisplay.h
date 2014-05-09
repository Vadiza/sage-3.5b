/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDisplay.h
 * Author : Byungil Jeong, Rajvikram Singh
 *
 *   Description:   This is the header file for the video display module of SAGE.
 *
 *   Notes   :    Since the display window may receive its many pieces from many servers, we need to pass multiple buffers
 *         to this class. Each of the buffers passed contains a frame which forms a part (or whole) of the image on
 *         the display side.
 *
 * Copyright (C) 2007 Electronic Visualization Laboratory,
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
 ***************************************************************************************************************************/

#ifndef SAGE_DISPLAY_H
#define SAGE_DISPLAY_H

#include "displayContext.h"
#include "sageDraw.h"
#include "sageTexture.h"
#include "sagePixelType.h"
#include "sageSharedData.h"



#define MAX_MONTAGE_NUM    50

extern int GLprintError(const char *file, int line);


#if ! defined(WIN32)

#ifndef __SON__TIMER__H
#define __SON__TIMER__H

#ifndef __SON__SYS__TIME__H
#define __SON__SYS__TIME__H
#include <sys/time.h>
#endif

#ifndef __SON__UNISTD__H
#define __SON__UNISTD__H
#include <unistd.h>
#endif


inline
unsigned long long rdtsc(void)
{
  //unsigned long long hi, lo;
  unsigned  hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32);

}

static inline
unsigned long long  __computeFrequency (void)
{
  unsigned long long start_cycle, stop_cycle, tdiff;
  struct timeval start_tv, stop_tv;
  unsigned long long  cyclediff, freq;

  static unsigned long long _cpufreq = 0;

  gettimeofday( &start_tv, 0 );
  start_cycle = rdtsc();

  sleep(1);

  gettimeofday( &stop_tv, 0 );
  stop_cycle = rdtsc();

  /* usec */
  tdiff =
    ( stop_tv.tv_sec  * 1000 * 1000 + stop_tv.tv_usec ) -
    ( start_tv.tv_sec * 1000 * 1000 + start_tv.tv_usec );

  cyclediff = stop_cycle - start_cycle;

  /* round out freq. */
  freq = (cyclediff/tdiff) * 1000;
  freq *= 1000; // Hz

  if( _cpufreq < freq )
    _cpufreq = freq;

  return _cpufreq;
}

extern const unsigned long long m_cpuFrequency;
extern const uint64_t m_msecsTicks;
extern const uint64_t m_usecsTicks;
extern const uint64_t m_nsecsTicks;

inline
void  __usecDelay_RDTSC ( int& usec)
{
  uint64_t start_ticks;
  start_ticks = rdtsc();

  uint64_t endticks = (m_usecsTicks * usec) +  start_ticks;

  while( rdtsc() < endticks)
  {

  }
}

inline
void  __msecDelay_RDTSC ( int& msec)
{
  uint64_t start_ticks;
  start_ticks = rdtsc();

  uint64_t endticks = (m_msecsTicks * msec) +  start_ticks;

  while( rdtsc() < endticks)
  {

  }
}
#endif

#endif


class sageBlock;
class sagePixelBlock;


// The following structure holds the parameters of the montage comprising the final window

/**
 * This holds the parameters of the montage comprising the final window
 */
class sageMontage : public sageRect {
private:
  displayContext *context;

public:
  sageTexture *sageTex;

  float    depth;
  int      monIdx;
  int      tileIdx;
  bool     visible;
  int      winID;

  sageMontage *otherMontage;
  double lastTime;
  bool   doFade;
  int    alpha;


  sageMontage(displayContext *dct, sagePixFmt pfmt);
  ~sageMontage();

  int init(sageRect &viewPort, sageRect &blockLayout, sageRotation orientation);
  //int copyConfig(sageMontage &mon);
  bool checkDispInfo(sagePixelBlock *block);
  void renewTexture();
  void deleteTexture();
  void genTexCoord();
  void loadPixelBlock(sagePixelBlock *block);  // load a pixel block into texture memory
  void update(double now);
  void setAlpha(int a);

  // Copy a block of pixels into the CPU memory buffer
  void copyIntoTexture(int _x, int _y, int _width, int _height, char *_block);
  // Copy a block of DXT pixels into the CPU memory buffer
  //void copyIntoDXTTexture(int _x, int _y, int _width, int _height, char *_block);
  // Upload the CPU memory buffer to a GPU buffer (pbo)
  void uploadTexture();

  void draw(int _tempAlpha);

  //inline void getImageOrg(int &x, int &y) { x = imgInfo.x, y = imgInfo.y; }
};

/**
 * \brief The video display module of SAGE.
 *
 * This class is for drawing pixel data onto screen, and all OpenGL related codes are here
 *
 * Since the display window may receive its many pieces from many servers, we need to pass multiple buffers to this class.
 * Each of the buffers passed contains a frame which forms a part (or whole) of the image on the display side.
 */
class sageDisplay {
protected:
  displayContext   *context;
  sageDisplayConfig configStruct;
  sageMontage*      montages[MAX_TILES_PER_NODE][MAX_MONTAGE_NUM];
  int      noOfMontages[MAX_TILES_PER_NODE];
  int      tileNum;
  bool     dirty;
  int      activetile;
  long     numframes;
  sageDraw drawObj;

public:
  sageDisplay(displayContext *dct, struct sageDisplayConfig &cfg);
  int addMontage(sageMontage *mon);  // register a new montage into the montage list
  int replaceMontage(sageMontage *mon); // replace the current montage by another montage
  int removeMontage(sageMontage *mon);  // delete montage from the montage list
  int updateScreen(dispSharedData *shared=NULL, bool barrierFlag=false, bool drawOverlays=true); // draw montages onto screen
  void update();          // update regularly if needed
  int changeBGColor(int red, int green, int blue);

  void saveScreenshot(char *data);
  int addDrawObjectInstance(char *data);
  int updateObjectPosition(char *data);
  int removeDrawObject(char *data);
  int forwardObjectMessage(char *data);
  int showObject(char *data);
  int updateAppBounds(int winID, int x, int y, int w, int h, sageRotation orientation);
  int updateAppBounds(char *data);
  int updateAppDepth(int winID, float depth);
  void onAppShutdown(int winID);  // when app shuts down
  void drawAppShadow(int winID);
  void drawAppMontage(sageMontage *mon, int tempAlpha=-1);
  void setupTile(int i);

  inline void setDirty() { dirty = true; }
  inline bool isDirty() { return dirty; }
  inline sageRect& getTileRect(int idx) { return configStruct.tileRect[idx]; }
  inline int getTileNum() { return tileNum; }

  ~sageDisplay();
};

#endif
