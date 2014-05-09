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
#include "YUV_RGB.h"

/*
  typedef struct tagRGBQUAD {
  BYTE    rgbBlue;
  BYTE    rgbGreen;
  BYTE    rgbRed;
  BYTE    rgbReserved;
  } RGBQUAD;

  typedef struct tagBITMAPINFOHEADER {
  DWORD      biSize;
  LONG       biWidth;
  LONG       biHeight;
  WORD       biPlanes;
  WORD       biBitCount;
  DWORD      biCompression;
  DWORD      biSizeImage;
  LONG       biXPelsPerMeter;
  LONG       biYPelsPerMeter;
  DWORD      biClrUsed;
  DWORD      biClrImportant;
  } BITMAPINFOHEADER;

  typedef struct tagBITMAPFILEHEADER {
  WORD    bfType;
  DWORD   bfSize;
  WORD    bfReserved1;
  WORD    bfReserved2;
  DWORD   bfOffBits;
  } BITMAPFILEHEADER;

  typedef struct tagBITMAPINFO {
  BITMAPINFOHEADER    bmiHeader;      //
  RGBQUAD             bmiColors[1];   // 생상 테이블
  } BITMAPINFO;
//*/


//-----------------------------------------------------------------------------
//    SaveBMP Function
//
//  주어진 경로로 저장.
void SaveToBmp(char *Path, unsigned char *img_buff, int width, int height, int bitcount) {
  ///*
  BITMAPFILEHEADER fh;    // bmp file header
  BITMAPINFOHEADER bh;    // bmp info header

  int palSize;
  DWORD dwWritten;
  HANDLE hFile;

  palSize = (bitcount==24 ? 0:1 << bitcount)*sizeof(RGBQUAD);

  // bmp info header
  bh.biSize = sizeof(BITMAPINFOHEADER); // Bmp Header Size
  bh.biWidth = width;           // Width
  bh.biHeight = height;         // Height
  bh.biPlanes = 1;              // Planes
  bh.biBitCount = 24;           // BitCount
  //  YUV - mmioFOURCC('I','4','2','0'), width*height*3/2),
  //  RGB24 (non compression)
  bh.biCompression = BI_RGB;        // Compression
  //  bh.biSizeImage = (((width * 3) + 3) / 4) * 4 * height;
  bh.biSizeImage = width * height * (bitcount/8);
  bh.biXPelsPerMeter = 0;         // X,Y Pixels per Meter
  bh.biYPelsPerMeter = 0;
  bh.biClrUsed = 0;     // Colors that used(if 0, Maximum colors)
  bh.biClrImportant = 0;    // Important colors that used(if 0, all colors are important)

  // BITMAPFILEHEADER.
  fh.bfType = 0x4d42;
  fh.bfReserved1 = 0;
  fh.bfReserved2 = 0;
  fh.bfSize = bh.biSize + palSize + bh.biSizeImage + sizeof(BITMAPFILEHEADER);
  fh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+palSize;

  // Write to File.
  hFile=CreateFile(Path,GENERIC_WRITE,0,NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  WriteFile(hFile, &fh, sizeof(fh), &dwWritten, NULL);
  WriteFile(hFile, &bh, sizeof(bh), &dwWritten, NULL);
  WriteFile(hFile, img_buff, bh.biSizeImage, &dwWritten, NULL);

  CloseHandle(hFile);
  //  */
}
