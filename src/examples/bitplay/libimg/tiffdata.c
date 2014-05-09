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

#ifndef lint
static char sccsid[] = "@(#)sgigt.c 1.8 4/25/89";
#endif

#include <stdio.h>
#include <gl.h>
#include <gl/device.h>
#include "tiffio.h"
#include "imginfo.h"

static u_long *raster;      /* displayable image */
static u_short  bitspersample;
static u_short  samplesperpixel;
static u_short  photometric;
static u_short  orientation;
static u_short  *redcmap, *greencmap, *bluecmap;/* colormap for palette images */

#define rgba(r, g, b, a)  ((a)<<24 | (b)<<16 | (g)<<8 | (r))
#define rgbi(r, g, b)   (0xFF000000 | (b)<<16 | (g)<<8 | (r))

IMG *
TIFFmakedisp(fname)
char *fname;
{
  TIFF *tif;
  short width, height;
  register IMG *im;

  tif = TIFFOpen(fname, "r");
  if(tif == NULL)
    return NULL;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  im = (IMG *)malloc(sizeof(IMG));
  im->type = IT_LONG;
  im->rowbytes = sizeof(long) * width;
  im->xsize = width;
  im->ysize = height;
  im->data = (unsigned char *)malloc(im->rowbytes * height);
  if(im->data == NULL) {
    fprintf(stderr, "Can't malloc %d bytes of memory for image\n",
            im->rowbytes * height);
    exit(2);
  }
  if(tiffgt(tif, width, height, (u_long *)im->data) == 0) {
    free(im->data);
    free(im);
    im = NULL;
  }
  TIFFClose(tif);
  return im;
}

RGBvalue **BWmap;

static
tiffgt(tif, w, h, raster)
  TIFF *tif;
int w, h;
u_long *raster;
{
  u_short minsamplevalue, maxsamplevalue, planarconfig;
  RGBvalue *Map;
  int e;

  if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric)) {
    switch (samplesperpixel) {
    case 1:
      photometric = PHOTOMETRIC_MINISBLACK;
      break;
    case 3: case 4:
      photometric = PHOTOMETRIC_RGB;
      break;
    default:
      fprintf(stderr, "Missing needed \"%s\" tag.\n",
              "PhotometricInterpretation");
      return (0);
    }
    printf("No \"PhotometricInterpretation\" tag, assuming %s.\n",
           photometric == PHOTOMETRIC_RGB ? "RGB" : "min-is-black");
  }
  if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel))
    samplesperpixel = 1;
  if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample))
    bitspersample = 8;
  if (!TIFFGetField(tif, TIFFTAG_MINSAMPLEVALUE, &minsamplevalue))
    minsamplevalue = 0;
  if (!TIFFGetField(tif, TIFFTAG_MAXSAMPLEVALUE, &maxsamplevalue))
    maxsamplevalue = (1<<bitspersample)-1;
  Map = NULL;
  switch (photometric) {
  case PHOTOMETRIC_RGB:
  case PHOTOMETRIC_MINISBLACK:
    if (minsamplevalue == 0 && maxsamplevalue == 255)
      break;
    /* fall thru... */
  case PHOTOMETRIC_MINISWHITE: {
    register int x, range;

    range = maxsamplevalue - minsamplevalue + 1;
    Map = (RGBvalue *)malloc(range * sizeof (RGBvalue));
    if (Map == NULL) {
      fprintf(stderr,
              "No space for photometric conversion table.\n");
      return (0);
    }
    if (photometric == PHOTOMETRIC_MINISWHITE) {
      for (x = 0; x < range; x++)
        Map[x] = ((range - x) * 255) / range;
    } else {
      for (x = 0; x < range; x++)
        Map[x] = (x * 255) / range;
    }
    if (bitspersample < 8 && photometric != PHOTOMETRIC_RGB) {
      if (!makebwmap(Map))
        return (0);
      /* no longer need Map, free it */
      free((char *)Map);
      Map = NULL;
    }
    break;
  }
  case PHOTOMETRIC_PALETTE:
    if (!TIFFGetField(tif, TIFFTAG_COLORMAP,
                      &redcmap, &greencmap, &bluecmap)) {
      fprintf(stderr, "Missing required \"Colormap\" tag.\n");
      return (0);
    }
    break;
  }
  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
  if (planarconfig == PLANARCONFIG_SEPARATE)
    e = gtseparate(tif, raster, Map, h, w);
  else
    e = gtcontig(tif, raster, Map, h, w);
  if (Map)
    free((char *)Map);
  return (e);
}

