/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu
 *****************************************************************************/


//A simple test program that pushes pixels to SAGE frame buffer
//written by Byung-il Jeong
//Feb 2005

#include <mpi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int rank   = 0;  // node rank
int nprocs = 0;  // number of nodes

// headers for SAGE
#include "sail.h"
#include "misc.h"
sail sageInf; // sail object


int main(int argc, char *argv[])
{
  int dimX, dimY, Xidx, Yidx;

  // MPI Initialization
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Barrier(MPI_COMM_WORLD);

  int resX = 800;
  int resY = 600;
  if (argc > 2) {
    resX = atoi(argv[1]);
    resY = atoi(argv[2]);
  }

  dimX = nprocs-1;
  dimY = 1;

  if (argc > 4) {
    dimX = atoi(argv[3]);
    dimY = atoi(argv[4]);
  }
  double rate = 40.0;
  if (argc > 5) {
    rate = atof(argv[5]);
  }

  char procname[MPI_MAX_PROCESSOR_NAME];
  int lenproc;
  MPI_Get_processor_name(procname, &lenproc);
  fprintf(stderr, "Processor %2d is machine [%s]\n", rank, procname);

  // control master
  if (rank == 0) {
    sailConfig scfg;
    scfg.init((char*)"checker-mpi.conf");
    scfg.setAppName((char*)"checker-mpi");
    scfg.rank = rank;
    scfg.nodeNum = nprocs-1;
    scfg.master = true;
    scfg.rendering = false;
    scfg.resX = resX;
    scfg.resY = resY;

    sageInf.init(scfg);
    SAGE_PRINTLOG("Sail initialized ");

    MPI_Barrier(MPI_COMM_WORLD);

    while(1) {
      sageMessage msg;
      if (sageInf.checkMsg(msg, false) > 0) {
        switch (msg.getCode()) {
        case APP_QUIT : {
          //sageInf.shutdown();
          int err;
          MPI_Abort(MPI_COMM_WORLD, err);

          // finalize
          MPI_Finalize();
          exit(0);
          break;
        }
        }
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }
  }
  // rendering slaves
  else {
    Xidx = (rank-1)%dimX;
    Yidx = (rank-1)/dimX;

    SAGE_PRINTLOG("Index x=%d y=%d", Xidx, Yidx);

    sageRect checkerImageMap;
    checkerImageMap.left = ((float)Xidx) / ((float)dimX);
    checkerImageMap.right = ((float)Xidx+1.0) / ((float)dimX);
    checkerImageMap.bottom = ((float)Yidx) / ((float)dimY);
    checkerImageMap.top = ((float)Yidx+1.0) / ((float)dimY);;
    SAGE_PRINTLOG("checkerImageMap: %g %g %g %g",
                  checkerImageMap.left, checkerImageMap.right, checkerImageMap.bottom, checkerImageMap.top);

    sailConfig scfg;
    scfg.init((char*)"checker-mpi.conf");
    scfg.setAppName((char*)"checker-mpi");
    scfg.rank = rank;
    scfg.resX = resX;
    scfg.resY = resY;
    scfg.imageMap = checkerImageMap;
    scfg.pixFmt = PIXFMT_888;
    scfg.rowOrd = BOTTOM_TO_TOP;
    scfg.nodeNum = nprocs-1;
    scfg.master = false;
    scfg.rendering = true;

    sageInf.init(scfg);
    SAGE_PRINTLOG("Sail initialized ");

    MPI_Barrier(MPI_COMM_WORLD);

    unsigned char color = rank*40; // offset the color by the rank to show processors
    void *buffer;
    double t1, t2, fps;
    while(1) {

      t1 = sage::getTime();
      buffer = sageInf.getBuffer();
      memset(buffer, color, resX*resY*3);
      sageInf.swapBuffer();
      color = (color + 1) % 256;
      t2 = sage::getTime();
      fps = (1000000.0/(t2-t1));
      if (fps > rate) {
        sage::usleep( (1000000.0/rate) - (t2-t1)  );
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }


    // finalize
    MPI_Finalize();
  }

  // exit
  return EXIT_SUCCESS;
}
