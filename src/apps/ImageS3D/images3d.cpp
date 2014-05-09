/*****************************************************************************************
 * images3d: loads an image sequence and sends it to SAGE for display
 *
 * Copyright (C) 2012 Electronic Visualization Laboratory,
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
 * Direct questions, comments etc to http://sagecommons.org
 *
 * Author: Luc Renambot
 *
 *****************************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <wand/magick-wand.h>
#include <libgen.h>


// headers for SAGE
#include "sail.h"
#include "misc.h"
#include "appWidgets.h"

#define STEREO_LEFT_FIRST 0
#define STEREO_RIGHT_FIRST  1
#define MONO      2

//int bpp = 3;
//sagePixFmt aPixFMT = PIXFMT_888;

int bpp = 6;
sagePixFmt aPixFMT = PIXFMT_RGBS3D;


// SAGE UI API
#include "suil.h"
suil uiLib;
int NeedRender = 0;

typedef unsigned char byte;
#define memalign(x,y) malloc((y))

// SAGE object
sail sageInf; // sail object
byte *rgbBuffer = NULL;
unsigned int width, height;
int stride;
byte *file_pixels;
char *filename;
int renderMode;

// Widgets
char stereoString[256];


// Render function: combines two pics into one
void render();



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


// -----------------------------------------------------------------------------


MagickWand* getWand(std::string fileName, unsigned int &w, unsigned int &h)
{
  // use ImageMagick to read all other formats
  MagickBooleanType status;
  MagickWand *wand;

  // read file
  wand=NewMagickWand();
  status=MagickReadImage(wand, fileName.data());
  if (status == MagickFalse)
    ThrowWandException(wand);

  // get the image size
  w = MagickGetImageWidth(wand);
  h = MagickGetImageHeight(wand);

  return wand;
}

byte * getRGB(std::string fileName, unsigned int &w, unsigned int &h)
{
  // use ImageMagick to read all other formats
  MagickBooleanType status;
  MagickWand *wand;

  // read file
  wand=NewMagickWand();
  status=MagickReadImage(wand, fileName.data());
  if (status == MagickFalse)
    ThrowWandException(wand);

  // get the image size
  w = MagickGetImageWidth(wand);
  h = MagickGetImageHeight(wand);

  byte *rgb = (byte*)malloc(w*h*3);
  memset(rgb, 0, w*h*3);

  // get the pixels
  //MagickGetImagePixels(wand, 0, 0, w, h, "RGB", CharPixel, rgb);
  MagickExportImagePixels(wand, 0, 0, w, h, "RGB", CharPixel, rgb);
  DestroyMagickWand(wand);

  return rgb;
}

// -----------------------------------------------------------------------------

void* readThread(void *args)
{
  suil *uiLib = (suil *)args;

  while(1) {
    sageMessage msg;
    msg.init(READ_BUF_SIZE);
    uiLib->rcvMessageBlk(msg);
    if ( msg.getCode() == (SAGE_DISPLAY_INFO) ) {
      int ret;
      int nbtiles, tileX, tileY, totalW, totalH, tileW, tileH, id;
      ret = sscanf((char*)msg.getData(), "%d %d %d\n%d %d\n%d %d %d\n",
                   &nbtiles, &tileX, &tileY, &totalW, &totalH, &tileW, &tileH, &id);
      if (ret == 8) {
        SAGE_PRINTLOG("Wall info: %d %d %d - %d %d - %d %d %d",
                      nbtiles, tileX, tileY, totalW, totalH, tileW, tileH, id);
      } else {
        SAGE_PRINTLOG("Wall info: parsing error [%s]", msg.getData());
      }
    }
    else if ( msg.getCode() == (APP_INFO_RETURN) ) {
      char appname[256], cmd[256];
      int ret;
      int appID, left, right, bottom, top, sailID, zValue;
      ret = sscanf((char*)msg.getData(), "%s %d %d %d %d %d %d %d",
                   appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);
      if (ret == 8) {
        if (sageInf.getWinID() == appID) {
          int x,y,w,h;
          x = left;
          y = bottom;
          w = right-left;
          h = top-bottom;
          SAGE_PRINTLOG("Position: %d x %d - %d x %d - %d", x,y,w,h, zValue);
          //NeedRender = 1;
        }
      }
    }
  }

  return NULL;
}


// -----------------------------------------------------------------------------

void onHomeBtn(int eventId, button *btnObj)
{
  renderMode = STEREO_LEFT_FIRST;
  SAGE_PRINTLOG("Render mode: STEREO_LEFT_FIRST");
  NeedRender = 1;
}
void onEndBtn(int eventId, button *btnObj)
{
  renderMode = STEREO_RIGHT_FIRST;
  SAGE_PRINTLOG("Render mode: STEREO_RIGHT_FIRST");
  NeedRender = 1;
}
void onNextBtn(int eventId, button *btnObj)
{
  renderMode = MONO;
  SAGE_PRINTLOG("Render mode: MONO");
  NeedRender = 1;
}


// -----------------------------------------------------------------------------

void makeWidgets()
{
  double mult = 1.5;
  double bs = 60 * stride / 3840;  // proportional to image width

  thumbnail *homeBtn = new thumbnail;
  homeBtn->setUpImage("images/stereo-lr.png");
  homeBtn->setSize(bs,bs);
  homeBtn->setScaleMultiplier(mult);
  homeBtn->setTransparency(80);
  homeBtn->alignX(CENTER, 10);
  homeBtn->onUp(&onHomeBtn);

  thumbnail *endBtn = new thumbnail;
  endBtn->setUpImage("images/stereo-rl.png");
  endBtn->setSize(bs,bs);
  endBtn->setScaleMultiplier(mult);
  endBtn->setTransparency(80);
  endBtn->alignX(CENTER, 10);
  endBtn->onUp(&onEndBtn);

  thumbnail *nextBtn = new thumbnail;
  nextBtn->setUpImage("images/stereo-mono.png");
  nextBtn->setSize(bs,bs);
  nextBtn->setScaleMultiplier(mult);
  nextBtn->setTransparency(80);
  nextBtn->alignX(CENTER, 10);
  nextBtn->onUp(&onNextBtn);

  sizer *s = new sizer(HORIZONTAL);
  s->align(CENTER, CENTER, 4,4);
  s->addChild(homeBtn);
  s->addChild(nextBtn);
  s->addChild(endBtn);

  label *fileLabel = new label;
  fileLabel->setSize(bs/2,bs/2);
  fileLabel->setLabel(basename(filename), 30);
  fileLabel->alignY(CENTER);
  fileLabel->setBackgroundColor(255,255,255);
  fileLabel->setFontColor(230,230,230);

  panel *p = new panel(37,37,37);
  p->align(CENTER, BOTTOM_OUTSIDE,5,4);
  p->setSize(stride, 30);
  p->fitInWidth(false);
  p->setTransparency(150);
  p->addChild(fileLabel);
  p->addChild(s);
  sageInf.addWidget(p);
}

// -----------------------------------------------------------------------------

void render()
{
  switch (renderMode) {
  case STEREO_LEFT_FIRST:
    for (int i=0;i<height;i++) {
      for (int j=0;j<stride;j++) {
        rgbBuffer[i*stride*bpp + j*bpp + 0] = file_pixels[i*width*3 + j*3 + 0]; // Red   left
        rgbBuffer[i*stride*bpp + j*bpp + 1] = file_pixels[i*width*3 + j*3 + 1]; // Green left
        rgbBuffer[i*stride*bpp + j*bpp + 2] = file_pixels[i*width*3 + j*3 + 2]; // Blue  left
        rgbBuffer[i*stride*bpp + j*bpp + 3] = file_pixels[i*width*3 + stride*3 + j*3 + 0]; // Red   right
        rgbBuffer[i*stride*bpp + j*bpp + 4] = file_pixels[i*width*3 + stride*3 + j*3 + 1]; // Green right
        rgbBuffer[i*stride*bpp + j*bpp + 5] = file_pixels[i*width*3 + stride*3 + j*3 + 2]; // Blue  right
      }
    }
    break;

  case STEREO_RIGHT_FIRST:
    for (int i=0;i<height;i++) {
      for (int j=0;j<stride;j++) {
        rgbBuffer[i*stride*bpp + j*bpp + 3] = file_pixels[i*width*3 + j*3 + 0]; // Red   left
        rgbBuffer[i*stride*bpp + j*bpp + 4] = file_pixels[i*width*3 + j*3 + 1]; // Green left
        rgbBuffer[i*stride*bpp + j*bpp + 5] = file_pixels[i*width*3 + j*3 + 2]; // Blue  left
        rgbBuffer[i*stride*bpp + j*bpp + 0] = file_pixels[i*width*3 + stride*3 + j*3 + 0]; // Red   right
        rgbBuffer[i*stride*bpp + j*bpp + 1] = file_pixels[i*width*3 + stride*3 + j*3 + 1]; // Green right
        rgbBuffer[i*stride*bpp + j*bpp + 2] = file_pixels[i*width*3 + stride*3 + j*3 + 2]; // Blue  right
      }
    }
    break;

  case MONO:
    for (int i=0;i<height;i++) {
      for (int j=0;j<stride;j++) {
        rgbBuffer[i*stride*bpp + j*bpp + 0] = file_pixels[i*width*3 + j*3 + 0]; // Red   left
        rgbBuffer[i*stride*bpp + j*bpp + 1] = file_pixels[i*width*3 + j*3 + 1]; // Green left
        rgbBuffer[i*stride*bpp + j*bpp + 2] = file_pixels[i*width*3 + j*3 + 2]; // Blue  left
        rgbBuffer[i*stride*bpp + j*bpp + 3] = file_pixels[i*width*3 + j*3 + 0]; // Red   left
        rgbBuffer[i*stride*bpp + j*bpp + 4] = file_pixels[i*width*3 + j*3 + 1]; // Green left
        rgbBuffer[i*stride*bpp + j*bpp + 5] = file_pixels[i*width*3 + j*3 + 2]; // Blue  left
      }
    }
    break;
  }

}

// -----------------------------------------------------------------------------


int main(int argc,char **argv)
{
  // parse command line arguments
  if (argc < 2){
    fprintf(stderr, "\n\nUSAGE: image3d [-lr|rl] file");
    return 0;
  }
  else if (argc == 2) {
    renderMode = STEREO_LEFT_FIRST;
    filename = strdup(argv[1]);
  }
  else if (argc == 3) {
    if (!strcmp(argv[1], "-lr"))
      renderMode = STEREO_LEFT_FIRST;
    else if (!strcmp(argv[1], "-rl"))
      renderMode = STEREO_RIGHT_FIRST;
    else
      renderMode = STEREO_LEFT_FIRST;
    filename = strdup(argv[2]);
  } else {
    fprintf(stderr, "\n\nUSAGE: image3d [-lr|rl] file");
    return 0;
  }


  file_pixels = getRGB(filename,width,height);
  stride = width/2;

  // initialize SAIL
  sage::initUtil();
  sailConfig scfg;

  // Search for a configuration file
  char *tmpconf = getenv("SAGE_APP_CONFIG");
  if (tmpconf) {
    SAGE_PRINTLOG("Stereo3D> found SAGE_APP_CONFIG variable: [%s]", tmpconf);
    scfg.init(tmpconf);
  }
  else {
    SAGE_PRINTLOG("Stereo3D> using default Stereo3D.conf");
    scfg.init((char*)"stereo3d.conf");
  }
  scfg.setAppName((char*)"stereo3d");

  scfg.resX = stride;
  scfg.resY = height;

  // if it hasn't been specified by the config file, use the app-determined size
  if (scfg.winWidth == -1 || scfg.winHeight == -1) {
    scfg.winWidth = stride;
    scfg.winHeight = height;
    SAGE_PRINTLOG("Setting wxh : %d x %d", scfg.winWidth, scfg.winHeight);
  }

  scfg.pixFmt = aPixFMT;
  scfg.rowOrd = TOP_TO_BOTTOM;

  rgbBuffer = (byte*)malloc(stride*height*bpp);
  memset(rgbBuffer, 0, stride*height*bpp);

  // make the widgets
  makeWidgets();

  // Render the picture
  render();


  // Initialization
  sageInf.init(scfg);

  uiLib.init((char*)"fsManager.conf");
  uiLib.connect(NULL);
  uiLib.sendMessage(SAGE_UI_REG,(char*)" ");
  pthread_t thId;
  if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0){
    SAGE_PRINTLOG("Cannot create a UI thread\n");
  }


  byte* sageBuffer;
  // get buffer from SAGE and fill it with data
  sageBuffer = (byte*)sageInf.getBuffer();
  memcpy(sageBuffer, rgbBuffer, stride*height*bpp);
  sageInf.swapBuffer();

  sageBuffer = (byte*)sageInf.getBuffer();
  memcpy(sageBuffer, rgbBuffer, stride*height*bpp);
  sageInf.swapBuffer();

  // Wait the end
  sageMessage msg;
  int neww, newh, d1, d2;
  char *content;
  MagickBooleanType res;
  while (1)
  {
    // Process SAGE messages
    if (sageInf.checkMsg(msg, false) > 0) {
      switch (msg.getCode()) {
      case APP_QUIT:
        SAGE_PRINTLOG("Done");
        sageInf.shutdown();
        goto bail;
        break;
      default:
        SAGE_PRINTLOG("Message: code %d", msg.getCode());
        break;
      }
    }
    if (NeedRender) {
      render();
      sageBuffer = (byte*)sageInf.getBuffer();
      memcpy(sageBuffer, rgbBuffer, stride*height*bpp);
      sageInf.swapBuffer();
      sageBuffer = (byte*)sageInf.getBuffer();
      memcpy(sageBuffer, rgbBuffer, stride*height*bpp);
      sageInf.swapBuffer();

      NeedRender = 0;
    }
    else
    sage:usleep(10000);

  }

  bail:
  // Release the pixel memory
  free(file_pixels);

  // Release the pixel memory
  free(rgbBuffer);


  return 0;
}
