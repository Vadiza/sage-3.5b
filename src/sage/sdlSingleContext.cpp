/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sdlSingleContext.cpp
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

#if defined(GLSL_YUV) || defined(SAGE_S3D)
#if !defined(WIN32)
#define GLEW_STATIC 1
#endif
#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <fcntl.h>
#endif


#include "sdlSingleContext.h"
#include "sageShader.h"



int sdlSingleContext::init(struct sageDisplayConfig &cfg)
{
  singleContext = true;
  configStruct = cfg;

#if defined(WIN32)
  // Weird position with windows
  _putenv("SDL_VIDEO_CENTERED=1");
#endif

  if ( configStruct.fullScreenFlag ) {
    char posStr[SAGE_NAME_LEN];
    sprintf(posStr, "SDL_VIDEO_WINDOW_POS=%d,%d",configStruct.winX, configStruct.winY);
    putenv(posStr);
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SAGE_PRINTLOG("Unable to initialize SDL: %s", SDL_GetError());
    return -1;
  }


  tileNum = cfg.dimX * cfg.dimY;
  if (tileNum > MAX_TILES_PER_NODE) {
    SAGE_PRINTLOG("sdlSingleContext::init() : The tile number exceeds the maximum");
    return -1;
  }

  if (!winCreatFlag) {
    Uint32 flags = SDL_OPENGL;
    if ( configStruct.fullScreenFlag ) {
      SAGE_PRINTLOG("Fullscreen> flag %d", configStruct.fullScreenFlag);
      if (configStruct.fullScreenFlag == 2)
        flags |= SDL_FULLSCREEN;
      else {
        flags |= SDL_NOFRAME;
        flags |= SDL_RESIZABLE;
      }
    }

    window_width  = cfg.width*cfg.dimX;
    window_height = cfg.height*cfg.dimY;
    surface = SDL_SetVideoMode(window_width, window_height, 0, flags);
    if (surface == NULL) {
      SAGE_PRINTLOG("Unable to create OpenGL screen: %s", SDL_GetError());
      SDL_Quit();
      exit(2);
    }

    SDL_WM_SetCaption("SAGE DISPLAY", NULL);
    winCreatFlag = true;

    if ( configStruct.fullScreenFlag )
      SDL_ShowCursor(SDL_DISABLE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

#if defined(GLSL_YUV) || defined(SAGE_S3D)
    // Initialize the "OpenGL Extension Wrangler" library
    glewInit();
#endif

    SAGE_PRINTLOG("sdlSingleContext:init(): Window created");
  }

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  // dark gray background
  //changeBackground(20,20,20);

  return 0;
} // End of sdlSingleContext::init()

void sdlSingleContext::clearScreen()
{
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void sdlSingleContext::setupViewport(int i, sageRect &tileRect)
{
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  glViewport(tileRect.x, tileRect.y, tileRect.width, tileRect.height);
}

void sdlSingleContext::refreshScreen()
{
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  SDL_GL_SwapBuffers();
}

void sdlSingleContext::changeBackground(int red, int green, int blue)
{
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  glClearColor((float)red/255.0f, (float)green/255.0f, (float)blue/255.0f, 0.0f);
}

sdlSingleContext::~sdlSingleContext()
{
  SDL_Quit();
} //End of sageDisplay::~sageDisplay()


void sdlSingleContext::checkEvent()
{
  // Event management with SDL
  SDL_Event e;

  // Service all queued events
  while (SDL_PollEvent(&e))
  {
    // Handle control keys
    if (e.type == SDL_KEYDOWN)
    {
      switch (e.key.keysym.sym)
      {
      case SDLK_ESCAPE:
        e.type = SDL_QUIT;
        break;
      case SDLK_PRINT:
        break;
      default:
        break;
      }
    }

    // More events
    switch (e.type)
    {
    case SDL_MOUSEMOTION:
      break;
    case SDL_MOUSEBUTTONDOWN:
      break;
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_JOYBUTTONDOWN:
      break;
    case SDL_JOYBUTTONUP:
      break;
    case SDL_KEYDOWN:
      break;
    case SDL_KEYUP:
      break;
    case SDL_USEREVENT:
      break;
    case SDL_VIDEOEXPOSE:
      break;
    default:
      break;
    }

    // Handle a clean exit.
    if (e.type == SDL_QUIT)
    {
      SAGE_PRINTLOG("Quitting\n");
      SDL_Quit();
      exit(2);
    }

    refreshScreen();
  }
}
