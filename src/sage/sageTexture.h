/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageTexture.h
 * Author : Luc Renambot
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
 * Direct questions, comments etc about SAGE to renambot@uic.edu
 * http://sagecommons.org/
 *
 ***************************************************************************************************************************/

#ifndef SAGE_TEXTURE_H
#define SAGE_TEXTURE_H

#ifndef GL_ABGR_EXT
#  define GL_ABGR_EXT 0x8000
#endif
#ifndef GL_BGR
#  define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#  define GL_BGRA 0x80E1
#endif


#ifndef GL_UNSIGNED_SHORT_5_6_5
#  define GL_UNSIGNED_SHORT_5_6_5      0x8363
#endif

// awf: more (similar) defines for win32...
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#  define GL_UNSIGNED_SHORT_5_5_5_1      0x8034
#endif
#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#  define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8066
#endif

#include "displayContext.h"
#include "sageDraw.h"
#include "sagePixelType.h"
#include "sageSharedData.h"


int GLprintError(const char *file, int line);

class sageBlock;
class sagePixelBlock;


// Pixel formats:
//     PIXFMT_NULL, PIXFMT_555, PIXFMT_555_INV, PIXFMT_565, PIXFMT_565_INV,
//     PIXFMT_888, PIXFMT_888_INV, PIXFMT_8888, PIXFMT_8888_INV, PIXFMT_RLE, PIXFMT_LUV,
//     PIXFMT_DXT, PIXFMT_YUV, PIXFMT_RGBS3D, PIXFMT_DXT5, PIXFMT_DXT5YCOCG};


class sageTexture
{
public:
  sageTexture(int _w, int _h, sagePixFmt _t);
  ~sageTexture();

public:
  void init(sageRect &viewPort, sageRect &blockLayout, sageRotation orientation);
  void genTexCoord();

  // Copy a block of pixels into the CPU memory buffer
  virtual void copyIntoTexture(int _x, int _y, int _width, int _height, char *_block) = 0;
  virtual void loadPixelBlock(sagePixelBlock *block) = 0;
  virtual void draw(float depth, int alpha, int tempAlpha,
                    float left, float right, float bottom, float top) = 0;
  virtual void renewTexture() = 0;
  virtual void uploadTexture();

  void drawQuad(float depth, int alpha, int tempAlpha,
                float left, float right, float bottom, float top);

  void deleteTexture();
  int  needsUpload() { return usePBO;}

protected:
  int         texWidth, texHeight;
  sagePixFmt  pixelType;

  // OpenGL texture
  GLuint     texHandle;
  GLubyte*   texture;

  sageRect   texCoord;
  GLenum     target;
  sageRect   texInfo;

  // OpenGL PBO support
  GLuint    pboIds[2];
  int       pindex;
  int       usePBO;

  double   bpp;
  sagePixelType pInfo;

};

class sageTextureRGB : public sageTexture
{
public:
  sageTextureRGB(int _w, int _h, sagePixFmt _t);
  virtual ~sageTextureRGB() {};

  virtual void renewTexture();
  virtual void copyIntoTexture(int _x, int _y, int _width, int _height, char *_block);
  virtual void loadPixelBlock(sagePixelBlock *block);
  virtual void draw(float depth, int alpha, int tempAlpha,
                    float left, float right, float bottom, float top);
};

class sageTextureYUV : public sageTexture
{
public:
  static GLhandleARB programHandleYUV;

public:
  sageTextureYUV(int _w, int _h, sagePixFmt _t);
  virtual ~sageTextureYUV() {};

  virtual void renewTexture();
  virtual void copyIntoTexture(int _x, int _y, int _width, int _height, char *_block);
  virtual void loadPixelBlock(sagePixelBlock *block);
  virtual void draw(float depth, int alpha, int tempAlpha,
                    float left, float right, float bottom, float top);
};


class sageTextureDXT : public sageTexture
{
public:
  static GLhandleARB programHandleDXT5YCOCG;

public:
  sageTextureDXT(int _w, int _h, sagePixFmt _t);
  virtual ~sageTextureDXT() {};

  virtual void renewTexture();
  virtual void copyIntoTexture(int _x, int _y, int _width, int _height, char *_block);
  virtual void loadPixelBlock(sagePixelBlock *block);
  virtual void draw(float depth, int alpha, int tempAlpha,
                    float left, float right, float bottom, float top);
};


class sageTextureS3DRGB : public sageTexture
{
protected:
  GLuint     texRHandle;
  GLubyte*   textureR;
  GLuint     pboIdsR[2];

public:
  static GLhandleARB programHandleS3DRGB;

public:
  sageTextureS3DRGB(int _w, int _h, sagePixFmt _t);
  virtual ~sageTextureS3DRGB() {};

  virtual void renewTexture();
  virtual void copyIntoTexture(int _x, int _y, int _width, int _height, char *_block);
  virtual void loadPixelBlock(sagePixelBlock *block);
  virtual void draw(float depth, int alpha, int tempAlpha,
                    float left, float right, float bottom, float top);
  virtual void uploadTexture();
};

#endif
