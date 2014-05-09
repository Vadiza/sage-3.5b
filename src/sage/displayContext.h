/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: displayContext.h
 * Author : Byungil Jeong
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

#ifndef DISPLAY_CONTEXT_H
#define DISPLAY_CONTEXT_H

#include "sage.h"

/**
 * sageDisplayConfig
 */
struct sageDisplayConfig{
  int   width;
  int   height;
  int   dimX;   // number of tiles in x for one sageDisplayManager
  int   dimY;    // number of tiles in y for one sageDisplayManager
  sageRect   tileRect[MAX_TILES_PER_NODE];

  int   mullionX;
  int   mullionY;
  int   xpos;
  int   ypos;
  int   red, green, blue;
  int   displayID;
  int   winX;
  int   winY;

  int  fullScreenFlag;
};

/**
 * class displayContext
 */
class displayContext {
protected:
  sageDisplayConfig configStruct;
  bool winCreatFlag;
  int tileNum;
  bool singleContext;

public:
  displayContext() : winCreatFlag(false) {}
  virtual int init(struct sageDisplayConfig &cfg) = 0;
  virtual void clearScreen() {}
  virtual void setupViewport(int i, sageRect &tileRect) {}
  virtual void refreshTile(int i) {}
  virtual void refreshScreen() {}
  virtual void changeBackground(int red, int green, int blue) {}
  virtual void switchContext(int i) {}
  virtual void checkEvent() {}
};

#endif
