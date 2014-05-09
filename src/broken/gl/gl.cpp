/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
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
 * Direct questions, comments etc about SAGE to http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>


#if defined(__APPLE__)
//#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
//#include <GL/glut.h>
#include <GL/glu.h>
#endif
#include <GL/glx.h>


#define SAGE2 1


// headers for SAGE
#include "sail.h"
#include "misc.h"
int winWidth, winHeight;
int maxwinWidth, maxwinHeight;
GLubyte *rgbBuffer  = NULL;
GLubyte *rgbBuffer2 = NULL;
sail sageInf; // sail object

typedef void (*glXSwapBuffersFunc_t) (Display*, GLXDrawable);
typedef void (*glViewportFunc_t) (GLint, GLint, GLsizei, GLsizei);
typedef void (*glFlushFunc_t) (void);
typedef void (*glFinishFunc_t) (void);

static glXSwapBuffersFunc_t funcswap = NULL;
static glViewportFunc_t     funcviewport = NULL;
static glFlushFunc_t        funcflush = NULL;
static glFinishFunc_t       funcfinish = NULL;

static int count = 0;

void InitTrap()
{
  void *result;
  void* handle;

#if defined(SAGE2)
  // SAGE Configuration
  sageRect renderImageMap;
  sailConfig scfg;

  scfg.init("gl.conf");
  scfg.setAppName("gl");
  scfg.rank = 0;

  scfg.resX = maxwinWidth;
  scfg.resY = maxwinHeight;

  // set the window size to double the movie size
  scfg.winWidth = scfg.resX*2;
  scfg.winHeight = scfg.resY*2;

  renderImageMap.left = 0.0;
  renderImageMap.right = 1.0;
  renderImageMap.bottom = 0.0;
  renderImageMap.top = 1.0;

  scfg.imageMap = renderImageMap;
  scfg.pixFmt = PIXFMT_8888;
  scfg.rowOrd = BOTTOM_TO_TOP;

  sageInf.init(scfg);
#else


  // Init SAGE
  sageRect glImageMap;
  glImageMap.left = 0.0;
  glImageMap.right = 1.0;
  glImageMap.bottom = 0.0;
  glImageMap.top = 1.0;

  sailConfig scfg;
  scfg.cfgFile = strdup("sage.conf");
  scfg.appName = strdup("gl");
  scfg.rank = 0;
  scfg.execConfig = NULL;
  scfg.ip = NULL;

  scfg.resX = maxwinWidth;
  scfg.resY = maxwinHeight;
  scfg.imageMap = glImageMap;
  scfg.colorDepth = 32;
  scfg.pixFmt = TVPIXFMT_8888;
  scfg.rowOrd = BOTTOM_TO_TOP;

  sageInf.init(scfg);
#endif

  fprintf(stderr, "GL> sail initialized\n");

  // Get function symbols
#if defined(__APPLE__)
  if ( ! (handle =  dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib",RTLD_NOW)) )
#else
    //if ( ! (handle =  dlopen("/usr/lib/tls/libGL.so.1",RTLD_NOW)) )
    //if ( ! (handle =  dlopen("/usr/lib64/libGL.so.1",RTLD_NOW)) )
    char *gllib = getenv("SAGE_GL");
  if (gllib == NULL)
    gllib = strdup("/usr/lib64/libGL.so.1");

  fprintf(stderr, "GL> Using [%s] OpenGL library\n", gllib);

  if ( ! (handle =  dlopen(gllib,RTLD_NOW)) )
  {
    fprintf(stderr, "GL> Error opening libGL\n");
    exit(0);
  }
#endif

  if ( ! (result = dlsym(handle,"glXSwapBuffers")) )
  {
    fprintf(stderr, "GL> Error getting symbol glXSwapBuffers\n");
    exit(0);
  }
  else
    funcswap     = (glXSwapBuffersFunc_t)result;

  if ( ! (result = dlsym(handle,"glFlush")) )
  {
    fprintf(stderr, "GL> Error getting symbol glFlush\n");
    exit(0);
  }
  else
    funcflush     = (glFlushFunc_t)result;

  if ( ! (result = dlsym(handle,"glFinish")) )
  {
    fprintf(stderr, "GL> Error getting symbol glFinish\n");
    exit(0);
  }
  else
    funcfinish     = (glFinishFunc_t)result;


  if ( ! (result = dlsym(handle,"glViewport")) )
  {
    fprintf(stderr, "GL> Error getting symbol glViewport\n");
    exit(0);
  }
  else
    funcviewport = (glViewportFunc_t)result;

  //dlclose(handle);

}

void glFlush(void)
{
  // Real call to glFlush
  if (funcflush) (*funcflush)();

  if (rgbBuffer)
  {
    //fprintf(stderr, "GL> GLFlush %d\n", count++);

    glReadBuffer(GL_FRONT);

    if ( (maxwinWidth == winWidth) & (maxwinHeight == winHeight) )
    {
#if defined(SAGE2)
      rgbBuffer = (unsigned char*)sageInf.getBuffer();
      glReadPixels(0, 0, maxwinWidth, maxwinHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer);
      sageInf.swapBuffer();
#else
      glReadPixels(0, 0, maxwinWidth, maxwinHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer);
      sageInf.swapBuffer((unsigned char *)rgbBuffer);
#endif
    }
    else
    {
#if defined(SAGE2)
      glReadPixels(0, 0, winWidth, winHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer2);
      rgbBuffer = (unsigned char*)sageInf.getBuffer();
      memset(rgbBuffer, 0, maxwinWidth*maxwinHeight*4);
      for (int k=0;k<winHeight;k++)
      {
        memcpy(rgbBuffer+(k*maxwinWidth*4), rgbBuffer2+(k*winWidth*4), winWidth*4);
      }
      sageInf.swapBuffer();
#else
      glReadPixels(0, 0, winWidth, winHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer2);
      memset(rgbBuffer, 0, maxwinWidth*maxwinHeight*4);
      for (int k=0;k<winHeight;k++)
      {
        memcpy(rgbBuffer+(k*maxwinWidth*4), rgbBuffer2+(k*winWidth*4), winWidth*4);
      }
      sageInf.swapBuffer((unsigned char *)rgbBuffer);
#endif

    }

  }
}

void glFinish(void)
{
  // Real call to glFinish
  if (funcfinish) (*funcfinish)();
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
  //fprintf(stderr, "glXSwapBuffers -- rgbBuffer %p\n", rgbBuffer);

#if defined(SAGE2)
  if (!rgbBuffer2)
  {
    maxwinWidth  = DisplayWidth(dpy, DefaultScreen(dpy));
    maxwinHeight = DisplayHeight(dpy, DefaultScreen(dpy));

    fprintf(stderr, "GL> creating %dx%d buffer\n", maxwinWidth, maxwinHeight);

    rgbBuffer2 = (GLubyte*)malloc(maxwinWidth*maxwinHeight*4);
    memset(rgbBuffer2, 0, maxwinWidth*maxwinHeight*4);

    InitTrap();

    rgbBuffer = (unsigned char*)sageInf.getBuffer();
  }
#else
  if (!rgbBuffer)
  {
    maxwinWidth  = DisplayWidth(dpy, DefaultScreen(dpy));
    maxwinHeight = DisplayHeight(dpy, DefaultScreen(dpy));

    fprintf(stderr, "GL> creating %dx%d buffer\n", maxwinWidth, maxwinHeight);

    if (rgbBuffer) free(rgbBuffer);
    rgbBuffer = (GLubyte*)malloc(maxwinWidth*maxwinHeight*4);
    memset(rgbBuffer, 0, maxwinWidth*maxwinHeight*4);
    rgbBuffer2 = (GLubyte*)malloc(maxwinWidth*maxwinHeight*4);
    memset(rgbBuffer2, 0, maxwinWidth*maxwinHeight*4);

    InitTrap();
  }
#endif

#if !defined(SAGE2)
  if (rgbBuffer)
#endif
  {
    //fprintf(stderr, "GL> Reading pixels %dx%d buffer\n", winWidth, winHeight);

    glReadBuffer(GL_BACK);

    if ( (maxwinWidth == winWidth) & (maxwinHeight == winHeight) )
    {
#if defined(SAGE2)
      rgbBuffer = (unsigned char*)sageInf.getBuffer();
      glReadPixels(0, 0, maxwinWidth, maxwinHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer);
      sageInf.swapBuffer();
#else
      glReadPixels(0, 0, maxwinWidth, maxwinHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer);
      sageInf.swapBuffer((unsigned char *)rgbBuffer);
#endif
    }
    else
    {
#if defined(SAGE2)
      rgbBuffer = (unsigned char*)sageInf.getBuffer();

      glReadPixels(0, 0, winWidth, winHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer2);
      memset(rgbBuffer, 0, maxwinWidth*maxwinHeight*4);
      for (int k=0;k<winHeight;k++)
      {
        memcpy(rgbBuffer+(k*maxwinWidth*4), rgbBuffer2+(k*winWidth*4), winWidth*4);
      }

      sageInf.swapBuffer();
#else
      glReadPixels(0, 0, winWidth, winHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuffer2);
      memset(rgbBuffer, 0, maxwinWidth*maxwinHeight*4);
      for (int k=0;k<winHeight;k++)
      {
        memcpy(rgbBuffer+(k*maxwinWidth*4), rgbBuffer2+(k*winWidth*4), winWidth*4);
      }
      sageInf.swapBuffer((unsigned char *)rgbBuffer);
#endif
    }

    count = 0;

    //fprintf(stderr, "GL> Sending pixels\n");

    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0) {
      switch (msg.getCode()) {
      case APP_QUIT : {
        exit(1);
        break;
      }
      }
    }

  }

  // Real call to glXSwapBuffers
  if (funcswap) (*funcswap)(dpy,drawable);

}



void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
  fprintf(stderr, "GL> glViewport %d %d %d %d\n", x,y,width,height);

  winWidth = width;
  winHeight = height;

  // Real call to glViewport
  if (funcviewport) (*funcviewport)(x,y,width,height);
}
