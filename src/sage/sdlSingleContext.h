/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sdlSingleContext.h
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

#ifndef SDL_SINGLE_CONTEXT_H
#define SDL_SINGLE_CONTEXT_H

#include "sageDraw.h"
#include "displayContext.h"

#if defined(__APPLE__)
#include <OpenGL/gl.h>  // Header File For The OpenGL Library
#include <OpenGL/glu.h> // Header File For The GLu Library
#else
#include <GL/gl.h>      // Header File For The OpenGL Library
#include <GL/glu.h>     // Header File For The GLu Library
#endif

#ifdef sun
#include <SDL.h>
#elif defined(__APPLE__)
#include <SDL/SDL.h>
#else
#include <SDL/SDL.h>
#endif


class sdlSingleContext : public displayContext {
protected:
  SDL_Surface *surface;
  int window_width, window_height;

public:
  int init(struct sageDisplayConfig &cfg);
  void clearScreen();
  void setupViewport(int i, sageRect &tileRect);
  void refreshScreen();
  void changeBackground(int red, int green, int blue);
  ~sdlSingleContext();
  void checkEvent();
};

#endif
