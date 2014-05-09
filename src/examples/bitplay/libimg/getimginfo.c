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
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>

#include "imginfo.h"

#include "image.h"
#include "tiffio.h"
#include "rasterfile.h"
#include "softpic.h"

/*
 * Generic image-information routine.
 * Given a file name, fills in an "imginfo" structure,
 * and returns the type of image.
 */


char *imgtypename[] = {
  "unknown", "tiff", "sgi", "ras", "soft", "gif", "pnm", "raw", "jpeg", NULL
};

#define MAXMAGIC  6

struct imgdesc {
  short magiclen; /* Bytes in magic number.  -1 terminates known list. */
  char magic[MAXMAGIC]; /* magic number */
  int  (*getinfo)(); /* getinfo(filename) loads our global vars */
};

static int tiffinfo(), sgiinfo(), rasinfo(), softinfo(), gifinfo(),
  pnminfo(), rawinfo(), jpeginfo();

static struct imgdesc known[] = {
  { 2, { 'M','M' }, tiffinfo },   /* Big-endian Tiff */
  { 2, { 'I','I' }, tiffinfo },   /* Little-endian */
  { 2, { 0x01,0xDA }, sgiinfo },    /* SGI */
  { 4, { 0x59,0xA6,0x6A,0x95 }, rasinfo }, /* Sun */
  { 4, { 0x53,0x80,0xF6,0x34 }, softinfo }, /* SoftImage */
  { 5, { 'G','I','F','8','7' }, gifinfo }, /* GIF */
  { 5, { 'G','I','F','8','9' }, gifinfo }, /* GIF */
  { 4, { 0xff, 0xd8, 0xff, 0xe0 }, jpeginfo }, /* JPEG */
  { 1, { 'P' }, pnminfo },    /* PBM/PGM/PPM */
  { 0, { 0 }, rawinfo },      /* raw + imginfo header */
  { -1, { 0 }, NULL }
};

extern int pnm_getint( FILE * );

int
getimginfo(fname, info)
  char *fname;
struct imginfo *info;
{
  register struct imgdesc *p;
  int fd;
  struct imginfo dummy;
  char buf[MAXMAGIC];

  if(info == NULL)
    info = &dummy;

  memset(info, 0, sizeof(struct imginfo));  /* Zero all fields */
  if((fd = open(fname, 0)) < 0) {
    fprintf(stderr, "%s: ", fname);
    fflush(stderr);
    perror("cannot open");
    return I_UNKNOWN;
  }
  memset(buf, -1, MAXMAGIC);
  (void) read(fd, buf, MAXMAGIC);
  for(p = known; ; p++) {
    if(p->magiclen < 0 ||
       ((p->magiclen == 0 || memcmp(p->magic, buf, p->magiclen) == 0)
        && (*p->getinfo)(info, fname, fd) == 0))
      break;
  }
  close(fd);
  return info->kind;
}

/*
 * Type-specific interpreters
 */

static int
softinfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  struct softpict pic;
  struct softpack packet;
  int fields = 0;

  lseek(fd, sizeof(struct softhdr), 0);
  if(read(fd, &pic, sizeof(struct softpict)) != sizeof(struct softpict) ||
     pic.id != SPICT_MAGIC)
    return -1;
  info->xsize = pic.width;
  info->ysize = pic.height;
  do {
    if(read(fd, &packet, sizeof(struct softpack)) != sizeof(struct softpack))
      return -1;
    packet.fields &= (SPICT_R|SPICT_G|SPICT_B|SPICT_A);
    if(packet.fields) {
      info->sampbits = packet.nbits;
      fields |= packet.fields;
    }
  } while(packet.chained);

  for(info->nsamp = 0; fields != 0; fields >>= 1)
    info->nsamp += (fields & 1);

  info->mapsize = info->mapsamp = 0;
  info->kind = I_SOFT;
  return 0;
}

/* big-endian to native conversions */

static uint16 betohs( uint16 bes )
{
  static int one = 1;
  return (*(char *)&one == 0)
    ? bes
    : ((bes>>8)&0xFF) | (bes&0xFF)<<8;
}

static int
sgiinfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  IMAGE imghdr;

  (void) lseek(fd, 0L, 0);
  (void) read(fd, &imghdr, sizeof(imghdr));

  info->xsize = betohs( imghdr.xsize );
  info->ysize = betohs( imghdr.ysize );
  info->nsamp = betohs( imghdr.zsize );
  info->sampbits = 8;
  info->mapsize = info->mapsamp = 0;
  info->kind = I_SGI;
  return 0;
}

static int
rasinfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  struct rasterfile ras;

  lseek(fd, 0L, 0);
  if(read(fd, &ras, sizeof(ras)) != sizeof(ras))
    return -1;
  info->xsize = ras.ras_width;
  info->ysize = ras.ras_height;
  info->sampbits = ras.ras_depth == 24 ? 8 : ras.ras_depth;
  if(ras.ras_maptype != RMT_NONE) {
    info->mapsize = ras.ras_maplength / 3;
    info->mapsamp = 3;
    info->nsamp = 1;
  } else {
    info->mapsize = info->mapsamp = 0;
    info->nsamp = (ras.ras_depth == 24) ? 3 : 1;
  }
  info->kind = I_SUNRAS;
  return 0;
}


