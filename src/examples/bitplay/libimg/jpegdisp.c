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
 * Given a JPEG file, return a DISPIMAGE
 * containing its raster data and with size <= (xsize, ysize).
 */

#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"
#include "imginfo.h"

#ifndef alloca
# include <alloca.h>
#endif

IMG *
jpegmakedisp(char *filename)
{
  FILE *f;
  IMG *di = NULL;
  int xsize, ysize, zsize;
  int x, y;
  struct jpeg_error_mgr jerr[1];
  struct jpeg_decompress_struct jp[1];
  JSAMPARRAY yx;
  JSAMPROW trow;

  f = fopen(filename, "r");
  if(f == NULL) {
    fprintf(stderr, "%s: cannot open: ", filename);
    perror("");
    return NULL;
  }

  jp->err = jpeg_std_error( jerr );
  jpeg_create_decompress( jp );
  jpeg_stdio_src( jp, f );
  if(jpeg_read_header( jp, TRUE ) != JPEG_HEADER_OK)
    goto fail;

  jpeg_start_decompress( jp );


  di = (IMG *)malloc(sizeof(IMG));
  di->xsize = xsize = jp->output_width;
  di->ysize = ysize = jp->output_height;
  zsize = jp->output_components;
  di->data = (unsigned char *)malloc( xsize * ysize * 4 * sizeof(char) );
  di->type = IT_LONG;
  if(di->data == NULL) {
    fprintf(stderr, "Not enough memory for %dx%dx%d image %s!\n",
            xsize, ysize, zsize, filename);
    goto fail;
  }

  yx = (JSAMPARRAY) alloca( jp->rec_outbuf_height * sizeof(JSAMPROW) );
  trow = (JSAMPROW) alloca( zsize * xsize * jp->rec_outbuf_height * sizeof(JSAMPLE) );

  for(y = 0; y < jp->rec_outbuf_height; y++)
    yx[y] = &trow[ y * xsize * zsize ];

  while(jp->output_scanline < ysize) {
    int y, got, nleft;
    JSAMPLE *ip = trow;
    y = jp->output_scanline;
    nleft = ysize - y;
    if(nleft > jp->rec_outbuf_height) nleft = jp->rec_outbuf_height;
    got = jpeg_read_scanlines( jp, yx, nleft );
    if(got <= 0) {
      fprintf(stderr, "%s: trouble reading JPEG row %d of 0..%d (nleft %d)\n", filename, y, ysize-1, nleft);
      break;
    }
    while(--got >= 0) {
      unsigned int *op = (unsigned int *)&di->data[ (ysize-1 - y) * xsize * 4 ];
      x = xsize;
      switch(zsize) {
      case 1:
        do {
          *op++ = (0xFF<<24) | (*ip++ * 0x010101);
        } while(--x > 0);
        break;
      case 3:
        do {
          *op++ = (0xFF<<24) | (ip[2]<<16) | (ip[1]<<8) | (ip[0]);
          ip += 3;
        } while(--x > 0);
        break;
      }
      y++;
    }
  }
  jpeg_finish_decompress( jp );
  jpeg_destroy( (j_common_ptr) jp );
  fclose(f);
  return di;

  fail:
  jpeg_destroy( (j_common_ptr) jp );
  if(di) {
    if(di->data) free(di->data);
    free(di);
  }
  if(f != NULL)
    fclose(f);
  return NULL;
}
