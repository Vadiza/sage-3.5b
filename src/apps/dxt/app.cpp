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
#include "dxt.h"
#include <wand/magick_wand.h>

// headers for SAGE
#include "sail.h"
#include "misc.h"

#define ThrowWandException(wand)                                        \
  {                                                                     \
    char                                                                \
      *description;                                                     \
                                                                        \
    ExceptionType                                                       \
      severity;                                                         \
                                                                        \
    description=MagickGetException(wand,&severity);                     \
    (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description);           \
    exit(-1);                                                           \
  }


int main(int argc, char *argv[])
{
  sail sageInf; // sail object
  int nodeID;

  byte *in;
  int nbbytes;

  nodeID = 0;

  /*
    Read an image.
  */
  MagickBooleanType status;
  MagickWand *magick_wand;

  int count = 1;
  int framenb = atoi(argv[2]);
  char fn[256];

  memset(fn, 0, 256);
  sprintf(fn, "%s/%08d.tga", argv[1], count++);

  magick_wand=NewMagickWand();
  status=MagickReadImage(magick_wand, fn);
  if (status == MagickFalse)
    ThrowWandException(magick_wand);

  unsigned long width = MagickGetImageWidth(magick_wand);
  unsigned long height = MagickGetImageHeight(magick_wand);

  int resX = width;
  int resY = height;

  std::cout << "File = " << argv[1] << std::endl;
  std::cout << std::endl << "ResX = " << resX << "  ResY = " << resY << std::endl;

  in = (byte*)malloc(width*height*4);
  memset(in, 0, width*height*4);

  fprintf(stderr, "Converting to raw: %ldx%ld\n", width, height);
  MagickGetImagePixels(magick_wand, 0, 0, width, height, "RGBA", CharPixel, in);

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
  //scfg.rowOrd = TOP_TO_BOTTOM;
  scfg.rowOrd = BOTTOM_TO_TOP;
  scfg.master = true;
  scfg.nwID = 1;

  sageInf.init(scfg);
  std::cout << "sail initialized " << std::endl;


  void *buffer = sageInf.getBuffer();
  nbbytes = 0;
  CompressImageDXT1( in, (byte*)buffer, width, height, nbbytes);

  sageInf.swapBuffer();
  buffer = sageInf.getBuffer();


  while(1) {
    fprintf(stderr, "Frame %d\n", count);
    memset(fn, 0, 256);
    sprintf(fn, "%s/%08d.tga", argv[1], count);
    count = (count + 1) % framenb;
    if (count == 0) count = 1;

    MagickRemoveImage(magick_wand);
    status=MagickReadImage(magick_wand, fn);
    if (status == MagickFalse)
      ThrowWandException(magick_wand);
    MagickGetImagePixels(magick_wand, 0, 0, width, height, "RGBA", CharPixel, in);

    nbbytes = 0;
    CompressImageDXT1( in, (byte*)buffer, width, height, nbbytes);
#if 0
    memset(fn, 0, 256);
    sprintf(fn, "%s/%08d.dxt", argv[1], count);
    FILE *f=fopen(fn, "w+");
    int res=fwrite(buffer, 1, nbbytes, f);
    fclose(f);
#endif
    sageInf.swapBuffer();
    buffer = sageInf.getBuffer();

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

  }

  magick_wand=DestroyMagickWand(magick_wand);
  MagickWandTerminus();

  return 0;
}
