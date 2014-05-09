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

#ifndef BPIO_H
#define BPIO_H 1

#define _FILE_OFFSET_BITS  64

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct bpbuf_s bpbuf_t;

typedef struct bpio_s {
  int nfillers; /* readers read from disk -> bufs */
  bpbuf_t *bpb; /* bpb[reader(0..nfillers-1)] */
  int bufsize;  /* matching that of bpb->bufsize */
  int readsize; /* ditto */
  int drain;    /* next bpbuf_t to drain (dynamic, but owned by drain thread) */
  int fwd;    /* bpb->incrpos = fwd * nfillers * bufsize */

} bpio_t;

/* Is someone waiting on our condition-variable?  Why? */
enum business { NO, EMPTY, FULL, PAUSED };

struct bpbuf_s {
  /* Static stuff */
  bpio_t    *bpio;
  int         nbufs;  /* number of buffers per reader */
  int         bufsize;
  int     readsize;
  unsigned char *bufspace;
  unsigned char **bufs; /* page-aligned buffers  bufs[0..nbufs-1] */
  int     bpbno;  /* our index in parent */
  int     fd;   /* file descriptor */

  volatile off_t *curpos; /* curpos[0..nbufs-1] -- filepos whence read */
  volatile off_t wrappos; /* wrap back to poswrap after passing EOF */
  volatile off_t eofpos;  /* here's the nominal EOF point */
  volatile off_t incrpos; /* increment from one read to the next */
  volatile int  doloop; /* automatically loop at EOF? */

  /* Dynamic stuff, held under locks */
  volatile int     fillwaiting; /* is filler waiting? */
  volatile int     drainwaiting; /* is drainer waiting? */
  volatile int     filling; /* is filler thread supposed to be running? */
  volatile int     busy;  /* is filler busy reading? (if "filling" was cleared, has it noticed?) */

  volatile off_t  filepos;/* file offset for next buffer to be filled*/
  volatile int  wp; /* next-buffer-to-be-drained (to display) index */
  volatile int  rp; /* next-buffer-to-be-filled (from file) index */

  pthread_t      bthread;
  pthread_mutex_t  bmut;
  pthread_cond_t   bfillwait; /* filler waiting (due to not-yet-setup or full queue)? */
  pthread_cond_t   bdrainwait; /* drainer wait (due to empty queue)? */

};

extern void bpclose( bpio_t *bpio );
extern int bpopen( bpio_t *bpio, char *fname );
extern int bpbfull( bpbuf_t *bpb );
extern int bpbempty( bpbuf_t *bpb );
extern bpio_t *bpinit( bpio_t *bpio, int nfillers, int bufsize, int readsize, int nbufseach );
extern void bpseek( bpio_t *bpio, off_t pos );
extern unsigned char *bpcurbuf( bpio_t *bpio );
extern off_t bptell( bpio_t *bpio );
extern void bpstart( bpio_t *bpio, int wrap );
extern void bpstop( bpio_t *bpio );
extern void bpforward( bpio_t *bpio, int fwd );
extern void bprange( bpio_t *bpio, off_t from, off_t to );

extern void *bpfiller( void *vbpb );
extern int  bpdrain( bpio_t *bpio, int (*sink)( unsigned char *buf, int nbytes, void *arg ), void *arg );
extern void bpsync( bpio_t *bpio );

#endif
