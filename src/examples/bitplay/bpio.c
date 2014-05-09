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
#define _GNU_SOURCE    1    /* for O_DIRECT from <fcntl.h> */
#define __EXTENSIONS__     1    /* for directio() in Solaris */

#include "bpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <sys/time.h>

extern int verbose;

static bpbuf_t *bpbufinit( bpbuf_t *bpb, bpio_t *bpio, int nbufs )
{
  int pagesize = getpagesize();
  int i, margin, stride;
  unsigned char *p;

  if(bpb == NULL)
    bpb = (bpbuf_t *)malloc( sizeof(bpbuf_t) );
  memset(bpb, 0, sizeof(bpbuf_t));

  bpb->bpio = bpio;
  pthread_mutex_init( &bpb->bmut, NULL );
  pthread_cond_init( &bpb->bfillwait, NULL );
  pthread_cond_init( &bpb->bdrainwait, NULL );
  bpb->fillwaiting = bpb->drainwaiting = 0;
  bpb->filepos = 0LL;   /* probably changed later by bpopen() */
  bpb->wrappos = 0LL;   /* filled in later by bpstart */
  bpb->eofpos  = 0LL;
  bpb->wp = bpb->rp = 0;  /* initially empty */

  bpb->bufsize = bpio->bufsize;
  bpb->readsize = bpio->readsize;
  stride = (bpb->bufsize + pagesize - 1) & ~(pagesize - 1);
  bpb->bufspace = (unsigned char *)malloc( pagesize-1 + nbufs*stride );


  p = bpb->bufspace;
  margin = (unsigned long)p & (pagesize - 1);
  if(margin != 0)
    p += (pagesize - margin);
  /* Now p is page-aligned */
  bpb->bufs = (unsigned char **)malloc( nbufs * sizeof(unsigned char *) );
  bpb->curpos = (off_t *)malloc( nbufs * sizeof(off_t) );
  bpb->nbufs = nbufs;
  for(i = 0; i < nbufs; i++) {
    bpb->bufs[i] = p + i*stride;
    bpb->curpos[i] = -1LL;
  }

  bpb->doloop = 0;
  bpb->fd = -1;
  bpb->filling = 0;

  /* Start the thread.  It will realize that filling==0 and go to sleep.
   */
  pthread_create( &bpb->bthread, NULL, bpfiller, (void *)bpb );

  return bpb;
}

void bpforward( bpio_t *bpio, int fwd )
{
  int i;
  off_t incr = (off_t)bpio->bufsize * (fwd * bpio->nfillers);

  if(incr != bpio->bpb[0].incrpos || bpio->fwd != fwd) {
    off_t here = bptell( bpio );

    for(i = 0; i < bpio->nfillers; i++) {
      bpio->bpb[i].incrpos = incr;
    }
    bpio->fwd = fwd;
    bpseek( bpio, here );
  }

}

bpio_t *bpinit( bpio_t *bpio, int nfillers, int bufsize, int readsize, int nbufseach )
{
  int i;

  if(bpio == NULL)
    bpio = (bpio_t *)malloc( sizeof(bpio_t) );
  memset(bpio, 0, sizeof(bpio_t));
  bpio->nfillers = nfillers;
  bpio->bufsize = bufsize;
  bpio->readsize = readsize;
  bpio->bpb = (bpbuf_t *)malloc( nfillers * sizeof(bpbuf_t) );
  for(i = 0; i < nfillers; i++)
    bpbufinit( &bpio->bpb[i], bpio, nbufseach );
  bpforward( bpio, 1 );
  return bpio;
}

int bpbempty( bpbuf_t *bpb )
{
  return bpb->wp == bpb->rp;
}

int bpbfull( bpbuf_t *bpb )
{
  return (bpb->wp+1) % bpb->nbufs == bpb->rp;
}

void bpbstop( bpbuf_t *bpb )
{
  bpb->filling = 0;
  if(bpb->fillwaiting)
    pthread_cond_signal( &bpb->bfillwait );
  if(bpb->drainwaiting)
    pthread_cond_signal( &bpb->bdrainwait );
}

