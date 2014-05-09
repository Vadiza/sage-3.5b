/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDraw.h
 * Author : Byungil Jeong
 *
 *   Description: This is the header file for the simple drawing class of SAGE.
 *
 * Copyright (C) 2007 Electronic Visualization Laboratory,
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
 *********************************************************************************/

#ifndef SAGE_DRAW_H
#define SAGE_DRAW_H

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <AGL/agl.h>
#include <OpenGL/OpenGL.h>

#else

#include <SDL/SDL.h>

#endif

#include "displayContext.h"
#include "sage.h"
#include "drawObjects.h"
#include "sageDrawObject.h"
#include <map>
#include <set>
#include <queue>


class sageDrawObject;

#if defined(__APPLE__)
#include <OpenGL/gl.h>   // Header File For The OpenGL Library
#include <OpenGL/glu.h>   // Header File For The GLu Library
#else
#include <GL/gl.h>   // Header File For The OpenGL Library
#include <GL/glu.h>   // Header File For The GLu Library
#endif

// store a list of appIDs and their current bounds
struct appInfo {
  int x,y,w,h;
  float z;
  sageRotation rot;
  double timeCreated;  // used for montage alpha...
  bool minimized;
  bool mouseOver;
};

// defined in sageShader.cpp
int GLprintError(const char *file, int line);


class sageDraw {
private:
  // hashes of drawObjects keyed by their ID for easy access
  std::map<const int, sageDrawObject *> objList;

  // widgets that are tied to their parents but parents'
  // info hasn't arrived yet...
  std::set<sageDrawObject *> objQueue;

  bool &dirtyBit;
  int displayID;
  sageDisplayConfig dispCfg;

  int addToParent(sageDrawObject* drawObj);
  void checkObjectQueue();

  // a queue of objects that temporarily need to be
  // drawn after their top-level parents (e.g. selected thumbnail)
  std::queue<sageDrawObject *> delayedDrawQueue;

public:

  int doShowSizers;

  sageDraw(bool &dirty, sageDisplayConfig cfg);
  sageDrawObject * createDrawObject(char *name);
  sageDrawObject * getDrawObject(int id);
  sageDrawObject * getAppOverlay(int appId);
  int addObjectInstance(char *data);
  int removeObject(int id);

  void setDirty();
  void update();  // updates all the drawObjects... if needed
  void draw(sageRect rect, int drawOrder, int winID=-1);  // draw all objects
  void delayDraw(sageDrawObject *obj);  // draw objects after their top-level parents
  int showObject(char *data);

  void onAppShutdown(int winID);
  int forwardObjectMessage(char *data);

  int updateObjectPosition(char *data) {return 0;}  // not in use currently
  void updateAppBounds(int winID, int x, int y, int w, int h, sageRotation orientation);
  void updateInitialBounds(int winID, int x, int y, int w, int h, sageRotation orientation, float depth);
  void updateAppDepth(int winID, float depth);
  void resetApp(int winID);
  void minimizeApp(int winID, bool minimized);
  ~sageDraw();

  // all of the applications' info
  std::map<const int, appInfo> apps;
};

#endif
