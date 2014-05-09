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

#include "stdafx.h"
#include "YUV.h"
#include <windows.h>

#define MYUCHAR(_flt_) ((_flt_>0 && _flt_<256)?_flt_:((_flt_<0)?0:255))


/*
  -. YUV422ToRGB24() added by shoong
  -. pbYUV : YUV array (Y0UY1V Y2UY3V ...)
  -. pbRGB : RGB array (RGB 변환된 결과값)
  -. nFileLen : YUV file length
*/
void CYUVtoRGB24::YUV422YUY2_ToRGB24(BYTE *pbYUV, BYTE *pbRGB, int nFileLen)
{
  float B_Cb128,R_Cr128,G_CrCb128;
  float Y0,U,Y1,V;
  float R,G,B;

  int j=0;

  for (int i = 0; i < m_yuv_len; )
  {
    Y0 = 1.164f * ((float)pbYUV[i++]-16.0f);
    U  = (float)pbYUV[i++]-128.0f;
    Y1 = 1.164f * ((float)pbYUV[i++]-16.0f);
    V  = (float)pbYUV[i++]-128.0f;

    R_Cr128   =  1.596f*V;
    G_CrCb128 = -0.813f*V - 0.391f*U;
    B_Cb128   =  2.018f*U;

    R= Y0 + R_Cr128;
    G = Y0 + G_CrCb128;
    B = Y0 + B_Cb128;

    pbRGB[j++] = (unsigned char)MYUCHAR(B);
    pbRGB[j++] = (unsigned char)MYUCHAR(G);
    pbRGB[j++] = (unsigned char)MYUCHAR(R);

    R= Y1 + R_Cr128;
    G = Y1 + G_CrCb128;
    B = Y1 + B_Cb128;

    pbRGB[j++] = (unsigned char)MYUCHAR(B);
    pbRGB[j++] = (unsigned char)MYUCHAR(G);
    pbRGB[j++] = (unsigned char)MYUCHAR(R);
  }
}


void CYUVtoRGB24::YUV422UYVY_ToRGB24(BYTE *pbYUV, BYTE *pbRGB, int width, int height, int Stride)
{
  float B_Cb128,R_Cr128,G_CrCb128;
  float Y0,U,Y1,V;
  float R,G,B;

  int j=0;

  for (DWORD y = 0; y < dwHeight; y++)
  {
    WORD *pwTarget = (WORD*)pbTarget;
    WORD *pwSource = (WORD*)pbSource;

    for (DWORD x = 0; x < dwWidth; x++)
    {

      // Each WORD is a 'UY' or 'VY' block.
      // Set the low byte (chroma) to 0x80 and leave the high byte (luma)

      WORD pixel = pwSource[x] & 0xFF00;
      pixel |= 0x0080;
      pwTarget[x] = pixel;
    }

    // Advance the stride on both buffers.

    pbTarget += lStrideOut;
    pbSource += lStrideIn;
  }


  for (int i = 0; i < m_yuv_len; )
  {
    Y0 = 1.164f * ((float)pbYUV[i++]-16.0f);
    U  = (float)pbYUV[i++]-128.0f;
    Y1 = 1.164f * ((float)pbYUV[i++]-16.0f);
    V  = (float)pbYUV[i++]-128.0f;

    R_Cr128   =  1.596f*V;
    G_CrCb128 = -0.813f*V - 0.391f*U;
    B_Cb128   =  2.018f*U;

    R= Y0 + R_Cr128;
    G = Y0 + G_CrCb128;
    B = Y0 + B_Cb128;

    pbRGB[j++] = (unsigned char)MYUCHAR(B);
    pbRGB[j++] = (unsigned char)MYUCHAR(G);
    pbRGB[j++] = (unsigned char)MYUCHAR(R);

    R= Y1 + R_Cr128;
    G = Y1 + G_CrCb128;
    B = Y1 + B_Cb128;

    pbRGB[j++] = (unsigned char)MYUCHAR(B);
    pbRGB[j++] = (unsigned char)MYUCHAR(G);
    pbRGB[j++] = (unsigned char)MYUCHAR(R);
  }
}