setorientation(tif, h)
TIFF *tif;
int h;
{
  int y;

  if (!TIFFGetField(tif, TIFFTAG_ORIENTATION, &orientation))
    orientation = ORIENTATION_TOPLEFT;
  switch (orientation) {
  case ORIENTATION_BOTRIGHT:
  case ORIENTATION_RIGHTBOT:  /* XXX */
  case ORIENTATION_LEFTBOT: /* XXX */
    printf("Warning, using bottom-left orientation.\n");
    orientation = ORIENTATION_BOTLEFT;
    /* fall thru... */
  case ORIENTATION_BOTLEFT:
    y = 0;
    break;
  case ORIENTATION_TOPRIGHT:
  case ORIENTATION_RIGHTTOP:  /* XXX */
  case ORIENTATION_LEFTTOP: /* XXX */
    printf("Warning, using top-left orientation.\n");
    orientation = ORIENTATION_TOPLEFT;
    /* fall thru... */
  case ORIENTATION_TOPLEFT:
    y = h-1;
    break;
  }
  return (y);
}

gtcontig(tif, raster, Map, h, w)
TIFF *tif;
u_long *raster;
register RGBvalue *Map;
int h, w;
{
  register u_char *pp;
  register u_long *cp;
  register int x;
  int scanline, row, y;
  u_char *buf;

  buf = (u_char *)malloc(TIFFScanlineSize(tif));
  if (buf == 0) {
    fprintf(stderr, "No space for scanline buffer\n");
    return (0);
  }
  y = setorientation(tif, h);
  for (row = 0; row < h; row++) {
    if (TIFFReadScanline(tif, buf, row, 0) < 0)
      break;
    pp = buf;
    cp = raster + y*w;
    switch (photometric) {
    case PHOTOMETRIC_RGB:
    rgb:
      switch (bitspersample) {
      case 8:
        if (Map) {
          for (x = w; x-- > 0;) {
            pp[0] = Map[pp[0]];
            pp[1] = Map[pp[1]];
            pp[2] = Map[pp[2]];
            pp += samplesperpixel;
          }
          pp = buf;
        }
        if(samplesperpixel == 4) {
          for(x = w; --x >= 0; pp += 4)
            *cp++ = rgba(pp[0], pp[1], pp[2],pp[3]);
        } else {
          for (x = w; x-- > 0; pp += samplesperpixel)
            *cp++ = rgbi(pp[0], pp[1], pp[2]);
        }
        break;
      case 16: {
        register u_short *wp;

        if (Map) {
          wp = (u_short *)pp;
          for (x = w; x-- > 0;) {
            wp[0] = Map[wp[0]];
            wp[1] = Map[wp[1]];
            wp[2] = Map[wp[2]];
            wp += samplesperpixel;
          }
        }
        wp = (u_short *)pp;
        for (x = w; x-- > 0;) {
          *cp++ = rgbi(wp[0], wp[1], wp[2]);
          wp += samplesperpixel;
        }
        break;
      }
      }
      break;
    case PHOTOMETRIC_PALETTE:
      for (x = w; x-- > 0;) {
        RGBvalue c = *pp++;
        *cp++ = rgbi(redcmap[c],
                     greencmap[c], bluecmap[c]);
      }
      break;
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
      if(samplesperpixel > 1)
        goto rgb;

      if (bitspersample == 8) {
        register RGBvalue c;

        for (x = w; x-- > 0;) {
          c = Map[*pp++];
          *cp++ = rgbi(c, c, c);
        }
      } else
        gtbw(bitspersample, w, cp, pp);
      break;
    }
    y += (orientation == ORIENTATION_TOPLEFT ? -1 : 1);
  }
  return (1);
}

