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
 *  img_seek, img_write, img_read, img_optseek -
 *
 *        Paul Haeberli - 1984
 *
 */
#include  <stdio.h>
#include  <errno.h>
#include  <unistd.h>
#include  "image.h"

int img_seek( IMAGE *image, int y, int z )
{
  if(img_badrow(image,y,z)) {
    i_errhdlr("imglib: row number out of range\n");
    return EOF;
  }
  image->x = 0;
  image->y = y;
  image->z = z;
  if(ISVERBATIM(image->type)) {
    switch(image->dim) {
    case 1:
      return img_optseek(image, 512L);
    case 2:
      return img_optseek(image,512L+(y*image->xsize)*BPP(image->type));
    case 3:
      return img_optseek(image,
                         512L+(y*image->xsize+z*image->xsize*image->ysize)*
                         BPP(image->type));
    default:
      i_errhdlr("img_seek: wierd dim\n");
      break;
    }
  } else if(ISRLE(image->type)) {
    switch(image->dim) {
    case 1:
      return img_optseek(image, image->rowstart[0]);
    case 2:
      return img_optseek(image, image->rowstart[y]);
    case 3:
      return img_optseek(image, image->rowstart[y+z*image->ysize]);
    default:
      i_errhdlr("img_seek: wierd dim\n");
      break;
    }
  } else
    i_errhdlr("img_seek: wierd image type\n");
  return EOF;
}

int img_badrow( IMAGE *image, int y, int z )
{
  if(y>=image->ysize || z>=image->zsize)
    return 1;
  else
    return 0;
}

int img_write( IMAGE *image, char *buffer, long count )
{
  long retval;

  retval =  write(image->file,buffer,count);
  if(retval == count)
    image->offset += count;
  else
    image->offset = -1;
  return retval;
}

int img_read( IMAGE *image, char *buffer, long count)
{
  long retval;

  retval =  read(image->file,buffer,count);
  if(retval == count)
    image->offset += count;
  else
    image->offset = -1;
  return retval;
}

unsigned long img_optseek( IMAGE *image, unsigned long offset )
{
  if(image->offset != offset) {
    image->offset = offset;
    return lseek(image->file,offset,0);
  }
  return offset;
}
