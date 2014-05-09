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
 *  ifilbuf -
 *
 *        Paul Haeberli - 1984
 *
 */
#include  "image.h"

ifilbuf( IMAGE *image )
{
  int size;

  if ((image->flags&_IOREAD) == 0)
    return(EOF);
  if (image->base==NULL) {
    size = IBUFSIZE(image->xsize);
    if ((image->base = ibufalloc(image)) == NULL) {
      i_errhdlr("can't alloc image buffer\n");
      return EOF;
    }
  }
  image->cnt = getrow(image,image->base,image->y,image->z);
  image->ptr = image->base;
  if (--image->cnt < 0) {
    if (image->cnt == -1) {
      image->flags |= _IOEOF;
      if (image->flags & _IORW)
        image->flags &= ~_IOREAD;
    } else
      image->flags |= _IOERR;
    image->cnt = 0;
    return -1;
  }
  if(++image->y >= image->ysize) {
    image->y = 0;
    if(++image->z >= image->zsize) {
      image->z = image->zsize-1;
      image->flags |= _IOEOF;
      return -1;
    }
  }
  return *image->ptr++ & 0xffff;
}