/*
  -. YUV420ToRGB24() added by shoong
  -. 아래와 같이 저장된 YUV420 file을 RGB로 변환 (IMC4 형태)
  +------------------+
  |                          |
  |           Y             |
  +------------------+
  |    Cb    |    Cr      |
  +--------+---------+
*/
void CYUVtoRGB24::YUV420ToRGB24(BYTE *pbYUV, BYTE *pbRGB, int nWidth, int nHeight)
{
  int i, j;
  int dataLeng = nHeight * nWidth;
  BYTE* yData = new BYTE[dataLeng];
  BYTE* uData = new BYTE[dataLeng];
  BYTE* vData = new BYTE[dataLeng];
  int nIndex = dataLeng;
  int offset;

  ZeroMemory(yData, dataLeng);
  ZeroMemory(uData, dataLeng);
  ZeroMemory(vData, dataLeng);

  // Y-Data 입력
  for(i = 0; i < dataLeng; i++)
  {
    yData[i] = pbYUV[i];
  }

  offset = dataLeng;
  // U-Data 입력
  for(i = 0; i < nHeight -2; i++)
  {
    for(j = 0; j < nWidth; j++)
      uData[i*nWidth + j] = pbYUV[dataLeng + (int(i/2))*nWidth/2 + (int(j/2))];
  }

  // V-Data 입력
  for(i = 0; i < nHeight -2; i++)
  {
    for(j = 0; j < nWidth; j++)
      vData[i*nWidth + j] = pbYUV[dataLeng * (int)5/4 + (int(i/2))*nWidth/2 + (int(j/2))];
  }

  // Convert to RGB
  BYTE r = 0;
  BYTE g = 0;
  BYTE b = 0;

  /*

    Y=0.3R+0.59G +0.11B
    U=(B-Y)x0.493
    V=(R-Y)x0.877

    R=Y+0.956U+0.621V
    G=Y+0.272U+0.647V
    B=Y+1.1061U+1.703V

  */
  for(i = 0; i < dataLeng; i++)
  {

    //    pbRGB[i*3] = max(0, min(yData[i] + 0.956 * uData[i] + 0.621 * vData[i], 255));
    //    pbRGB[i*3+1] = max(0, min(yData[i] + 0.272 * uData[i] + 0.647 * vData[i], 255));
    //    pbRGB[i*3+2] = max(0, min(yData[i] + 1.1061 * uData[i] + 1.703 * vData[i], 255));

    pbRGB[i*3] = CYUVTable::GetB((int)yData[i], (int)uData[i], (int)vData[i]);
    pbRGB[i*3+1] = CYUVTable::GetG((int)yData[i], (int)uData[i], (int)vData[i]);
    pbRGB[i*3+2] = CYUVTable::GetR((int)yData[i], (int)uData[i], (int)vData[i]);
  }

  delete [] yData;
  delete [] uData;
  delete [] vData;
}



CYUVtoRGB24::CYUVtoRGB24() {

  m_yuv_len = m_rgb_len = m_yuv_type = 0;
  m_rgb_buff = m_yuv_buff = NULL;
}

CYUVtoRGB24::~CYUVtoRGB24() {
  free(m_yuv_buff);
  free(m_rgb_buff);
}

// copy image data to yuv_buff
int CYUVtoRGB24::SetYuvImage(unsigned char* buff, int yuv_len, int width, int height, int type = YUV420) {

  if(!buff) return 0;

  m_width = width;
  m_height = height;
  m_yuv_len = yuv_len;
  m_rgb_len = m_width * m_height * 3;

  if(type == YUV420) m_yuv_type = YUV420;
  else if(type == YUV422) m_yuv_type = YUV422;
  else return 0;

  // If previous yuv_image exist, delete and allocate at new
  if(m_yuv_buff) free(m_yuv_buff);
  m_yuv_buff = (unsigned char*)malloc(sizeof(char) *  m_yuv_len);

  memcpy(m_yuv_buff, buff, m_yuv_len);
}


// Conversion yuv data to RGB32 as defined yuv_type
int CYUVtoRGB24::YUVtoRGB(void) {

  if(!m_yuv_buff) return 0;

  switch(m_yuv_type) {
  case YUV420 :
    if(!m_rgb_buff) free(m_rgb_buff);
    m_rgb_buff = (unsigned char*)malloc(m_width * m_height * 3);

    YUV420ToRGB24(m_yuv_buff, m_rgb_buff, m_width, m_height);
    break;
  case YUV422 :
    if(!m_rgb_buff) free(m_rgb_buff);
    m_rgb_buff = (unsigned char*)malloc(m_width * m_height * 3);

    YUV422ToRGB24(m_yuv_buff, m_rgb_buff, m_yuv_len) ;
    break;
  default :
    return 0;
  }

  return 1;

}

// return rgb image data
unsigned char* CYUVtoRGB24::GetRgbImage(void) {
  return m_rgb_buff;
}
