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
#ifndef _YUV_RGB_
#define _YUV_RGB_

#include <windows.h>
#include "stdafx.h"

// nominal luma range
#define MIN_LUMA    16
#define MAX_LUMA    235
#define LUMA_RANGE  (MAX_LUMA - MIN_LUMA + 1)  // Total number of valid luma values

#define clipRGB(x) clip((x), 0, 255)
#define clipYUV(x) clip((x), MIN_LUMA, MAX_LUMA)

typedef struct _UYVY {
  unsigned char u;
  unsigned char y0;
  unsigned char v;
  unsigned char y1;
}UYVY_ARRAY;

void SaveToBmp(char *Path, unsigned char *img_buff, int width, int height, int bitcount);

inline long clip(long i, long min, long max)
{
  if (i < min) return min;
  if (i > max) return max;
  return i;
}

// RGB2YUV: Convert an RGB pixel to YUV

inline BYTE RGB2Y(DWORD r, DWORD g, DWORD b)
{
  BYTE luma = static_cast<BYTE>(( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16);
  _ASSERTE(luma >= MIN_LUMA);
  _ASSERTE(luma <= MAX_LUMA);
  return luma;
}

inline BYTE RGB2U(DWORD r, DWORD g, DWORD b)
{
  return static_cast<BYTE>(( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128);
}

inline BYTE RGB2V(DWORD r, DWORD g, DWORD b)
{
  return static_cast<BYTE>(( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128);
}

// Convert YUV to RGB

inline BYTE YUV2Red(long y, long u, long v)
{
  return static_cast<BYTE>(clipRGB(( 298 * (y - 16) + 409 * (v - 128) + 128) >> 8));
}


inline BYTE YUV2Green(long y, long u, long v)
{
  return static_cast<BYTE>((clipRGB(( 298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8)));
}

inline BYTE YUV2Blue(long y, long u, long v)
{
  return static_cast<BYTE>((clipRGB(( 298 * (y - 16) + 516 * (u - 128) + 128) >> 8)));
}


#endif
