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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

float dt() {
  static struct timeval then;
  struct timeval now;
  float delta;

  gettimeofday( &now, NULL );
  if(then.tv_sec == 0)
    then = now;
  delta = now.tv_sec - then.tv_sec + 1e-6*(now.tv_usec - then.tv_usec);
  then = now;
  return delta;
}

int main(int argc, char *argv[]) {
  int pagesize = getpagesize();
  int pagemask = (pagesize - 1);
  char *fname = argv[1];
  int bufsize = ( argc>2 ? atoi(argv[2]) : 128 ) * 1024;
  char *buf = (char *)malloc( bufsize + pagesize );
  int count = argc>3 ? atoi( argv[3] ) : 1024;
  int interval = argc>4 ? atoi(argv[4]) : 32;
  int offset = 0;
  int skip = 0;
  float total = 0;
  int i, margin, fd;
  char *p;

  if(argc <= 1) {
    fprintf(stderr, "Usage: %s [-]infilename  kbyteseach  totalbufs  printinterval\n\
Read from infilename, in buffers of kbyteseach*1024 bytes (rounded up to 4K page size).\n\
Read totalbufs times, reporting average MBytes/sec every printinterval times,\n\
and average transfer rate at end of run.\n", argv[0]);
    exit(1);
  }

  if(argc>5)
    sscanf(argv[5], "%d%*c%d", &offset, &skip);

  bufsize = (bufsize + pagesize - 1) & ~pagemask;

  offset = offset * bufsize;

#ifdef sun
  fd = open(fname, O_RDONLY);
  if (fname[0]=='-')
    directio(fd, DIRECTIO_ON);
#else
  fd = fname[0]=='-' ? open(fname+1, O_RDONLY | O_DIRECT) : open(fname, O_RDONLY);
#endif
  if(fd < 0) {
    perror(fname[0]=='-' ? fname+1 : fname);
    exit(1);
  }

  lseek( fd, offset, SEEK_SET );

  margin = (buf - (char *)NULL) & pagemask;
  p = buf - margin;

  dt();
  for(i = 0; i < count; i++) {
    if(read(fd, p, bufsize) < bufsize) {
      perror("read");
      break;
    }
    if(skip > 1)
      lseek(fd, (skip-1)*bufsize, SEEK_CUR);

    if(interval>0 && (i+1)%interval == 0) {
      float deltat = dt();
      total += deltat;
      printf("%6.2f ", interval * bufsize / (1048576 * deltat));
      fflush(stdout);
    }
  }
  total += dt();
  if(interval > 0)
    printf("\n");
  printf("%.2f\n", i * bufsize / (1048576 * total));
  return 0;
}
