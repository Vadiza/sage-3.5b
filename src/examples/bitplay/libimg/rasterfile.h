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

/*  @(#)rasterfile.h 1.1 86/07/07 SMI */

/*
 * Description of header for files containing raster images
 */
struct rasterfile {
  int ras_magic;    /* magic number */
  int ras_width;    /* width (pixels) of image */
  int ras_height;   /* height (pixels) of image */
  int ras_depth;    /* depth (1, 8, or 24 bits) of pixel */
  int ras_length;   /* length (bytes) of image */
  int ras_type;   /* type of file; see RT_* below */
  int ras_maptype;    /* type of colormap; see RMT_* below */
  int ras_maplength;    /* length (bytes) of following map */
  /* color map follows for ras_maplength bytes, followed by image */
};
#define RAS_MAGIC 0x59a66a95

/* Sun supported ras_type's */
#define RT_OLD    0 /* Raw pixrect image in 68000 byte order */
#define RT_STANDARD 1 /* Raw pixrect image in 68000 byte order */
#define RT_BYTE_ENCODED 2 /* Run-length compression of bytes */
#define RT_EXPERIMENTAL 0xffff  /* Reserved for testing */

/* Sun registered ras_maptype's */
#define RMT_RAW   2
/* Sun supported ras_maptype's */
#define RMT_NONE  0 /* ras_maplength is expected to be 0 */
#define RMT_EQUAL_RGB 1 /* red[ras_maplength/3],green[],blue[] */

/*
 * NOTES:
 *  Each line of the image is rounded out to a multiple of 16 bits.
 *   This corresponds to the rounding convention used by the memory pixrect
 *   package (/usr/include/pixrect/memvar.h) of the SunWindows system.
 *  The ras_encoding field (always set to 0 by Sun's supported software)
 *   was renamed to ras_length in release 2.0.  As a result, rasterfiles
 *   of type 0 generated by the old software claim to have 0 length; for
 *   compatibility, code reading rasterfiles must be prepared to compute the
 *   true length from the width, height, and depth fields.
 */