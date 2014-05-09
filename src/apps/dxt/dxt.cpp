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

// From:
//    Real-Time DXT Compression
//    May 20th 2006 J.M.P. van Waveren
//    (c) 2006, Id Software, Inc.

#include "dxt.h"

byte *globalOutData;

void ExtractBlock( const byte *inPtr, int width, byte *colorBlock );

void GetMinMaxColors( const byte *colorBlock, byte *minColor, byte *maxColor );
void GetMinMaxColorsByLuminance( const byte *colorBlock, byte *minColor, byte *maxColor );
void GetMinMaxColorsByBBox( const byte *colorBlock, byte *minColor, byte *maxColor );

word ColorTo565( const byte *color );
void EmitByte( byte b );
void EmitWord( word s );
void EmitDoubleWord( dword i );

void EmitColorIndices( const byte *colorBlock, const byte *minColor, const byte *maxColor );
void EmitColorIndicesFast( const byte *colorBlock, const byte *minColor, const byte *maxColor );


void CompressImageDXT1( const byte *inBuf, byte *outBuf,
                        int width, int height, int &outputBytes )
{
#if 0
  ALIGN16( byte block[64] );
  ALIGN16( byte minColor[4] );
  ALIGN16( byte maxColor[4] );
#else
  byte block[64];
  byte minColor[4];
  byte maxColor[4];
#endif

  globalOutData = outBuf;
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4 ) {
    for ( int i = 0; i < width; i += 4 ) {
      ExtractBlock( inBuf + i * 4, width, block );
      GetMinMaxColorsByBBox( block, minColor, maxColor );
      EmitWord( ColorTo565( maxColor ) );
      EmitWord( ColorTo565( minColor ) );
      EmitColorIndicesFast( block, minColor, maxColor );
    }
  }
  outputBytes = globalOutData - outBuf;
}


void ExtractBlock( const byte *inPtr, int width, byte *colorBlock )
{
  for ( int j = 0; j < 4; j++ ) {
    memcpy( &colorBlock[j*4*4], inPtr, 4*4 );
    inPtr += width * 4;
  }
}

word ColorTo565( const byte *color )
{
  return ( ( color[ 0 ] >> 3 ) << 11 ) |
    ( ( color[ 1 ] >> 2 ) <<  5 ) |
    (   color[ 2 ] >> 3 );
}

void EmitByte( byte b )
{
  globalOutData[0] = b;
  globalOutData += 1;
}

void EmitWord( word s )
{
  globalOutData[0] = ( s >> 0 ) & 255;
  globalOutData[1] = ( s >> 8 ) & 255;
  globalOutData += 2;
}

void EmitDoubleWord( dword i )
{
  globalOutData[0] = ( i >> 0 ) & 255;
  globalOutData[1] = ( i >> 8 ) & 255;
  globalOutData[2] = ( i >> 16 ) & 255;
  globalOutData[3] = ( i >> 24 ) & 255;
  globalOutData += 4;
}


int ColorDistance( const byte *c1, const byte *c2 )
{
  return ( ( c1[0] - c2[0] ) * ( c1[0] - c2[0] ) ) +
    ( ( c1[1] - c2[1] ) * ( c1[1] - c2[1] ) ) +
    ( ( c1[2] - c2[2] ) * ( c1[2] - c2[2] ) );
}

void SwapColors( byte *c1, byte *c2 )
{
  byte tm[3];
  memcpy( tm, c1, 3 );
  memcpy( c1, c2, 3 );
  memcpy( c2, tm, 3 );
}

void GetMinMaxColors( const byte *colorBlock, byte *minColor, byte *maxColor )
{
  int maxDistance = -1;
  for ( int i = 0; i < 64 - 4; i += 4 ) {
    for ( int j = i + 4; j < 64; j += 4 ) {
      int distance = ColorDistance( &colorBlock[i], &colorBlock[j] );
      if ( distance > maxDistance ) {
        maxDistance = distance;
        memcpy( minColor, colorBlock+i, 3 );
        memcpy( maxColor, colorBlock+j, 3 );
      }
    }
  }
  if ( ColorTo565( maxColor ) < ColorTo565( minColor ) ) {
    SwapColors( minColor, maxColor );
  }
}


int ColorLuminance( const byte *color )
{
  return ( color[0] + color[1] * 2 + color[2] );
}


void GetMinMaxColorsByLuminance( const byte *colorBlock, byte *minColor, byte *maxColor )
{
  int maxLuminance = -1, minLuminance = MAX_INT;
  for (int i = 0; i < 16; i++ ) {
    int luminance = ColorLuminance( colorBlock+i*4 );
    if ( luminance > maxLuminance ) {
      maxLuminance = luminance;
      memcpy( maxColor, colorBlock+i*4, 3 );
    }
    if ( luminance < minLuminance ) {
      minLuminance = luminance;
      memcpy( minColor, colorBlock+i*4, 3 );
    }
  }
  if ( ColorTo565( maxColor ) < ColorTo565( minColor ) ) {
    SwapColors( minColor, maxColor );
  }
}

