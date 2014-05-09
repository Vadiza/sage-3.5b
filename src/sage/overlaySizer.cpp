/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlaySizer.cpp
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

#include "overlaySizer.h"



overlaySizer::overlaySizer()
{
  strcpy(objectType, SIZER);
  dir = HORIZONTAL;
}

void overlaySizer::draw()
{
  if (drawParent->doShowSizers)
    drawOutline();
}

void overlaySizer::drawOutline()
{
  glDisable(GL_TEXTURE_2D);
  glColor4f(1,1,1,1);

  glBegin(GL_LINE_LOOP);
  glVertex3f(x, y, z+Z_STEP*2);
  glVertex3f(x, y+height, z+Z_STEP*2);
  glVertex3f(x+width, y+height, z+Z_STEP*2);
  glVertex3f(x+width, y, z+Z_STEP*2);
  glEnd();
}

void overlaySizer::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *s = parent->FirstChildElement("sizer");
  if (!s)
    return;

  s->QueryIntAttribute("direction", &dir);
}


// loop through all the children (if any) and resize ourselves around them
// based on the direction of the sizer
void overlaySizer::fitAroundChildren()
{
  int w=0, h=0;

  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++) {
    sageDrawObject *child = (*it).second;
    if (child->visible && !child->slideHidden) {
      if (dir == HORIZONTAL) {
        w += child->width + child->pos.xMargin;
        if ( (child->height + child->pos.yMargin) > h)
          h = child->height + child->pos.yMargin;
      }
      else {
        h += child->height + child->pos.yMargin;
        if ((child->width + child->pos.xMargin) > w)
          w = child->width + child->pos.xMargin;
      }
    }
  }

  height = h;
  width = w;
}



void overlaySizer::positionChildren()
{
  int currX = x;
  int currY = y+height;

  // update the position of all children
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++)
  {
    sageDrawObject *child = (*it).second;
    if (child->visible && !child->slideHidden) {
      if (dir == HORIZONTAL) {
        child->x = currX + child->pos.xMargin;
        child->updateY(*this);
        currX += child->width + child->pos.xMargin;
      }
      else {   // vertical
        child->updateX(*this);
        child->y = currY - child->height - child->pos.yMargin;
        currY = currY - child->height - child->pos.yMargin;
      }

      // since we changed the position of the widget, update the font
      child->updateBoundary();
      child->updateFontPosition();
      child->positionChildren();
    }
  }
}
