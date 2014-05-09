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
 * Bulk movieplayer.  Plays uncompressed movies as encoded by "img2bmv",
 * using bpio threaded I/O library and OpenGL texturing.
 *
 * Stuart Levy, NCSA, University of Illinois Urbana-Champaign, June, 2004.
 */

#include "bpio.h" /* Need this first since it #defines some large-file options */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <getopt.h>

#include "bpmovie.h"

// headers for SAGE
#include "sail.h"
#include "appWidgets.h"
#include "misc.h"
sail sageInf; // sail object
unsigned char *rgbBuffer = 0;
float pixelSize = 3.0;


/*
  struct txcodes {
  int internalfmt;
  int format;
  int type;
  } txcode[] = {
  { 0,0,0 },
  { GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE },
  { GL_RGB,       GL_RGB,       GL_UNSIGNED_SHORT_5_6_5 },
  { GL_RGB,       GL_RGB,       GL_UNSIGNED_BYTE },
  { GL_RGBA,      GL_RGBA,      GL_UNSIGNED_BYTE }
  };
*/

int nfillers = 4;
int nbufseach = 5;
int readsize = 8388608;    // 8MB
int port = 0;

char *prog;

char *controlfname;

enum stereomode { MONO, CROSSEYED, QUADBUFFERED };

enum stereomode stereo = CROSSEYED;


/* State variables */
int winx, winy;   /* current window size */
int newtex = 1;   /* newly minted textures? (need full init rather than subload) */

int paused = 1;   /* Movie paused? */
int doloop = 0;
int majorloop = 1;
int playfwd = 1;
int quitnow = 0;
int rateoverride = 0; /* Should defaultrate override preferred-rate setting in movie? */
int defaultrate = 10; /* default target frames/sec */
int framems = 100;  /* target milliseconds per frame */
int msfudge = 35; /* framems = 1000/rate - msfudge */
int verbose = 0;
int skipby = 1;   /* skip -- show every Nth frame */
int preloadms = 0;  /* After switching movies, wait this long for startup */
int shiftoverride = 0;
int defaultshift = 0;
int relshift = 0;
int nwID = 1;

static int swapinterval = -1;

bpio_t     *playlist;
bpmvhead_t *mvlist;
int    nplay;
int    curplay;

float accumdt = 0, accumweight = 0;

void resumemovie( int loop );
void pausemovie();
int openmovie( bpio_t *bpio, char *fname, bpmvhead_t *bpmv );

off_t bpstartpos( bpio_t *bpio )
{
  return bpio==NULL ? 0 : bpio->bpb[0].wrappos;
}

off_t bpendpos( bpio_t *bpio )
{
  return bpio==NULL ? 0 : bpio->bpb[0].eofpos;
}

off_t bpincrpos( bpio_t *bpio )
{
  return bpio==NULL ? 0 : bpio->bufsize;
}


int bpawaitone( bpio_t *bpio )
{
  bpbuf_t *bpb = &playlist[curplay].bpb[ bpio->drain ];

  pthread_mutex_lock( &bpb->bmut );
  while(bpbempty( bpb ) && bpb->filling) {
    bpb->drainwaiting = 1;
    pthread_cond_wait( &bpb->bdrainwait, &bpb->bmut );
    bpb->drainwaiting = 0;
  }
  pthread_mutex_unlock( &bpb->bmut );

  return !bpbempty(bpb);
}

int bpconsume1( bpio_t *bpio )
{
  bpbuf_t *bpb =  &bpio->bpb[ bpio->drain ];

  if(!bpb->filling) /* if EOF or bpstop() or etc. */
    return 0;

  pthread_mutex_lock( &bpb->bmut );
  bpb->wp = (bpb->wp + 1) % bpb->nbufs;

  if(bpb->fillwaiting)
    pthread_cond_signal( &bpb->bfillwait );
  pthread_mutex_unlock( &bpb->bmut );

  bpio->drain = (bpio->drain+1) % bpio->nfillers;

  return bpawaitone( bpio );
}

int bpframeof( bpio_t *bpio, off_t pos )
{
  off_t incr = bpio->bpb[0].incrpos;
  return incr > 0 ? (pos - bpstartpos(bpio)) / incr :
    incr < 0 ? (pos - bpstartpos(bpio)) / (-incr) :
    -1;
}

int defaultratefor( int curplay )
{
  return rateoverride
    || !(mvlist[curplay].flags & BPF_PREFRATE)
    || mvlist[curplay].prefrate == -1  /* bug in some versions of img2bmv */
    ? defaultrate : mvlist[curplay].prefrate;
}

int defaultshiftfor( int curplay )
{
  return shiftoverride || !(mvlist[curplay].flags & BPF_PREFSHIFT)
    ? defaultshift : mvlist[curplay].prefshift;
}

int lasttimems;

int timems()
{
  static struct timeval then;
  struct timeval now;

  gettimeofday( &now, NULL );
  if(then.tv_sec == 0)
    then = now;
  return (now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec) / 1000;
}