void GetMinMaxColorsByBBox( const byte *colorBlock, byte *minColor, byte *maxColor )
{
  int i;
  byte inset[3];
  minColor[0] = minColor[1] = minColor[2] = 255;
  maxColor[0] = maxColor[1] = maxColor[2] = 0;
  for ( i = 0; i < 16; i++ ) {
    if ( colorBlock[i*4+0] < minColor[0] ) { minColor[0] = colorBlock[i*4+0]; }
    if ( colorBlock[i*4+1] < minColor[1] ) { minColor[1] = colorBlock[i*4+1]; }
    if ( colorBlock[i*4+2] < minColor[2] ) { minColor[2] = colorBlock[i*4+2]; }
    if ( colorBlock[i*4+0] > maxColor[0] ) { maxColor[0] = colorBlock[i*4+0]; }
    if ( colorBlock[i*4+1] > maxColor[1] ) { maxColor[1] = colorBlock[i*4+1]; }
    if ( colorBlock[i*4+2] > maxColor[2] ) { maxColor[2] = colorBlock[i*4+2]; }
  }
  inset[0] = ( maxColor[0] - minColor[0] ) >> INSET_SHIFT;
  inset[1] = ( maxColor[1] - minColor[1] ) >> INSET_SHIFT;
  inset[2] = ( maxColor[2] - minColor[2] ) >> INSET_SHIFT;
  minColor[0] = ( minColor[0] + inset[0] <= 255 ) ? minColor[0] + inset[0] : 255;
  minColor[1] = ( minColor[1] + inset[1] <= 255 ) ? minColor[1] + inset[1] : 255;
  minColor[2] = ( minColor[2] + inset[2] <= 255 ) ? minColor[2] + inset[2] : 255;
  maxColor[0] = ( maxColor[0] >= inset[0] ) ? maxColor[0] - inset[0] : 0;
  maxColor[1] = ( maxColor[1] >= inset[1] ) ? maxColor[1] - inset[1] : 0;
  maxColor[2] = ( maxColor[2] >= inset[2] ) ? maxColor[2] - inset[2] : 0;
}


void EmitColorIndices( const byte *colorBlock, const byte *minColor, const byte *maxColor )
{
  byte colors[4][4];
  unsigned int indices[16];

  colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
  colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );
  colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 5 );
  colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
  colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );
  colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 5 );
  colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
  colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
  colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
  colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
  colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
  colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;

  for ( int i = 0; i < 16; i++ ) {
    unsigned int minDistance = MAX_INT;
    for ( int j = 0; j < 4; j++ ) {
      unsigned int dist = ColorDistance( &colorBlock[i*4], &colors[j][0] );
      if ( dist < minDistance ) {
        minDistance = dist;
        indices[i] = j;
      }
    }
  }
  dword result = 0;
  for ( int i = 0; i < 16; i++ ) {
    result |= ( indices[i] << (unsigned int)( i << 1 ) );
  }
  EmitDoubleWord( result );
}


void EmitColorIndicesFast( const byte *colorBlock, const byte *minColor, const byte *maxColor )
{
  word colors[4][4];
  dword result = 0;

  colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
  colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );
  colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 5 );
  colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
  colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );
  colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 5 );
  colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
  colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
  colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
  colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
  colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
  colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;

  for ( int i = 15; i >= 0; i-- ) {
    int c0 = colorBlock[i*4+0];
    int c1 = colorBlock[i*4+1];
    int c2 = colorBlock[i*4+2];
    int d0 = abs( colors[0][0] - c0 ) + abs( colors[0][1] - c1 ) + abs( colors[0][2] - c2 );
    int d1 = abs( colors[1][0] - c0 ) + abs( colors[1][1] - c1 ) + abs( colors[1][2] - c2 );
    int d2 = abs( colors[2][0] - c0 ) + abs( colors[2][1] - c1 ) + abs( colors[2][2] - c2 );
    int d3 = abs( colors[3][0] - c0 ) + abs( colors[3][1] - c1 ) + abs( colors[3][2] - c2 );
    int b0 = d0 > d3;
    int b1 = d1 > d2;
    int b2 = d0 > d2;
    int b3 = d1 > d3;
    int b4 = d2 > d3;
    int x0 = b1 & b2;
    int x1 = b0 & b3;
    int x2 = b0 & b4;
    result |= ( x2 | ( ( x0 | x1 ) << 1 ) ) << ( i << 1 );
  }
  EmitDoubleWord( result );
}
