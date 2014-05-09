/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayApp.cpp
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

#include "overlayApp.h"


// custom messages for the appOverlay
#define RESET  0
#define DRAG   1
#define RESIZE 2
#define ON_CORNER 4
#define ZOOM 5
#define MINIMIZE 6
#define MOUSE_OVER 7
#define DIM 8
#define CLOSING_FADE 9
#define RESET_CLOSING_FADE 10


// STATES for drawing pointers
#define NORMAL_STATE 10
#define DRAG_STATE   11
#define RESIZE_STATE 12
#define MINIMIZED_STATE 13

// areas
#define BOTTOM       0
#define TOP          1
#define BOTTOM_LEFT  3
#define TOP_LEFT     4
#define TOP_RIGHT    5
#define BOTTOM_RIGHT 6
#define TOP_RIGHT_X  7



overlayApp::overlayApp()
{
  strcpy(objectType, APP);
  state = NORMAL_STATE;
  aspectRatio = 1;
  cornerSize = 250;
  mouseOver = false;
  dim = false;
  closing = false;
  curtainAlpha = 0.4;

  for(int i=0; i<8; i++)
    activeCorners[i] = 0;

  border_color[0] = 1.0;
  border_color[1] = 1.0;
  border_color[2] = 1.0;

  corner_color[0] = 1.0;//0.059;
  corner_color[1] = 1.0;//0.57;
  corner_color[2] = 1.0;//0.345;
  corner_color[3] = 0.3;

  color = border_color;
}


void overlayApp::draw()
{
  glDisable(GL_TEXTURE_2D);

  glPushMatrix();

  //SAGE_PRINTLOG( "\nDRAWING AT: %f %f %f %f", left, right, top, bottom);

  /**********    DRAW THE STUFF   ***********/
  if (state == NORMAL_STATE)
  {
    if (selected) {
      drawSelection();
    }
    else {
      glLineWidth( lineW(4*displayScale));
      if (mouseOver) drawOutline();
    }

    glEnable(GL_LINE_STIPPLE);
    glLineStipple(8, 21845);

    drawCorners();

    glDisable(GL_LINE_STIPPLE);
  }
  else if (state == DRAG_STATE || state == RESIZE_STATE)
  {
    glEnable(GL_LINE_STIPPLE);

    glLineStipple(8, 21845);
    glLineWidth( lineW(5*displayScale));
    drawOutline();
    drawCorners();

    glDisable(GL_LINE_STIPPLE);
  }
  else if (state == MINIMIZED_STATE)
  {
    glLineWidth(1);
    drawOutline();
  }


  if (!tooltip.empty() && mouseOver && !dim && state!=MINIMIZED_STATE) {  // draw the tooltip
    glTranslatef(x, y, 0);
    drawFontBackground(0,(height)-ttSize, width, ttSize);
    ft = getFont(ttSize);

    int a = 255;
    if (closing)
      a = (int)255*curtainAlpha;
    ft->drawText(tooltip.c_str(),3,(height)-ttSize,z+Z_STEP, fontColor, a);
  }

  if (dim)
    drawCurtain();

  glPopMatrix();
}



void overlayApp::drawFontBackground(float x, float y, float w, float h)
{
  int a = 220;
  if (closing)
    a = (int)255*curtainAlpha;
  glColor4ub(50,50,50,220);

  glBegin(GL_QUADS);
  glVertex3f(x, y, z);
  glVertex3f(x, y+h, z);
  glVertex3f(x+w, y+h, z);
  glVertex3f(x+w, y, z);
  glEnd();
}