/* file-reader (bpb-queue-filler) thread */
void *bpfiller( void *vbpb )
{
  bpbuf_t *bpb = (bpbuf_t *)vbpb;
  int want;
  unsigned char *p;
  off_t filepos;
  int normaleof;

  pthread_mutex_lock( &bpb->bmut );

  for(;;) {

    /* Can we run now? */
    while(!bpb->filling || bpbfull(bpb)) {
      /* No, sleep until told to start and have room to put data */
      bpb->fillwaiting = 1;
      bpb->busy = 0;
      pthread_cond_wait( &bpb->bfillwait, &bpb->bmut );
      bpb->fillwaiting = 0;
    }


    /* Now we fill bpb->bufs[ bpb->rp ] */
    /* It's all ours now, so run unlocked */

    want = bpb->bufsize;
    p = bpb->bufs[ bpb->rp ];

    if(bpb->filepos >= bpb->eofpos) {
      if(!bpb->doloop || bpb->wrappos >= bpb->eofpos) {
        bpb->filling = 0;
      } else {
        /* Going forwards beyond EOF, need to wrap backwards */
        bpb->filepos = bpb->wrappos + (bpb->filepos - bpb->eofpos);
      }

    } else if(bpb->filepos < bpb->wrappos) {
      if(!bpb->doloop || bpb->wrappos >= bpb->eofpos) {
        bpb->filling = 0;
      } else {
        /* Going backwards, need to wrap forwards */
        bpb->filepos = bpb->eofpos + (bpb->filepos - bpb->wrappos);
      }
    }

    if(bpb->filepos <0||verbose>=3)
      printf("T%02d r%02d w%02d: rpos %02lld -> %lld\n",
             (int) (bpb - bpb->bpio->bpb), bpb->rp, bpb->wp,
             (long long)(bpb->filepos / bpb->bufsize), (long long)bpb->filepos);


    bpb->curpos[ bpb->rp ] = bpb->filepos;
    filepos = bpb->filepos;
    bpb->filepos += bpb->incrpos;
    bpb->busy = 1;

    pthread_mutex_unlock( &bpb->bmut );

    lseek( bpb->fd, filepos, 0 );

    normaleof = 0;
    while(want > 0 && bpb->filling) {
      int now = want < bpb->readsize ? want : bpb->readsize;
      int got = read( bpb->fd, p, now );
      if(got < 0 && errno == EINTR)
        continue;
      if(got <= 0) {
        /* 0-fill remainder of this buffer and stop reading. */
        memset(p, 0, want);
        bpbstop( bpb );
        if(got == 0) normaleof = 1;
        break;

      } else {
        want -= got;
        p += got;
      }
    }

    pthread_mutex_lock( &bpb->bmut );

    if(bpb->filling || normaleof) {
      bpb->rp = (bpb->rp + 1) % bpb->nbufs;
    } else {
      bpb->curpos[ bpb->rp] = -1LL; /* mark buffer as invalid */
      bpb->busy = 0;
    }

    if(bpb->drainwaiting) /* is drainer waiting for data? */
      pthread_cond_signal( &bpb->bdrainwait );
  }

  /* In case anyone ever 'break's from above loop */
  pthread_mutex_unlock( &bpb->bmut );
  return NULL;
}

void bpclose( bpio_t *bpio )
{
  int i;

  for(i = 0; i < bpio->nfillers; i++) {
    bpbuf_t *bpb = &bpio->bpb[i];
    bpbstop( bpb );
    if(bpb->fd >= 0) {
      close(bpb->fd);
      bpb->fd = -1;
    }
  }
}

/*
 * bpopen() opens a file across a collection of buffers.
 */
int bpopen( bpio_t *bpio, char *fname )
{
  int i;

  bpclose( bpio );
  for(i = 0; i < bpio->nfillers; i++) {
    bpbuf_t *bpb = &bpio->bpb[i];
#if defined(O_DIRECT) && !USE_NODIRECT
    bpb->fd = open(fname, O_RDONLY | O_DIRECT); /* try direct I/O if available */
#else
    bpb->fd = open(fname, O_RDONLY);
#endif
    if(bpb->fd < 0) {
      perror(fname);
      return -1;
    }
    /* Request direct I/O in Solaris' way too, but don't worry if we can't */
#ifdef DIRECTIO_ON
    directio( bpb->fd, DIRECTIO_ON );
#endif

  }
  bprange( bpio, (off_t) 0, (off_t) lseek( bpio->bpb[0].fd, 0, SEEK_END ) );
  bpseek( bpio, (off_t) 0 );
  return 0;
}

/* return pointer to current buffer */
unsigned char *bpcurbuf( bpio_t *bpio )
{
  bpbuf_t *bpb;
  if(bpio == NULL || (unsigned int)bpio->drain >= bpio->nfillers)
    return NULL;
  bpb = &bpio->bpb[ bpio->drain ];
  if(bpbempty(bpb) || bpb->bufs == NULL)
    return NULL;
  return bpb->bufs[ bpb->wp ];
}

