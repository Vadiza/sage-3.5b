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

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS   64

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifndef sun
#include <endian.h>
#endif

#include "bpmovie.h"
#include "imginfo.h"

/* Macros for (A<<24) + (B<<16) + (G<<8) + R-format pixels as from getimgdata */

#if defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)
# define ROFF 3
# define GOFF   2
# define BOFF   1
#else /* assume little-endian */
# define ROFF   0
# define GOFF   1
# define BOFF   2
#endif

#define PR(p)  (p)[ROFF]
#define PG(p)  (p)[GOFF]
#define PB(p)  (p)[BOFF]

char Usage[] = "\
Usage: %s -o OUTFILE.bmv  [-w XSIZExYSIZE] [-e OFFSET[m|k|f]@OUTDISK] [-p BPP] [-f rate] FROM-TO%%INCR LEFTPATTERN [RIGHTPATTERN]\n\
Encode a sequence of TIFF/SGI/JPEG/etc. files as a .bmv movie,\n\
optionally storing the image data in a separate file or disk partition.\n\
Options:\n\
    -o outfile.bmv  Write movie (or its header at least) to this file.  Required.\n\
    -e offset[m|k|f]@outdisk   If given, store the movie data at the given\n\
          offset on the given device or file.  THe offset may be\n\
      specified as a number with suffix 'm', 'k' or 'f'\n\
      in which case it's in units of 2^20 (MB), 2^10 (KB),\n\
      or the frame size, respectively.\n\
    -p bytesperpixel\n\
      Selects pixel format:  1: grayscale  2: 16-bit 5-6-5 RGB  3: 24-bit RGB\n\
    -f rate   preferred frame rate (if any), frames/sec\n\
\n\
    from-to%%incr range of frame numbers to apply to left+righpattern\n\
    leftpattern   printf format string for image name, e.g. /some/where/left.%%04d.tif\n\
    rightpattern  similar for right-hand stereo image, or \"-\" to omit\n";


char *prog;

static int bytesper[5] = { 0, 1, 2, 3, 4 }; /* indexed by enum pixeltype values */

int skipmissing = 1;
int prefrate = 0;

#define MAXALIGN  16384 /* largest credible page size */

/* Find head with both hands -- compute tile arrangement */
void findhead( bpmvhead_t *head, int xwin, int ywin, int xtile, int ytile, enum pixeltype format )
{
  head->magic = BPMV3_MAGIC;
  head->xsize = xwin;
  head->ysize = ywin;
  head->format = format;
  head->xtile = xtile;
  head->ytile = ytile;
  head->nxtile = (xwin + xtile - 1) / xtile;
  head->nytile = (ywin + ytile - 1) / ytile;

  /* Configure for fully packed tiles, which should be most
   * efficient for stuffing at graphics card
   */

  head->tilerowstride = bytesper[format] * xtile;
  head->xtilestride = head->tilerowstride * ytile;
  head->ytilestride = head->xtilestride * head->nxtile;
  head->imagestride = (head->ytilestride * head->nytile + MAXALIGN-1) & ~(MAXALIGN-1);

  head->start = head->imagestride;
}

int writeheader( char *outfname, off_t foffset, bpmvhead_t *head )
{
  int outfd;

  outfd = outfname == NULL ? 1 : open(outfname, O_LARGEFILE|O_CREAT|O_TRUNC|O_WRONLY, 0666);

  if(outfd < 0) {
    fprintf(stderr, "%s: Can't create output: %s: %s\n",
            prog, outfname, strerror(errno));
    exit(1);
  }

  if(head->extfname[0] == '\0') {
    /* All in one file. */
    char *headbuf = (char *)malloc( head->start );

    memset(headbuf, 0, head->start);
    memcpy( headbuf, head, sizeof(*head) );
    if(write( outfd, headbuf, head->start ) != head->start) {
      fprintf(stderr, "%s: %s: can't write %lld-byte header: %s\n",
              prog, outfname, head->start, strerror(errno));
      exit(1);
    }
    free(headbuf);
    return outfd;

  } else {
    int headsize = &head->cmd[ strlen(head->cmd) + 1 ] - (char *)head;
    if(write( outfd, head, headsize ) != headsize) {
      fprintf(stderr, "%s: %s: can't write %d-byte header: %s\n",
              prog, outfname, headsize, strerror(errno));
      exit(1);
    }

    close(outfd);

    outfd = open( head->extfname, O_LARGEFILE|O_CREAT|O_WRONLY, 0666 );
    if(outfd < 0) {
      fprintf(stderr, "%s: %s: Can't open external-data file for writing: %s\n",
              prog, head->extfname, strerror(errno));
      exit(1);
    }

    if(lseek(outfd, head->start, SEEK_SET) < 0) {
      fprintf(stderr, "%s: %s: Can't seek to %lld on external-data file: %s\n",
              prog, head->extfname, (long long) head->start, strerror(errno));
      exit(1);
    }

    return outfd;
  }
}



