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
 *  img_getrowsize, img_setrowsize, img_rle_compact, img_rle_expand -
 *
 *        Paul Haeberli - 1984
 *
 */
#include  <stdio.h>
#include  <errno.h>
#include  "image.h"

int img_getrowsize( IMAGE *image )
{
  switch(image->dim) {
  case 1:
    return image->rowsize[0];
  case 2:
    return image->rowsize[image->y];
  case 3:
    return image->rowsize[image->y+image->z*image->ysize];
  }
  return -1;
}

void img_setrowsize( IMAGE *image, int cnt, int y, int z )
{
  IMulong *sizeptr;

  if(img_badrow(image,y,z))
    return;
  switch(image->dim) {
  case 1:
    sizeptr = &image->rowsize[0];
    image->rowstart[0] = image->rleend;
    break;
  case 2:
    sizeptr = &image->rowsize[y];
    image->rowstart[y] = image->rleend;
    break;
  case 3:
    sizeptr = &image->rowsize[y+z*image->ysize];
    image->rowstart[y+z*image->ysize] = image->rleend;
  default:
    return;
  }
  if(*sizeptr != -1)
    image->wastebytes += *sizeptr;
  *sizeptr = cnt;
  image->rleend += cnt;
}

#define docompact                                                       \
  while(iptr<ibufend) {                                                 \
    sptr = iptr;                                                        \
    iptr += 2;                                                          \
    while((iptr<ibufend)&&((iptr[-2]!=iptr[-1])||(iptr[-1]!=iptr[0])))  \
      iptr++;                                                           \
    iptr -= 2;                                                          \
    count = iptr-sptr;                                                  \
    while(count) {                                                      \
      todo = count>126 ? 126:count;                                     \
      count -= todo;                                                    \
      *optr++ = 0x80|todo;                                              \
      while(todo--)                                                     \
        *optr++ = *sptr++;                                              \
    }                                                                   \
    sptr = iptr;                                                        \
    cc = *iptr++;                                                       \
    while( (iptr<ibufend) && (*iptr == cc) )                            \
      iptr++;                                                           \
    count = iptr-sptr;                                                  \
    while(count) {                                                      \
      todo = count>126 ? 126:count;                                     \
      count -= todo;                                                    \
      *optr++ = todo;                                                   \
      *optr++ = cc;                                                     \
    }                                                                   \
  }                                                                     \
  *optr++ = 0;

int img_rle_compact( IMushort *expbuf, int ibpp, IMushort *rlebuf, int obpp, int cnt)
{
if(ibpp == 1 && obpp == 1) {
register unsigned char *iptr = (unsigned char *)expbuf;
register unsigned char *ibufend = iptr+cnt;
register unsigned char *sptr;
register unsigned char *optr = (unsigned char *)rlebuf;
register short todo, cc;
register long count;

docompact;
return optr - (unsigned char *)rlebuf;
} else if(ibpp == 1 && obpp == 2) {
register unsigned char *iptr = (unsigned char *)expbuf;
register unsigned char *ibufend = iptr+cnt;
register unsigned char *sptr;
register unsigned short *optr = rlebuf;
register short todo, cc;
register long count;

docompact;
return optr - rlebuf;
} else if(ibpp == 2 && obpp == 1) {
register unsigned short *iptr = expbuf;
register unsigned short *ibufend = iptr+cnt;
register unsigned short *sptr;
register unsigned char *optr = (unsigned char *)rlebuf;
register short todo, cc;
register long count;

docompact;
return optr - (unsigned char *)rlebuf;
} else if(ibpp == 2 && obpp == 2) {
register unsigned short *iptr = expbuf;
register unsigned short *ibufend = iptr+cnt;
register unsigned short *sptr;
register unsigned short *optr = rlebuf;
register short todo, cc;
register long count;

docompact;
return optr - rlebuf;
} else  {
i_errhdlr("rle_compact: bad bpp: %d %d\n",ibpp,obpp);
return 0;
}
}

#define doexpand                                          \
  while(1) {                                              \
            pixel = *iptr++;                              \
            if ( !(count = (pixel & 0x7f)) )              \
              return;                                     \
            if(pixel & 0x80) {                            \
                              while(count--)              \
                                *optr++ = *iptr++;        \
                              } else {                    \
                                      pixel = *iptr++;    \
                                      while(count--)      \
                                        *optr++ = pixel;  \
                                      }                   \
            }

void img_rle_expand( IMushort *rlebuf, int ibpp, IMushort *expbuf, int obpp)
{
  if(ibpp == 1 && obpp == 1) {
    register unsigned char *iptr = (unsigned char *)rlebuf;
    register unsigned char *optr = (unsigned char *)expbuf;
    register unsigned short pixel,count;

    doexpand;
  } else if(ibpp == 1 && obpp == 2) {
    register unsigned char *iptr = (unsigned char *)rlebuf;
    register unsigned short *optr = expbuf;
    register unsigned short pixel,count;

    doexpand;
  } else if(ibpp == 2 && obpp == 1) {
    register unsigned short *iptr = rlebuf;
    register unsigned char  *optr = (unsigned char *)expbuf;
    register unsigned short pixel,count;

    doexpand;
  } else if(ibpp == 2 && obpp == 2) {
    register unsigned short *iptr = rlebuf;
    register unsigned short *optr = expbuf;
    register unsigned short pixel,count;

    doexpand;
  } else
    i_errhdlr("rle_expand: bad bpp: %d %d\n",ibpp,obpp);
}