/* Return file offset of current (drain'th) buffer. */
/* Can be -1 if not read yet. */
off_t bptell( bpio_t *bpio )
{
  bpbuf_t *bpb = &bpio->bpb[ bpio->drain ];
  return bpb->curpos[ bpb->wp ];
}

void bpseek( bpio_t *bpio, off_t pos )
{
  int i, b;

  if(verbose >= 3) printf("S %02lld -> %lld\n", (long long)(pos / bpio->bufsize), (long long)pos);

  bpstop( bpio );
  bpsync( bpio );   /* wait until all is stable */

  bpio->drain = 0;    /* next-to-drain is first reader */
  for(i = 0; i < bpio->nfillers; i++) {
    bpbuf_t *bpb = &bpio->bpb[i];

    pthread_mutex_lock( &bpb->bmut );

    bpbstop( bpb );

    bpb->rp = bpb->wp = 0;
    for(b = 0; b < bpb->nbufs; b++)
      bpb->curpos[b] = -1LL;

    bpb->filepos = pos + bpio->bufsize * (bpio->fwd * i);

    if(bpb->eofpos > bpb->wrappos) {
      while(bpb->filepos > bpb->eofpos)
        bpb->filepos -= bpb->eofpos - bpb->wrappos;
      while(bpb->filepos < bpb->wrappos)
        bpb->filepos += bpb->eofpos - bpb->wrappos;
    }

    /* we changed buffer pointers -- awaken any sleepers */
    if(bpb->drainwaiting)
      pthread_cond_signal( &bpb->bdrainwait );
    if(bpb->fillwaiting)
      pthread_cond_signal( &bpb->bfillwait );

    pthread_mutex_unlock( &bpb->bmut );
  }
}

void bprange( bpio_t *bpio, off_t from, off_t to )
{
  int i;
  for(i = 0; i < bpio->nfillers; i++) {
    bpbuf_t *bpb = &bpio->bpb[i];
    if(from != -1LL) bpb->wrappos = from;
    if(to != -1LL) bpb->eofpos = to;
    bpbstop( bpb );
  }
}

void bpstart( bpio_t *bpio, int wrap )
{
  int i;

  for(i = 0; i < bpio->nfillers; i++) {
    bpbuf_t *bpb = &bpio->bpb[i];
    bpb->doloop = wrap;
    bpb->filling = 1;
    if(bpb->fillwaiting)
      pthread_cond_signal( &bpb->bfillwait );
  }
}

/*
 * If we're supposed to be stopped,
 * wait until all filler threads have noticed that.
 */
void bpsync( bpio_t *bpio )
{
  int i, some;
  int around = 0;

  do {
    some = 0;
    for(i = 0; i < bpio->nfillers; i++) {
      bpbuf_t *bpb = &bpio->bpb[i];
      pthread_mutex_lock( &bpb->bmut );
      if(!bpb->filling && bpb->busy)
        some = 1;
      pthread_mutex_unlock( &bpb->bmut );
    }
    if(some)
      usleep(50000);
    around += some;
  } while(some);
  if(verbose>=2) printf("sync %d\n", around);
}

void bpstop( bpio_t *bpio )
{
  int i;
  for(i = 0; i < bpio->nfillers; i++)
    bpbstop( &bpio->bpb[i] );
}


int bpdrain( bpio_t *bpio, int (*sink)( unsigned char *, int, void * ), void *arg )
{
  int status;

  for(;;) {
    bpbuf_t *bpb = &bpio->bpb[ bpio->drain ];

    if(!bpb->filling) /* if EOF or bpstop() or etc. */
      break;

    pthread_mutex_lock( &bpb->bmut );
    while(bpbempty( bpb )) {
      bpb->drainwaiting = 1;
      pthread_cond_wait( &bpb->bdrainwait, &bpb->bmut );
      bpb->drainwaiting = 0;
    }
    pthread_mutex_unlock( &bpb->bmut );

    /* Make use of bpb->bufs[ bpb->wp ] someday */

    status = (*sink)( bpb->bufs[ bpb->wp ], bpb->bufsize, arg );
    if(status)
      return status;

    pthread_mutex_lock( &bpb->bmut );
    bpb->wp = (bpb->wp + 1) % bpb->nbufs;

    if(bpb->fillwaiting)
      pthread_cond_signal( &bpb->bfillwait );
    pthread_mutex_unlock( &bpb->bmut );

    bpio->drain = (bpio->drain+1) % bpio->nfillers;
  }

  return 0;
}
