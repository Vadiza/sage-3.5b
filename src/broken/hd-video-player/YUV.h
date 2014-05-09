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

#ifndef _YUV_H
#define _YUV_H

#include <windows.h>
#include "stdafx.h"

#define YUV422  422
#define YUV420  420

class CYUVTable
{
public :
  // Static methods
  static BYTE GetY(const BYTE p_nR, const BYTE p_nG, const BYTE p_nB)
  {
    int y = (int)(0.299 * p_nR + 0.587 * p_nG + 0.114 * p_nB);
    return max(0, min(y, 255));
  };
  static BYTE GetU(const BYTE p_nR, const BYTE p_nG, const BYTE p_nB)
  {
    int u = (int)(-0.169 * p_nR - 0.311 * p_nG + 0.500 * p_nB) + 128;
    return max(0, min(u, 255));
  };
  static BYTE GetV(const BYTE p_nR, const BYTE p_nG, const BYTE p_nB)
  {
    int v = (int)(0.500 * p_nR - 0.419 * p_nG - 0.081 * p_nB) + 128;
    return max(0, min(v, 255));
  };

  //    pbRGB[i*3] = Y+0.956U+0.621V
  //    pbRGB[i*3+1] = Y+0.272U+0.647V
  //    pbRGB[i*3+2] = Y+1.1061U+1.703V

  static BYTE GetR(const int p_nY, const int p_nU, const int p_nV)
  {
    int r = (int)(p_nY - 0.001 * (p_nU - 128) + 1.402 * (p_nV - 128));
    return max(0, min(r, 255));
  };
  static BYTE GetG(const int p_nY, const int p_nU, const int p_nV)
  {
    int g = (int)(p_nY - 0.344 * (p_nU - 128) - 0.714 * (p_nV - 128));
    return max(0, min(g, 255));
  };
  static BYTE GetB(const int p_nY, const int p_nU, const int p_nV)
  {
    int b = (int)(p_nY + 1.772 * (p_nU - 128) + 0.001 * (p_nV - 128));
    return max(0, min(b, 255));
  };
};


class CYUVtoRGB24 {

  int m_width, m_height;

  int m_yuv_len;
  int m_rgb_len;
  int m_yuv_type;

  unsigned char *m_rgb_buff;
  unsigned char *m_yuv_buff;

private :

  void YUV422UYVY_ToRGB24(unsigned char *m_pbYUV, unsigned char *m_pbRGB, int m_yuv_len);
  void YUV422YUY2_ToRGB24(unsigned char *m_pbYUV, unsigned char *m_pbRGB, int m_yuv_len);
  void YUV420ToRGB24(unsigned char *m_pbYUV, unsigned char *m_pbRGB, int m_width, int m_height);
  void YUV422UYVY_ToRGB24(BYTE *pbYUV, BYTE *pbRGB, int width, int height, int Stride);

public :

  CYUVtoRGB24();
  ~CYUVtoRGB24();

  // copy image data to yuv_buff
  int SetYuvImage(unsigned char *buff, int len, int width, int height, int type);

  // Conversion yuv data to RGB32 as defined yuv_type
  int YUVtoRGB(void);

  // return rgb image data
  unsigned char* GetRgbImage(void);

}; // class C_YUVtoRGB32

#endif
