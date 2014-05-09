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

#ifndef BPMOVIE_H
#define BPMOVIE_H 1

#define BPMV3_MAGIC   ( ('b'<<24) | ('p'<<16) | ('m'<<8) | '3' )

enum pixeltype {
  GRAY8    = 1,       /* 8-bit grayscale */
  RGB565   = 2,       /* 16-bit (5-bit-R)<<11 | (6-bit-G)<<5 | (5-bit-R) */
  RGB888   = 3,       /* 8-bit R, G, B bytes in that order */
  ABGR8888 = 4,       /* 8-bit A, B, G, R bytes in that order */
  COMPRESSDXT1 = 5    /* S3TC DXT1 compressed texture (16 texels in 8 bytes) */
};

typedef struct {
  int magic;    /* 32-bit int BPMV_MAGIC in native byte order */
  int xsize;    /* image size */
  int ysize;
  enum pixeltype format;
  int imagestride;  /* bytes per image, including page-alignment padding */


  int xtile;    /* X-direction size of each tile */
  int ytile;    /* Y-direction size of each tile */
  int nxtile;   /* number of tiles in X-direction */
  int nytile;   /* number of tiles in Y-direction */
  int tilerowstride;  /* spacing (bytes) from one row to next within a tile */
  int xtilestride;  /* spacing (bytes) from one tile to next in X */
  int ytilestride;  /* spacing (bytes) from one tile to next in Y */

  int nframes;  /* number of frames in movie, if known, else MAXINT */

  int flags;    /* movie-type flags */
#define BPF_EXTDATA 0x01  /* Image data in separate file 'extfname' */
#define BPF_STEREO  0x02  /* Each frame is a stereo pair */
#define BPF_PREFRATE 0x04 /* Has "preferred play rate" setting */
#define BPF_PREFSHIFT 0x08  /* Has "preferred relative image shift" setting */

  long long start;  /* file offset of first image data */

  int prefrate; /* preferred play rate, frames/sec */
  int prefshift;  /* preferred relative image shift, pixels */

  char pad[56]; /* some room for extensions */

  char extfname[1024]; /* name of file with image data, if flags&BPF_EXTDATA */
  char cmd[1024]; /* command used to generate this */


} bpmvhead_t;

#endif
