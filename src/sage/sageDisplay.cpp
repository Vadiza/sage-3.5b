/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDisplay.cpp - the display module in SAGE Receiver
 * Author : Byungil Jeong, Rajvikram Singh, Luc Renambot
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
 * Direct questions, comments etc about SAGE to renambot@uic.edu
 * http://sagecommons.org/
 *
 *****************************************************************************/


#include "sageShader.h"
#include "sageDisplay.h"
#include "sageBlock.h"
#include "sageTexture.h"
#include "pixelDownloader.h"
#include "drawObjects.h"
#include "overlayApp.h"
#include "image.h"

#if ! defined(WIN32)
/////////////////////////////////////////////////////////////////////////
const unsigned long long m_cpuFrequency = __computeFrequency();
const uint64_t m_msecsTicks = m_cpuFrequency / 1000;
const uint64_t m_usecsTicks = m_cpuFrequency / 1000000;
const uint64_t m_nsecsTicks = m_cpuFrequency / 1000000000;
/////////////////////////////////////////////////////////////////////////
#endif



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


sageMontage::sageMontage(displayContext *dct, sagePixFmt pfmt) :
  depth(1.0), monIdx(-1), tileIdx(-1), visible(true), context(dct), doFade(true), lastTime(0),
  otherMontage(NULL), alpha(0), sageTex(NULL)
{
  switch(pfmt) {
  case PIXFMT_555:
  case PIXFMT_555_INV:
  case PIXFMT_565:
  case PIXFMT_565_INV:
  case PIXFMT_888:
  case PIXFMT_888_INV:
  case PIXFMT_8888:
  case PIXFMT_8888_INV:
    sageTex = new sageTextureRGB(0,0,pfmt);
    break;
  case PIXFMT_DXT:
  case PIXFMT_DXT5:
  case PIXFMT_DXT5YCOCG:
    sageTex = new sageTextureDXT(0,0,pfmt);
    break;
  case PIXFMT_YUV:
    sageTex = new sageTextureYUV(0,0,pfmt);
    break;
  case PIXFMT_RGBS3D:
    sageTex = new sageTextureS3DRGB(0,0,pfmt);
    break;
  default:
    break;
  }
}

sageMontage::~sageMontage()
{
  delete sageTex;
}


int sageMontage::init(sageRect &viewPort, sageRect &blockLayout, sageRotation orientation)
{
  sageTex->init(viewPort, blockLayout, orientation);

  updateBoundary();
  renewTexture();
  genTexCoord();

  return 0;
}


void sageMontage::genTexCoord()
{
  sageTex->genTexCoord();
}

void sageMontage::copyIntoTexture(int _x, int _y, int _width, int _height, char *_block)
{
  sageTex->copyIntoTexture(_x,_y,_width,_height,_block);
}

// void sageMontage::copyIntoDXTTexture(int _x, int _y, int _width, int _height, char *_block)
// {
//  sageTex->copyIntoDXTTexture(_x,_y,_width,_height,_block);
// }


void sageMontage::loadPixelBlock(sagePixelBlock *block)
{
  context->switchContext(tileIdx);

  sageTex->loadPixelBlock(block);
}

void sageMontage::uploadTexture()
{
  sageTex->uploadTexture();
}


void sageMontage::renewTexture()
{
  context->switchContext(tileIdx);

  sageTex->renewTexture();
}

void sageMontage::deleteTexture()
{
  sageTex->deleteTexture();
}

void sageMontage::draw(int _tempAlpha)
{
  sageTex->draw(depth, alpha, _tempAlpha, left, right, bottom, top);
}


void sageMontage::update(double now)
{
  float rate = 20.0;
  if (doFade) {
    if (now - lastTime > 1000000.0/rate) {
      lastTime = now;

      // change the alpha by a little bit each time
      alpha += int(255.0 * (1.0/(rate*0.5)));  // make it run for 2 secs

      // keep the alpha within appropriate limits
      if (alpha > 255) {
        alpha = 255;
        doFade = false;
      }
    }
  }
}