void overlayApp::drawSelection()
{
  int w = right - left;
  int h = top - bottom;

  glLineWidth( lineW(10*displayScale));

  float c = 1.0;
  float c2 = 0.6;
  float lw = 4;

  glBegin(GL_LINE_LOOP);
  glColor4f(c, c, 0, selLineWidth/255.0);
  glVertex3f(left-lw, bottom-lw, z);

  glColor4f(c2, 0, 0, selLineWidth/255.0);
  glVertex3f(left-lw, top+lw - h/2.0, z);

  glColor4f(c, c, 0, selLineWidth/255.0);
  glVertex3f(left-lw, top+lw, z);

  glColor4f(c2, 0, 0, selLineWidth/255.0);
  glVertex3f(left-lw+w/2.0, top+lw, z);

  glColor4f(c, c, 0, selLineWidth/255.0);
  glVertex3f(right+lw, top+lw, z);

  glColor4f(c2, 0, 0, selLineWidth/255.0);
  glVertex3f(right+lw, top+lw-h/2.0, z);

  glColor4f(c, c, 0, selLineWidth/255.0);
  glVertex3f(right+lw, bottom-lw, z);

  glColor4f(c2, 0, 0, selLineWidth/255.0);
  glVertex3f(right+lw-w/2.0, bottom-lw, z);
  glEnd();


  glColor4f(0.2, 0.2, 0.2, 255.0);
  glLineWidth( lineW(3*displayScale));
  glBegin(GL_LINE_LOOP);
  glVertex3f(left+1, bottom+1, z);
  glVertex3f(left+1, top-1, z);
  glVertex3f(right-1, top-1, z);
  glVertex3f(right-1, bottom+1, z);
  glEnd();
  /*
    glEnable(GL_LINE_STIPPLE);
    glLineWidth(4);

    float c = 0.8;
    float lw = 2;

    if (selPulseDir > 0)
    glLineStipple(1, 0x00FF);
    else
    glLineStipple(1, 0xFF00);

    glBegin(GL_LINE_LOOP);
    glColor4f(c, c, 0, 255);
    glVertex3f(left-lw, bottom-lw, z);
    glColor4f(c, c, 0, 255);
    glVertex3f(left-lw, top+lw, z);
    glColor4f(c, c, 0, 255);
    glVertex3f(right+lw, top+lw, z);
    glColor4f(c, c, 0, 255);
    glVertex3f(right+lw, bottom-lw, z);
    glEnd();


    if (selPulseDir > 0)
    glLineStipple(1, 0xFF00);
    else
    glLineStipple(1, 0x00FF);

    c = 0.5;
    glBegin(GL_LINE_LOOP);
    glColor4f(c, c, c, 255);
    glVertex3f(left-lw, bottom-lw, z);
    glColor4f(c, c, c, 255);
    glVertex3f(left-lw, top+lw, z);
    glColor4f(c, c, c, 255);
    glVertex3f(right+lw, top+lw, z);
    glColor4f(c, c, c, 255);
    glVertex3f(right+lw, bottom-lw, z);
    glEnd();

    glDisable(GL_LINE_STIPPLE);
  */
  /*glBegin(GL_LINE_LOOP);
    glColor4f(c, c, 0.0, (alpha/255.0));
    glVertex3f(left, bottom, z);
    glColor4f(c, c, 0.0, (alpha/255.0));
    glVertex3f(left, top, z);
    glColor4f(c, c, 0.0, (alpha/255.0));
    glVertex3f(right, top, z);
    glColor4f(c, c, 0.0, (alpha/255.0));
    glVertex3f(right, bottom, z);
    glEnd();*/
}

void overlayApp::drawOutline()
{
  // draw the outline of the app we are resizing
  float a;
  if (closing)
    a = curtainAlpha;
  else
    a = alpha/255.0;

  glBegin(GL_LINE_LOOP);
  glColor4f(0.5, 0.5, 0.5, a);
  glVertex3f(left, bottom, z);
  glColor4f(color[0], color[1], color[2], a);
  glVertex3f(left, top, z);
  glColor4f(color[0], color[1], color[2], a);
  glVertex3f(right, top, z);
  glColor4f(0.5, 0.5, 0.5, a);
  glVertex3f(right, bottom, z);
  glEnd();

  int o;
  glGetIntegerv(GL_LINE_WIDTH, &o);

  // now draw the black one
  glColor4f(0.0, 0.0, 0.0, a);
  glBegin(GL_LINE_LOOP);
  glColor4f(0.0, 0.0, 0.0, a);
  glVertex3f(left-o, bottom-o, z);
  glColor4f(0.5, 0.5, 0.5, a);
  glVertex3f(left-o, top+o, z);
  glColor4f(0.5, 0.5, 0.5, a);
  glVertex3f(right+o, top+o, z);
  glColor4f(0.0, 0.0, 0.0, a);
  glVertex3f(right+o, bottom-o, z);
  glEnd();
}


