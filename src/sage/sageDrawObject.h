/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDrawObject.h
 * Author : Byungil Jeong, Ratko Jagodic
 *
 *   Description: the super class of simple drawing objects
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

#ifndef SAGE_DRAW_OBJECT_H
#define SAGE_DRAW_OBJECT_H

#include "sageBase.h"
#include "sageDraw.h"
#include "font.h"
//#include <wand/magick-wand.h>
#include <map>
#include <string>
#include <algorithm>  // stl algorithms

#if defined(__APPLE__)
#include <OpenGL/gl.h>   // Header File For The OpenGL Library
#include <OpenGL/glu.h>   // Header File For The GLu Library
#include <OpenGL/OpenGL.h>

#else
#include <GL/gl.h>   // Header File For The OpenGL Library
#include <GL/glu.h>   // Header File For The GLu Library
#endif

class sageDraw;


// for loading textures easily
GLuint getTextureFromXml(TiXmlElement *parent, const char *elementName);
GLuint getTextureFromFile(const char *filename);
GLuint getTextureFromBuffer(const char *imgData, int len);

int getRGBA(char *filename, GLubyte **rgba,
            unsigned int &width, unsigned int &height);
int getRGBA(const void *imgData, int len, GLubyte **rgba,
            unsigned int &width, unsigned int &height);


using namespace std;


const float DEG2RAD = 3.14159/180;

class sageDrawObject : public sageRect {

protected:
  sageDraw *drawParent;  //sageDraw class pointer
  char objectType[SAGE_NAME_LEN];
  int objectID;
  bool global;
  sageRect tileRect;
  float appZ;
  float zOffset;  // for placing widgets on different levels
  bool delayDraw; // some objects need to be drawn later (eg thumbnails)
  bool selected;

  // ordered hash of all the children in the sizer (keys are the order)
  map<int, sageDrawObject*> children;

  // for debugging the bounds...
  bool drawDimBounds;
  sageRect dimBounds;

  // label stuff
  int fx, fy;   // label draw position (lower left corner)
  int ftSize;   // current point size used to draw the font
  int labelAlignment;   // alignment of text in the label
  float relativeFontSize;  // fontHeight/appHeight
  Font *ft;   // current font in use

  // tooltip stuff
  int ttSize;
  float relativeTooltipSize;
  string tooltip;
  int ttWidth;  // in pixels

  //int setGlobalView();
  //int setLocalView();


  //int moveBuf[50];


public:
  string label;
  int drawOrder;
  int displayID;
  int winID;     // the window we are attached to... if any... if none, then -1
  int widgetID;  // as assigned by the app... otherwise -1 if widget requested by DIM
  int parentID;  // if associated with parent it will be != -1
  int parentOrder;  // position of the widget inside a sizer
  sageRect parentBounds;  // used if the object is tied to the app
  bool parentBoundsSet;
  bool visible;
  bool slideHidden;   // whether the widget is "slided" out of view... if allowed
  bool canSlide;
  int slideY;
  float z;  // used for drawing
  WIDGET_color fontColor, backgroundColor;

  // for pulsing the selection
  static int selLineWidth;
  static float selPulseDir;  // pulse direction... 1 OR -1
  static bool selUpdated;  // whether it was already updated this draw cycle

  // for fading in
  int initAlpha;  // alpha set by the user
  int endAlpha;   // used during fading
  int alpha;      // drawing alpha value
  int fadeAlpha;  // alpha used only for fading effect
  double lastTime;
  bool doFade;
  float fadeTime;  // how long is the fadein process (in s)
  fadeDir fadeDirection;
  bool directlyHidden;  // was this object hidden directly or was it hidden by a parent?

  // object size and pos (normalized or WIDGET_ABSOLUTE) as defined
  objectSize size;
  objectPos pos;
  float scale;  // how much should the widget scale from the base size specified by w,h?
  float displayScale; // the size factor accounting for different display sizes... sent by DIM

  static void initFonts();

  sageDrawObject();
  virtual ~sageDrawObject();
  int getID() { return objectID; }
  char *getType() {return objectType;}
  void init(int id, sageDraw *dp, char *objType);
  bool isAppWidget() { return (widgetID > -1);}
  //int setViewport(sageRect &rect);
  void showObject(bool doShow, bool directCall=true);
  void updateParentBounds(int x, int y, int w, int h, sageRotation o);
  void updateBounds();
  void updateX(sageRect pb);  // calculates x relative to pb
  void updateY(sageRect pb);  // calculates y relative to pb
  void updateParentDepth(float depth);
  void updateFontPosition();  // where the label is within the widget
  void fitAroundLabel();
  void parseXml(TiXmlElement *root);
  Font * getFont(int h);
  void setDirty();  // i.e. "please refresh the screen"
  bool operator==(const char *wType);
  bool operator!=(const char *wType);
  int lineW(float lw) { return MIN( MAX(1, int(lw)), 4); }

  void addChild(sageDrawObject *widget);
  void removeChild(int id);
  void updateChildrenBounds();
  //void updateChildrenZ();
  void drawChildren();
  void refresh();
  void redraw(bool delayedCall=false);

  // start the fade in/out process
  void fade(fadeDir dir, int startAlpha=-1, int endAlpha=-1);

  // common messages have msgCode < 0 and widget
  // specific messages have positive msg codes
  void parseCommonMessage(char *msg);

  virtual void update(double now);  // do any updates here for animation
  virtual void onNewParentDepth();
  virtual void onNewParentBounds();
  virtual void init();
  virtual void parseGraphicsInfo(TiXmlElement *parent);
  virtual void parseSpecificInfo(TiXmlElement *parent);
  virtual void positionChildren();
  virtual void fitAroundChildren();
  virtual void drawTooltip();

  virtual void draw() = 0;
  virtual int parseMessage(char *msg) = 0;
};

#endif