int playlistadvance( int sign )
{
  bpio_t *bpio;
  int waspaused = paused;
  int oldplay = curplay;

  if(curplay >= 0)
    pausemovie();

  curplay += sign;

  if(majorloop)
    curplay = (curplay + nplay) % nplay;

  if(curplay < 0 || curplay >= nplay) {
    pausemovie();
    return 0;
  }

  if(oldplay != curplay) {
    int rate = defaultratefor( curplay );

    framems = (rate > 0) ? 1000 / rate - msfudge: -rate;

    relshift = defaultshiftfor( curplay );
  }

  bpio = &playlist[curplay];

  bpforward( bpio, playfwd * skipby );
  bpsync( bpio );
  bpseek( bpio, playfwd>0 ? bpstartpos( bpio )
          : bpendpos( bpio ) - bpio->bufsize * skipby );

  if(verbose >= 2)
    fprintf(stderr, "advance %d  framems %d\n", sign, framems);

  if(!waspaused)
    resumemovie( doloop );

  if(preloadms)
    usleep( preloadms * 1000 );

  return 1;
}

void idler(void);

void showlater( int junk )
{
  //glutPostRedisplay();
  //glutIdleFunc( paused ? NULL : idler );
}


void idler()
{
  if(quitnow) {
    exit(1);
  }
  if(verbose>=3) { printf("I"); fflush(stdout); }

  if(bpconsume1( &playlist[curplay] )) {
    /* Got a buffer ready.  Should we show it now? */
    if(swapinterval >= 0) {
      /* Rather than timing here, we'll depend on glXSwapIntervalSGI() */
      //glutPostRedisplay();
    } else {
      /* Nope, we do our own timing */
      int now = timems();
      if(now >= lasttimems + framems) {
        //glutPostRedisplay();
      } else {
        // glutTimerFunc( (lasttimems + framems) - now, showlater,
        //           (lasttimems+framems) - now );
        // glutIdleFunc( NULL );
      }
    }
  } else {
    if( ! playlistadvance( playfwd ) )
    {
      // glutIdleFunc( NULL );
    }
  }
}

void catch_quit(int sig) {
  quitnow = 1;
}

int ntimes = 50;

void resumemovie( int loop )
{
  if(verbose>=3) printf("resume %d p%d\n", loop, paused);
  bpstart( &playlist[curplay], loop );
  if(bpawaitone( &playlist[curplay] )) {
    // glutPostRedisplay();
    if(paused)
    {
      // glutIdleFunc( idler );
    }
  } else {
    printf("resume: bpawaitone -> 0?\n");
  }
  paused = 0;
}

void startmovie( int loop )
{
  if(verbose>=2) printf("start %d\n", loop);
  bpseek( &playlist[curplay], bpstartpos( &playlist[curplay] ) );
  bpsync( &playlist[curplay] );
  resumemovie( loop );
}

void pausemovie()
{
  //glutIdleFunc( NULL );
  paused = 1;
}

static int val, hasnum;

int getval( int def ) {
  return hasnum < 0 ? -val : hasnum > 0 ? val : def;
}

void kb( unsigned char key, int x, int y );

void specialkb( int skey, int x, int y )
{
  bpio_t *bpio = &playlist[curplay];

  /*
    switch(skey) {
    case GLUT_KEY_PAGE_UP:
    playlistadvance( -1 );
    break;

    case GLUT_KEY_PAGE_DOWN:
    playlistadvance( 1 );
    break;

    case GLUT_KEY_UP:
    skipby++;
    bpforward( bpio, skipby * playfwd );
    break;

    case GLUT_KEY_DOWN:
    if(skipby>1)
    skipby--;
    bpforward( bpio, skipby * playfwd );
    break;

    case GLUT_KEY_LEFT:
    if(glutGetModifiers() & GLUT_ACTIVE_CTRL)
    kb( 'L', x, y );
    else
    kb( '<', x, y );
    break;

    case GLUT_KEY_RIGHT:
    if(glutGetModifiers() & GLUT_ACTIVE_CTRL)
    kb( 'R', x, y );
    else
    kb( '>', x, y );
    break;

    case GLUT_KEY_HOME:
    startmovie( doloop );
    break;
    }
  */
  hasnum = val = 0;
}

