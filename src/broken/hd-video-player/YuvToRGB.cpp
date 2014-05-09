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

#include <windows.h>
#include "stdafx.h"
#include "YuvToRGB.h"

#define MYUCHAR(_flt_) ((_flt_>0 && _flt_<256)?_flt_:((_flt_<0)?0:255))

// U0 Y0 Vo Y1 | U2 Y2 V2 Y3 | U4 Y4 V4 Y5 | ...
void YUV422UYVY_ToRGB24(unsigned char* pbYUV, unsigned char* pbRGB, DWORD yuv_len)
{

  //   /*
  int B_Cb128,R_Cr128,G_CrCb128;
  int Y0,U,Y1,V;
  int R,G,B;

  BYTE *yuv_index;
  BYTE *rgb_index;

  //  long lStride_out = (((dwWidth * 3) + 3) / 4) * 4;

  yuv_index = pbYUV;
  rgb_index = pbRGB;

  for (DWORD i = 0, j = 0; i < yuv_len; )
  {
    // Each WORD is a 'UY' or 'VY' block.
    // Set the low byte (chroma) to 0x80 and leave the high byte (luma)
    //            WORD pixel = pwSource[x] & 0xFF00;
    //          pixel |= 0x0080;
    //        pwTarget[x] = pixel;

    // R = 1.164(Y - 16)          + 1.596(V - 128)
    // G = 1.164(Y - 16)          - 0.813(V - 128) - 0.391(U - 128)
    // B = 1.164(Y - 16)                   + 2.018(U - 128)

    U  = (int)((float)yuv_index[i++]-128.0f);
    Y0 = (int)(1.164f * ((float)pbYUV[i++]-16.0f));
    V  = (int)((float)yuv_index[i++]-128.0f);
    Y1 = (int)(1.164f * ((float)yuv_index[i++]-16.0f));

    R_Cr128   =  (int)(1.596f*V);
    G_CrCb128 = (int)(-0.813f*V - 0.391f*U);
    B_Cb128   =  (int)(2.018f*U);

    R= Y0 + R_Cr128;
    G = Y0 + G_CrCb128;
    B = Y0 + B_Cb128;

    rgb_index[j++] = max(0, min(B, 255));
    rgb_index[j++] = max(0, min(G, 255));
    rgb_index[j++] = max(0, min(R, 255));

    R= Y1 + R_Cr128;
    G = Y1 + G_CrCb128;
    B = Y1 + B_Cb128;

    rgb_index[j++] = max(0, min(B, 255));
    rgb_index[j++] = max(0, min(G, 255));
    rgb_index[j++] = max(0, min(R, 255));
  }
  //  */
}
