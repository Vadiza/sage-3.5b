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
 * Copyright 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 *
 *  image.h
 *
 *  $Revision: 1.2 $
 *  $Date: 2004/06/25 03:36:29 $
 */

#ifndef __GL_IMAGE__
#define __GL_IMAGE__

/*
 *  Defines for image files . . . .
 *
 *        Paul Haeberli - 1984
 *
 *  Brutally hacked into a c++ wrapper by Dave Springer, 1988.
 *      reworked for C jimh 1991.
 *
 */

#include <stdio.h>
#include <sys/types.h>  /* for ushort16 and uint32 */

#ifndef _IOREAD
#  define _IOREAD 0x01
#endif
#ifndef _IOWRT
#  define _IOWRT  0x02
#endif
#ifndef _IORW
#  define _IORW   0x04
#endif
#ifndef _IOEOF
#  define _IOEOF  0x08
#endif
#ifndef _IOERR
#  define _IOERR  0x10
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef sun
  typedef uint16_t  IMushort;
  typedef uint32_t  IMulong;
#else
  typedef u_int16_t  IMushort;
  typedef u_int32_t  IMulong;
#endif

  typedef struct {
    IMushort  imagic;   /* stuff saved on disk . . */
    IMushort  type;
    IMushort  dim;
    IMushort  xsize;
    IMushort  ysize;
    IMushort  zsize;
    IMulong   min;
    IMulong   max;
    IMulong wastebytes;
    char  name[80];
    IMulong colormap;

    int   file;   /* stuff used in core only */
    IMushort  flags;
    int16_t dorev;
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t cnt;
    IMushort  *ptr;
    IMushort  *base;
    IMushort  *tmpbuf;
    IMulong offset;
    IMulong rleend;   /* for rle images */
    IMulong *rowstart;  /* for rle images */
    IMulong *rowsize; /* for rle images */
  } IMAGE;


  typedef struct {
    IMushort type;
    IMulong colormap;
    int xsize, ysize;
    IMulong *rowstart;
    IMulong *rowsize;
    IMushort *pixels;
  } MEMIMAGE ;

#define IMAGIC  0732

  /* type of the image */
#define CM_NORMAL   0
#define CM_DITHERED   1
#define CM_SCREEN   2
#define CM_COLORMAP   3

#define TYPEMASK  0xff00
#define BPPMASK   0x00ff
#define ITYPE_VERBATIM  0x0000
#define ITYPE_RLE 0x0100
#define ISRLE(type)   (((type) & 0xff00) == ITYPE_RLE)
#define ISVERBATIM(type)  (((type) & 0xff00) == ITYPE_VERBATIM)
#define BPP(type)   ((type) & BPPMASK)
#define RLE(bpp)    (ITYPE_RLE | (bpp))
#define VERBATIM(bpp)   (ITYPE_VERBATIM | (bpp))
#define IBUFSIZE(pixels)  ((pixels+(pixels>>6))<<2)
#define RLE_NOP   0x00

#define ierror(p) (((p)->flags&_IOERR)!=0)
#define ifileno(p)  ((p)->file)

#ifdef PIXMACROS
#define getpix(p) (--(p)->cnt>=0? *(p)->ptr++:ifilbuf(p))
#define putpix(p,x) (--(p)->cnt>=0? ((int)(*(p)->ptr++=(unsigned)(x))):iflsbuf(p,(unsigned)(x)))
#endif

  extern int  iclose(IMAGE* image);
  extern int  iflush(IMAGE* image);
  extern int  ifilbuf(IMAGE* image);
  extern int  iflsbuf(IMAGE* image, IMushort c);
  extern MEMIMAGE *newimage(int xsize, int ysize, int colormap);
  extern void freeimage(MEMIMAGE* mimage);
  extern void drawimage(int xorg, int yorg, MEMIMAGE* mimage);
  extern MEMIMAGE *readimage(char* name);
  extern MEMIMAGE *readrleimage(char* name);
  extern void img_transtoscreen(unsigned short* buf, int n);
  extern void img_setpixelortho(void);
  extern void img_makexmap(int colormap);
  extern void isetname(IMAGE* image, char* name);
  extern void isetcolormap(IMAGE* image, int colormap);
  extern IMAGE  *iopen(char* file, char* mode, ...);
  /*
    extern IMAGE  *iopen(char* file, char* mode, unsigned int type,
    unsigned int dim, unsigned int xsize,
    unsigned int ysize, unsigned int zsize);
  */
  extern IMAGE  *fiopen(int f, char* mode, unsigned int type,
                        unsigned int dim, unsigned int xsize,
                        unsigned int ysize, unsigned int zsize);
  extern IMAGE  *imgopen(int f, char* file, char* mode, unsigned int type,
                         unsigned int dim, unsigned int xsize,
                         unsigned int ysize, unsigned int zsize);
  extern unsigned short* ibufalloc(IMAGE* image);
  extern int  reverse(IMulong lwrd);
  extern void cvtshorts(IMushort* buffer, int n);
  extern void cvtlongs(IMulong* buffer, int n);
  extern void cvtimage(IMAGE *buffer);
  extern IMushort getpix(IMAGE* image);
  extern IMushort putpix(IMAGE* image, IMushort pix);
  extern int  img_seek(IMAGE* image, int y, int z);
  extern int  img_write(IMAGE* image, char* buffer, long count);
  extern int  img_read(IMAGE* image, char* buffer, long count);
  extern unsigned long img_optseek(IMAGE* image, unsigned long offset);
  extern int  img_getrowsize(register IMAGE* image);
  extern void img_setrowsize(IMAGE* image, int cnt, int y, int z);
  extern int  img_rle_compact(IMushort* expbuf, int ibpp,
                              IMushort* rlebuf, int obpp, int cnt);
  extern void img_rle_expand(IMushort* rlebuf, int ibpp,
                             IMushort* expbuf, int obpp);
  extern int  putrow(IMAGE* image, IMushort* buffer, int y, int z);
  extern int  getrow(IMAGE* image, IMushort* buffer, int y, int z);

  extern void i_errhdlr(char *fmt, ...);
  extern int  img_badrow( IMAGE *image, int y, int z );

#ifdef __cplusplus
}
#endif

#endif /* _GL_IMAGE */
