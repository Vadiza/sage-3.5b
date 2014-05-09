/*
 *  video_out_sage.c
 *
 *  Copyright (C) Aaron Holtzman - June 2000
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *
 *  mpeg2dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  mpeg2dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
extern "C" {
#include "config.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "sub/sub.h"
#include "sub/osd.h"
#include "osdep/keycodes.h"
#include "mp_fifo.h"
#include "mp_core.h"
#include "subopt-helper.h"
}

// headers for SAGE
#include "sail.h"
#include "misc.h"
#include "appWidgets.h"

// sail object
sail sageInf;
unsigned char *rgbBuffer = NULL;
int sage_ready = 0;
static int int_muted;
static int int_pause;

//////////////////////////////////////////////////////////////
//#include "SageAudioClient.h"
#include "sam_client.h"


// Access to the SAM audio plugin object
extern sam::StreamingAudioClient *sac;
//////////////////////////////////////////////////////////////


static const vo_info_t info =
  {
    "SAGE video output",
    "sage",
    "Luc RENAMBOT",
    ""
  };

extern "C" {
  extern const LIBVO_EXTERN(sage)
}


//////////////////////////////////////////////////////////////
// Hack to get window position   /////////////////////////////
//////////////////////////////////////////////////////////////

// SAGE UI API
#include "suil.h"
suil uiLib;


void* readThread(void *args)
{
  suil *uiLib = (suil *)args;

  while(1) {
    sageMessage msg;
    msg.init(READ_BUF_SIZE);
    uiLib->rcvMessageBlk(msg);
    if ( msg.getCode() == (APP_INFO_RETURN) ) {
      //SAGE_PRINTLOG("UI Message> %d --- %s", msg.getCode(), (char *)msg.getData());
      // format:   decklinkcapture 13 1972 5172 390 2190 1045 0 0 0 0 none 1280 720 1
      char appname[256], cmd[256];
      int ret;
      int appID, left, right, bottom, top, sailID, zValue;
      ret = sscanf((char*)msg.getData(), "%s %d %d %d %d %d %d %d",
                   appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);
      if (ret == 8) {
        if (sageInf.getWinID() == appID) {
          int x,y,w,h;
          x = left;
          y = top;
          w = right-left;
          h = top-bottom;
          if (sac) {
            int ret;
            SAGE_PRINTLOG("My window location and size: %d x %d - %d x %d - %d", x,y,w,h, zValue);
            ret = sac->setPosition(x,y,w,h,zValue);
          }
        }
      }
    }
  }

  return NULL;
}



//////////////////////////////////////////////////////////////



extern float vo_fps; // The original frame rate

static uint32_t image_width, image_height;
static uint32_t image_format;


static int stereo_mode;
static int unsquish;
static int rightfirst;
static int guessformat;
static const opt_t subopts[] = {
  {"stereo",      OPT_ARG_BOOL,  &stereo_mode,  NULL},
  {"unsquish",    OPT_ARG_INT,   &unsquish,     NULL},
  {"rightfirst",  OPT_ARG_BOOL,  &rightfirst,   NULL},
  {"guessformat", OPT_ARG_INT,   &guessformat,  NULL},
  {NULL}
};


static const char help_msg[] =
  "\n-vo sage command line help:\n"
  "Example: mplayer -vo sage:stereo\n"
  "\nOptions:\n"
  "  stereo\n"
  "    will take a side-by-side movie (2*width) and streams it in stereo mode\n"
  "    default: off\n"
  "  unsquish\n"
  "    expend the width to double-width\n"
  "    default: off\n"
  "  rightfirst\n"
  "    flip the stereo\n"
  "    default: off\n"
  "  guessformat\n"
  "    guess stereo format: full-width or half-width\n"
  "    default: on\n"
  ;


static void (*draw_osd_fnc) (int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride);

static void draw_osd_24(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride)
{
  vo_draw_alpha_rgb24(w,h,src,srca,stride,rgbBuffer+3*(y0*image_width+x0),3*image_width);
}
static void draw_osd_yuv(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride)
{
  vo_draw_alpha_uyvy(w,h,src,srca,stride,rgbBuffer+2*(y0*image_width+x0),2*image_width);
}

static int preinit(const char *arg)
{
  // default values for options
  stereo_mode =  0;
  unsquish    = -1;
  rightfirst  =  0;
  guessformat =  1;
  // parse options
  if (subopt_parse(arg, subopts) != 0) {
    mp_msg(MSGT_VO, MSGL_FATAL, help_msg);
    return -1;
  }
  // If user specified 'unsquish', do not guess
  if (unsquish==1 || unsquish==0)
    guessformat = 0;
  // if unsquish wasnt specified, it's set to no
  if (unsquish==-1)
    unsquish = 0;

  return 0;
}

void qfunc(sail *sig)
{
  SAGE_PRINTLOG("MPLAYER> Event APP_QUIT");
  if (sac) delete sac;
  sageInf.shutdown();
  exit_player(EXIT_QUIT);
  _exit(1);
}

void onQuitBtn(int eventId, button *btnObj)
{
  qfunc(NULL);
}

void onInfoBtn(int eventId, button *btnObj)
{
  // OSD control
  mplayer_put_key(KEY_MENU);
}


// mplayer_put_key(KEY_PAUSE);
// mplayer_put_key(KEY_PLAYPAUSE);
// mplayer_put_key(KEY_VOLUME_UP);
// mplayer_put_key(KEY_VOLUME_DOWN);

void frameFunc(int eventId, button * btnObj)
{
  if(eventId == BUTTON_UP) {
    SAGE_PRINTLOG("SAGE: frameFunc");
    mplayer_put_key('.');
  }
}

void restartFunc(int eventId, button * btnObj)
{
  if(eventId == BUTTON_UP) {
    SAGE_PRINTLOG("SAGE: restartFunc");
    mplayer_put_key(KEY_ENTER);
  }
}

void onFlipBtn(int eventId, button * btnObj)
{
  if (rightfirst)
    rightfirst = 0;
  else
    rightfirst = 1;
}

void forwardFunc(int eventId, button * btnObj)
{
  if(eventId == BUTTON_UP) {
    SAGE_PRINTLOG("SAGE: forwardFunc");
    mplayer_put_key(KEY_RIGHT);
  }
}
void backwardFunc(int eventId, button * btnObj)
{
  if(eventId == BUTTON_UP) {
    SAGE_PRINTLOG("SAGE: backwardFunc");
    mplayer_put_key(KEY_ENTER);
    mplayer_put_key(KEY_PAUSE);
  }
}

void muteFunc(int eventId, button * btnObj)
{
  if (int_muted) int_muted = 0; else int_muted = 1;
  if (sac)
    sac->setMute(int_muted);
  else
    mplayer_put_key(KEY_MUTE);
  SAGE_PRINTLOG("SAGE: muteFunc: muted %d", int_muted);
}

void pauseFunc(int eventId, button * btnObj)
{
  if(eventId == BUTTON_UP) {
    SAGE_PRINTLOG("SAGE UP");
    mplayer_put_key(KEY_PAUSE);
  }
  if(eventId == BUTTON_DOWN) {
    SAGE_PRINTLOG("SAGE DOWN");
    mplayer_put_key(KEY_PAUSE);
  }
}


void create_widgets()
{
  // SAGE Widgets
  ///////////////
  double mult = 1.5;
  double bw;
  uint32_t stride;

  if (stereo_mode == 1) {
    if (unsquish == 0) {
      stride = image_width/2;
      bw = 40 * image_width / 1920;  // proportional to image width
    }
    else {
      stride = image_width/2;
      bw = 40 * image_width / 1920;  // proportional to image width
    }
  }
  else {
    stride = image_width;
    bw = 60 * stride / 1920;  // proportional to image width
    //bw = 40;
  }

  thumbnail *pauseBtn = new thumbnail;
  pauseBtn->setSize(bw, bw);
  pauseBtn->align(CENTER, BOTTOM);
  pauseBtn->setUpImage("images/mplayer-icons/mplayer-pause.png");
  pauseBtn->setDownImage("images/mplayer-icons/mplayer-play.png");
  pauseBtn->setToggle(true);
  pauseBtn->onUp(&pauseFunc);
  pauseBtn->onDown(&pauseFunc);
  pauseBtn->setScaleMultiplier(mult);
  pauseBtn->setTransparency(80);

  // thumbnail *restartBtn = new thumbnail;
  // restartBtn->setUpImage("images/mplayer-icons/mplayer-step.png");
  // restartBtn->setSize(bw, bw);
  // restartBtn->setScaleMultiplier(mult);
  // restartBtn->onUp(&restartFunc);
  // restartBtn->setTransparency(50);

  thumbnail *flipBtn;
  if (stereo_mode) {
    flipBtn = new thumbnail;
    flipBtn->setUpImage("images/stereo-lr.png");
    flipBtn->setSize(bw,bw);
    flipBtn->setScaleMultiplier(mult);
    flipBtn->onUp(&onFlipBtn);
    flipBtn->setTransparency(80);
  }
  thumbnail *stepfBtn = new thumbnail;
  stepfBtn->setUpImage("images/mplayer-icons/mplayer-step.png");
  stepfBtn->setSize(bw, bw);
  stepfBtn->setScaleMultiplier(mult);
  stepfBtn->onUp(&forwardFunc);
  stepfBtn->setTransparency(80);

  thumbnail *stepbBtn = new thumbnail;
  stepbBtn->setUpImage("images/mplayer-icons/mplayer-stepback.png");
  stepbBtn->setSize(bw, bw);
  stepbBtn->setScaleMultiplier(mult);
  stepbBtn->onUp(&backwardFunc);
  stepbBtn->setTransparency(80);

  thumbnail *muteBtn = new thumbnail;
  if (int_muted) {
    muteBtn->setDownImage("images/mplayer-icons/mplayer-volume.png");
    muteBtn->setUpImage("images/mplayer-icons/mplayer-mute.png");
  }
  else {
    muteBtn->setUpImage("images/mplayer-icons/mplayer-volume.png");
    muteBtn->setDownImage("images/mplayer-icons/mplayer-mute.png");
  }
  muteBtn->setToggle(true);
  muteBtn->setSize(bw, bw);
  muteBtn->setScaleMultiplier(mult);
  muteBtn->onUp(&muteFunc);
  muteBtn->onDown(&muteFunc);
  muteBtn->alignX(LEFT, 50 * stride / 1920);
  muteBtn->setTransparency(80);

  sizer *asizer = new sizer(HORIZONTAL);
  asizer->align(CENTER, CENTER, 2,2);
  if (stereo_mode)
    asizer->addChild(flipBtn);
  asizer->addChild(stepbBtn);
  asizer->addChild(pauseBtn);
  asizer->addChild(stepfBtn);
  asizer->addChild(muteBtn);

  thumbnail *quitBtn = new thumbnail;
  quitBtn->setUpImage("images/close_over.png");
  quitBtn->setSize(bw,bw);
  quitBtn->setScaleMultiplier(mult);
  // put the quit button in the bottom left corner
  quitBtn->setPos(0,0);
  quitBtn->setTransparency(100);
  quitBtn->onUp(&onQuitBtn);

  thumbnail *infoBtn = new thumbnail;
  infoBtn->setUpImage("images/vertical_divider_btn_up.png");
  infoBtn->setSize(bw,bw);
  infoBtn->setScaleMultiplier(mult);
  // put the quit button in the bottom left corner
  infoBtn->setPos(stride-bw,0);
  infoBtn->setTransparency(100);
  infoBtn->onUp(&onInfoBtn);

  // bottom panel
  panel *bottomPanel = new panel(37,37,37);
  bottomPanel->align(CENTER, BOTTOM_OUTSIDE,0,4);
  bottomPanel->setSize(stride, bw);
  bottomPanel->fitInWidth(false);
  bottomPanel->fitInHeight(false);
  bottomPanel->setTransparency(150);
  bottomPanel->addChild(asizer);
  bottomPanel->addChild(quitBtn);
  bottomPanel->addChild(infoBtn);
  sageInf.addWidget(bottomPanel);

  //sageInf.addWidget(asizer);
}

static int config(uint32_t width, uint32_t height, uint32_t d_width, uint32_t d_height, uint32_t fullscreen, char *title, uint32_t format)
{
  //     Set up the video system. You get the dimensions and flags.
  //     width, height: size of the source image
  //     d_width, d_height: wanted scaled/display size (it's a hint)

  sage::initUtil();

  sailConfig scfg;
  sageRect renderImageMap;

  image_format = format;
  image_width  = width;
  image_height = height;

  int_pause = 0;

  if (vo_wintitle)
    SAGE_PRINTLOG("SAGE: for title [%s]",vo_wintitle);

  //int bpp = IMGFMT_IS_BGR(format)?IMGFMT_BGR_DEPTH(format):IMGFMT_RGB_DEPTH(format);
  //SAGE_PRINTLOG("SAGE: bpp %d", bpp);

  switch (format) {
  case IMGFMT_UYVY:
    if (! sage_ready)
    {
      SAGE_PRINTLOG("SAGE: we're good with yuv");

      // Search for a configuration file
      char *tmpconf = getenv("SAGE_APP_CONFIG");
      if (tmpconf) {
        SAGE_PRINTLOG("MPlayer> found SAGE_APP_CONFIG variable: [%s]", tmpconf);
        scfg.init(tmpconf);
      }
      else {
        SAGE_PRINTLOG("MPlayer> using default mplayer.conf");
        scfg.init((char*)"mplayer.conf");
      }

      scfg.setAppName((char*)"mplayer");
      scfg.rank = 0;

      scfg.resX = image_width;
      scfg.resY = image_height;

      // set the window size to native movie size
      // Test to -1 to keep position from config file
      if (scfg.winWidth == -1 || scfg.winHeight == -1) {
        // using d_width and d_height because of non-square pixels
        scfg.winWidth  = d_width;
        scfg.winHeight = d_height;
      }

      renderImageMap.left = 0.0;
      renderImageMap.right = 1.0;
      renderImageMap.bottom = 0.0;
      renderImageMap.top = 1.0;

      scfg.imageMap = renderImageMap;
      scfg.pixFmt = PIXFMT_YUV;
      scfg.rowOrd = TOP_TO_BOTTOM;

      //
      // set the frame rate - Sungwon
      //
      scfg.frameRate = ceil(vo_fps);
      SAGE_PRINTLOG("MPLAYER> frame rate %d", scfg.frameRate);

      // SAGE Widgets
      create_widgets();

      // SAGE init
      sageInf.init(scfg);
      SAGE_PRINTLOG("MPLAYER> SAGE: sage init done");

      if (rgbBuffer)
        free(rgbBuffer);
      rgbBuffer = (unsigned char*)sageInf.getBuffer();
      memset(rgbBuffer, 0, image_width*image_height*2);
      SAGE_PRINTLOG("MPLAYER> SAGE: create buffer %d %d\n", image_width,image_height);
      sage_ready = 1;

      uiLib.init((char*)"fsManager.conf");
      uiLib.connect(NULL);
      uiLib.sendMessage(SAGE_UI_REG,(char*)" ");
      pthread_t thId;
      if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0){
        SAGE_PRINTLOG("Cannot create a UI thread\n");
      }

      //choose osd drawing function
      draw_osd_fnc=draw_osd_yuv;

      // not muted
      int_muted = 0;
      SAGE_PRINTLOG("SAGE: initialized muted %d", int_muted);
    }
  case IMGFMT_RGB24:
    if ( (! sage_ready) && (stereo_mode==1) )
    {
      SAGE_PRINTLOG("SAGE: we're good with 24bpp stereo");

      // Search for a configuration file
      char *tmpconf = getenv("SAGE_APP_CONFIG");
      if (tmpconf) {
        SAGE_PRINTLOG("MPlayer> found SAGE_APP_CONFIG variable: [%s]", tmpconf);
        scfg.init(tmpconf);
      }
      else {
        SAGE_PRINTLOG("MPlayer> using default mplayer.conf");
        scfg.init((char*)"mplayer.conf");
      }

      scfg.setAppName((char*)"mplayer");
      scfg.rank = 0;

      scfg.resX = image_width/2;
      scfg.resY = image_height;

      // set the window size to native movie size
      // Test to -1 to keep position from config file
      if (scfg.winWidth == -1 || scfg.winHeight == -1) {
        // using d_width and d_height because of non-square pixels
        if (unsquish) {
          scfg.winWidth = d_width;
        }
        else {
          if (guessformat==1) {
            if ( (((double)d_width/(double)d_height)) > 2.6 )
              scfg.winWidth  = d_width/2;
            else
              scfg.winWidth  = d_width;
          } else {
            scfg.winWidth  = d_width/2;
          }
        }
        scfg.winHeight = d_height;
      }

      renderImageMap.left = 0.0;
      renderImageMap.right = 1.0;
      renderImageMap.bottom = 0.0;
      renderImageMap.top = 1.0;

      scfg.imageMap = renderImageMap;
      scfg.pixFmt = PIXFMT_RGBS3D; //PIXFMT_888;
      scfg.rowOrd = TOP_TO_BOTTOM;

      //
      // set the frame rate - Sungwon
      //
      scfg.frameRate = ceil(vo_fps);
      SAGE_PRINTLOG("MPLAYER> frame rate %d", scfg.frameRate);

      // SAGE Widgets
      create_widgets();

      // SAGE init
      sageInf.init(scfg);
      SAGE_PRINTLOG("MPLAYER> SAGE: sage init done");

      if (rgbBuffer)
        free(rgbBuffer);
      rgbBuffer = (unsigned char*)sageInf.getBuffer();
      memset(rgbBuffer, 0, image_width*image_height*3);
      SAGE_PRINTLOG("MPLAYER> SAGE: create buffer %d %d\n", image_width,image_height);
      sage_ready = 1;

      uiLib.init((char*)"fsManager.conf");
      uiLib.connect(NULL);
      uiLib.sendMessage(SAGE_UI_REG,(char*)" ");
      pthread_t thId;
      if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0){
        SAGE_PRINTLOG("Cannot create a UI thread\n");
      }

      //choose osd drawing function
      //choose osd drawing function
      draw_osd_fnc=draw_osd_24;

      // not muted
      int_muted = 0;
      SAGE_PRINTLOG("SAGE: initialized muted %d", int_muted);
    }

    break;
  case IMGFMT_RGBA:
    break;
  case IMGFMT_BGR15:
    break;
  case IMGFMT_BGR16:
    break;
  case IMGFMT_BGR24:
    break;
  case IMGFMT_BGRA:
    break;
  default:
    break;
  }

  if (sage_ready > 1) {
    // If movie loops and was muted, try to keep it muted again
    if (sac)
      sac->setMute(int_muted);
    SAGE_PRINTLOG("SAGE: muted %d", int_muted);
    //mplayer_put_key(KEY_MUTE);
    //mplayer_put_key(KEY_MUTE);
  }
  sage_ready ++;

  return 0;
}

static int query_format(uint32_t format)
{
  //SAGE_PRINTLOG("SAGE: query_format %d", format);

  int caps = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW;

#if 0
  if( (!IMGFMT_IS_RGB(format)) && (!IMGFMT_IS_BGR(format)) ) return 0;
  if (format == IMGFMT_RGB24)
    return caps;
#else
  if (stereo_mode == 1) {
    // For stere mode, we need RGB24 for now
    if (format == IMGFMT_RGB24) {
      //SAGE_PRINTLOG("query_format> since stereo mode, got IMGFMT_RGB24");
      return caps;
    }
  } else {
    if (format == IMGFMT_UYVY) {
      //SAGE_PRINTLOG("query_format> Got IMGFMT_UYVY");
      return caps;
    }
  }
#endif

  return 0;
}

static void draw_osd(void)
{
  //SAGE_PRINTLOG("MPLAYER> OSD");

  if (rgbBuffer && sage_ready) vo_draw_text(image_width,image_height,draw_osd_fnc);
}

static int draw_frame(uint8_t *src[])
{
  //SAGE_PRINTLOG("MPLAYER> DRAW");

  uint8_t *ImageData=src[0];
  if (rgbBuffer && sage_ready)
  {
    //fprintf(stderr, "SAGE: draw_frame %d %d\n", image_width,image_height);
    //memset(rgbBuffer, 0, image_width*image_height*3);
    rgbBuffer = (unsigned char*)sageInf.getBuffer();
    if (image_format==IMGFMT_UYVY) {
      memcpy(rgbBuffer, (unsigned char *)ImageData, image_width*image_height*2);
    }
    else {
      if (image_format==IMGFMT_RGB24 && stereo_mode==1) {
        // memcpy(rgbBuffer, (unsigned char *)ImageData, image_width*image_height*3);
        int stride = image_width/2;
        int bpp = 6;
        if (rightfirst)
          for (int i=0;i<image_height;i++) {
            for (int j=0;j<stride;j++) {
              rgbBuffer[i*stride*bpp + j*bpp + 0] = ImageData[i*image_width*3 + j*3 + 0]; // Red   left
              rgbBuffer[i*stride*bpp + j*bpp + 1] = ImageData[i*image_width*3 + j*3 + 1]; // Green left
              rgbBuffer[i*stride*bpp + j*bpp + 2] = ImageData[i*image_width*3 + j*3 + 2]; // Blue  left
              rgbBuffer[i*stride*bpp + j*bpp + 3] = ImageData[i*image_width*3 + stride*3 + j*3 + 0]; // Red   right
              rgbBuffer[i*stride*bpp + j*bpp + 4] = ImageData[i*image_width*3 + stride*3 + j*3 + 1]; // Green right
              rgbBuffer[i*stride*bpp + j*bpp + 5] = ImageData[i*image_width*3 + stride*3 + j*3 + 2]; // Blue  right
            }
          }
        else
          for (int i=0;i<image_height;i++) {
            for (int j=0;j<stride;j++) {
              rgbBuffer[i*stride*bpp + j*bpp + 3] = ImageData[i*image_width*3 + j*3 + 0]; // Red   left
              rgbBuffer[i*stride*bpp + j*bpp + 4] = ImageData[i*image_width*3 + j*3 + 1]; // Green left
              rgbBuffer[i*stride*bpp + j*bpp + 5] = ImageData[i*image_width*3 + j*3 + 2]; // Blue  left
              rgbBuffer[i*stride*bpp + j*bpp + 0] = ImageData[i*image_width*3 + stride*3 + j*3 + 0]; // Red   right
              rgbBuffer[i*stride*bpp + j*bpp + 1] = ImageData[i*image_width*3 + stride*3 + j*3 + 1]; // Green right
              rgbBuffer[i*stride*bpp + j*bpp + 2] = ImageData[i*image_width*3 + stride*3 + j*3 + 2]; // Blue  right
            }
          }

      } else {
        // not yet implemented
      }
    }
  }

  return 0;
}

static int draw_slice(uint8_t *image[], int stride[], int w,int h,int x,int y)
{
  //SAGE_PRINTLOG("MPLAYER> DRAW SLICE");
  return 0;
}


static void check_events(void)
{
  //SAGE_PRINTLOG("MPLAYER> CHECK EVENT");

  // check for SAGE messages
  // this is where the UI messages will be checked for as well
  if ( sage_ready) {
    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0) {
      SAGE_PRINTLOG("MPLAYER> Event code %d", msg.getCode() );
      switch (msg.getCode()) {
      case APP_QUIT: {
        SAGE_PRINTLOG("MPLAYER> Event APP_QUIT");
        if (sac) delete sac;
        exit_player(EXIT_QUIT);
        exit(1);
        break;
      }

      case EVT_APP_SYNC: {
        SAGE_PRINTLOG("MPLAYER> Received order to SYNC: rewind and pause");
        mplayer_put_key(KEY_ENTER);
        mplayer_put_key(KEY_PAUSE);
        break;
      }

        //
        // this is for SAGENext - Sungwon
        //
      case EVT_KEY: {
        char data[16];
        sscanf((char *)msg.getData(), "%s", data);
        SAGE_PRINTLOG("MPLAYER> EVT_KEY from SAGENext: %s", data);
        if (strcmp(data, "play")==0 || strcmp(data,"pause")==0) {
          //fprintf(stderr, "\t\t PLAY/PAUSE !!\n");
          mplayer_put_key(' ');
        }
        else if (strcmp(data, "rewind")==0) {
          mplayer_put_key(KEY_LEFT);
        }
        else if (strcmp(data, "forward")==0) {
          mplayer_put_key(KEY_RIGHT);
        }
        break;
      }
      }
    }
  }
}

static void flip_page(void)
{
  // SAGE_PRINTLOG("MPLAYER> PAGE FLIP");
  //     flip_page(): this is called after each frame, this diplays the buffer for
  //        real. This is 'swapbuffers' when doublebuffering.

  if (rgbBuffer && sage_ready)
  {
    sageInf.swapBuffer();
  }
  static int first = 1;
  if (first) {
    // start the movies paused: doesn't seem to work fine
    //mplayer_put_key(KEY_PAUSE);
  }
}

static void uninit(void)
{
  SAGE_PRINTLOG("MPLAYER> Uninit");
}

static int control(uint32_t request, void *data)
{
  //SAGE_PRINTLOG("MPLAYER> CONTROL");

  switch (request) {
  case VOCTRL_QUERY_FORMAT:
    return query_format(*((uint32_t*)data));

  case VOCTRL_PAUSE:
  case VOCTRL_RESUME:
    int_pause = (request == VOCTRL_PAUSE);
    return VO_TRUE;

    //case VOCTRL_PAUSE:
    //return int_pause = 1;
    //case VOCTRL_RESUME:
    //return int_pause = 0;
    //case VOCTRL_GUISUPPORT:
    //return VO_TRUE;
  }
  return VO_NOTIMPL;
}