sageDisplay::sageDisplay(displayContext *dct, struct sageDisplayConfig &cfg):
  context(dct), dirty(false), drawObj(dirty, cfg), activetile(-1), numframes(0)
{
  configStruct = cfg;
  tileNum = cfg.dimX * cfg.dimY;
  if (tileNum > MAX_TILES_PER_NODE) {
    SAGE_PRINTLOG("sageDisplay::init() : The tile number exceeds the maximum");
  }

  for (int i=0; i<tileNum; i++) {
    noOfMontages[i] = 0;
    for (int j=0; j<MAX_MONTAGE_NUM; j++)
      montages[i][j] = NULL;
  }

#if defined(WIN32)
  SAGE_PRINTLOG("Init GL functions: %p %p\n", glCompressedTexImage2D, glCompressedTexSubImage2D);
  glCompressedTexImage2D  = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)
    glGetProcAddress("glCompressedTexImage2D");
  glCompressedTexSubImage2D  = (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)
    glGetProcAddress("glCompressedTexSubImage2D");
  SAGE_PRINTLOG("----> GL functions: %p %p\n", glCompressedTexImage2D, glCompressedTexSubImage2D);
#endif

}

int sageDisplay::changeBGColor(int red, int green, int blue)
{
  context->changeBackground(red, green, blue);
  dirty = true;

  return 0;
}

int sageDisplay::addMontage(sageMontage *mon)
{
  if (!mon) {
    SAGE_PRINTLOG("sageDisplay::addMontage : Error - input is NULL");
    return -1;
  }

  if (mon->tileIdx < 0)
    return -1;

  mon->visible = false;

  for (int i=0; i<MAX_MONTAGE_NUM; i++) {
    if (montages[mon->tileIdx][i] == NULL) {
      montages[mon->tileIdx][i] = mon;
      mon->monIdx = i;
      if (i+1 > noOfMontages[mon->tileIdx])
        noOfMontages[mon->tileIdx] = i+1;
      return i;
    }
  }

  SAGE_PRINTLOG("sageDisplay::addMontage : Error - montage list is full");
  return -1;
}

int sageDisplay::removeMontage(sageMontage *mon)
{
  if (!mon) {
    SAGE_PRINTLOG("sageDisplay::removeMontage : Error - input is NULL");
    return -1;
  }

  if (mon->tileIdx < 0 || mon->monIdx < 0)
    return 0;

  montages[mon->tileIdx][mon->monIdx] = NULL;
  mon->monIdx = -1;

  return 0;
}

int sageDisplay::replaceMontage(sageMontage *mon)
{
  if (!mon) {
    SAGE_PRINTLOG("sageDisplay::replaceMontage : Error - input is NULL");
    return -1;
  }

  if (mon->tileIdx < 0 || mon->monIdx < 0)
    return -1;

  mon->visible = true;
  montages[mon->tileIdx][mon->monIdx] = mon;

  if (mon->sageTex->needsUpload()) {
    mon->uploadTexture();
  }

  return 0;
}

void sageDisplay::onAppShutdown(int winID)
{
  drawObj.onAppShutdown(winID);
}

void sageDisplay::update()
{
  double currentTime = sage::getTime();  // in microsecs
  bool doRefresh = false;

  for (int i=0; i<tileNum; i++)   {
    for (int j=0; j<noOfMontages[i]; j++) {
      sageMontage *mon = montages[i][j];
      if (mon && mon->visible) {

        if (currentTime - drawObj.apps[mon->winID].timeCreated > 2000000) {
          mon->doFade = false;
          mon->alpha = 255;
          if(mon->otherMontage) {
            mon->otherMontage->doFade = false;
            mon->otherMontage->alpha = 255;
          }

        }
        else {
          if (mon->doFade)
            doRefresh = true;

          mon->update(currentTime);

          // update the back one as well
          if (mon->otherMontage)
            mon->otherMontage->update(currentTime);
        }
      }
    }
  }

  if (doRefresh)
    dirty = true;

  drawObj.update();
}

