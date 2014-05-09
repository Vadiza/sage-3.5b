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
 * Given a Sun raster file, return an IMG containing its raster data.
 */

#include <stdio.h>
#include <stdlib.h>
#include "rasterfile.h"
#include "imginfo.h"

IMG *
rasmakedisp(filename)
char *filename;
{
  FILE *inf;
  struct rasterfile ras;
  IMG *im = NULL;
  int xsize, ysize;
  register long *rp;
  unsigned char *rowbuf = NULL;
  long cmap[256];

  inf = fopen(filename, "r");
  if(inf == NULL) {
    fprintf(stderr, "%s: cannot open: ", filename);
    perror("");
    return NULL;
  }

  if(fread(&ras, sizeof(ras), 1, inf) <= 0 || ras.ras_magic != RAS_MAGIC)
    goto fail;

  xsize = ras.ras_width;
  ysize = ras.ras_height;

  im = (IMG *)malloc(sizeof(IMG));
  im->rowbytes = xsize * sizeof(long);
  im->type = IT_LONG;
  im->xsize = xsize;
  im->ysize = ysize;
  im->data = (unsigned char *)malloc(im->rowbytes * ysize);
  if(im->data == NULL) {
    fprintf(stderr, "Can't malloc %ld bytes of memory for image\n",
            sizeof(long) * xsize * ysize);
    exit(2);
  }
  rp = (long *)im->data;

  switch(ras.ras_maptype) {
    unsigned char map[768];
    register int i;

  case RMT_EQUAL_RGB:
    if(ras.ras_maplength != 3*256) {
      fprintf(stderr, "%s: can't handle colormap with %d != 256 entries",
              filename, ras.ras_maplength / 3);
      goto fail;
    }
    if(fread(map, sizeof(map), 1, inf) <= 0) {
      fprintf(stderr, "%s: premature EOF reading colormap", filename);
      goto fail;
    }
    for(i = 0; i < 256; i++)
      cmap[i] = map[i] | (map[i+256] << 8) | (map[i+512] << 16);
    break;

  case RMT_NONE:
    if(ras.ras_depth == 8) {
      for(i = 0; i < 256; i++)
        cmap[i] = i * 0x010101;
    }
    break;

  default:
    fprintf(stderr, "%s: can't handle Sun colormap type %d\n",
            filename, ras.ras_maptype);
    goto fail;
  }

  if(ras.ras_type == RT_BYTE_ENCODED) {
    if(ras.ras_depth != 8) {
      fprintf(stderr, "%s: can't handle byte-encoded %d-bit images\n",
              filename, ras.ras_depth);
      goto fail;
    }
    rowbuf = (unsigned char *)malloc(ras.ras_width*sizeof(unsigned char *));
  }

  if(ras.ras_width > 0) {
    register int i, c;
    int row;

    for(row = 0; row < ras.ras_height; row++) {
      rp = (long *)(im->data + im->rowbytes*(ysize-1 - row));
      i = xsize;
      switch(ras.ras_depth) {
      case 1:
        while((c = getc(inf)) != EOF && i >= 8) {
          *rp++ = (c&0x80) ? 0xFFFFFF : 0;
          *rp++ = (c&0x40) ? 0xFFFFFF : 0;
          *rp++ = (c&0x20) ? 0xFFFFFF : 0;
          *rp++ = (c&0x10) ? 0xFFFFFF : 0;
          *rp++ = (c&0x08) ? 0xFFFFFF : 0;
          *rp++ = (c&0x04) ? 0xFFFFFF : 0;
          *rp++ = (c&0x02) ? 0xFFFFFF : 0;
          *rp++ = (c&0x01) ? 0xFFFFFF : 0;
        }
        if(c != EOF) {
          while(--i >= 0) {
            *rp++ = (c&0x80) ? 0xFFFFFF : 0;
            c <<= 1;
          }
        }
        break;

      case 24:
        do {
          c = getc(inf);
          c |= getc(inf) << 8;
          *rp++ = c | getc(inf) << 16;
        } while(--i > 0);
        break;

      case 8:
        if(ras.ras_type == RT_BYTE_ENCODED) {
          register unsigned char *rowp = rowbuf;
          int nleft = ras.ras_width;
          int seen = 0;

          do {
            seen++;
            c = getc(inf);
            if(c == 128) {
              seen++;
              c = getc(inf);
              if(c == 0) {
                *rowp++ = 128;
              } else {
                seen++;
                nleft -= c;
                i = getc(inf);
                do { *rowp++ = i; } while(--c >= 0);
              }
            } else {
              *rowp++ = c;
            }
          } while(--nleft > 0);
          if(seen&1)
            (void) getc(inf);
          /*
           * Copy uncompressed data.
           */
          i = xsize;
          rowp = rowbuf;
          do { *rp++ = cmap[*rowp++]; } while(--i > 0);

        } else {

          do {
            *rp++ = cmap[getc(inf)];
          } while(--i > 0);
        }
        break;
      }

      if(feof(inf)) {
        fprintf(stderr, "%s: premature EOF reading row %d of %d\n",
                filename, row, ras.ras_height);
        break;
      }

      if(ras.ras_type != RT_BYTE_ENCODED) {
        i = (((ras.ras_width*ras.ras_depth + 15) & ~15) >> 3) - xsize;
        while(--i >= 0)
          (void) getc(inf);
      }
    }
  }
  if(rowbuf != NULL)
    free(rowbuf);
  if(inf != NULL)
    fclose(inf);
  return im;

  fail:
  if(im) {
    if(im->data) free(im->data);
    free(im);
  }
  if(inf != NULL)
    fclose(inf);
  return NULL;
}
