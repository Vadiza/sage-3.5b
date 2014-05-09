/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sail.cpp - the main source of SAGE Application Interface Library
 * Author : Luc Renambot
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
 * Direct questions, comments etc about SAGE to the forums on
 * http://sagecommons.org/
 *
 *****************************************************************************/

// headers for SAGE
#include "libsage.h"
#include <list>

// SAGE UI API
#include "suil.h"
suil uiLib;

list<sail*> Clients;

static int _initialized = 0;


int getWallSize(int &width, int &height)
{
  int done = 0;
  suil *ui = new suil();

  ui->init((char*)"fsManager.conf");
  ui->connect(NULL);
  ui->sendMessage(SAGE_UI_REG,(char*)" ");

  sageMessage msg;

  while( ! done ) {
    msg.init(READ_BUF_SIZE);
    ui->rcvMessageBlk(msg);

    if ( msg.getCode() == (SAGE_DISPLAY_INFO) ) {
      // Wall information
      char displayStr[STRBUF_SIZE];
      memset(displayStr, 0, STRBUF_SIZE);
      int tileNum, dimX, dimY, vdtWidth, vdtHeight, tileWidth, tileHeight, id;
      int ret = sscanf((char*)msg.getData(), "%d %d %d\n%d %d\n%d %d %d\n",
                       &tileNum, &dimX, &dimY, &vdtWidth, &vdtHeight, &tileWidth, &tileHeight, &id);
      if (ret == 8) {
        width  = vdtWidth;
        height = vdtHeight;
        return 1;
      }
    }

  }

  delete ui;
  return 0;
}



sail* createSAIL(const char *appname, int ww, int hh, enum sagePixFmt pixelfmt, const char *fsIP, int roworder, int frate, sageWidgetFunc wFunc)
{
  sailConfig scfg;
  sail *sageInf;

  // Initialize a few things
  if (! _initialized) {
    sage::initUtil();

    // Create a user interface message thread
    uiLib.init((char*)"fsManager.conf");
    uiLib.connect(NULL);
    uiLib.sendMessage(SAGE_UI_REG,(char*)" ");

    _initialized = 1;
  }

  // Allocate the sail object
  sageInf = new sail();
  if (sageInf == NULL)
    return NULL;

  // Search for a configuration file
  char *tmpconf = getenv("SAGE_APP_CONFIG");
  if (tmpconf) {
    SAGE_PRINTLOG("%s> found SAGE_APP_CONFIG variable: [%s]", appname, tmpconf);
    scfg.init(tmpconf);
  }
  else {
    SAGE_PRINTLOG("%s> using default %s.conf", appname, appname);
    char appNameConf[1024];
    memset(appNameConf, 0, 1024);
    sprintf(appNameConf, "%s.conf", appname);
    scfg.init((char*)appNameConf);
  }

  // Set the name of the application (used in UI components)
  scfg.setAppName((char*)appname);

  // Set the rendering dimension
  scfg.resX = ww;
  scfg.resY = hh;

  // if it hasn't been specified by the config file, use the app-determined display size
  if (scfg.winWidth == -1 || scfg.winHeight == -1)
  {
    scfg.winWidth  = ww;
    scfg.winHeight = hh;
  }

  // Set up the render portion (default, whole image)
  sageRect appImageMap;
  appImageMap.left   = 0.0;
  appImageMap.right  = 1.0;
  appImageMap.bottom = 0.0;
  appImageMap.top    = 1.0;

  scfg.imageMap  = appImageMap;
  scfg.rowOrd    = roworder;
  scfg.rank      = 0;
  scfg.master    = true;
  scfg.rendering = true;
  scfg.pixFmt    = pixelfmt;
  scfg.frameRate = frate;
  scfg.appID     = _initialized;  // I don't think this helps, but....
  _initialized ++;

  // Copy the provided fsManager IP into the SAGE variable
  if (fsIP) {
    SAGE_PRINTLOG("Application provided new fsIP: [%s]", fsIP);
    memset(scfg.fsIP, 0, SAGE_IP_LEN);
    strncpy(scfg.fsIP, fsIP, SAGE_IP_LEN);
  } else {
    SAGE_PRINTLOG("Using application file provided fsIP: [%s]", scfg.fsIP);
  }

  if (wFunc) {
    (*wFunc)(sageInf);
  }

  sageInf->init(scfg);

  return sageInf;
}


void deleteSAIL(sail *sageInf)
{
  sageInf->shutdown();
  delete sageInf;
}


int getRealBufferSize(sail *sageInf)
{
  int res = sageInf->getBufSize();
  if (sageInf->Config().pixFmt == PIXFMT_DXT || sageInf->Config().pixFmt == PIXFMT_DXT5 || sageInf->Config().pixFmt == PIXFMT_DXT5YCOCG)
    res = res / 16;
  return res;
}

unsigned char* nextBuffer(sail *sageInf)
{
  unsigned char *ptr = (unsigned char*) sageInf->getBuffer();
  //memset(ptr, 0, getRealBufferSize(sageInf));
  return ptr;
}

void swapWithBuffer(sail *sageInf, unsigned char *pixptr)
{
  // Swap the main sail object
  unsigned char *ptr = (unsigned char*) sageInf->getBuffer();
  memcpy(ptr, pixptr, getRealBufferSize(sageInf));
  sageInf->swapBuffer();

  // Swap the copies
  list<sail*>::const_iterator cii;
  for(cii=Clients.begin(); cii!=Clients.end(); cii++)
  {
    sail* cl = *cii;
    unsigned char *ptr = (unsigned char*) cl->getBuffer();
    memcpy(ptr, pixptr, getRealBufferSize(sageInf));
    cl->swapBuffer();
  }
}