gtseparate(tif, raster, Map, h, w)
TIFF *tif;
u_long *raster;
register RGBvalue *Map;
int h, w;
{
  register u_long *cp;
  register int x;
  u_char *red;
  int scanline, row, y;

  scanline = TIFFScanlineSize(tif);
  switch (samplesperpixel) {
  case 1:
    red = (u_char *)malloc(scanline);
    break;
  case 3: case 4:
    red = (u_char *)malloc(4*scanline);
    break;
  }
  y = setorientation(tif, h);
  for (row = 0; row < h; row++) {
    cp = raster + y*w;
    if (TIFFReadScanline(tif, red, row, 0) < 0)
      break;
    switch (photometric) {
    case PHOTOMETRIC_RGB: {
      register u_char *r, *g, *b, *a;

      r = red;
      if (TIFFReadScanline(tif, g = r + scanline, row, 1) < 0)
        break;
      if (TIFFReadScanline(tif, b = g + scanline, row, 2) < 0)
        break;
      if(samplesperpixel == 4) {
        a = b + scanline;
        if(TIFFReadScanline(tif, a, row, 3) < 0)
          break;
      }
      switch (bitspersample) {
      case 8:
        if(samplesperpixel == 4) {
          for(x = w; --x >= 0; )
            *cp++ = rgba(*r++, *g++, *b++, *a++);
        } else {
          for (x = w; x-- > 0;)
            *cp++ = rgbi(*r++, *g++, *b++);
        }
        break;
      case 16:
#define wp(x) ((u_short *)(x))
        for (x = 0; x < w; x++)
          *cp++ = rgbi(
                       Map[wp(r)[x]],
                       Map[wp(g)[x]],
                       Map[wp(b)[x]]);
        break;
#undef  wp
      }
      break;
    }
    case PHOTOMETRIC_PALETTE: {
      register u_char *pp = red;
      for (x = w; x-- > 0;) {
        RGBvalue c = *pp++;
        *cp++ = rgbi(redcmap[c],
                     greencmap[c], bluecmap[c]);
      }
      break;
    }
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
      if (bitspersample == 8) {
        register u_short *pp = (u_short *)red;
        register RGBvalue c;

        for (x = w; x-- > 0;) {
          c = Map[*pp++];
          *cp++ = rgbi(c, c, c);
        }
      } else
        gtbw(bitspersample, w, Map, cp, red);
      break;
    }
    y += (orientation == ORIENTATION_TOPLEFT ? -1 : 1);
  }
  if (red)
    free(red);
  return (1);
}

/*
 * Greyscale images with less than 8 bits/sample are handled
 * with a table to avoid lots of masks.  The table
 * is setup so that gtbw (below) can retrieve 8/bitspersample
 * pixel values simply by indexing into the table with one
 * number.
 */
makebwmap(Map)
RGBvalue *Map;
{
  register int i;
  int nsamples = 8 / bitspersample;
  register RGBvalue *p;

  BWmap = (RGBvalue **)malloc(
                              256*sizeof (RGBvalue *)+(256*nsamples*sizeof(RGBvalue)));
  if (BWmap == NULL) {
    fprintf(stderr, "No space for B&W mapping table.\n");
    return (0);
  }
  p = (RGBvalue *)(BWmap + 256);
  for (i = 0; i < 256; i++) {
    BWmap[i] = p;
    switch (bitspersample) {
    case 1:
      *p++ = Map[i>>7];
      *p++ = Map[(i>>6)&1];
      *p++ = Map[(i>>5)&1];
      *p++ = Map[(i>>4)&1];
      *p++ = Map[(i>>3)&1];
      *p++ = Map[(i>>2)&1];
      *p++ = Map[(i>>1)&1];
      *p++ = Map[i&1];
      break;
    case 2:
      *p++ = Map[i>>6];
      *p++ = Map[(i>>4)&3];
      *p++ = Map[(i>>2)&3];
      *p++ = Map[i&3];
      break;
    case 4:
      *p++ = Map[i>>4];
      *p++ = Map[i&0xf];
      break;
    }
  }
  return (1);
}

#define REPEAT8(op) REPEAT4(op); REPEAT4(op)
#define REPEAT4(op) REPEAT2(op); REPEAT2(op)
#define REPEAT2(op) op; op

gtbw(bitspersample, w, cp, pp)
int bitspersample, w;
register u_long *cp;
register u_char *pp;
{
  register RGBvalue c, *bw;
  register int x;

  switch (bitspersample) {
  case 1:
    for (x = w; x > 0; x -= 8) {
      bw = BWmap[*pp++];
      REPEAT8(c = *bw++; *cp++ = rgbi(c, c, c));
    }
    break;
  case 2:
    for (x = w; x > 0; x -= 4) {
      bw = BWmap[*pp++];
      REPEAT4(c = *bw++; *cp++ = rgbi(c, c, c));
    }
    break;
  case 4:
    for (x = w; x > 0; x -= 2) {
      bw = BWmap[*pp++];
      REPEAT2(c = *bw++; *cp++ = rgbi(c, c, c));
    }
    break;
  }
}
