/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayPanel.cpp
 * Author : Ratko Jagodic
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

#include "overlayPanel.h"


// custom messages for the panel
#define MOVE  0
#define SET_POSITION 1
#define DRAW_CURTAIN 2
#define RESET_CURTAIN 3


overlayPanel::overlayPanel()
{
  strcpy(objectType, PANEL);
  z = TOP_Z;
  borderWidth = 0;

  showCurtain = false;
  curtainDir = VERTICAL;
  curtainPos = 0.0;
}


void overlayPanel::draw()
{
  glDisable(GL_TEXTURE_2D);

  /**********    DRAW THE PANEL   ***********/

  if (showCurtain && curtainDir == VERTICAL)
    drawCurtain();
  else
    drawBackground();


  if (borderWidth > 0) {
    glLineWidth( lineW(borderWidth*displayScale) );
    glColor4ub(255,255,255,alpha);
    glBegin(GL_LINE_LOOP);
    glVertex3f(x-borderWidth, y-borderWidth, z+Z_STEP*2);
    glVertex3f(x-borderWidth, y+height+borderWidth, z+Z_STEP*2);
    glVertex3f(x+width+borderWidth, y+height+borderWidth, z+Z_STEP*2);
    glVertex3f(x+width+borderWidth, y-borderWidth, z+Z_STEP*2);
    glEnd();
  }
}


void overlayPanel::drawCurtain()
{

  float o, l, r, t, b;

  if (curtainDir == VERTICAL) {
    o = height*0.25;
    l=x;
    r=x+width;
    t=y+height;
    b=t-int(height*curtainPos)+o/2.0;
  }
  else {
    o = width*0.25;
    t=y+height;
    b=y;
    if (curtainPos > 0) {
      l=x;
      r=l+int(width*curtainPos)-o/2.0;
    }
    else {
      r=x+width;
      l=r+int(width*curtainPos)+o/2.0;
    }
  }


  // use the same color as the background...
  float clr[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clr);
  float red=clr[0], green=clr[1], blue=clr[2], al=clr[3];
  glColor4f(red, green, blue, 0.9);

  // solid one
  glBegin(GL_QUADS);
  glVertex3f(l, b, z);
  glVertex3f(l, t, z);
  glVertex3f(r, t, z);
  glVertex3f(r, b, z);
  glEnd();

  // gradient one
  if (curtainDir == VERTICAL) {
    glBegin(GL_QUADS);
    glVertex3f(l, b, z);
    glVertex3f(r, b, z);
    glColor4f(red, green, blue, 0.0);
    glVertex3f(r, b-o, z);
    glVertex3f(l, b-o, z);
    glEnd();
  }
  else {
    if (curtainPos > 0) {
      glBegin(GL_QUADS);
      glVertex3f(l, t, z);
      glVertex3f(l, b, z);
      glColor4f(red, green, blue, 0.0);
      glVertex3f(r+o, b, z);
      glVertex3f(r+o, t, z);
      glEnd();
    }
    else {
      glBegin(GL_QUADS);
      glVertex3f(r, b, z);
      glVertex3f(r, t, z);
      glColor4f(red, green, blue, 0.0);
      glVertex3f(l-o/2.0, t, z);
      glVertex3f(l-o/2.0, b, z);
      glEnd();
    }

  }
}


void overlayPanel::drawBackground()
{
  glColor4ub(backgroundColor.r, backgroundColor.g, backgroundColor.b, alpha);

  glBegin(GL_QUADS);
  glVertex3f(x, y, z);
  glVertex3f(x, y+height, z);
  glVertex3f(x+width, y+height, z);
  glVertex3f(x+width, y, z);
  glEnd();
}

void overlayPanel::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *f = parent->FirstChildElement("fit");
  if (f) {
    int w,h;
    f->QueryIntAttribute("width", &w);
    f->QueryIntAttribute("height", &h);
    fitWidth = bool(w);
    fitHeight = bool(h);
  }

  TiXmlElement *b = parent->FirstChildElement("border");
  if (b)
    b->QueryIntAttribute("width", &borderWidth);
}


void overlayPanel::fitAroundChildren()
{
  map<int, sageDrawObject*>::iterator it;

  if (fitWidth) {
    int w=0;
    for (it=children.begin(); it!=children.end(); it++) {
      sageDrawObject *child = (*it).second;
      w += child->width + child->pos.xMargin;
    }
    width = w;
  }
  else if (size.wType == WIDGET_NORMALIZED)
    width = int(floor(parentBounds.width * size.normW));
  else
    width = int(size.w * scale);


  if (fitHeight) {
    int h=0;
    for (it=children.begin(); it!=children.end(); it++) {
      sageDrawObject *child = (*it).second;
      h += child->height + child->pos.yMargin;
    }
    height = h;
  }
  else if (size.hType == WIDGET_NORMALIZED)
    height = int(floor(parentBounds.height * size.normH));
  else
    height = int(size.h * scale);
}



void overlayPanel::positionChildren()
{
  // update the position of all children
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++)
  {
    sageDrawObject *child = (*it).second;
    child->updateY(*this);
    child->updateX(*this);

    // since we changed the position of the widget, update the font
    child->updateBoundary();
    child->updateFontPosition();
    child->positionChildren();
  }
}



int overlayPanel::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  if (code == MOVE) {
    int dx, dy;
    sscanf(msg, "%d %d %d %d", &id, &code, &dx, &dy);

    // if the panel moves, override any previous settings
    pos.xType = WIDGET_ABSOLUTE;
    pos.yType = WIDGET_ABSOLUTE;
    pos.x = x;
    pos.y = y;
    pos.x += dx;
    pos.y += dy;

    refresh();
  }

  else if(code == SET_POSITION) {
    int newX, newY;
    sscanf(msg, "%d %d %d %d", &id, &code, &newX, &newY);

    // if the panel moves, override any previous settings
    pos.xType = WIDGET_ABSOLUTE;
    pos.yType = WIDGET_ABSOLUTE;
    pos.x = newX;
    pos.y = newY;

    refresh();

  }

  else if(code == DRAW_CURTAIN) {
    showCurtain = true;
    sscanf(msg, "%d %d %d %f", &id, &code, &curtainDir, &curtainPos);
    //delayDraw = true;
    //drawOrder = SAGE_INTER_DRAW;
  }

  else if(code == RESET_CURTAIN) {
    showCurtain = false;
    //delayDraw = false;
    //drawOrder = SAGE_PRE_DRAW;
  }


  return 0;
}