void overlayApp::drawCurtain()
{
  int o;
  glGetIntegerv(GL_LINE_WIDTH, &o);
  glDisable(GL_TEXTURE_2D);

  // now draw the black one
  glColor4f(0.0, 0.0, 0.0, curtainAlpha);
  glBegin(GL_QUADS);
  glVertex3f(left-o, bottom-o, z-0.8);
  glVertex3f(left-o, top+o, z-0.8);
  glVertex3f(right+o, top+o, z-0.8);
  glVertex3f(right+o, bottom-o, z-0.8);
  glEnd();
}


void overlayApp::drawCorners()
{
  int cs = cornerSize;

  glLineWidth(2);
  glPushMatrix();

  if (activeCorners[TOP_LEFT] == 1) {
    glTranslated(left, top, 0);
    glRotatef(270, 0.0, 0.0, 1.0);
    drawOneCorner();
  }
  if(activeCorners[TOP_RIGHT] == 1) {
    glTranslated(right, top, 0);
    glRotatef(180, 0.0, 0.0, 1.0);
    drawXCorner();
  }
  if(activeCorners[BOTTOM_RIGHT] == 1) {
    glTranslated(right, bottom, 0);
    glRotatef(90, 0.0, 0.0, 1.0);
    drawOneCorner();
  }
  if(activeCorners[BOTTOM_LEFT] == 1) {
    glTranslated(left, bottom, 0);
    drawOneCorner();
  }
  if(activeCorners[TOP_RIGHT_X] == 1) {
    glTranslated(right-cs, top, 0);
    glRotatef(180, 0.0, 0.0, 1.0);
    drawXCorner();
  }

  glPopMatrix();
}

void overlayApp::drawXCorner()
{
	  int cs = cornerSize;

	  glColor4f(color[0], color[1], color[2], 0.7);
	  glBegin(GL_LINE_STRIP);
	  glVertex3f(0, cs, z);
	  glVertex3f(cs, cs, z);
	  glVertex3f(cs, 0, z);
	  glEnd();

	  glColor4f(0.0, 0.0, 0.0, 0.7);
	  glBegin(GL_LINE_STRIP);
	  glVertex3f(0+2, cs-2, z);
	  glVertex3f(cs-2, cs-2, z);
	  glVertex3f(cs-2, 0+2, z);
	  glEnd();

	  glColor4f(1.0f, 0.0f, 0.0f, 0.7);
	  glBegin(GL_QUADS);
	  glVertex3f(0, cs, z+Z_STEP);
	  glVertex3f(cs, cs, z+Z_STEP);
	  glVertex3f(cs, 0, z+Z_STEP);
	  glVertex3f(0, 0, z+Z_STEP);
	  glEnd();
}


void overlayApp::drawOneCorner()
{
  int cs = cornerSize;

  glColor4f(color[0], color[1], color[2], 0.7);
  glBegin(GL_LINE_STRIP);
  glVertex3f(0, cs, z);
  glVertex3f(cs, cs, z);
  glVertex3f(cs, 0, z);
  glEnd();

  glColor4f(0.0, 0.0, 0.0, 0.7);
  glBegin(GL_LINE_STRIP);
  glVertex3f(0+2, cs-2, z);
  glVertex3f(cs-2, cs-2, z);
  glVertex3f(cs-2, 0+2, z);
  glEnd();

  glColor4f(corner_color[0], corner_color[1],
            corner_color[2], corner_color[3]);
  glBegin(GL_QUADS);
  glVertex3f(0, cs, z+Z_STEP);
  glVertex3f(cs, cs, z+Z_STEP);
  glVertex3f(cs, 0, z+Z_STEP);
  glVertex3f(0, 0, z+Z_STEP);
  glEnd();
}


