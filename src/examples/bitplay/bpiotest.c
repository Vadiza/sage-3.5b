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

#include "bpio.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

off_t totalbytes = 0;
int nreads = 0;
int interval = 200;
int verbose = 0;
struct timeval then;

int timesink( unsigned char *buf, int nbytes, void *junk )
{
  struct timeval now;
  static struct timeval prev;

  /* if we're too soon, sleep a while */
  if(junk && *(int *)junk) {
    int dmsec;
    gettimeofday(&now, NULL);
    if(prev.tv_sec == 0)
      prev = now;
    dmsec = (now.tv_sec - prev.tv_sec) * 1000 + (now.tv_usec - prev.tv_usec) / 1000;
    dmsec = *(int *)junk - dmsec - 15;  /* -10 ms since that's clock resolution */
    if(dmsec > 0)
      usleep(1000 * dmsec);
    prev = now;
  }
  /* ignore the data, just measure read performance */
  totalbytes += nbytes;
  if(nreads % interval == 0) {
    gettimeofday( &now, NULL );
    if(nreads > 0) {
      double dt = (now.tv_usec - then.tv_usec) * 1e-6 + (now.tv_sec - then.tv_sec);
      if(interval > 0)
        fprintf(stderr, "%.1f ", 1e-6 * totalbytes / dt);
      else
        fprintf(stderr, "%d ", (int)(dt*1000));
      totalbytes = 0;
    }
    then = now;
  }
  nreads++;
  return 0;
}


int
main( int argc, char *argv[] )
{
  bpio_t bpio;
  char *fname = argv[1];
  int nfillers = argc>2 ? atoi(argv[2]) : 10;
  int bufsize = argc>3 ? atoi(argv[3]) : 65536;
  int nbufseach = argc>4 ? atoi(argv[4]) : 8;
  int readsize = bufsize;
  int mstarget = 0;

  if(argc>3 && strchr(argv[3], '/'))
    readsize = atoi(strchr(argv[3], '/')+1);

  if(argc > 5) interval = atoi(argv[5]);
  if(argc > 6) mstarget = atoi(argv[6]);


  if(fname == NULL) {
    fprintf(stderr, "Usage: %s  infilename   [nfillthreads  [bufbytes[/readsize]  [nbufsperthread  [printinterval [mstargetrate]]]]]\n\
Runs concurrently in 'nfillthreads' threads,\n\
each reading chunks of 'bufbytes' into queues of 'nbufsperthread' such buffers,\n\
printing MBytes/sec after every 'printinterval' reads.\n\
Defaults:  %d threads, %d bytes/buffer, %d buffers/thread, printinterval %d\n", argv[0],
            nfillers, bufsize, nbufseach, interval);
    exit(1);
  }

  bpinit( &bpio, nfillers, bufsize, readsize, nbufseach );
  bpopen( &bpio, argv[1] );
  bpstart( &bpio, 0 );

  // sleep(2);
  bpdrain( &bpio, timesink, &mstarget );
  return 0;
}
