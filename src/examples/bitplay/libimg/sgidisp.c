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

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include "image.h"
#include "imginfo.h"

IMG *
SGImakedisp(fname)
char *fname;
{
  IMAGE *image;
  register IMG *im;
  unsigned short *trow;
  register int k;
  int width, height;
  int y, z;
  static int one = 1;

  int littleendian = (*(char *)&one == 1);

  image = iopen(fname, "r");
  if(image == NULL)
    return NULL;

  width = image->xsize;
  height = image->ysize;
  im = (IMG *)malloc(sizeof(IMG));
  im->rowbytes = sizeof(IMulong) * width;
  im->type = IT_LONG;
  im->xsize = width;
  im->ysize = height;
  im->data = (unsigned char *)malloc(im->rowbytes * height + (sizeof(short)*width));
  if(im->data == NULL) {
    fprintf(stderr, "Can't malloc %d bytes of memory for image\n",
            im->rowbytes * height);
    exit(2);
  }
  trow = (unsigned short *)(im->data + im->rowbytes * height);
  if(image->zsize == 3)
    memset(im->data, 255, im->rowbytes * height);

  for(y = 0; y < image->ysize; y++) {
    for(z = 0; z < image->zsize; z++) {
      register unsigned char *op;
      register unsigned short *ip;


      getrow(image, trow, y, z);
      k = image->xsize;
      op = (unsigned char *) (im->data + im->rowbytes * y)
        + (littleendian ? z : 3 - z);
      ip = trow;
      do {
        *op = *ip++;
        op += sizeof(IMulong);
      } while(--k > 0);
    }
  }
  if(image->zsize == 1) {
    register IMulong *op;
    register int v;

    k = image->xsize * image->ysize;
    op = (IMulong *)im->data;
    do {
      v = (*op & 0xFF); /* XXX byte-order dependent */
      *op++ = v | (v<<8) | (v<<16) | 0xFF000000;
    } while(--k > 0);
  }
  iclose(image);

  return im;
}