int overlayApp::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  switch(code)
  {
  case DRAG: {
    int dx, dy, cId;
    sscanf(msg, "%d %d %d %d %d", &id, &code, &dx, &dy, &cId);
    left += dx;
    right += dx;
    top += dy;
    bottom += dy;
    state = DRAG_STATE;
    break;
  }
  case RESIZE: {
    int dx,dy,cId;
    state = RESIZE_STATE;
    sscanf(msg, "%d %d %d %d %d", &id, &code, &dx, &dy, &cId);
    cId += 2;  // the corners in the interaction manager are smaller by 2
    if (cId == BOTTOM_LEFT)
    {
      left += dx;
      bottom += dx/aspectRatio;
    }
    else if(cId == TOP_LEFT)
    {
      left += dx;
      top -= dx/aspectRatio;
    }
    else if(cId == TOP_RIGHT)
    {
      right += dx;
      top += dx/aspectRatio;
    }
    else if(cId == BOTTOM_RIGHT)
    {
      right += dx;
      bottom -= dx/aspectRatio;
    }
    //SAGE_PRINTLOG( "\nRESIZE (%d): %.1f %.1f %.1f %.1f", objectID, left, right, top, bottom);
    activeCorners[cId] = 1;
    break;
  }
  case ZOOM: {
    int l,r,t,b;
    state = RESIZE_STATE;
    sscanf(msg, "%d %d %d %d %d %d %d", &id, &code, &l, &r, &t, &b);
    left = l;
    right = r;
    top = t;
    bottom = b;
    break;
  }
  case RESET: {
    int dispId, cs;
    if (state != MINIMIZED_STATE)  // minimize will return itself to a normal state
      state = NORMAL_STATE;
    sscanf(msg, "%d %d %d %d", &id, &code, &cs, &dispId);
    cornerSize = cs;
    displayID = dispId;
    color = border_color;
    for(int i=0; i<8; i++)
      activeCorners[i] = 0;

    break;
  }
  case ON_CORNER: {
    int cId, isOn;
    sscanf(msg, "%d %d %d %d", &id, &code, &cId, &isOn);
    cId += 2;  // the corners in the interaction manager are smaller by 2
    activeCorners[cId] = isOn;
    break;
  }
  case MINIMIZE: {
    int appId, minimized;
    sscanf(msg, "%d %d %d %d", &id, &code, &appId, &minimized);

    if(bool(minimized))
      state = MINIMIZED_STATE;
    else
      state = NORMAL_STATE;
    drawParent->minimizeApp(appId, bool(minimized));
    break;
  }
  case MOUSE_OVER: {
    int appId;
    int ov;
    sscanf(msg, "%d %d %d %d", &id, &code, &ov, &appId);
    mouseOver = bool(ov);
    (drawParent->apps[appId]).mouseOver = mouseOver;
    break;
  }
  case DIM: {
    int doDim;
    sscanf(msg, "%d %d %d", &id, &code, &doDim);
    dim = bool(doDim);
    break;
  }
  case CLOSING_FADE: {
    sscanf(msg, "%d %d %f", &id, &code, &curtainAlpha);
    closing = true;
    //setDirty();
    break;
  }
  case RESET_CLOSING_FADE: {
    closing = false;
    curtainAlpha = 1.0;
    state = NORMAL_STATE;
    //setDirty();
    break;
  }
  }

  return 0;

}


void overlayApp::onNewParentBounds()
{
  // draw it outside the bounds of the app
  x -= 1;
  y -= 1;
  width += 2;
  height += 2;
  updateBoundary();
  aspectRatio = width/float(height);
}
