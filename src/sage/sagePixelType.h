/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sagePixelType.h
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

#ifndef SAGE_PIXEL_TYPE_H_
#define SAGE_PIXEL_TYPE_H_

#include "sage.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#if defined (__SVR4) && defined (__sun)
typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_LUMINANCE_ALPHA                0x190A
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#else
#include <GL/gl.h>
#include <GL/glu.h>
#if defined(WIN32)
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#endif
#endif

class sagePixelType {
public:
  GLenum pixelFormat;
  GLint  internalFormat;
  GLenum pixelDataType;
  int    bytesPerPixel;

  sagePixelType() : pixelFormat(GL_RGB), internalFormat(4), pixelDataType(GL_UNSIGNED_BYTE),
                    bytesPerPixel(3) {}
  sagePixelType(sagePixFmt pix) { initType(pix); }

  void initType(sagePixFmt pixelType)
  {
    switch (pixelType)   {
    case PIXFMT_RGBS3D:
      //internalFormat = GL_RGB16;
      //pixelFormat = GL_RGB;
      //pixelDataType = GL_UNSIGNED_SHORT;
      internalFormat = GL_RGB8;
      pixelFormat = GL_RGB;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_555:
      internalFormat = GL_RGBA;
      pixelFormat = GL_RGB;
      pixelDataType = GL_UNSIGNED_SHORT_5_5_5_1;
      break;

    case PIXFMT_555_INV:
      internalFormat = GL_RGBA;
      pixelFormat = GL_BGR;
      pixelDataType = GL_UNSIGNED_SHORT_5_5_5_1;
      break;

    case PIXFMT_YUV:
#if defined(__APPLE__)
      internalFormat = GL_RGBA8;
      pixelFormat = GL_YCBCR_422_APPLE;
      pixelDataType = GL_UNSIGNED_SHORT_8_8_APPLE;
#else
      internalFormat =  2;
      pixelFormat = GL_LUMINANCE_ALPHA;
      pixelDataType = GL_UNSIGNED_BYTE;
#endif
      break;

    case PIXFMT_DXT:
      internalFormat = GL_RGBA;
      pixelFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_DXT5:
      internalFormat = GL_RGBA;
      pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_DXT5YCOCG:
      internalFormat = GL_RGBA;
      pixelFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_565:
      internalFormat = GL_RGBA;
      pixelFormat = GL_RGB;
      pixelDataType = GL_UNSIGNED_SHORT_5_6_5;
      break;

    case PIXFMT_565_INV:
      internalFormat = GL_RGBA;
      pixelFormat = GL_BGR;
      pixelDataType = GL_UNSIGNED_SHORT_5_6_5;
      break;

    case PIXFMT_888:
      internalFormat = GL_RGB8;
      pixelFormat = GL_RGB;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_888_INV:
      internalFormat = GL_RGB8;
      pixelFormat = GL_BGR;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_8888:
      internalFormat = GL_RGBA8;
      pixelFormat = GL_RGBA;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    case PIXFMT_8888_INV:
      internalFormat = GL_RGBA8;
      pixelFormat = GL_BGRA;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;

    default:
      internalFormat = GL_RGBA;
      pixelFormat = GL_RGB;
      pixelDataType = GL_UNSIGNED_BYTE;
      break;
    }

    bytesPerPixel = getPixelSize(pixelType);
  }
};

#endif