// Setup a tile for display
// optimization; if only one tile per machine (no setup over and over)
// - Luc -
void sageDisplay::setupTile(int i)
{
  if (i != activetile) {
    sageRect tileRect = configStruct.tileRect[i];
    tileRect.updateBoundary();

    int tileX = (i % configStruct.dimX) * configStruct.width;
    int tileY = (i / configStruct.dimX) * configStruct.height;
    tileRect.x = tileX;
    tileRect.y = tileY;

    context->setupViewport(i, tileRect);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(tileRect.left, tileRect.right, tileRect.bottom, tileRect.top, 0, 100000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // move the camera so that we can draw the stuff above z=0
    // (e.g. overlays above the apps that are at z=0)
    glTranslatef(0.0, 0.0, -2.0);

    // we'll draw everything in order without the depth buffer
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // remember the active tile number
    activetile = i;

    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  }
}


int sageDisplay::updateScreen(dispSharedData *shared, bool barrierFlag, bool drawOverlays)
{
  static double pixel_time = sage::getTime();
  //SAGE_PRINTLOG("UpdateScreen - %d", barrierFlag);

  context->clearScreen();

  for (int i=0; i<tileNum; i++)   {
    // Prepare the OpenGL context (viewport, settings, ...)
    setupTile(i);

    // for drawing minimized apps later
    std::vector<sageMontage*>minimizedMontages;

    // first, sort the apps by z order
    std::map<float, int>appsByZ;
    std::map<const int, appInfo>::iterator iter;
    for (iter = drawObj.apps.begin(); iter != drawObj.apps.end(); iter++) {
      appsByZ[(*iter).second.z] = (*iter).first;
    }


    // draw montages in order... so first sort them by depth
    std::map<int, sageMontage*>montagesById;
    for (int j=0; j<noOfMontages[i]; j++) {
      sageMontage *mon = montages[i][j];
      if (mon)
        montagesById[ mon->winID ] = mon;
    }

    sageRect tileRect = configStruct.tileRect[i];

    // draw the pre-draw stuff first
    if (drawOverlays)
      drawObj.draw(tileRect, SAGE_PRE_DRAW);

    // now draw all the apps and their overlays in order based on app z
    // (reverse order of appsByZ map)
    std::map<float, int>::reverse_iterator rit;
    for (rit = appsByZ.rbegin(); rit != appsByZ.rend(); rit++)
    {
      // does the montage for this app exist on this tile?
      // If so, draw it, if not still draw it's overlays
      sageMontage *mon = NULL;
      if (montagesById.count((*rit).second) > 0) {
        mon = montagesById[(*rit).second];
      }


      if (mon && mon->visible && !(drawObj.apps[mon->winID]).minimized)
      {
        // start fading out the app if we are closing it
        int tempAlpha = -1;
        overlayApp * appOverlay = (overlayApp*) drawObj.getAppOverlay((*rit).second);
        if (appOverlay && appOverlay->closing)
          tempAlpha = (int) 255 * appOverlay->curtainAlpha;

        // draw the app pixels themselves
        drawAppMontage(mon, tempAlpha);

        // draw the app-related overlays
        if (drawOverlays)
          drawObj.draw(tileRect, SAGE_INTER_DRAW, (*rit).second);
      }
      else if(mon && (drawObj.apps[mon->winID]).minimized) {
        // we will draw minimized apps on top, later

        minimizedMontages.push_back(mon);
      }
      else {
        // draw the app-related overlays
        if (drawOverlays)
          drawObj.draw(tileRect, SAGE_INTER_DRAW, (*rit).second);
      }
    }

    // now draw all the other overlays in SAGE_INTER_DRAW layer that
    // are not tied to an application (global widgets)
    if (drawOverlays)
      drawObj.draw(tileRect, SAGE_INTER_DRAW);

    // draw the minimized apps that should be on top of others
    for(int m=0; m<minimizedMontages.size(); m++) {
      // draw the app pixels
      GLprintError(__FILE__, __LINE__);
      drawAppMontage(minimizedMontages[m]);

      // draw the app-related overlays
      if (drawOverlays)
        drawObj.draw(tileRect, SAGE_INTER_DRAW, minimizedMontages[m]->winID);
    }

    // now draw the very top stuff (pointers...)
    if (drawOverlays)
      drawObj.draw(tileRect, SAGE_POST_DRAW);

    context->refreshTile(i);

  }

  /** BARRIER **/

  if ( barrierFlag ) {
    shared->syncClientObj->sendRefreshBarrier(shared->nodeID);
    //SAGE_PRINTLOG("node %d sent to barrier\n", shared->nodeID);
    shared->syncClientObj->recvRefreshBarrier(false); // blocking (set true for nonblock)
    //SAGE_PRINTLOG("node %d recved frm barrier\n", shared->nodeID);

  }

#ifdef DELAY_COMPENSATION
  if ( shared ) {
    gettimeofday(&shared->localT, NULL); // my time

    //SAGE_PRINTLOG("SM sent %ld,%ld\n", shared->syncMasterT.tv_sec, shared->syncMasterT.tv_usec);
    //SAGE_PRINTLOG("SDM%d is %ld,%ld\n", shared->nodeID, shared->localT.tv_sec, shared->localT.tv_usec);

    double A = (double)shared->syncMasterT.tv_sec + 0.000001*(double)shared->syncMasterT.tv_usec;
    A = A + ((double)shared->deltaT * 0.000001);
    double B = (double)shared->localT.tv_sec + 0.000001*(double)shared->localT.tv_usec;
    double wait = A - B;
    wait *= 1000000.0;

    int llBusyWait = (int)wait;
    if (  llBusyWait > 0 ) {
      __usecDelay_RDTSC( llBusyWait );
    }

    //SAGE_PRINTLOG("SDM %d: %.6f msec waited\n", shared->nodeID, elapsed*1000.0);
  }
#endif

  glFlush();
  if (drawOverlays) {
    context->refreshScreen();  // actual swapBuffer occurs in here at displayConext's instance
    dirty = false;
  }

#if 0
  // Print the Mpixels/sec downloaded to the GPU
  double now_time = sage::getTime();
  numframes ++;
  if ((now_time-pixel_time) >= 1000000.0) { // 1 second
    SAGE_PRINTLOG("Pixel download [%dx%d]: %6.3f MB/sec -- %5.2f FPS",
                  configStruct.xpos, configStruct.ypos,
                  ((double)pixel_bytes / (1024.0*1024.0)) / ((now_time-pixel_time)/1000000.0),
                  ((double)numframes) / ((now_time-pixel_time)/1000000.0) );
    pixel_bytes = 0;
    pixel_time = now_time;
    numframes = 0;
  }
#endif

  return 0;
}


void sageDisplay::drawAppMontage(sageMontage *mon, int tempAlpha)
{
  // Draw shadows when not minized
  //if ( !(drawObj.apps[mon->winID]).minimized ) drawAppShadow(mon->winID);
  // Draw shadows when mouseOver
  if ( (drawObj.apps[mon->winID]).mouseOver ) drawAppShadow(mon->winID);

  mon->draw(tempAlpha);
}

void sageDisplay::drawAppShadow(int winID)
{

  glDisable(GL_TEXTURE_2D);
  int w = drawObj.apps[winID].w;
  int h = drawObj.apps[winID].h;
  int l = drawObj.apps[winID].x;
  int b = drawObj.apps[winID].y;
  int t = drawObj.apps[winID].y + h;
  int r = drawObj.apps[winID].x + w;
  int o, min=25, max=75; // the size of drop shadow
  float a = 0.5;  // opacity


  if (w > h)
    o = (int)h*0.05;
  else
    o = (int)w*0.05;

  if (o > max)  o=max;
  else if(o<min) o=min;


  // left
  glBegin(GL_QUADS);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(l-o, t);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(l, t);
  glColor4f(0.1, 0.1, 0.1, a);
  glVertex2f(l, b);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(l-o, b-o);
  glEnd();

  // bottom
  glBegin(GL_QUADS);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(l-o, b-o);
  glColor4f(0.1, 0.1, 0.1, a);
  glVertex2f(l, b);
  glColor4f(0.1, 0.1, 0.1, a);
  glVertex2f(r, b);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(r+o, b-o);
  glEnd();

  // right
  glBegin(GL_QUADS);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(r+o, t);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(r, t);
  glColor4f(0.1, 0.1, 0.1, a);
  glVertex2f(r, b);
  glColor4f(0.1, 0.1, 0.1, 0.0);
  glVertex2f(r+o, b-o);
  glEnd();
  glEnable(GL_TEXTURE_2D);
}



void sageDisplay::saveScreenshot(char *data)
{
  // first redraw once without the overlays... we don't want them in the screenshot
  //updateScreen(NULL, false, false);

  unsigned char *rgbBuffer;
  char saveDir[512];   // just the directory where to save the files
  char imagePath[512];  // includes image names as well
  int dispW, dispH, numX, numY;  // total sage display w and h
  int tileOffsetX, tileOffsetY, tileX, tileY;
  int downsize;  // should the images be downsized? NO if we are just grabbing a section of the display
  sscanf((char *)data, "%s %d %d %d", saveDir, &downsize, &dispW, &dispH);
  //SAGE_PRINTLOG("data [%s] - %s %d %d %d", data, saveDir, downsize, dispW, dispH);

  // We do RGB readback, so alignment should be 1, not 4 (default)
  int align;
  glGetIntegerv(GL_PACK_ALIGNMENT, &align);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  glReadBuffer(GL_BACK);
  for (int i=0; i<tileNum; i++)   {

    sageRect tileRect = configStruct.tileRect[i];
    tileRect.updateBoundary();

    tileOffsetX = (i % configStruct.dimX) * configStruct.width;
    tileOffsetY = (i / configStruct.dimX) * configStruct.height;

    numX = sage::round(dispW / (float)tileRect.width);
    numY = sage::round(dispH / (float)tileRect.height);

    //tileX = numX - (int) roundf(dispW / (float)tileRect.right);
    //tileY = numY - (int) roundf(dispH / (float)tileRect.top);
    // Luc changed
    tileX = (int) (tileRect.right / configStruct.width) -1;
    tileY = (int) (tileRect.top / configStruct.height) -1;

    // make the full path to the image
    sprintf(imagePath, "%s/screen-%d-%d.jpg", saveDir, tileX, tileY);

    // read the framebuffer
    rgbBuffer = (unsigned char *) malloc( configStruct.width*configStruct.height*3 );
    memset(rgbBuffer, 0, configStruct.width*configStruct.height*3 );
    glReadPixels(tileOffsetX, tileOffsetY, configStruct.width, configStruct.height, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer);
    glFinish();
    //SAGE_PRINTLOG("Reading: %d x %d  - %d x %d - size %d", tileOffsetX, tileOffsetY, configStruct.width, configStruct.height, configStruct.width*configStruct.height*3);

    // Now using RLK image functions (see image.h)
    image_flip(configStruct.width, configStruct.height, 3, 1, rgbBuffer);
    image_write(imagePath, configStruct.width, configStruct.height, 3, 1, rgbBuffer);

    free(rgbBuffer);

    //SAGE_PRINTLOG("Tile %d of %d -- %d x %d -- Screenshot saved to: %s", i, tileNum, tileX, tileY, imagePath);
    //SAGE_PRINTLOG("\t tileX: numX %d dispW %d tileRect.right %f -> %d", numX, dispW, tileRect.right, tileX);
    //SAGE_PRINTLOG("\t tile offset %d x %d - round %d", tileOffsetX, tileOffsetY, (int) roundf(dispW / (float)tileRect.right) );
  }

  // restore the alignment values, in case so other code changed it
  glPixelStorei(GL_PACK_ALIGNMENT, align);
}

int sageDisplay::addDrawObjectInstance(char *data)
{
  drawObj.addObjectInstance(data);
  dirty = true;

  return 0;
}

int sageDisplay::updateObjectPosition(char *data)
{
  drawObj.updateObjectPosition(data);
  dirty = true;

  return 0;
}

int sageDisplay::removeDrawObject(char *data)
{
  int id;
  sscanf(data, "%d", &id);
  drawObj.removeObject(id);
  dirty = true;

  return 0;
}

int sageDisplay::forwardObjectMessage(char *data)
{
  drawObj.forwardObjectMessage(data);
  dirty = true;

  return 0;
}

int sageDisplay::showObject(char *data)
{
  drawObj.showObject(data);
  dirty = true;

  return 0;
}

int sageDisplay::updateAppBounds(int winID, int x, int y, int w, int h, sageRotation orientation)
{
  drawObj.updateAppBounds(winID, x, y, w, h, orientation);
  dirty = true;

  return 0;
}

int sageDisplay::updateAppBounds(char *data)
{
  sageRotation orientation;
  int x,y,w,h,winID;
  sscanf(data, "%d %d %d %d %d %d", &winID, &x, &y, &w, &h, (int *)&orientation);
  updateAppBounds(winID, x, y, w, h, orientation);
  dirty = true;

  return 0;
}

int sageDisplay::updateAppDepth(int winID, float depth)
{
  drawObj.updateAppDepth(winID, depth);
  dirty = true;

  return 0;
}


sageDisplay::~sageDisplay()
{
} //End of sageDisplay::~sageDisplay()



// int sageMontage::copyConfig(sageMontage &mon)
// {
//    x = mon.x;
//    y = mon.y;
//    width = mon.width;
//    height = mon.height;
//    left = mon.left;
//    right = mon.right;
//    bottom = mon.bottom;
//    top = mon.top;

//    pInfo = mon.pInfo;
//    pixelType = mon.pixelType;
//    texCoord = mon.texCoord;
//    texInfo = mon.texInfo;

//    renewTexture();

//    return 0;
// }
