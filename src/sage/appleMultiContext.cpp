/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appleMultiContext.cpp
 * Author : Luc Renambot, Byungil Jeong
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

#include "appleMultiContext.h"

#if defined(__APPLE__)

// Shared AGL pixel format and context:
AGLPixelFormat agl_fmt = 0;
AGLContext agl_ctx = 0;

// Critical section object:
MPCriticalRegionID cr = 0;

///////////////////////////
// Define a playback window

OSStatus
keyboard_handler(EventHandlerCallRef,
                 EventRef event,
                 void *)
{
  unsigned char key;
  GetEventParameter(event,kEventParamKeyMacCharCodes,typeChar,0,sizeof(key),0,&key);

  MPEnterCriticalRegion(cr,kDurationForever);

  if (key == 27) {
    exit(0);
  }

  MPExitCriticalRegion(cr);

  return noErr;
}


Window::Window(const Rect & rect, int fulls)
{
  m_rect = rect;
  m_win = 0;
  m_init = false;
  m_ctx = 0;

  //
  // Create the Carbon window:
  OSStatus s;

  if ( fulls ) {
    s = CreateNewWindow(kOverlayWindowClass,
                        kWindowStandardHandlerAttribute | kWindowCompositingAttribute,
                        &m_rect, &m_win);
    if(s != noErr) {
      SAGE_PRINTLOG("failed to create window at %i,%i,%i,%i\n",m_rect.left,m_rect.top,m_rect.right,m_rect.bottom);
      abort();
    }

    // Hide Cursor
    CGCaptureAllDisplays();
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGReleaseAllDisplays();

  }
  else
  {
    // just a little offset when opening the window
    s = CreateNewWindow(kDocumentWindowClass,
                        kWindowStandardDocumentAttributes | kWindowStandardHandlerAttribute| kWindowInWindowMenuAttribute,
                        &m_rect, &m_win);
    if(s != noErr) {
      SAGE_PRINTLOG("failed to create window at %i,%i,%i,%i\n",m_rect.left,m_rect.top,m_rect.right,m_rect.bottom);
      abort();
    }

    // Put a title
    SetWindowTitleWithCFString (m_win, CFSTR("SAGE GStream Receiver"));
  }

  SetWindowActivationScope(m_win,kWindowActivationScopeAll);
  ShowWindow(m_win);
  ActivateWindow(m_win, true);


  if ( ! fulls ) {
    Rect arect;
    short hGlobal, vGlobal;
    GetWindowBounds(m_win, kWindowStructureRgn, &arect);
    MoveWindow (m_win, arect.left, (arect.top < 0) ? 40 : arect.top+50, true);
  }

  // Create context
  m_ctx = aglCreateContext(agl_fmt,0);
  if(m_ctx == 0) {
    SAGE_PRINTLOG("failed to create OpenGL context\n");
    abort();
  }

  // Set the global context, shared by other windows:
  if(agl_ctx == 0) {
    agl_ctx = m_ctx;
  }

  //
  // All your window are belong to us:
  GLboolean e;
  e = aglSetDrawable(m_ctx,GetWindowPort(m_win));
  if(!e) {
    SAGE_PRINTLOG("failed aglSetDrawable\n");
    abort();
  }

  // Initialize the OpenGL
  aglSetCurrentContext(m_ctx);
  glViewport(0,0,m_rect.right - m_rect.left,m_rect.bottom - rect.top);

  /* glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     glOrtho(m_rect.left,m_rect.right,m_rect.top,m_rect.bottom,-1,1);

     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
  */

  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);

  aglSwapBuffers(m_ctx);
}

// Note that this function has to be called once. To recreate textures, call the renewTex() member function.
int appleMultiContext::init(struct sageDisplayConfig &cfg)
{
  // Make this a "faceful app" that can receive input events:
  ProcessSerialNumber psn;
  OSStatus s;
  s = GetCurrentProcess(&psn); assert(s == noErr);
  s = TransformProcessType(&psn,kProcessTransformToForegroundApplication); assert(s == noErr);
  s = SetFrontProcess(&psn); assert(s == noErr);

  singleContext = false;

  configStruct = cfg;
  tileNum = cfg.dimX * cfg.dimY;
  if (tileNum > MAX_TILES_PER_NODE) {
    SAGE_PRINTLOG("displayContext::init() : The tile number exceeds the maximum");
    return -1;
  }

  if (!winCreatFlag) {
    //
    // Set up us the OpenGL:
    // Choose global pixel format:
    static GLint agl_fmt_list[] = { AGL_RGBA,
                                    AGL_RED_SIZE, 8,
                                    AGL_GREEN_SIZE, 8,
                                    AGL_BLUE_SIZE, 8,
                                    AGL_ALPHA_SIZE, 8,
                                    AGL_DEPTH_SIZE, 24,
                                    AGL_DOUBLEBUFFER,
                                    AGL_NONE };
    agl_fmt = aglChoosePixelFormat(0,0,agl_fmt_list);
    if (agl_fmt == 0) {
      SAGE_PRINTLOG("failed to choose OpenGL pixel format\n");
      abort();
    }

    // The critical region object
    MPCreateCriticalRegion(&cr);

    // Install event handlers
    //
    EventTargetRef target = GetApplicationEventTarget();

    EventTypeSpec eventTypes[] = {
      { kEventClassKeyboard, kEventRawKeyDown }
    };

    InstallEventHandler(target,NewEventHandlerUPP(keyboard_handler),
                        sizeof(eventTypes) / sizeof(EventTypeSpec),eventTypes,
                        0,0);

    // Create Windows
    for (int k = 0; k < tileNum; k++) {
      int tileX = (k % cfg.dimX) * cfg.tileRect[k].width;
      int tileY = (k / cfg.dimX) * cfg.tileRect[k].height;

      Rect wrect;
      wrect.left = tileX; // cfg.tileRect[k].x;
      wrect.top = tileY;  // cfg.tileRect[k].y;
      wrect.right  = wrect.left + cfg.tileRect[k].width;
      wrect.bottom = wrect.top  + cfg.tileRect[k].height;

      //SAGE_PRINTLOG("Create window %d at %i,%i,%i,%i\n",k, wrect.left,wrect.top,wrect.right,wrect.bottom);

      windows[k] = new Window(wrect, configStruct.fullScreenFlag);
      windows[k]->beginGL();
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_TEXTURE_2D);
    }

    winCreatFlag = true;
  }

  return 0;
} // End of appleMultiContext::init()

void appleMultiContext::setupViewport(int i, sageRect &tileRect)
{
  windows[i]->beginGL();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void appleMultiContext::changeBackground(int red, int green, int blue)
{
  for (int i=0; i<tileNum; i++) {
    windows[i]->contextSwitching();
    glClearColor((float)red/255.0, (float)green/255.0, (float)blue/255.0, 0.0f);
  }
}

void appleMultiContext::checkEvent()
{
  // Event management for Apple
  EventRef theEvent;
  EventTargetRef theTarget;
  theTarget = GetEventDispatcherTarget();
  while (ReceiveNextEvent(0, NULL,kEventDurationNoWait,true, &theEvent) == noErr) {
    SendEventToEventTarget (theEvent, theTarget);
    ReleaseEvent(theEvent);
  }
}

#endif