void kb( unsigned char key, int x, int y )
{
  int isdigit = 0;
  bpio_t *bpio = &playlist[curplay];

  switch(key) {
  case '\033':
    exit(0);

  case ' ':
    paused = !paused;
    if(paused)
      pausemovie();
    else
      resumemovie( doloop );
    break;

  case 'p':
    pausemovie();
    break;

  case '.': case '<':
    bpstop( bpio );
    bpsync( bpio );
    bpforward( bpio, skipby );
    resumemovie( doloop );
    bpconsume1( bpio );
    pausemovie();
    break;

  case ',': case '>':
    bpstop( bpio );
    bpsync( bpio );
    bpforward( bpio, -skipby );
    resumemovie( doloop );
    bpconsume1( bpio );
    pausemovie();
    break;

  case '-':
    hasnum = -1;
    isdigit = 1;
    break;

  case 'n':
    if(hasnum)
      curplay = getval(0) - 1;
    playlistadvance( 1 );
    break;

  case 'N':
    playlistadvance( -1 );
    break;

  case 'P':
    preloadms = getval( 500 );
    break;

  case 'f':
    playfwd = 1;
    bpforward( bpio, playfwd * skipby );
    bpsync( bpio );
    resumemovie( doloop );
    break;

  case 'b':
    playfwd = -1;
    bpforward( bpio, playfwd * skipby );
    bpsync( bpio );
    resumemovie( doloop );
    break;

  case 'l':
    doloop = hasnum ? val : !doloop;
    if(doloop != 0)
      playfwd = doloop;
    pausemovie();
    resumemovie( doloop );
    break;

  case 'R':
    relshift = getval( relshift+1 );
    // glutPostRedisplay();
    if(verbose>=2) printf("relshift %d\n", relshift);
    break;

  case 'L':
    relshift = -getval( -(relshift-1) );
    // glutPostRedisplay();
    if(verbose>=2) printf("relshift %d\n", relshift);
    break;


  case 'z':
    playfwd = 1;
    skipby = 1;
    bpforward( bpio, playfwd * skipby );
    bpsync( bpio );
    startmovie( doloop ); /* schedule first frame */
    pausemovie();
    break;

  case 'v':
    verbose = getval( !verbose );
    break;

  case 'g': {
    int frameno = getval(0);
    int nframes = mvlist[curplay].nframes;
    if(frameno >= nframes-1)
      frameno = nframes-1;
    else if(frameno < 0)
      frameno = 0;

    bpseek( bpio, bpstartpos( bpio ) + frameno * (off_t) bpio->bufsize );
    bpsync( bpio );
    resumemovie( doloop );
    bpawaitone( bpio );
    pausemovie();
  }
    break;

  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    val = val*10 + key - '0';
    hasnum = 1;
    isdigit = 1;
    break;


  case 's': {
    int rate = getval( defaultratefor(curplay) );
    framems = (int) ( rate>0 ? 1000.0/rate - msfudge : -rate );
  }
    if(verbose>=2)
      fprintf(stderr, "%d fps -> allot %d ms/frame with 10ms fudge\n", val, framems);
    break;

  case 't':
    framems = getval( framems );
    break;

  case 'S':
    skipby = getval( 1 );
    bpforward( bpio, playfwd * skipby );
    break;

  case '=': {
    int frameno = ( bptell( bpio ) - bpstartpos( bpio ) ) / bpincrpos( bpio );
    float fps = accumdt==0 ? 0 : 1000 * accumweight / accumdt;
    fprintf(stderr, "frame %04d   %5.1f fps\n", frameno, fps);
  }
    break;

  case 'h':
    fprintf(stderr, "bplay keyboard commands:\n\
  <NNN>s   aim for NNN frames/sec\n\
  <NNN>S   skip -- show every NNN'th frame\n\
  <NNN>g   go to NNN'th frame and pause (first = 0)\n\
   .       one frame forward\n\
   ,       one frame backward\n\
   f       run forward\n\
   b       run backward\n\
   v       toggle verbose (on-screen framenumber & fps counter)\n\
   p       pause\n\
   l     toggle loop/stop-at-end\n\
  SPACE    toggle run/pause\n\
  HOME     go to beginning of movie and play (like \"0gf\")\n");
    break;


  }
  if(!isdigit) {
    if(verbose>=2) {
      if(hasnum)  printf("\"%d%c\"\n", getval(0), key);
      else  printf("\"%c\"", key);
    }
    val = hasnum = 0;
  }
}

void swab32( void *ap, int nbytes )
{
  unsigned int *p = (unsigned int *)ap;
  while((nbytes -= 4) >= 0) {
    unsigned int v = *p;
    *p++ = ((v>>24)&0xFF) | ((v>>8)&0xFF00) | ((v&0xFF00)<<8) | ((v&0xFF)<<24);
  }
}

void swabbpmv( bpmvhead_t *bpmv )
{
  swab32( bpmv, (char *)&bpmv->extfname - (char *)bpmv );

  /* bpmv->start is a 64-bit quantity and needs its words swapped */
  bpmv->start = ((bpmv->start & 0xFFFFFFFFLL) << 32)
    | ((bpmv->start >> 32) & 0xFFFFFFFFLL);
}