unsigned char dither3[8][8];  /* dither bias values as function of position mod 8 */

/* 3-bit (0..7) dither pattern */
static void initdither3( unsigned char dith3[8][8] )
{
  int i, j;

  for(i = 0; i < 8; i++) {
    for(j = 0; j < 8; j++) {
      int ij = i^j;

      /* Discard bottom 3 bits of 6-bit dither pattern */
      dith3[i][j] =
        ( ((ij&4)<<3) | ((i&4)<<2) | ((ij&2)<<2) | ((i&2)<<1) | ((ij&1)<<1) | (i&1)
          ) >> 3;
    }
  }
}

static void encode( unsigned char *aus, bpmvhead_t *head,
                    int aox0, int xwin, int ywin,
                    unsigned char *data, struct imginfo *info )
{
  int xtile = head->xtile;
  int ytile = head->ytile;
  int xtilestride = head->xtilestride;
  int ytilestride = head->ytilestride;
  int tilerowstride = head->tilerowstride;
  enum pixeltype format = head->format;
  int obpp = bytesper[ format ];

  int inx, iny;   /* size of intersection between image and output */
  int ix0, iy0;   /* initial position within image */
  int irowstride;   /* row-stride in image */
  int tox0, tox1, toy0, toy1; /* range of tiles in image-output intersection */
  int tox, toy;   /* current tile number */
  unsigned int ox0, oy0;

#define IBPP  4   /* bytes per input-image pixel, as provided by getimgdata() */


  irowstride = IBPP * info->xsize; /* getimgdata() was told to make 4-byte-per-pixel format */

  /* How does image fit within [xwin,ywin]? */
  if(info->xsize <= xwin) {
    /* Just right or too small.  Center image in output area */
    ix0 = 0;
    inx = info->xsize;
    ox0 = aox0 + (xwin - info->xsize) / 2;
  } else {
    /* Too large.  Center and crop image to fit. */
    ix0 = (info->xsize - xwin) / 2;
    inx = xwin;
    ox0 = aox0;
  }
  if(info->ysize <= ywin) {
    /* Just right or too small.  Center image in output area */
    iy0 = 0;
    iny = info->ysize;
    oy0 = (ywin - info->ysize) / 2;
  } else {
    /* Too large.  Center and crop image to fit. */
    iy0 = (info->ysize - ywin) / 2;
    iny = ywin;
    oy0 = 0;
  }

  tox0 = ox0 / xtile;   tox1 = (ox0 + inx-1) / xtile;
  toy0 = oy0 / ytile;   toy1 = (oy0 + iny-1) / ytile;

  /* Loop over tiles */
  for(toy = toy0; toy <= toy1; toy++) {
    int iytile = iy0 + ytile*(toy-toy0);
    int oytend = (iny - iytile >= ytile) ? ytile : iny - iytile;
    int oxstart = ox0 % xtile;

    for(tox = tox0; tox <= tox1; tox++, oxstart = 0) {
      /* Loop within this tile */
      int ixtile = ix0 + xtile*(tox-tox0);
      int oxtend = (inx - ixtile >= xtile) ? xtile : inx - ixtile;
      unsigned char *ipstart = data + IBPP * ixtile + irowstride * iytile;
      unsigned char *opstart = aus
        + toy * ytilestride
        + tox * xtilestride
        + oxstart * obpp;
      int oxt, oyt;

      for(oyt = 0; oyt < oytend; oyt++) {
        unsigned char *ip = ipstart + oyt * irowstride;
        unsigned char *op = opstart + oyt * tilerowstride;
        switch( format ) {

        case GRAY8:
          /* Convert RGB to grayscale using NTSC weighting convention */
          for(oxt = oxstart; oxt < oxtend; oxt++, ip += IBPP, op++)
            *op = (77*PR(ip) + 151*PG(ip) + 28*PB(ip)) >> 8;
          break;

        case RGB565:
          {
            unsigned char *dith3row = &dither3[oyt & 7][0];
            for(oxt = oxstart; oxt < oxtend; oxt++, ip += IBPP, op += 2) {
              /* Apply ordered dither while packing into 5-6-5 format */
              unsigned short r, g, b;
              unsigned char dith3 = dith3row[oxt & 7] >> 3;
              r = PR(ip) + dith3;       if(r > 255) r = 255;
              g = PG(ip) + (dith3>>1);  if(g > 255) g = 255;
              b = PB(ip) + dith3;       if(b > 255) b = 255;
              *(unsigned short *)op =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                ((b >> 3) & 0x1F);
            }
            break;
          }

        case RGB888:
          for(oxt = oxstart; oxt < oxtend; oxt++, ip += IBPP, op += 3) {
            op[0] = PR(ip);
            op[1] = PG(ip);
            op[2] = PB(ip);
          }
          break;

        case ABGR8888:
          for(oxt = oxstart; oxt < oxtend; oxt++, ip += IBPP, op += 4) {
            op[0] = 255;
            op[1] = PB(ip);
            op[2] = PG(ip);
            op[3] = PR(ip);
          }
          break;
        }
      }
    }
  }
}

