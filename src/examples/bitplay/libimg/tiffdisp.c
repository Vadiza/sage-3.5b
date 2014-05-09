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
#include "tiffio.h"
#include "imginfo.h"
#include <sys/types.h>


#define rgba(r, g, b, a)  ((a)<<24 | (b)<<16 | (g)<<8 | (r))
#define rgbi(r, g, b)   (0xFF000000 | (b)<<16 | (g)<<8 | (r))

IMG *
TIFFmakedisp( char *fname )
{
  TIFF *tif;
  register IMG *im;
  long width, height;

  tif = TIFFOpen(fname, "r");
  if(tif == NULL)
    return NULL;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  im = (IMG *)malloc(sizeof(IMG));
  im->type = IT_LONG;
  im->rowbytes = width * sizeof(long);
  im->xsize = width;
  im->ysize = height;
  im->data = (unsigned char *)malloc(im->rowbytes * height);
  if(im->data == NULL) {
    fprintf(stderr, "Can't malloc %ld bytes of memory for image\n",
            (long) (im->rowbytes * height));
    exit(2);
  }
  if(TIFFReadRGBAImage(tif, width, height, (uint32 *)im->data, 0) == 0) {
    free(im->data);
    free(im);
    im = NULL;
  }
  TIFFClose(tif);
  return im;
}
