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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// headers for SAGE
#include "sail.h"
#include "misc.h"

//
// Streams a YUV buffer (or an image)
//    ./yuv 0 width height fps filename
//

int main(int argc, char *argv[])
{
  double t1, t2, fps;
  char *fn;
  void *pixels = NULL;
  sail sageInf; // sail object
  int nodeID;

  if (argc < 2)
    nodeID = 0;
  else
    nodeID = atoi(argv[1]);

  int resX = 400, resY = 400;
  if (argc >= 4) {
    resX = atoi(argv[2]);
    resY = atoi(argv[3]);
  }
  double rate = 10.0;
  if (argc >= 5) {
    rate = atof(argv[4]);
  }
  if (argc >= 6) {
    fn = strdup(argv[5]);
    FILE *f = fopen(fn, "r");
    pixels = malloc(resX*resY*2);
    memset(pixels, 0, resX*resY*2);
    int nb = fread(pixels, 1, resX*resY*2, f);
    fclose(f);
  }

  std::cout << std::endl << "ResX = " << resX << "  ResY = " << resY << std::endl;

  sailConfig scfg;
  scfg.init("yuv.conf");
  scfg.setAppName("yuv");
  scfg.rank = nodeID;

  scfg.resX = resX;
  scfg.resY = resY;

  sageRect renderImageMap;
  renderImageMap.left = 0.0;
  renderImageMap.right = 1.0;
  renderImageMap.bottom = 0.0;
  renderImageMap.top = 1.0;

  scfg.imageMap = renderImageMap;
  scfg.pixFmt = PIXFMT_YUV;
  scfg.rowOrd = TOP_TO_BOTTOM;
  scfg.master = true;
  scfg.nwID = 1;

  sageInf.init(scfg);
  std::cout << "sail initialized " << std::endl;

  void *buffer = sageInf.getBuffer();
  unsigned char color = 200;

  while(1) {
    t1 = sage::getTime();
    if (pixels)
      memcpy(buffer, pixels, resX*resY*2);
    else
      memset(buffer, color, resX*resY*2);
    sageInf.swapBuffer();
    buffer = sageInf.getBuffer();
    color = (color + 1) % 256;
    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0) {
      switch (msg.getCode()) {
      case APP_QUIT : {
        sageInf.shutdown();
        exit(0);
        break;
      }
      }
    }
    t2 = sage::getTime();
    fps = (1000000.0/(t2-t1));
    if (fps > rate) {
      sage::usleep( (1000000.0/rate) - (t2-t1)  );
    }
  }

  return 0;
}