int openmovie( bpio_t *bpio, char *fname, bpmvhead_t *bpmv )
{
  int fd;
  int n;
  bpmvhead_t head;

  if(fname == NULL)
    return -1;

  fd = open(fname, O_RDONLY);

  if(fd < 0) {
    fprintf(stderr, "%s: %s: can't open: %s\n",
            prog, fname, strerror(errno));
    return -1;
  }

  n = read( fd, &head, sizeof(head) );
  if(n <= &head.extfname[0] - (char *)&head) {
    fprintf(stderr, "%s: %s: can't read header: %s\n",
            prog, fname, strerror(errno));
    return -1;
  }

  if(head.magic != BPMV3_MAGIC)
    swabbpmv( &head );
  if(head.magic != BPMV3_MAGIC) {
    fprintf(stderr, "%s: %s doesn't look like a bpmovie file (as made by img2bpmv).\n",
            prog, fname);
    return -2;
  }

  close(fd);

  /* OK then. */

  newtex = 1;

  if(bpio->nfillers == 0)
    bpinit( bpio, nfillers, head.imagestride, readsize, nbufseach );

  if(head.flags & BPF_EXTDATA) {
    if(bpopen( bpio, head.extfname ) < 0) {
      fprintf(stderr, "%s: %s refers to movie data in \"%s\": %s\n",
              prog, fname, head.extfname, strerror(errno));
      return -1;
    }
  } else {
    if(bpopen( bpio, fname ) < 0)
      return -1;
  }

  if(bpmv)
    *bpmv = head;

  if(head.flags & BPF_EXTDATA) {
    off_t end = head.start + (long long)head.imagestride * (long long)head.nframes;
    bprange( bpio, head.start, end );
  } else {
    bprange( bpio, head.start, -1LL );
  }
  bpseek( bpio, head.start );
  bpsync( bpio );

  return head.nframes;
}

void draweye( bpmvhead_t *bpmv, unsigned int *txs, unsigned char *tilebuf,
              float scl, float imxsz, float xoff, float yoff, int x0, int x1, float *xagain )
{
  //struct txcodes *txc = &txcode[ bpmv->format ];
  int i,j,k, ii;

  for(i = k = 0; i < bpmv->nytile; i++) {
    for(j = x0; j < x1; j++, k++) {

      float xx0 = xoff + scl * j * bpmv->xtile;
      float xx1 = xoff + scl * (j+1) * bpmv->xtile;
      float yy0 = yoff + scl * i * bpmv->ytile;
      float yy1 = yoff + scl * (i+1) * bpmv->ytile;

      /*
        fprintf(stderr, "%d %d %d %d - stride %d %d %d\n",
        j * bpmv->xtile,
        (j+1) * bpmv->xtile,
        i * bpmv->ytile,
        (i+1) * bpmv->ytile,
        bpmv->xtilestride,
        bpmv->ytilestride, bpmv->tilerowstride);
      */

      unsigned char *ptr = tilebuf + j*bpmv->xtilestride + i*bpmv->ytilestride;
      unsigned char *dst = rgbBuffer + int(i*bpmv->xtile*x1*bpmv->ytile*pixelSize) + int(j*bpmv->xtile*pixelSize);
      for (ii=0;ii<bpmv->ytile;ii++)
      {
        memcpy(dst, ptr, int(bpmv->xtile * pixelSize));
        dst += int(bpmv->xtile * x1 * pixelSize);
        ptr += int(bpmv->xtile * pixelSize);
      }


      //glBindTexture( GL_TEXTURE_2D, txs[k] );

      if (newtex) {

        /*                 glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL ); */
        /*                 glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); */
        /*                 glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); */

        /*                 glTexImage2D( GL_TEXTURE_2D, 0, */
        /*                               txc->internalfmt, bpmv->xtile, bpmv->ytile, 0, */
        /*                               txc->format, txc->type,  */
        /*                               tilebuf + j*bpmv->xtilestride + i*bpmv->ytilestride ); */
      } else {
        /*                 glTexSubImage2D( GL_TEXTURE_2D, 0,  0,0, */
        /*                                  bpmv->xtile, bpmv->ytile, txc->format, txc->type, */
        /*                                  tilebuf + j*bpmv->xtilestride + i*bpmv->ytilestride ); */

      }

      /*             glBegin( GL_QUADS ); */

      /*             glTexCoord2f( 0, 0 ); */
      /*             glVertex3f( xx0, yy0, 0 ); */

      /*             glTexCoord2f( 0, 1 ); */
      /*             glVertex3f( xx0, yy1, 0 ); */

      /*             glTexCoord2f( 1, 1 ); */
      /*             glVertex3f( xx1, yy1, 0 ); */

      /*             glTexCoord2f( 1, 0 ); */
      /*             glVertex3f( xx1, yy0, 0 ); */

      /* If it's a mono movie and we're in stereo,
       * we need to draw this tile twice.
       */
      if(xagain != NULL) {
        /*                 glTexCoord2f( 0, 0 ); */
        /*                 glVertex3f( xx0 + *xagain, yy0, 0 ); */

        /*                 glTexCoord2f( 0, 1 ); */
        /*                 glVertex3f( xx0 + *xagain, yy1, 0 ); */

        /*                 glTexCoord2f( 1, 1 ); */
        /*                 glVertex3f( xx1 + *xagain, yy1, 0 ); */

        /*                 glTexCoord2f( 1, 0 ); */
        /*                 glVertex3f( xx1 + *xagain, yy0, 0 ); */
      }

      /*             glEnd(); */

      {
        /*                 GLuint e = glGetError(); */
        /*                 if(e != 0) fprintf(stderr, "E%d ", e); */
      }
    }
  }

#if 0
  static int count = 1;
  char fn[256];
  sprintf(fn, "out%d.raw", count++);
  FILE *ff = fopen(fn, "w+");
  fwrite(rgbBuffer, 1, x1*bpmv->xtile*bpmv->nytile*bpmv->ytile*3, ff);
  fclose(ff);
#else

  sageInf.swapBuffer();
  rgbBuffer = (unsigned char *)sageInf.getBuffer();

#endif
}

