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
 *  iopen -
 *
 *        Paul Haeberli - 1984
 *
 */
#include  <stdio.h>
#include  <errno.h>
#include  <stdarg.h>
#include  <stdlib.h>
#include  <memory.h>
#include  <unistd.h>
#include  <fcntl.h>
#include  <stdarg.h>

#include  "image.h"

IMAGE *imgopen();

IMAGE *iopen(char *file,
             register char *mode,
             ...)
{
  unsigned int type, dim, xsize, ysize, zsize;
  va_list ap;
  va_start(ap, mode);
  type = va_arg(ap, unsigned int);
  dim  = va_arg(ap, unsigned int);
  xsize = va_arg(ap, unsigned int);
  ysize = va_arg(ap, unsigned int);
  zsize = va_arg(ap, unsigned int);
  va_end(ap);

  return(imgopen(0, file, mode, type, dim, xsize, ysize, zsize));
}

IMAGE *fiopen(f, mode, type, dim, xsize, ysize, zsize)
int f;
register char *mode;
unsigned int type, dim, xsize, ysize, zsize;
{
  return(imgopen(f, 0, mode, type, dim, xsize, ysize, zsize));
}

IMAGE *imgopen(f, file, mode, type, dim, xsize, ysize, zsize)
char *file;
int f;
register char *mode;
unsigned int type, dim, xsize, ysize, zsize;
{
  register IMAGE  *image;
  extern int errno;
  register int rw;
  int tablesize;
  register int i, max;

  image = (IMAGE*)malloc(sizeof(IMAGE));
  bzero(image,sizeof(IMAGE));
  rw = mode[1] == '+';
  if(rw) {
    i_errhdlr("iopen: read/write mode not supported\n");
    return NULL;
  }
  if (*mode=='w') {
    if (file) {
      f = creat(file, 0666);
      if (rw && f>=0) {
        close(f);
        f = open(file, 2);
      }
    }
    if (f < 0) {
      i_errhdlr("iopen: can't open output file %s\n",file);
      return NULL;
    }
    image->imagic = IMAGIC;
    image->type = type;
    image->xsize = xsize;
    image->ysize = 1;
    image->zsize = 1;
    if (dim>1)
      image->ysize = ysize;
    if (dim>2)
      image->zsize = zsize;
    if(image->zsize == 1) {
      image->dim = 2;
      if(image->ysize == 1)
        image->dim = 1;
    } else {
      image->dim = 3;
    }
    image->min = 10000000;
    image->max = 0;
    isetname(image,"no name");
    image->wastebytes = 0;
    {
      static int one = 1;
      /* external format should be big-endian */
      image->dorev = (*(char *)&one == 1) ? 1 : 0;
    }
    if (write(f,image,sizeof(IMAGE)) != sizeof(IMAGE)) {
      i_errhdlr("iopen: error on write of image header\n");
      return NULL;
    }
  } else {
    if (file)
      f = open(file, rw? 2: 0);
    if (f < 0)
      return(NULL);
    if (read(f,image,sizeof(IMAGE)) != sizeof(IMAGE)) {
      i_errhdlr("iopen: error on read of image header\n");
      return NULL;
    }
    if( ((image->imagic>>8) | ((image->imagic&0xff)<<8))
        == IMAGIC ) {
      image->dorev = 1;
      cvtimage(image);
    } else
      image->dorev = 0;
    if (image->imagic != IMAGIC) {
      i_errhdlr("iopen: bad magic in image file %x\n",image->imagic);
      return NULL;
    }
  }
  if (rw)
    image->flags = _IORW;
  else if (*mode != 'r')
    image->flags = _IOWRT;
  else
    image->flags = _IOREAD;
  if(ISRLE(image->type)) {
    tablesize = image->ysize*image->zsize*sizeof(IMulong);
    image->rowstart = (IMulong *)malloc(tablesize);
    image->rowsize = (IMulong *)malloc(tablesize);
    if( image->rowstart == 0 || image->rowsize == 0 ) {
      i_errhdlr("iopen: error on table alloc\n");
      return NULL;
    }
    image->rleend = 512L+2*tablesize;
    if (*mode=='w') {
      max = image->ysize*image->zsize;
      for(i=0; i<max; i++) {
        image->rowstart[i] = 0;
        image->rowsize[i] = -1;
      }
    } else {
      tablesize = image->ysize*image->zsize*sizeof(IMulong);
      lseek(f, 512L, 0);
      if (read(f,image->rowstart,tablesize) != tablesize) {
        i_errhdlr("iopen: error on read of rowstart\n");
        return NULL;
      }
      if(image->dorev)
        cvtlongs(image->rowstart,tablesize);
      if (read(f,image->rowsize,tablesize) != tablesize) {
        i_errhdlr("iopen: error on read of rowsize\n");
        return NULL;
      }
      if(image->dorev)
        cvtlongs(image->rowsize,tablesize);
    }
  }
  image->cnt = 0;
  image->ptr = 0;
  image->base = 0;
  if( (image->tmpbuf = ibufalloc(image)) == 0 ) {
    i_errhdlr("iopen: error on tmpbuf alloc %d\n",image->xsize);
    return NULL;
  }
  image->x = image->y = image->z = 0;
  image->file = f;
  image->offset = 512L;     /* set up for img_optseek */
  lseek(image->file, 512L, 0);
  return(image);
}

IMushort *ibufalloc( IMAGE *image )
{
  return (IMushort *)malloc(IBUFSIZE(image->xsize));
}

int reverse( IMulong lwrd )
{
  return ((lwrd>>24)    |
          (lwrd>>8 & 0xff00)  |
          (lwrd<<8 & 0xff0000) |
          (lwrd<<24)    );
}

void cvtshorts( IMushort *buffer, int nbytes )
{
  register short i;
  register int nshorts = nbytes>>1;
  register IMushort swrd;

  for(i=0; i<nshorts; i++) {
    swrd = *buffer;
    *buffer++ = (swrd>>8) | (swrd<<8);
  }
}

void cvtlongs( IMulong *buffer, int nbytes )
{
  register short i;
  register int nlongs = nbytes>>2;
  register IMulong lwrd;

  for(i=0; i<nlongs; i++) {
    lwrd = buffer[i];
    buffer[i] =     ((lwrd>>24)     |
                     (lwrd>>8 & 0xff00)   |
                     (lwrd<<8 & 0xff0000)   |
                     (lwrd<<24)     );
  }
}

void cvtimage( IMAGE *buffer )
{
  cvtshorts(&buffer->imagic,12);
  cvtlongs(&buffer->min,12);
  cvtlongs(&buffer->colormap,4);
}

static void (*i_errfunc)();

/*  error handler for the image library.  If the iseterror() routine
    has been called, sprintf's the args into a string and calls the
    error function.  Otherwise calls fprintf with the args and then
    exit.  This allows 'old' programs to assume that no errors
    ever need be worried about, while programs that know how and
    want to can handle the errors themselves.  Olson, 11/88
*/
void i_errhdlr(char *fmt, ...)  /* most args currently used is 2 */
{
  va_list ap;

  va_start(ap, fmt);

  if(i_errfunc) {
    char ebuf[2048];  /* be generous; if an error includes a
                         pathname, the maxlen is 1024, so we shouldn't ever
                         overflow this! */
    vsprintf(ebuf, fmt, ap);
    (*i_errfunc)(ebuf);
  } else {
    vfprintf(stderr, fmt, ap);
  }
  va_end(ap);
  exit(1);
}

/* this function sets the error handler for i_errhdlr */
void i_seterror( void (*func)() )
{
  i_errfunc = func;
}