static int
tiffinfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  TIFF *tif;
  uint16 photometric;
  uint16 bits = 8;
  uint16 nsamp = 1;
  uint32 xsize, ysize;

  tif = TIFFOpen(fname, "r");
  if(tif == NULL)
    return -1;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &xsize);
  info->xsize = xsize;
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &ysize);
  info->ysize = ysize;
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits);
  info->sampbits = bits;
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nsamp);
  info->nsamp = nsamp;
  if(TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric) &&
     photometric == PHOTOMETRIC_PALETTE) {
    info->mapsize = 1 << info->sampbits;
    info->mapsamp = 3;
  } else {
    info->mapsize = info->mapsamp = 0;
  }
  info->kind = I_TIFF;
  TIFFClose(tif);
  return 0;
}

static int
gifinfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  unsigned char tmp[10], ch, a, b;

  lseek(fd, 0L, 0);
  read(fd, tmp, 6);       /* skip magic # */
  if(tmp[5] != 'a' && tmp[5] != 'A')    /* "GIF87A" or "GIF87a" */
    return -1;
  read(fd, &a, 1);
  read(fd, &b, 1);
  info->xsize = a + 0x100 * b;
  read(fd, &a, 1);
  read(fd, &b, 1);
  info->ysize = a + 0x100 * b;
  read(fd, &ch, 1);
  info->sampbits = (ch & 7) + 1;
  info->mapsize = 1 << info->sampbits;
  info->mapsamp = 3;
  info->nsamp = 1;
  info->kind = I_GIF;
  return 0;
}

#include <jpeglib.h>

static int
jpeginfo( struct imginfo *info, char *fname, int fd )
{
  struct jpeg_decompress_struct jp[1];
  struct jpeg_error_mgr jerr[1];
  FILE *jf = fdopen( fd, "r" );
  int fail = -1;

  if(jf == NULL)
    return -1;
  lseek( fd, 0, 0 );
  jp->err = jpeg_std_error( jerr );
  jpeg_create_decompress( jp );
  jpeg_stdio_src( jp, jf );
  if(jpeg_read_header( jp, TRUE ) != JPEG_HEADER_OK)
    goto done;

  jpeg_calc_output_dimensions( jp );
  info->xsize = jp->output_width;
  info->ysize = jp->output_height;
  info->nsamp = jp->output_components;
  info->mapsize = 0;
  info->mapsamp = 0;
  info->sampbits = 8;
  info->kind = I_JPEG;
  fail = 0;
  done:
  jpeg_destroy( (j_common_ptr)jp );
  fclose(jf);
  return fail;
}

static int
rawinfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  char buf[256];
  register int i, n;

  for(n = sizeof(buf)-1; (i = read(fd, buf+sizeof(buf)-1-n, n)) > 0; n -= i)
    ;

  n = sizeof(buf)-1 - n;
  if(n <= 0)
    return -1;
  buf[n] = '\0';
  if(sscanf(buf, "%*s %d %d %d %d %d %d",
            &info->xsize, &info->ysize, &info->nsamp, &info->mapsize,
            &info->mapsamp, &info->sampbits) == 6) {
    info->kind = I_RAW;
    return 0;
  }
  return -1;
}

static int
pnminfo(info, fname, fd)
  register struct imginfo *info;
char *fname;
int fd;
{
  FILE *f = fdopen(fd, "r");
  int c;

  fseek(f, 1L, 0);
  c = fgetc(f);
  info->mapsize = info->mapsamp = 0;
  switch(c) {
  case '1': case '4':
    info->sampbits = 1;
    info->nsamp = 1;
    break;
  case '2': case '5':
    info->sampbits = 8;
    info->nsamp = 1;
    break;
  case '3': case '6':
    info->sampbits = 8;
    info->nsamp = 3;
    break;
  default:
    return -1;
  }

  info->xsize = pnm_getint(f);
  info->ysize = pnm_getint(f);
  fclose(f);
  if(info->xsize <= 0 || info->ysize <= 0)
    return -1;

  info->kind = I_PNM;
  return 0;
}

int
pnm_skipwhite(f)
  register FILE *f;
{
  register int c;

  for(;;) {
    switch(c = fgetc(f)) {
    case '#':
      do { c = fgetc(f); } while(c != '\n' && c != EOF);
      if(c == EOF)
        return EOF;
      break;
    case ' ': case '\t': case '\n': case '\r': break;

    case EOF: return EOF;
    default:
      ungetc(c, f);
      return c;
    }
  }
}

int
pnm_getint(f)
  register FILE *f;
{
  register int n = 0;
  register int c;
  register int neg = 0;

  switch(pnm_skipwhite(f)) {
  case '-': neg = 1;
  case '+': fgetc(f);
  }
  while((c = fgetc(f)) >= '0' && c <= '9')
    n = n*10 + c-'0';
  if(c != EOF)
    ungetc(c, f);
  return neg ? -n : n;
}