void redraw()
{
  static unsigned int *txs;
  static int ntxs = 0;

  bpmvhead_t *bpmv = &mvlist[curplay];
  float xscl, yscl, scl, eyex, imxsz, xoff, yoff;
  float xright;

  unsigned char *tilebuf;
  int now;

  float dt, decay;

  tilebuf = bpcurbuf( &playlist[curplay] );

  if(tilebuf == NULL) {
    /*         glClearColor( 0, 0, 0.3, 1 ); */
    /*         glClear( GL_COLOR_BUFFER_BIT ); */
    /*         glutSwapBuffers(); */
    return;
  }

  /* fprintf(stderr, "<%d:%d> ", playlist[curplay].drain, *(int *)tilebuf); */

  if(playlist[curplay].bufsize != bpmv->imagestride) {
    /* I/O mismatch */
    /*         glClearColor( 0.5, 0, 0, 1 ); */
    /*         glClear( GL_COLOR_BUFFER_BIT ); */
    /*         glutSwapBuffers(); */
    return;
  }

  if(verbose < 4) {
    /*         glClearColor( 0, 0, 0, 1 ); */
    /*         if(stereo != QUADBUFFERED) */
    /*             glClear( GL_COLOR_BUFFER_BIT ); */
  }

  /*     glMatrixMode( GL_MODELVIEW ); */
  /*     glLoadIdentity(); */

  /* OK, use textures from current bpio buffer */

  if(bpmv->nxtile * bpmv->nytile != ntxs) {
    if(ntxs > 0) {
      //glDeleteTextures( ntxs, txs );
      free( txs );
    }

    ntxs = bpmv->nxtile * bpmv->nytile;
    txs = (unsigned int *) malloc( ntxs * sizeof(unsigned int) );
    //glGenTextures( ntxs, txs );
    newtex = 1;
  }

  // glEnable( GL_TEXTURE_2D );

  /*
   * How big should this image be drawn?
   * Scale to fit the available area, centered.
   * If image is stereo and we're in crosseyed-stereo mode,
   * translate to keep center of image on center of display.
   */

  eyex = (stereo==CROSSEYED) ? winx/2 : winx;
  imxsz = (bpmv->flags&BPF_STEREO) ? bpmv->xsize/2 : bpmv->xsize;
  xscl = eyex / (float)imxsz;
  yscl = winy / (float)bpmv->ysize;
  scl = (xscl < yscl) ? xscl : yscl;
  xoff = 0.5f * (eyex - scl * imxsz);
  yoff = 0.5f * (winy - scl * bpmv->ysize);
  xright = winx / 2;
  if(verbose >= 5)
    fprintf(stderr, "s%.3g xo%g yo%g\n", scl, xoff, yoff);

  if(verbose<4) {
    int stereomovie = bpmv->flags & BPF_STEREO;
    float xoff = 0.5f * (eyex - scl * imxsz);
    float yoff = 0.5f * (winy - scl * bpmv->ysize);
    int nxtile = bpmv->nxtile;

    switch(stereo) {
    case MONO:
      /*
       * If it's a stereo movie and we're in mono mode,
       * just display the left half.
       */
      draweye( bpmv, txs, tilebuf, scl, imxsz, xoff, yoff,
               0, stereomovie ? nxtile/2 : nxtile, NULL );
      break;

    case CROSSEYED:
      if(stereomovie) {
        if(relshift == 0) {
          draweye( bpmv, txs, tilebuf, scl, imxsz, xoff, yoff, 0, nxtile, NULL );
        } else {
          draweye( bpmv, txs, tilebuf, scl, imxsz, xoff+relshift, yoff, 0, nxtile/2, NULL );
          draweye( bpmv, txs, tilebuf, scl, imxsz, xoff-relshift, yoff, nxtile/2, nxtile, NULL );
        }
      } else {
        /* mono movie in stereo mode -- draw twice */
        draweye( bpmv, txs, tilebuf, scl, imxsz, xoff, yoff, 0, nxtile, &xright );
      }
      break;

    case QUADBUFFERED:
      /*                 if(stereomovie) { */
      /*                     glDrawBuffer( GL_BACK_RIGHT ); */
      /*                     glClear( GL_COLOR_BUFFER_BIT ); */
      /*                     draweye( bpmv, txs, tilebuf, scl, imxsz, xoff-relshift, yoff, nxtile/2, nxtile, NULL ); */
      /*                     glDrawBuffer( GL_BACK_LEFT ); */
      /*                     glClear( GL_COLOR_BUFFER_BIT ); */
      /*                     draweye( bpmv, txs, tilebuf, scl, imxsz, xoff+relshift, yoff, 0, nxtile/2, NULL ); */
      /*                 } else { */
      /*                     glDrawBuffer( GL_BACK ); */
      /*                     glClear( GL_COLOR_BUFFER_BIT ); */
      /*                     draweye( bpmv, txs, tilebuf, scl, imxsz, xoff+relshift, yoff, 0, nxtile, NULL ); */
      /*                 } */
      break;
    }
  }

  if(verbose) {
    char leg[80];
    int frameno = *(int *)( tilebuf + bpmv->imagestride - 4); /* img2bmv easter egg */
    float fps = accumdt==0 ? 0 : 1000 * accumweight / accumdt;
    bpio_t *bpio = &playlist[curplay];
    int slotno  = (int) ( (   bptell(bpio) - bpstartpos(bpio) ) / bpincrpos(bpio) );
    int howmany = (int) ( ( bpendpos(bpio) - bpstartpos(bpio) ) / bpincrpos(bpio) );
    int i;

    if(frameno > 0 && frameno < 20000)
      sprintf(leg, "%5d of 0..%-5d  %4.1f fps  frame %04d", slotno, howmany-1, fps, frameno);
    else
      sprintf(leg, "%5d of 0..%-5d  %4.1f fps", slotno, howmany-1, fps);

    /*         glMatrixMode( GL_MODELVIEW ); */
    /*         glPushMatrix(); */

    /*         glTranslatef( 18, winy-20, 0 ); */
    /*         glScalef( 0.1, 0.1, 0.1 ); */

    /*         glDisable( GL_TEXTURE_2D ); */
    /*         glColor3f( 1, 1, 0.5 ); */

    /*         for(i = 0; leg[i] != '\0'; i++) */
    /*             glutStrokeCharacter( GLUT_STROKE_ROMAN, leg[i] ); */

    /*         glPopMatrix(); */
  }

  if(swapinterval < 0) {
    /* if we can't depend on glXSwapIntervalSGI() for timing */
    now = timems();
  }

  // glutSwapBuffers();


  now = timems();
  dt = (now - lasttimems);
  decay = exp( - dt / 100 );
  accumdt =     (accumdt * decay)     + dt;
  accumweight = (accumweight * decay) + 1;

  lasttimems = now;

  if(verbose >= 2) {
    bpio_t *bpio = &playlist[curplay];
    bpbuf_t *bpb = &bpio->bpb[ bpio->drain ];
    int frameno = tilebuf ? *(int *)( tilebuf + bpmv->imagestride - 4) : -9999; /* img2bmv easter egg */
    printf("<%02d:%02d~%02d> f%d fw%d r%d dw%d w%d %d..%d  %.0fms\n",
           bpio->drain,
           bpb->curpos[bpb->wp] < 0 ? -1 : (int) (bpb->curpos[bpb->wp] / bpio->bufsize),
           frameno,
           bpb->filling,
           bpb->fillwaiting, bpb->rp, bpb->drainwaiting, bpb->wp,
           (int) (bpb->wrappos / bpio->bufsize),
           (int) (bpb->eofpos / bpio->bufsize),
           dt);
  }

  newtex = 0;

}