void swapBuffer(sail *sageInf)
{
  // Swap the derived clients
  list<sail*>::const_iterator cii;
  unsigned char *pixels = (unsigned char*) sageInf->getBuffer();
  int bs = getRealBufferSize(sageInf);

  for(cii=Clients.begin(); cii!=Clients.end(); cii++)
  {
    sail* cl = *cii;
    unsigned char *ptr = (unsigned char*) cl->getBuffer();
    memcpy(ptr, pixels, bs);
    cl->swapBuffer();
  }

  // Swap buffer for the main object
  sageInf->swapBuffer();
}

unsigned char* swapAndNextBuffer(sail *sageInf)
{
  sageInf->swapBuffer();
  return nextBuffer(sageInf);
}

void addNewClient(sail *sageInf, char *fsIP)
{
  sailConfig scfg2;

  // Copy the existing configuration
  scfg2 = sageInf->Config();
  // Clear the destination IP
  memset(scfg2.fsIP, 0, SAGE_IP_LEN);
  // Copy the new destination IP
  strncpy(scfg2.fsIP, fsIP, SAGE_IP_LEN);
  // Allocate the sail object
  sail *ptr = new sail();
  Clients.push_back( ptr );
  ptr->init(scfg2);
}

// Create a copy of itself
void replicateClient(sail *sageInf)
{
  sailConfig scfg2;

  // Copy the existing configuration
  scfg2 = sageInf->Config();
  // Clear the destination IP
  memset(scfg2.fsIP, 0, SAGE_IP_LEN);
  // Copy the new destination IP
  strncpy(scfg2.fsIP, sageInf->Config().fsIP, SAGE_IP_LEN);
  // Allocate the sail object
  sail *ptr = new sail();
  Clients.push_back( ptr );
  ptr->init(scfg2);
}

void processMessages(sail *sageInf, application_update_t *up, sageQuitFunc qfunc, sageSyncFunc sfunc)
{
  sageMessage msg;
  application_update_t ret;

  if (sageInf->checkMsg(msg, false) > 0) {
    switch (msg.getCode()) {

    case APP_QUIT:
      if (qfunc) (*qfunc)(sageInf);
      deleteSAIL(sageInf);
      exit(1);
      break;

    case EVT_APP_SHARE:
      SAGE_PRINTLOG("Got a SHARE order with: IP [%s]", msg.getData());
      addNewClient(sageInf, (char*)msg.getData());
      break;

    case EVT_APP_SYNC:
      SAGE_PRINTLOG("Got a SYNC order");
      if (sfunc) (*sfunc)(sageInf);
      break;

    default:
      SAGE_PRINTLOG("--------------------");
      SAGE_PRINTLOG("Got message: buffer [%s]", msg.getBuffer());
      SAGE_PRINTLOG("\t: data [%s]",            msg.getData());
      SAGE_PRINTLOG("\t: code [%d]",            msg.getCode());
      SAGE_PRINTLOG("\t: app code [%d]",        msg.getAppCode());
      SAGE_PRINTLOG("--------------------");
      break;
    }
  }
  msg.destroy();

  sageMessage umsg;
  umsg.init(READ_BUF_SIZE);
  if (uiLib.rcvMessage(umsg) > 0 ) {

    if ( umsg.getCode() == (APP_INFO_RETURN) ) {
      // Application update

      // SAGE_PRINTLOG("UI Message> %d --- %s", umsg.getCode(), (char *)umsg.getData());
      //   format:   decklinkcapture 13 1972 5172 390 2190 1045 0 0 0 0 none 1280 720 1

      char appname[256], cmd[256];
      int ret;
      int appID, left, right, bottom, top, sailID, zValue;
      ret = sscanf((char*)umsg.getData(), "%s %d %d %d %d %d %d %d",
                   appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);
      if (ret == 8) {  // we parsed all the fields
        if (sageInf->getWinID() == appID) { // it is my application
          int x,y,w,h;
          x = left;
          y = top;
          w = right-left;
          h = top-bottom;
          if (up) {
            up->updated = 1;
            up->app_id = appID;
            up->app_x = x;
            up->app_y = y;
            up->app_w = w;
            up->app_h = h;
          }
        }
      }
    }

    else if ( umsg.getCode() == (SAGE_DISPLAY_INFO) ) {
      // Wall information
      char displayStr[STRBUF_SIZE];
      memset(displayStr, 0, STRBUF_SIZE);
      int tileNum, dimX, dimY, vdtWidth, vdtHeight, tileWidth, tileHeight, id;
      int ret = sscanf((char*)umsg.getData(), "%d %d %d\n%d %d\n%d %d %d\n",
                       &tileNum, &dimX, &dimY, &vdtWidth, &vdtHeight, &tileWidth, &tileHeight, &id);
      if (ret == 8) { // we parsed all the fields
        if (up) {
          up->wall_width  = vdtWidth;
          up->wall_height = vdtHeight;
        }
      }
    }


    else if ( msg.getCode() == (UI_APP_SHUTDOWN) ) {
      // Somebody died
    }

    else if ( msg.getCode() == (UI_PERF_INFO) ) {
      // Performance info
    }


  }
  umsg.destroy();

}
