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
 *  iclose and iflush -
 *
 *        Paul Haeberli - 1984
 *
 */
#include  <stdio.h>
#include  <errno.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <fcntl.h>
#include  "image.h"

int iclose(IMAGE *image)
{
  long tablesize;

  iflush(image);
  img_optseek(image, 0);
  if (image->flags&_IOWRT) {
    if(image->dorev)
      cvtimage(image);
    if (img_write(image,(char *)image,sizeof(IMAGE)) != sizeof(IMAGE)) {
      i_errhdlr("iflush: error on write of image header\n");
      return EOF;
    }
    if(image->dorev)
      cvtimage(image);
    if(ISRLE(image->type)) {
      img_optseek(image, 512L);
      tablesize = image->ysize*image->zsize*sizeof(long);
      if(image->dorev)
        cvtlongs(image->rowstart,tablesize);
      if (img_write(image,(char *)image->rowstart,tablesize) != tablesize) {
        i_errhdlr("iflush: error on write of rowstart\n");
        return EOF;
      }
      if(image->dorev)
        cvtlongs(image->rowsize,tablesize);
      if (img_write(image,(char *)image->rowsize,tablesize) != tablesize) {
        i_errhdlr("iflush: error on write of rowsize\n");
        return EOF;
      }
    }
  }
  if(image->base) {
    free(image->base);
    image->base = 0;
  }
  if(image->tmpbuf) {
    free(image->tmpbuf);
    image->tmpbuf = 0;
  }
  if(ISRLE(image->type)) {
    free(image->rowstart);
    image->rowstart = 0;
    free(image->rowsize);
    image->rowsize = 0;
  }
  return(close(image->file));
}

int iflush(IMAGE *image)
{
  IMushort *base;

  if ( (image->flags&_IOWRT)
       && (base=image->base)!=NULL && (image->ptr-base)>0) {
    if (putrow(image, base, image->y,image->z)!=image->xsize) {
      image->flags |= _IOERR;
      return(EOF);
    }
  }
  return 0;
}
