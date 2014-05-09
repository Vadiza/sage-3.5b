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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "softpic.h"
#include "imginfo.h"

#define TRUE  1
#define FALSE 0

#define DEBUG FALSE

static int xsize, ysize, nbits[10], file_chained[10], fields[10], file_type[10], ndsc;
FILE *in_fle;
float m, n;
char *liner, *lineg, *lineb, *linea;

static int check_file_header(fle)
  FILE *fle;
{
  struct softhdr hd;

  fread(&hd, sizeof(struct softhdr), 1, fle);
  if (hd.id != SOFT_MAGIC)
    return FALSE;
  if (DEBUG) {
    fprintf(stderr, "creator version: %f\n", hd.version);
    fprintf(stderr, "internal comment: %s\n", hd.comment);
  }
  return TRUE;
}

static int check_pic_header(fle)
  FILE *fle;
{
  struct softpict hd;

  fread(&hd, sizeof(struct softpict), 1, fle);
  if (hd.id != SPICT_MAGIC)
    return FALSE;
  if (DEBUG) {
    fprintf(stderr, "picture dimensions: %d X %d.\n", hd.width, hd.height);
    fprintf(stderr, "picture pixel ratio (X/Y): %f.\n", hd.ratio);
    fprintf(stderr, "picture fields: %s.\n", (hd.fields == 1)?"odd":(hd.fields == 2)?"even":"full-frame");
  }
  xsize = hd.width;
  ysize = hd.height;
  liner = (char *)malloc(xsize * sizeof(char));
  lineg = (char *)malloc(xsize * sizeof(char));
  lineb = (char *)malloc(xsize * sizeof(char));
  linea = (char *)malloc(xsize * sizeof(char));
  bzero(liner, xsize*sizeof(char));
  bzero(lineg, xsize*sizeof(char));
  bzero(lineb, xsize*sizeof(char));
  bzero(linea, xsize*sizeof(char));
  return TRUE;
}

static void get_pack_chunk( FILE *fle, int n)
{
  struct softpack hd;

  fread(&hd, sizeof(struct softpack), 1, fle);
  file_chained[n] = hd.chained;
  nbits[n] = hd.nbits;
  file_type[n] = hd.type;
  fields[n] = hd.fields;
  if (DEBUG) {
    fprintf(stderr, "pack info #%d:\n", n);
    fprintf(stderr, "\tnbits = %d\n", nbits[n]);
    fprintf(stderr, "\ttype = %x\n", file_type[n]);
    fprintf(stderr, "\tfields = %x\n", fields[n]);
  }
  ndsc = n;
  if (nbits[n] != 8) {
    fprintf(stderr, "Error: This program only handles 8 bit data.\n");
    exit(1);
  }
  if (file_chained[n] != 0) get_pack_chunk(fle, (n+1));
}

static void mixed_read_rle_part( FILE *fle, short cnt, int lne, int strt, int fld )
{
  register int i;
  register int mask;
  char r, g, b, a;

  mask = fields[fld];
  if ((mask & SPICT_R) != 0)
    r = getc(fle);
  if ((mask & SPICT_G) != 0)
    g = getc(fle);
  if ((mask & SPICT_B) != 0)
    b = getc(fle);
  if ((mask & SPICT_A) != 0)
    a = getc(fle);
  for (i = strt; i<=strt+cnt; i++) {
    if ((mask & SPICT_R) != 0)
      liner[i] = r;
    if ((mask & SPICT_G) != 0)
      lineg[i] = g;
    if ((mask & SPICT_B) != 0)
      lineb[i] = b;
    if ((mask & SPICT_A) != 0)
      linea[i] = a;
  }
}

static void mixed_read_norm_part( FILE *fle, short cnt, int lne, int strt, int fld )
{
  register int i;

  for (i = strt; i<=strt+cnt; i++) {
    if ((fields[fld] & SPICT_R) != 0)
      liner[i] = getc(fle);
    if ((fields[fld] & SPICT_G) != 0)
      lineg[i] = getc(fle);
    if ((fields[fld] & SPICT_B) != 0)
      lineb[i] = getc(fle);
    if ((fields[fld] & SPICT_A) != 0)
      linea[i] = getc(fle);
  }
}

static void mixed_read_line( FILE *fle, int lne, int fld )
{
  int x = 0;
  short size;
  int cnt;

  do {
    cnt = getc(fle);
    if (cnt == 0x80) {
      size = getc(fle) << 8;
      size |= getc(fle);
      size--;
    } else
      size = cnt & 0x7f;
    if ((cnt & 0x80) == 0)
      mixed_read_norm_part(fle, size, lne, x, fld);
    else
      mixed_read_rle_part(fle, size, lne, x, fld);
    x += size + 1;
  } while (x < xsize);
}

static void output_line( int lne, IMG *im )
{
  int i;
  register char *l;
  register char *r, *g, *b, *a;

  l = (char *)&im->data[(ysize - lne - 1)*im->rowbytes];
  a = linea; r = liner; g = lineg; b = lineb;
  i = xsize;
  do {
    *l++ = *a++;
    *l++ = *b++;
    *l++ = *g++;
    *l++ = *r++;
  } while(--i > 0);
}

static void load_file( FILE *fle, IMG *im )
{
  int y, fcnt;

  im->xsize = xsize;
  im->ysize = ysize;
  im->rowbytes = xsize*sizeof(long);
  im->data = malloc(ysize * im->rowbytes);
  for (y = 0; y < ysize; y++) {
    for (fcnt = 0; fcnt <= ndsc; fcnt++) {
      if (file_type[fcnt] == SPICT_MIXEDRLE) {
        mixed_read_line(fle, y, fcnt);
      } else {
        fprintf(stderr, "don't know pack type 0x%x\n", file_type[fcnt]);
        exit(1);
      }
    }
    output_line(y, im);
  }
}

static void close_files( FILE *in )
{
  fclose(in);
  free(liner);
  free(lineg);
  free(lineb);
  free(linea);
}

IMG *softmakedisp(name)
char *name;
{
  IMG *out;

  in_fle = fopen(name, "rb");
  if(in_fle == NULL)
    return NULL;
  if (!check_file_header(in_fle)) {
    fprintf(stderr, "ERROR in file header! The world has already come to an end!\n");
    return NULL;
  }
  if (!check_pic_header(in_fle)) {
    fprintf(stderr, "ERROR in picure header! The world will soon come to an end.\n");
    return NULL;
  }
  out = (IMG *)malloc(sizeof(IMG));
  out->type = IT_LONG;
  get_pack_chunk(in_fle, 0);
  load_file(in_fle, out);
  close_files(in_fle);
  return out;
}