int main( int argc, char *argv[] )
{
  char *outname = NULL;
  char *extout = NULL;
  long long outoffset = 0;
  int offsetframes = 0;

  char *left, *right;
  int ffrom, fto, finc, frameno;
  int i, count;

  enum pixeltype pixelformat = RGB565;
  int xwin = 1920, ywin = 1080;
  int xtile = 512, ytile = 512;
  bpmvhead_t head;
  int outfd = -1;
  int relshift = 0;

  unsigned char *aus;

  memset( &head, 0, sizeof(head));

  for(i = 0, count = 0; i < argc; i++) {
    int len = strlen(argv[i]);
    if(count + len > sizeof(head.cmd)-2)
      len = sizeof(head.cmd)-2 - count;
    memcpy(head.cmd+count, argv[i], len);
    count += len;
    head.cmd[count++] = ' ';
  }
  head.cmd[--count] = '\0';

  prog = argv[0];

  while(argc>2 && argv[1][0] == '-' && argv[1][1] != '\0') {
    switch(argv[1][1]) {
    case 'o':
      outname = argv[2];
      argc -= 2, argv += 2;
      break;

    case 'f': /* preferred play rate */
      prefrate = atoi(argv[2]);
      if(strchr(argv[2], 'm') != NULL)
        prefrate = -abs(prefrate);  /* in ms/frame rather than frames/sec */
      argc -= 2, argv += 2;
      break;

    case 'e': {
      char *cp = strchr(argv[2], '@');
      if(cp) {
        char *ep;
        outoffset = strtoll(argv[2], &ep, 0);
        switch(*ep) {
        case 'k': case 'K': outoffset <<= 10; break;
        case 'm': case 'M': outoffset <<= 20; break;
        case 'f': case 'F': offsetframes = 1; break;
        }
        extout = cp+1;
      } else {
        extout = argv[2];
      }
      argc -= 2, argv += 2;
    }
      break;

    case 'L':
      relshift = -atoi(argv[2]);  argc -= 2, argv += 2;
      break;

    case 'R':
      relshift = atoi(argv[2]);  argc -= 2, argv += 2;
      break;

    case 'p':
      pixelformat = (enum pixeltype) atoi(argv[2]);
      if(pixelformat < 1 || pixelformat > 4) {
        fprintf(stderr, "%s -p: expected pixel format 1 (gray), 2 (rgr565), 3 (rgb24), 4 (rgba), not %s\n", prog, argv[2]);
        exit(1);
      }
      argc -= 2, argv += 2;
      break;

    case 'w':
      sscanf(argv[2], "%d%*c%d", &xwin, &ywin);
      argc -= 2, argv += 2;
      break;

    default:
      argc = 0;
    }
  }

  if(argc < 3) {
    fprintf(stderr, Usage, prog);
    exit(1);
  }

  ffrom = 0;
  fto = -1;
  finc = 1;

  if(sscanf(argv[1], "%d-%d%%%d", &ffrom, &fto, &finc) <= 0) {
    fprintf(stderr, "%s: expected from-to%%incr, got %s\n", prog, argv[1]);
    exit(1);
  }

  left = argv[2];
  right = argc>3 ? argv[3] : NULL;

  if(fto < 0)
    fto = ffrom;
  if(finc == 0)
    finc = 1;

  count = (fto - ffrom) / finc + 1;

  findhead( &head, right ? 2*xwin : xwin, ywin, xtile, ytile, pixelformat );


  head.nframes = count;

  if(relshift != 0) {
    head.prefshift = relshift;
    head.flags |= BPF_PREFSHIFT;
  }

  if(prefrate != 0) {
    head.prefrate = prefrate;
    head.flags |= BPF_PREFRATE;
  }

  if(right != NULL)
    head.flags |= BPF_STEREO;

  if(extout != NULL) {
    if(offsetframes)
      outoffset *= head.imagestride;
    head.flags |= BPF_EXTDATA;
    head.start = outoffset;
    strncpy( head.extfname, extout, sizeof(head.extfname)-1 );
  }

  aus = (unsigned char *) malloc( head.imagestride );
  if(aus == NULL) {
    fprintf(stderr, "%s: Can't allocate %d bytes for image buffer\n",
            prog, head.imagestride);
    exit(1);
  }
  memset(aus, 0, head.imagestride);

  if(pixelformat == RGB565)
    initdither3( dither3 );

  fprintf(stderr, "RIGHT [%s], XWIN %d YWIN %d XTILE %d YTILE %d\n", right, xwin, ywin, xtile, ytile);

  for(frameno = ffrom, i = 0; i < count; i++, frameno += finc) {
    struct imginfo lefti, righti;
    unsigned char *leftd = NULL, *rightd = NULL;
    char leftname[300], rightname[300];

    /* Find names of images for this frame */
    sprintf(leftname, left, frameno);

    /* Read images.  Give up if left-hand is missing. */
    leftd = getimgdata( leftname, NULL, &lefti, 1, 4, 1 );
    if(leftd == NULL) {
      fprintf(stderr, "%s: can't open %s\n", prog, leftname);
      if(skipmissing)
        continue;
      else
        break;
    }

    /* Is this first time?  Emit header if so. */

    if(i == 0) {
      outfd = writeheader( outname, outoffset, &head );
    }

    /* Re-encode left (or only) image into buffer */

    encode( aus, &head, 0,    xwin, ywin,  leftd, &lefti   );

    if(right != NULL) {
      /* If there's a right-hand image, encode it too. */
      sprintf(rightname, right, frameno);
      rightd = (0==strcmp(rightname, "-")) ? NULL
        : getimgdata( rightname, NULL, &righti, 1, 4, 1 );
      if(rightd == NULL) {
        if(rightname[0] != '-')
          fprintf(stderr, "%s: can't open %s\n", prog, rightname);
        rightd = leftd;
        righti = lefti;
      }

      encode( aus, &head, xwin, xwin, ywin, rightd, &righti );
    }

    /* DEBUG */
    *(int *)( aus + head.imagestride-4 ) = frameno;

    errno = 0;
    if(write( outfd, aus, head.imagestride ) != head.imagestride ) {
      fprintf(stderr, "%s: %s: write error: %s\n",
              prog, outname ? outname : "stdout", strerror(errno));
      exit(1);
    }
    free(leftd);
    if(rightd != leftd && rightd != NULL)
      free(rightd);
  }
  return 0;
}
