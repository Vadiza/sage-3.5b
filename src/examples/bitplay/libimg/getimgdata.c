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

#define USEDISP 1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "imginfo.h"

#ifdef sun
typedef uint32_t u_int32_t;
#endif

/*
 * Routine to read an image in simple raster form.
 * Given a file name, may fill in an "imginfo" structure,
 * and returns the type of image.
 */
#if USEDISP
extern IMG *TIFFmakedisp(), *SGImakedisp(), *softmakedisp(),
  *rasmakedisp(), *gifmakedisp(), *pnmmakedisp(),
  *jpegmakedisp();

static IMG *(*imgdatafunc[])() = {
  NULL,
  TIFFmakedisp,
  SGImakedisp,
  rasmakedisp,
  softmakedisp,
  gifmakedisp,
  pnmmakedisp,
  NULL/*rawmakedisp?*/,
  jpegmakedisp
};

#else /*!USEDISP*/

extern unsigned char *img_tiffdata();
extern unsigned char *img_sgidata();
extern unsigned char *img_rasdata();
extern unsigned char *img_softdata();
extern unsigned char *img_gifdata();
extern unsigned char *img_pnmdata();
extern unsigned char *img_rawdata();
extern unsigned char *img_jpegdata();

static unsigned char *(*imgdatafunc[])() = {
  NULL,
  img_tiffdata,
  img_sgidata,
  img_rasdata,
  img_softdata,
  img_gifdata,
  img_pnmdata,
  /*img_rawdata*/NULL,
  img_jpegdata,
};
#endif /*!USEDISP*/


unsigned char *
getimgdata(
           const char *fname,   /* file name */
           const struct imginfo *knowninfo, /* image info if already got by getimginfo */
           struct imginfo *wantinfo,  /* image info if wanted */
           /* Flags for desired image format */
           int yup,     /* Y increases up */
           int bpp,     /* bytes per pixel, or 0 for image's own */
           int byteorder)   /* 0: RGBA, 1: ABGR */
{
  struct imginfo dummy;
  struct imginfo *info;
#if USEDISP
  register unsigned char *p;
  register int i, n;
  int y;
  IMG *im;
#endif

  if(knowninfo != NULL) {
    info = (struct imginfo *)knowninfo;
  } else {
    info = wantinfo ? wantinfo : &dummy;
    getimginfo((char *)fname, info);
  }

  if((unsigned)info->kind >= sizeof(imgdatafunc)/sizeof(imgdatafunc[0])
     || imgdatafunc[info->kind] == NULL)
    return NULL;

#if USEDISP
  /* just for now ... (ha!) */
  if(bpp != 4 && bpp != 0) {
    fprintf(stderr, "getimgdata: can only handle 4 byte/pixel requests\n");
    return NULL;
  }
  im = (*imgdatafunc[info->kind])(fname);
  if(im == NULL || im->data == NULL)
    return NULL;
  switch(im->type) {
  case IT_LONG:
    p = im->data;
    if(!yup) {
      register unsigned char *q = p + 4 * im->xsize * (im->ysize - 1);
      if(byteorder) {
        for(n = im->ysize / 2; --n >= 0; q -= 2*4*im->xsize) {
          register int k = im->xsize;
          do {
            i = *(u_int32_t *)p;  *(u_int32_t *)p = *(u_int32_t *)q;  *(u_int32_t *)q = i;
            p += sizeof(u_int32_t);  q += sizeof(u_int32_t);
          } while(--k);
        }
      } else {
        for(n = im->ysize / 2; --n >= 0; q -= 2*4*im->xsize) {
          register int k = im->xsize;
          do {
            i = p[0]; p[0] = q[3]; q[3] = i;
            i = p[1]; p[1] = q[2]; q[2] = i;
            i = p[2]; p[2] = q[1]; q[1] = i;
            i = p[3]; p[3] = q[0]; q[0] = i;
            p += 4;
            q += 4;
          } while(--k);
        }
      }
      if(im->ysize & 1) {
        register int k = im->xsize;
        do {
          i = p[0]; p[0] = p[3]; p[3] = i;
          i = p[1]; p[1] = p[2]; p[2] = i;
          p += 4;
        } while(--k);
      }
    } else if(byteorder) {
      i = im->xsize * im->ysize;
      do {
        *(u_int32_t *)p = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
        p += sizeof(u_int32_t);
      } while(--i);
    }
    p = im->data;
    break;

  case IT_BIT:
    {
      unsigned char *obuf = (unsigned char *)malloc(sizeof(u_int32_t)*(im->xsize*im->ysize + 7));
      register u_int32_t *q = (u_int32_t *)obuf;

      for(y = im->ysize; --y >= 0; ) {
        p = &im->data[y*im->rowbytes];
        i = im->xsize >> 3;
        do {
          n = *p++;
          *q++ = n&0x80 ? 0xFFFFFFFF : 0;
          *q++ = n&0x40 ? 0xFFFFFFFF : 0;
          *q++ = n&0x20 ? 0xFFFFFFFF : 0;
          *q++ = n&0x10 ? 0xFFFFFFFF : 0;
          *q++ = n&0x08 ? 0xFFFFFFFF : 0;
          *q++ = n&0x04 ? 0xFFFFFFFF : 0;
          *q++ = n&0x02 ? 0xFFFFFFFF : 0;
          *q++ = n&0x01 ? 0xFFFFFFFF : 0;
        } while(--i > 0);
      }
      free(im->data);
      p = obuf;
      break;
    }

  default:
    p = NULL;
    break;
  }
  free(im);
  return p;
#else /*!USEDISP*/
  return (*imgdatafunc[info->kind])(fname, info->xsize, info->ysize,
                                    yup, bpp, byteorder);
#endif /*!USEDISP*/
}

/*
 * Utility routine shared by some _data routines
 * Selects/reorders pixels in a row, in place.
 */
void
_img_rerow(row, xsize, hasbpp, wantbpp, reorder)
  unsigned char *row;
int xsize;
int hasbpp;
int wantbpp;
int reorder;
{
  register unsigned char *p, *q;
  register int j, k;

  if(xsize <= 0)
    return;

  k = xsize;
  p = row;
  if(hasbpp == wantbpp) {
    if(!reorder)
      return;

    switch(wantbpp) {
    case 2:
    case 3:
      do {
        j = *p;
        *p = *q;
        *q = j;
        p = q;
        p += wantbpp;
        q += wantbpp;
      } while(--k);
      break;

    case 4:
      do {
        j = *p;
        *p = p[3];
        p[3] = j;
        j = p[1];
        p[1] = p[2];
        p[2] = j;
        p += 4;
      } while(--k);
      break;
    }
  } else if(hasbpp > wantbpp) {
    /* Shorten the row; we can copy scanning forward */
    q = p;
    if(reorder) {
      /* ... */
    }
  } else {
    /* ... */
  }
}
