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
 * Header for generic-image access routines:
 *  getimginfo(), getimgdata()
 */


#define I_UNKNOWN 0   /* unknown type or inaccessible */
#define I_TIFF  1   /* TIFF raster image */
#define I_SGI 2   /* SGI raster image */
#define I_SUNRAS 3    /* Sun raster file */
#define I_SOFT  4   /* SoftImage raster image */
#define I_GIF   5   /* GIF image */
#define I_PNM 6   /* Portable bit/gray/pixmap */
#define I_RAW 7   /* raw image with imginfo header */
#define I_JPEG  8   /* JPEG image */

struct imginfo {
  int kind;   /* Image type -- I_TIFF, I_SGI, ... */
  int xsize, ysize; /* Image size */
  int nsamp;    /* Samples per pixel of image data */
  int sampbits;   /* Bits per sample */
  int mapsize;    /* Colormap entries (0 if absent) */
  int mapsamp;    /* Samples per colormap entry (0 if absent) */
};

typedef struct img {
#define IT_LONG 0   /* 32 bits per, ABGR, A is low-address byte */
#define IT_BIT  1   /* bit packed in bytes, MSB first */
#define IT_RGB  2   /* 24 bits per RGB, R is low-address byte */
#define IT_BYTE 3   /* 8 bits gray-scale */
  int type;
  int xsize, ysize; /* image size in pixels */
  int rowbytes;   /* size of each image row, in bytes */
  unsigned char *data;  /* image data */
} IMG;

/*
 * getimginfo returns image type, one of the I_* below.
 * It prints an error (only) if the file cannot even be opened.
 *
 * getimgdata diagnoses image type and also
 */
extern int getimginfo( char *fname, struct imginfo *info );

extern unsigned char *getimgdata(
                                 const char *fname,   /* file name */
                                 const struct imginfo *knowninfo, /* image info, if you have it */
                                 struct imginfo *info,    /* image info, filled in by getimgdata */
                                 int yup,     /* first row is... 0:bottom, 1:top */
                                 int bpp,     /* bytes per pixel, or 0:use image */
                                 int byteorder      /* 0: RGBA, 1: ABGR */
                                 );
extern char *imgtypename[];
