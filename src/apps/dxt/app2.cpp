/******************************************************************************
 * Author : Byungil Jeong
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/


//A simple test program that pushes DXT pixels to SAGE frame buffer
//written by Luc Renambot
//Feb 2007

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// headers for SAGE
#include "sail.h"
#include "misc.h"

int main(int argc, char *argv[])
{
  sail sageInf; // sail object
  int nodeID;
  char fn[256];
  FILE *fin;

  nodeID = 0;

  int resX = atoi(argv[1]);
  int resY = atoi(argv[2]);

  GLsizei sz = 8 * (resX / 4) * (resY / 4);

  std::cout << std::endl << "ResX = " << resX << "  ResY = " << resY << std::endl;
  std::cout << "File = " << argv[3] << std::endl;

  sailConfig scfg;
  scfg.init("dxt.conf");
  scfg.setAppName("dxt");
  scfg.rank = nodeID;

  scfg.resX = resX;
  scfg.resY = resY;

  sageRect renderImageMap;
  renderImageMap.left = 0.0;
  renderImageMap.right = 1.0;
  renderImageMap.bottom = 0.0;
  renderImageMap.top = 1.0;

  scfg.imageMap = renderImageMap;
  scfg.pixFmt = PIXFMT_DXT;
  scfg.rowOrd = BOTTOM_TO_TOP;
  scfg.master = true;
  scfg.nwID = 1;

  sageInf.init(scfg);
  std::cout << "sail initialized " << std::endl;

  void *buffer, *pix;

  pix = malloc(sz);
  memset(pix, 0, sz);
  int dummy_w, dummy_h;
  fin = fopen(argv[3], "rb");
  fread(&dummy_w, 1, sizeof(int), fin);
  fread(&dummy_h, 1, sizeof(int), fin);
  fread(pix, 1, sz, fin);
  fclose(fin);


  while(1) {

    buffer = sageInf.getBuffer();
    memcpy(buffer, pix, sz);

    sageInf.swapBuffer();
    sageMessage msg;

    if (sageInf.checkMsg(msg, false) > 0) {
      switch (msg.getCode()) {
      case APP_QUIT : {
        exit(0);
        break;
      }
      }
    }
  }

  return 0;
}
