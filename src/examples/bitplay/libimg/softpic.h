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

/*
 * SoftImage picture file format (.pic).
 * Transcribed from their printed documentation.
 *
 * Ratio: is the aspect ratio of pixels for this image.
 *  This aspect is the ratio X/Y (if a pixel is 2x wider than tall,
 *   the ratio is 2.0).  For displaying images on output devices
 *   with aspect ratio different from 1.0.
 *
 * N.B. The pixel ratio is computed according to Full Frame.
 *
 * Fields: which scan-lines are present in the picture file.
 * For normal images this will be Full-frame (3), but if you use video
 * output you will probably want to double the time sampling and compute
 * only fields (every alternate scan line).
 * The field containing the top scan-line is the ODD one (1), the other
 * is EVEN (2).  The field sequence for NTSC is E-O-E-O...,
 * while for PAL O-E-O-E...
 *
 * Chained: this flag indicates the presence of other packets, generally
 * it is 1 except for the last packet where it's 0.
 *
 * Encoding part: all encoding data are coded byte-by-byte (with some
 * exceptions) so when we code raw information, we have each byte following
 * each other.  For example, here is a 512x512 RGBA picture:
 *
 * <FILE-CHUNK>
 *  id = 0x5380f643
 *  vers = 1.2 (only informatiive info)
 *  comment = 'none'
 *
 * <PICT-CHUNK>
 *  id='PICT'
 *  width=512
 *  height=512
 *  ratio=1.0
 *  fields=3 (full-frame)
 *  padding=(unused)
 *
 * <PACK-CHUNK>
 *  chained=1
 *  nb bits=8
 *  type=0
 *  fields=224 (red+green+blue)
 *
 * <PACK-CHUNK>
 *  chained=0
 *  nb bits=8
 *  type=0
 *  fields=16 (alpha)
 *
 * <SCAN-LINE 1>
 *  R,G,B,... (3x512 bytes)
 *  A,A,A,... (512 bytes)
 *
 * <SCAN-LINE 2>
 *  R,G,B,...
 *  A,...
 * ...
 *
 */

/*
 * All binary files have a similar header:
 */

struct softhdr {
  unsigned long id; /* SoftImage magic number */
#define SOFT_MAGIC  0x5380f634

  float version;  /* Version of system that produced it */
  char  comment[80];  /* user-definable comment */
};

/*
 * Then follows the specific chunk for the type of file (here .pic):
 */

struct softpict {
  unsigned long id; /* 'PICT' */
#define SPICT_MAGIC 0x50494354

  short width, height;  /* in pixels */
  float ratio;    /* Pixel ratio (X/Y) */
  short fields;   /* 1=odd, 2=even, 3=full-frame */
  short padding;  /* (to pad this struct to 4-byte multiple) */
};

/*
 * Now the associated data.
 * For a picture file we find first a list of descriptor packets.
 * A packet describes how the different fields of an image are coded, i.e.
 * which fields are coded together, the type of data (integer, float &c)
 * and the compression technique.
 */

struct softpack {   /* Pack chunk */
  unsigned char chained;  /* another pack chunk follows this one */
  unsigned char nbits;  /* bits per data value (8?) */
  unsigned char type; /* bit-field describing data format: */
#define SPICT_UINT  0x00  /* unsigned int */
#define SPICT_SINT  0x10  /* signed int */
#define SPICT_FLOAT 0x20  /* float */
#define SPICT_NOCOMP  0x00  /* uncompressed */
#define SPICT_PURERLE 0x01  /* "pure" run-length encoded */
#define SPICT_MIXEDRLE  0x02  /* "mixed" run-length encoded */

  unsigned char fields; /* bit-field describing what data are present */
#define SPICT_R   0x80  /*  red */
#define SPICT_G   0x40  /*  green */
#define SPICT_B   0x20  /*  blue */
#define SPICT_A   0x10  /*  alpha */
#define SPICT_SHADOW  0x08  /*  shadow */
#define SPICT_DEPTH 0x04  /*  depth */
#define SPICT_AUX1  0x02  /*  auxiliary 1 */
#define SPICT_AUX2  0x01  /*  auxiliary 2 */

};
