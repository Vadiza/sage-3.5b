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
 *  putrow, getrow -
 *
 *        Paul Haeberli - 1984
 *
 */
#include  <stdio.h>
#include  <errno.h>
#include  "image.h"

int putrow( IMAGE *image, IMushort *buffer, int y, int z )
{
  register IMushort   *sptr;
  register unsigned char      *cptr;
  register unsigned int x;
  register IMulong min, max;
  register int cnt;

  if( !(image->flags & (_IORW|_IOWRT)) )
    return -1;
  if(image->dim<3)
    z = 0;
  if(image->dim<2)
    y = 0;
  if(ISVERBATIM(image->type)) {
    switch(BPP(image->type)) {
    case 1:
      min = image->min;
      max = image->max;
      cptr = (unsigned char *)image->tmpbuf;
      sptr = buffer;
      for(x=image->xsize; x--;) {
        *cptr = *sptr++;
        if (*cptr > max) max = *cptr;
        if (*cptr < min) min = *cptr;
        cptr++;
      }
      image->min = min;
      image->max = max;
      img_seek(image,y,z);
      cnt = image->xsize;
      if (img_write(image,(char *)image->tmpbuf,cnt) != cnt)
        return -1;
      else
        return cnt;

    case 2:
      min = image->min;
      max = image->max;
      sptr = buffer;
      for(x=image->xsize; x--;) {
        if (*sptr > max) max = *sptr;
        if (*sptr < min) min = *sptr;
        sptr++;
      }
      image->min = min;
      image->max = max;
      img_seek(image,y,z);
      cnt = image->xsize<<1;
      if(image->dorev)
        cvtshorts(buffer,cnt);
      if (img_write(image,(char *)buffer,cnt) != cnt) {
        if(image->dorev)
          cvtshorts(buffer,cnt);
        return -1;
      } else {
        if(image->dorev)
          cvtshorts(buffer,cnt);
        return image->xsize;
      }

    default:
      i_errhdlr("putrow: wierd bpp\n");
    }
  } else if(ISRLE(image->type)) {
    switch(BPP(image->type)) {
    case 1:
      min = image->min;
      max = image->max;
      sptr = buffer;
      for(x=image->xsize; x--;) {
        if (*sptr > max) max = *sptr;
        if (*sptr < min) min = *sptr;
        sptr++;
      }
      image->min = min;
      image->max = max;
      cnt = img_rle_compact(buffer,2,image->tmpbuf,1,image->xsize);
      img_setrowsize(image,cnt,y,z);
      img_seek(image,y,z);
      if (img_write(image,(char *)image->tmpbuf,cnt) != cnt)
        return -1;
      else
        return image->xsize;
      break;

    case 2:
      min = image->min;
      max = image->max;
      sptr = buffer;
      for(x=image->xsize; x--;) {
        if (*sptr > max) max = *sptr;
        if (*sptr < min) min = *sptr;
        sptr++;
      }
      image->min = min;
      image->max = max;
      cnt = img_rle_compact(buffer,2,image->tmpbuf,2,image->xsize);
      cnt <<= 1;
      img_setrowsize(image,cnt,y,z);
      img_seek(image,y,z);
      if(image->dorev)
        cvtshorts(image->tmpbuf,cnt);
      if (img_write(image,(char *)image->tmpbuf,cnt) != cnt) {
        if(image->dorev)
          cvtshorts(image->tmpbuf,cnt);
        return -1;
      } else {
        if(image->dorev)
          cvtshorts(image->tmpbuf,cnt);
        return image->xsize;
      }
      break;

    default:
      i_errhdlr("putrow: wierd bpp\n");
    }
  } else
    i_errhdlr("putrow: wierd image type\n");
  return 0;
}

int getrow( IMAGE *image, IMushort *buffer, int y, int z )
{
  register int i;
  register unsigned char *cptr;
  register IMushort *sptr;
  register int cnt;

  if( !(image->flags & (_IORW|_IOREAD)) )
    return -1;
  if(image->dim<3)
    z = 0;
  if(image->dim<2)
    y = 0;
  img_seek(image, y, z);
  if(ISVERBATIM(image->type)) {
    switch(BPP(image->type)) {
    case 1:
      if (img_read(image,(char *)image->tmpbuf,image->xsize)
          != image->xsize)
        return -1;
      else {
        cptr = (unsigned char *)image->tmpbuf;
        sptr = buffer;
        for(i=image->xsize; i--;)
          *sptr++ = *cptr++;
      }
      return image->xsize;
    case 2:
      cnt = image->xsize<<1;
      if (img_read(image,(char *)buffer,cnt) != cnt)
        return -1;
      else {
        if(image->dorev)
          cvtshorts(buffer,cnt);
        return image->xsize;
      }
    default:
      i_errhdlr("getrow: wierd bpp\n");
      break;
    }
  } else if(ISRLE(image->type)) {
    switch(BPP(image->type)) {
    case 1:
      if( (cnt = img_getrowsize(image)) == -1 )
        return -1;
      if( img_read(image,(char *)image->tmpbuf,cnt) != cnt )
        return -1;
      else {
        img_rle_expand(image->tmpbuf,1,buffer,2);
        return image->xsize;
      }
    case 2:
      if( (cnt = img_getrowsize(image)) == -1 )
        return -1;
      if( cnt != img_read(image,(char *)image->tmpbuf,cnt) )
        return -1;
      else {
        if(image->dorev)
          cvtshorts(image->tmpbuf,cnt);
        img_rle_expand(image->tmpbuf,2,buffer,2);
        return image->xsize;
      }
    default:
      i_errhdlr("getrow: wierd bpp\n");
      break;
    }
  } else
    i_errhdlr("getrow: wierd image type\n");
  return 0;
}
