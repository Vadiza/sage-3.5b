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
 * Given a Sun raster file, return a DISPIMAGE
 * containing its raster data and with size <= (xsize, ysize).
 */

#include <stdio.h>
#include <stdlib.h>
/* #include "rasterfile.h" */
#include "imginfo.h"

extern int pnm_getint( FILE * );

IMG *
pnmmakedisp(filename)
char *filename;
{
  register FILE *f;
  IMG *di = NULL;
  int maxpix = 0, pnmkind;
  int xsize, ysize;
  int y;
  int c;

  f = fopen(filename, "r");
  if(f == NULL) {
    fprintf(stderr, "%s: cannot open: ", filename);
    perror("");
    return NULL;
  }

  if(fgetc(f) != 'P')
    goto fail;

  pnmkind = fgetc(f);

  xsize = pnm_getint(f);
  ysize = pnm_getint(f);
  switch(pnmkind) {
  case '1': case '4':
    maxpix = 1;
    break;
  case '2': case '5': /* fall into ... */
  case '3': case '6':
    maxpix = pnm_getint(f);
    break;
  default:
    goto fail;
  }
  if(xsize <= 0 || ysize <= 0 || maxpix <= 0)
    goto fail;

  if(pnmkind >= '4') {
    /* Skip to end of line, after which the binary data should follow */
    while((c = fgetc(f)) != EOF && c != '\n')
      ;
  }

  di = (IMG *)malloc(sizeof(IMG));
  if(pnmkind == '1' || pnmkind == '4') {
    di->type = IT_BIT;
    di->rowbytes = (xsize+31)/32 * 4;
  } else {
    di->type = IT_LONG;
    di->rowbytes = xsize*4;
  }
  di->xsize = xsize;
  di->ysize = ysize;
  di->data = malloc(di->rowbytes * ysize);
  if(di->data == NULL) {
    fprintf(stderr, "Can't malloc %d bytes of memory for image\n",
            di->rowbytes*ysize);
    exit(2);
  }
  for(y = ysize; --y >= 0; ) {
    register int v;
    register long *rp;
    register int x;

    rp = (long *)(di->data + y*di->rowbytes);
    x = xsize;
    switch(pnmkind) {
    case '1':
      do {
        *rp++ = 0xff000000 | -pnm_getint(f);
      } while(--x > 0);
      break;
    case '2':
      do {
        v = (255 * pnm_getint(f) / maxpix);
        *rp++ = 0xff000000 | (v << 16) | (v<<8) | v;
      } while(--x > 0);
      break;
    case '3':
      do {
        v = (255 * pnm_getint(f) / maxpix);
        v |= (255 * pnm_getint(f) / maxpix) << 8;
        *rp++ = v | (255 * pnm_getint(f) / maxpix) << 16;
      } while(--x > 0);
      break;
    case '4':
      fread(rp, (xsize+7)/8, 1, f); /* Bit mode */
      break;
    case '5':
      if(maxpix != 255) {
        do {
          v = 255 * getc(f) / maxpix;
          *rp++ = 0xff000000 | (v << 16) | (v << 8) | v;
        } while(--x > 0);
      } else {
        do {
          v = getc(f);
          *rp++ = 0xff000000 | (v << 16) | (v << 8) | v;
        } while(--x > 0);
      }
      break;
    case '6':
      if(maxpix != 255) {
        do {
          v = 255 * getc(f) / maxpix;
          v |= (255 * getc(f) / maxpix) << 8;
          *rp++ = 0xff000000 | ((255 * getc(f) / maxpix) << 16) | v;
        } while(--x > 0);
      } else {
        do {
          v = getc(f);
          v |= getc(f) << 8;
          *rp++ = 0xff000000 | (getc(f) << 16) | v;
        } while(--x > 0);
      }
      break;
    }
  }

  if(f != NULL)
    fclose(f);
  return di;

  fail:
  if(di) {
    if(di->data) free(di->data);
    free(di);
  }
  if(f != NULL)
    fclose(f);
  return NULL;
}