void visible( int yes )
{
  static int first = 1;
  if(yes && first)
    startmovie( doloop );
  else
    pausemovie();
  first = 0;
}



void playpause(int eventId, button * btnObj)
{
  if(eventId == BUTTON_DOWN)
    pausemovie();
  else
    resumemovie(doloop);
}



int main(int argc, char *argv[])
{
  int c, i;
  char *hostIP;

  static char Usage[] = "Usage: %s [options] file.bmv ...\n\
   Plays uncompressed bulk movies in the format created by img2bmv.\n\
   Options:\n\
   -f NNN target NNN frames/sec or -f NNNm milliseconds/frame\n\
   -t NTHREADS  number of reader threads\n\
   -r READSIZE  size of each read() in bytes (default = image size)\n\
   -M msfudge fudge factor for ms/frame timing estimates (default %d)\n\
   -v   verbose\n\
  -n nwID network ID for SAGE\n";

  prog = argv[0];

  signal(SIGINT, catch_quit);

  hostIP = NULL;

  while((c = getopt(argc, argv, "t:b:r:p:S:F:f:P:L:R:vsmqM:c:n:")) != EOF) {
    switch(c) {
    case 'c':
      hostIP = strdup(optarg);
      break;

    case 't':
      nfillers = atoi(optarg);
      if(nfillers < 1) {
        fprintf(stderr, "%s: Need at least 1 reader thread\n",
                prog);
        exit(1);
      }
      break;

    case 'b':
      nbufseach = atoi(optarg);
      if(nbufseach < 2) {
        fprintf(stderr, "%s: Need at least 2 buffers per reader thread\n",
                prog);
        exit(1);
      }
      break;

    case 'r':
      readsize = atoi(optarg);
      if(readsize <= 0 || readsize % getpagesize() != 0) {
        fprintf(stderr, "%s: readsize must be a multiple of pagesize (%d)\n",
                prog, getpagesize());
        exit(1);
      }
      break;

    case 'p':
      port = atoi(optarg);  /* Not implemented yet */
      break;

    case 'P':
      preloadms = atoi(optarg);
      break;

    case 'M':
      msfudge = atoi(optarg);
      break;

    case 'S':
      skipby = atoi(optarg);
      if(skipby <= 0) skipby = 1;
      break;

    case 'F':
      rateoverride = 1; /* and fall into ... */
    case 'f':
      {
        double v = atof(optarg);
        if(strchr(optarg, '!'))
          rateoverride = 1;
        if(strchr(optarg, 'm'))
          defaultrate = (int) -v;
        else if(v>0)
          defaultrate = (int) v;
        else {
          fprintf(stderr, "%s: -f %s: expected frame rate (frames/sec) or frame time in milliseconds (with \"m\" suffix).\n",
                  prog, optarg);
          exit(1);
        }
      }
      break;

    case 's':
      stereo = CROSSEYED;
      break;

    case 'm':
      stereo = MONO;
      break;

    case 'q':
      stereo = QUADBUFFERED;  /* Not implemented yet! */
      break;

    case 'R':
      defaultshift = atoi(optarg);
      shiftoverride = (strchr(optarg, '!') != NULL);
      break;

    case 'L':
      defaultshift = -atoi(optarg);
      shiftoverride = (strchr(optarg, '!') != NULL);
      break;

    case 'v':
      verbose++;
      break;

    case 'n':
      nwID = atoi(optarg);
      break;

    default:
      fprintf(stderr, "%s: unknown option -%c\n", prog, c);
      argc=1;
      break;
    }
  }


  nplay = argc - optind;
  playlist = (bpio_t *) calloc( nplay, sizeof(bpio_t) );
  mvlist = (bpmvhead_t *) calloc( nplay, sizeof(bpmvhead_t) );

  int totalframes=0, frames;
  for(i = 0; i < nplay; i++) {

    frames =  openmovie( &playlist[i], argv[i+optind], &mvlist[i] );
    if( frames < 0 ) {
      fprintf(stderr, "%s: %s: cannot open: %s\n",
              prog, argv[i+optind], strerror(errno));
      exit(1);
    }
    totalframes +=  frames;
  }

  if(nplay <= 0 && port == 0) {
    fprintf(stderr, Usage, prog, msfudge);
    exit(1);
  }

  // ----------------------------

  sageRect hdmovieImageMap;
  hdmovieImageMap.left = 0.0;
  hdmovieImageMap.right = 1.0;
  hdmovieImageMap.bottom = 0.0;
  hdmovieImageMap.top = 1.0;

  sailConfig scfg;
  // Search for a configuration file
  char *tmpconf = getenv("SAGE_APP_CONFIG");
  if (tmpconf) {
    SAGE_PRINTLOG("Bitplay> found SAGE_APP_CONFIG variable: [%s]", tmpconf);
    scfg.init(tmpconf);
  }
  else {
    SAGE_PRINTLOG("Bitplay> using default bitplayer.conf");
    scfg.init((char*)"bitplayer.conf");
  }

  scfg.setAppName((char*)"bitplayer");
  scfg.rank = 0;
  scfg.totalFrames = totalframes;
  scfg.asyncUpdate = true;

  int stereomovie = mvlist[0].flags & BPF_STEREO;
  if (stereomovie)
    fprintf(stderr, "It's a Stereo Movie !!!\n");

  if (stereomovie && (stereo == MONO))
    scfg.resX = mvlist[0].nxtile*mvlist[0].xtile/2;
  else
    scfg.resX = mvlist[0].nxtile*mvlist[0].xtile;

  scfg.resY      = mvlist[0].nytile*mvlist[0].ytile;
  scfg.imageMap  = hdmovieImageMap;
  scfg.rowOrd    = BOTTOM_TO_TOP;
  scfg.nwID      = nwID;
  scfg.winWidth  = scfg.resX;
  scfg.winHeight = scfg.resY;

  if (stereomovie && (stereo == CROSSEYED)) {
    scfg.winWidth *= 2; // display it twice as wide as a mono movie
  }

  /*button *homeBtn = new button;
    homeBtn->setUpImage("images/prev2.png");
    homeBtn->setSize(100,100);
    homeBtn->setTransparency(150);

    button *endBtn = new button;
    endBtn->setUpImage("images/next2.png");
    endBtn->setSize(100,100);
    endBtn->alignX(LEFT, 25);
    endBtn->setTransparency(150);

    button *ffBtn = new button;
    ffBtn->setUpImage("images/ff2.png");
    ffBtn->setSize(100,100);
    ffBtn->alignX(LEFT, 25);
    ffBtn->setTransparency(150);

    button *rwBtn = new button;
    rwBtn->setUpImage("images/rw2.png");
    rwBtn->setSize(100,100);
    rwBtn->alignX(LEFT, 25);
    rwBtn->setTransparency(150);*/

  button *pauseBtn = new button;
  pauseBtn->setSize(100,100);
  //pauseBtn->alignX(LEFT, 25);
  pauseBtn->align(CENTER, BOTTOM);
  pauseBtn->setUpImage("images/pause2.png");
  pauseBtn->setDownImage("images/play2.png");
  pauseBtn->setToggle(true);
  pauseBtn->onDown(&playpause);
  pauseBtn->onUp(&playpause);
  pauseBtn->setTransparency(150);

  // main sizer
  /*sizer *mainSizer = new sizer(HORIZONTAL_SIZER);
    mainSizer->addChild(homeBtn);
    mainSizer->addChild(rwBtn);
    mainSizer->addChild(pauseBtn);
    mainSizer->addChild(ffBtn);
    mainSizer->addChild(endBtn);
    mainSizer->align(CENTER, CENTER, 5,5);

    // bottom panel
    panel *bottomPanel = new panel(37,37,37);
    bottomPanel->align(CENTER, BOTTOM,0,0);
    bottomPanel->setSize(650, 130);
    bottomPanel->fitInWidth(false);
    bottomPanel->setTransparency(150);
    bottomPanel->addChild(mainSizer);*/
  //sageInf.addWidget(bottomPanel);
  sageInf.addWidget(pauseBtn);


  if (mvlist[0].format == RGB565)
  {
    scfg.pixFmt = PIXFMT_565;
    pixelSize = 2;
  }
  else
    if (mvlist[0].format == RGB888)
    {
      scfg.pixFmt = PIXFMT_888;
      pixelSize = 3;
    }
    else
      if (mvlist[0].format == ABGR8888)
      {
        scfg.pixFmt = PIXFMT_8888;
        pixelSize = 4;
      }
      else
        if (mvlist[0].format == COMPRESSDXT1)
        {
          scfg.pixFmt = PIXFMT_DXT;
          pixelSize = 0.5;
        }
        else {
          fprintf(stderr, "format not supported\n");
          exit(1);
        }


  sageInf.init(scfg);

  rgbBuffer = (unsigned char *)sageInf.getBuffer();
  memset(rgbBuffer, 0, int(scfg.resX*scfg.resY*pixelSize));

  std::cout << "sail initialized :" << scfg.resX << "x" << scfg.resY << std::endl;
  // ----------------------------

  curplay = -1;
  playlistadvance(1);

  startmovie( doloop );
  sageMessage msg;
  int firstClickTime = 0, dir=0;
  double fps, t1, t2;

  while (1)
  {
    t1 = sage::getTime();

    if(!paused)
    {
      redraw();
      idler();
    }

    if (sageInf.checkMsg(msg, false) > 0) {
      char* data = (char*) msg.getData();

      switch (msg.getCode()) {
      case APP_QUIT : {
        std::cout << "Quit by SAGE" << std::endl;
        exit(1);
        break;
      }/*
         case SAGE_EVT_CLICK: {

         float clickX, clickY;
         int clickDeviceId, clickButtonId, clickIsDown, clickEvent;

         // Parse message
         sscanf(data, "%d %f %f %d %d %d",
         &clickDeviceId, &clickX, &clickY,
         &clickButtonId, &clickIsDown, &clickEvent);

         if(clickIsDown && clickButtonId==1)
         {
         if(!paused)
         pausemovie();
         else
         resumemovie(doloop);
         }*/

        // stop the ff or rewind
        /*else if(!clickIsDown && clickButtonId==1 && dir!=0)
          {
          dir = 0;
          bpio_t *bpio = &playlist[curplay];
          bpforward( bpio, playfwd * skipby );
          bpsync( bpio );
          resumemovie( doloop );
          }*/

        // break;
        //}
        /*case SAGE_EVT_PAN: {
          int panDeviceId;
          float panX, panY, panDX, panDY, panDZ;
          sscanf(data, "%d %f %f %f %f %f",
          &panDeviceId, &panX, &panY, &panDX, &panDY, &panDZ);

          bpio_t *bpio = &playlist[curplay];

          // if we weren't ff or rewinding OR the direction changed...
          if (dir == 0 || (panDX < 0 && dir > 0) || (panDX > 0 && dir < 0))
          {
          if(panDX < 0)
          dir = -1;
          else
          dir = 1;

          bpforward( bpio, dir * 15 );
          bpsync( bpio );
          resumemovie( doloop );
          }

          break;
          }*/
      }
    }

    //sage::usleep( (1000/defaultrate)*1000 );
    t2 = sage::getTime();
    fps = (1000000.0/(t2-t1));
    if (fps > defaultrate) {
      sage::usleep( (1000000.0/defaultrate) - (t2-t1)  );
    }

  }


  return 0;
}
