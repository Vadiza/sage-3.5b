/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sagePointer.cpp
 * Author : Ratko Jagodic, Byungil Jeong
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

#include "overlayPointer.h"


// custom messages for the pointers
#define RESET  0
#define MOVE   1
#define DRAG   2
#define RESIZE 3
#define DOWN   4
#define UP     5
#define IN_APP 6  // just a color change
#define ANGLE  7
#define ZOOM   8
#define ROTATE 9
#define COLOR  10
#define SELECT 11
#define SHOW_SPLITTER 12
#define CAPTURE 13
#define CROSS 14


// STATES for drawing pointers
#define NORMAL_SHAPE 10
#define DRAG_SHAPE 11
#define RESIZE_SHAPE 12
#define ZOOM_SHAPE 13
#define ROTATE_SHAPE 14
#define CROSS_SHAPE 15



overlayPointer::overlayPointer()
{
  strcpy(objectType, POINTER);
  ptrShape = NORMAL_SHAPE;
  orientation = 0;
  angle = 0;
  inApp = false;
  selection = false;
  capture = false;

  up_color[0] = 0.9;
  up_color[1] = 0.1;
  up_color[2] = 0.1;

  down_color[0] = 0.7;
  down_color[1] = 0.0;
  down_color[2] = 0.0;

  in_app_color[0] = 0.8;
  in_app_color[1] = 0.85;
  in_app_color[2] = 0.0;

  color = up_color;

  doDrawSplitter = false;
  splX1, splY1, splX2, splY2 = 0;
  splW = 50;

  tex = 0;
  //SAGE_PRINTLOG( "\n\n POINTER CREATED!\n");
}


void overlayPointer::update(double now)
{
  if (tooltip.find("pqlabs") == string::npos)
    return;

  float rate = 20.0;
  fadeTime = 0.5;
  if (now - lastTime > 1000000.0/rate) {
    lastTime = now;
    if (doFade) {
      fadeAlpha -= int(255.0 * (1.0/(rate*fadeTime)));  // make it run for 2 secs
      //SAGE_PRINTLOG( "alpha
      if(fadeAlpha < 0) {
        fadeAlpha = 0;
        doFade = false;
        visible = false;
      }
      alpha = fadeAlpha;

      // redraw
      setDirty();
    }
  }
}



void overlayPointer::drawTexture(GLuint tex)
{
  glEnable(GL_TEXTURE_2D);
  glTranslatef(x, y, 0);

  glBindTexture (GL_TEXTURE_2D, tex);
  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(-width/2.0, -height/2.0, z);

  glTexCoord2f(0.0, 1.0);
  glVertex3f(-width/2.0, height/2.0, z);

  glTexCoord2f(1.0, 1.0);
  glVertex3f(width/2.0, height/2.0, z);

  glTexCoord2f(1.0, 0.0);
  glVertex3f(width/2.0, -height/2.0, z);

  glEnd();
}


void overlayPointer::draw()
{

  if (doDrawSplitter)
    drawSplitter();


  if ( (tooltip.find("pqlabs") != string::npos) && visible) {
    width = height;
    glPushMatrix();
    glColor4ub(255, 255, 255, alpha);
    drawTexture(tex);
    glPopMatrix();
    //SAGE_PRINTLOG( "\n POINTER alpha %d DRAWN at %d %d!!!!\n", alpha, x, y);
  }
  else {
    glDisable(GL_TEXTURE_2D);
    float w = width;
    float h = height;

    // set the pointer color depending on the state it's in
    glColor4f(color[0], color[1], color[2], 1.0);

    glPushMatrix();


    /**********    DRAW CURSOR   ***********/
    int i = 0;
    if (ptrShape == DRAG_SHAPE)
    {
      w = w/2;
      h = h/2;
      for(i; i<4; i++)
      {
        glPushMatrix();

        glTranslatef(x, y, 0);
        glRotatef(90*i+angle, 0,0,1);
        glTranslatef(0, h+10, 0);
        drawCursor(w, h);

        glPopMatrix();
      }
    }

    else if(ptrShape == RESIZE_SHAPE)
    {
      w = w/1.5;
      h = h/1.5;
      int deg = 0;
      if (orientation == 1 || orientation == 3)
        deg = 135;
      else
        deg = 45;

      for(i; i<2; i++)
      {
        glPushMatrix();

        glTranslatef(x, y, 0);
        glRotatef(deg + 180*i, 0,0,1);
        glTranslatef(0, h, 0);
        drawCursor(w, h);

        glPopMatrix();
      }
    }

    else if(ptrShape == ZOOM_SHAPE)
    {
      float degInRad = 0.0;
      float radius = w/2;
      float hl = 270 - radius * 0.1;  //handle left
      float hr = 270 + radius * 0.1;  //handle right
      float hh = radius*1.7;  //handle height

      glPushMatrix();

      // translate and rotate to the position of the pointer
      glTranslatef(x, y, 0);
      glRotated(angle, 0,0,1);


      /**************  draw magnifying glass ****************/

      glPushMatrix();
      glRotated(35, 0,0,1);


      // inner transparent circle
      glColor4f(1.0, 1.0, 1.0, 0.2);
      glBegin(GL_POLYGON);
      for (int i=0; i < 360; i+=5)
      {
        degInRad = i*DEG2RAD;
        glVertex3f(cos(degInRad)*radius,sin(degInRad)*radius, z);
      }
      glVertex3f(cos(DEG2RAD)*radius,sin(DEG2RAD)*radius, z); //final point
      glEnd();


      // outline of the circle
      glColor4f(1, 1, 1, 1.0);
      glLineWidth(radius/10);
      glBegin(GL_LINE_LOOP);
      for (int i=0; i < 360; i+=5)
      {
        degInRad = i*DEG2RAD;
        glVertex3f(cos(degInRad)*radius,sin(degInRad)*radius, z);
      }
      glEnd();


      // draw the handle of the zoom lens
      glBegin(GL_POLYGON);
      glVertex3f(cos(hl*DEG2RAD)*radius,sin(hl*DEG2RAD)*radius, z);
      glVertex3f(cos(hl*DEG2RAD)*radius,sin(hl*DEG2RAD)*radius - radius*1.7, z);
      glVertex3f(cos(hr*DEG2RAD)*radius,sin(hl*DEG2RAD)*radius - radius*1.7, z);
      glVertex3f(cos(hr*DEG2RAD)*radius,sin(hl*DEG2RAD)*radius, z);
      glVertex3f(cos(hl*DEG2RAD)*radius,sin(hl*DEG2RAD)*radius, z);
      glEnd();

      glPopMatrix();


      // plus/minus
      float pw = radius - radius*0.1;  // line length
      float po = radius - radius*0.5;  // plus/minus offset

      glBegin(GL_LINES);
      // plus
      glVertex3f(-pw/2, po/2, z);
      glVertex3f(pw/2,  po/2, z);
      glVertex3f(0, -pw/2+po/2, z);
      glVertex3f(0,  pw/2+po/2, z);

      // minus
      glVertex3f(-pw/2, -po, z);
      glVertex3f(pw/2, -po, z);
      glEnd();





      /**************  draw up/down arrows ****************/

      glPushMatrix();
      w = radius * 0.7;
      h = w*1.5;

      glTranslatef(radius+w, 0, 0);
      for(int i=0; i<2; i++)
      {
        glPushMatrix();

        glRotated(180*i, 0,0,1);
        glTranslatef(0, h, 0);
        drawCursor(w, h);

        glPopMatrix();
      }
      glPopMatrix();


      glPopMatrix();
    }

    else if (ptrShape == ROTATE_SHAPE)
    {
      w = w/1.5;
      h = h/1.5;
      float radius = w*1.2;
      glPushMatrix();

      // translate and rotate to the position of the pointer
      glTranslatef(x, y, 0);
      glRotated(angle, 0,0,1);
      //glRotated(45, 1,1,0);

      glColor4f(1, 1, 1, 1.0);
      glLineWidth(radius/10);

      // the rotate arrows
      glPushMatrix();
      drawRotateArrows(radius);
      glRotated(180,0,0,1);
      drawRotateArrows(radius);
      glPopMatrix();


      // the inner directional arrows
      for(int i=0; i<4; i++)
      {
        glPushMatrix();

        //glTranslatef(x, y, 0);
        glRotatef(90*i+angle, 0,0,1);
        glTranslatef(0, h/2.5+10, 0);
        drawCursor(w/2.5, h/2.5);

        glPopMatrix();
      }

      glPopMatrix();
    }

    else if (ptrShape == CROSS_SHAPE) {
      float radius = w/5;
      float degInRad = 0.0;

      if (capture)
        drawCapture();

      // inner transparent circle
      glPushMatrix();
      glTranslatef(x, y, 0);

      glColor4f(0.7, 0.7, 0.7, 0.7);
      glBegin(GL_POLYGON);
      for (int i=0; i < 360; i+=20)
      {
        degInRad = i*DEG2RAD;
        glVertex3f(cos(degInRad)*radius,sin(degInRad)*radius, z);
      }
      glVertex3f(cos(DEG2RAD)*radius,sin(DEG2RAD)*radius, z); //final point
      glEnd();
      glPopMatrix();


      // cross
      glLineWidth( lineW(1*displayScale));
      glColor4f(0.8, 0.8, 0.8, 0.7);
      glBegin(GL_LINES);
      glVertex3f(x-h/2, y, z);
      glVertex3f(x+h/2, y, z);
      glVertex3f(x, y-h/2, z);
      glVertex3f(x, y+h/2, z);
      glEnd();

    }

    else // NORMAL_SHAPE
    {
      if (selection)
        drawSelection();

      glTranslatef(x, y, 0);

      // draw the tooltip
      glPushMatrix();
      glTranslatef(w/1.7, -h-ttSize/4, 0);
      drawTooltip();
      glPopMatrix();

      // draw the cursor
      glRotatef(30+angle, 0,0,1);
      glDisable(GL_TEXTURE_2D);
      drawCursor(w, h);
    }

    glPopMatrix();
  }

}


void overlayPointer::drawSelection()
{
  float c = 0.8;

  glLineWidth( lineW(2*displayScale) );

  glColor4f(1, 1, 1, 0.17);
  glBegin(GL_QUADS);
  glVertex3f(selL, selB, z);
  glVertex3f(selL, selT, z);
  glVertex3f(selR, selT, z);
  glVertex3f(selR, selB, z);
  glEnd();


  //glColor4f(c, c, 0, 0.8);
  glColor4ub(137, 224, 224, 204);
  glBegin(GL_LINE_LOOP);
  glVertex3f(selL, selB, z);
  glVertex3f(selL, selT, z);
  glVertex3f(selR, selT, z);
  glVertex3f(selR, selB, z);
  glEnd();

}


void overlayPointer::drawCapture()
{
  float c = 0.8;
  int lw = lineW(2*displayScale);
  float lo = lw;  // line offset to not include the line in screen capture

  glLineWidth(lw);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(8, 21845);

  glColor4f(c, c, c, 0.7);
  glBegin(GL_LINE_LOOP);
  glVertex3f(selL-lo, selB-lo, z);
  glVertex3f(selL-lo, selT+lo, z);
  glVertex3f(selR+lo, selT+lo, z);
  glVertex3f(selR+lo, selB-lo, z);
  glEnd();

  glDisable(GL_LINE_STIPPLE);
}


void overlayPointer::drawSplitter()
{
  glDisable(GL_TEXTURE_2D);
  int dir = HORIZONTAL;
  if (splX1 == splX2)
    dir = VERTICAL;
  int d = splW;

  if (dir == HORIZONTAL) {
    glBegin(GL_QUADS);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX1, splY1, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX1, splY1-d, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX2, splY1-d, z);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX2, splY1, z);
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX1, splY1, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX1, splY1+d, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX2, splY1+d, z);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX2, splY1, z);
    glEnd();
  }
  else {   // VERTICAL
    glBegin(GL_QUADS);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX1, splY1, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX1-d, splY1, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX1-d, splY2, z);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX1, splY2, z);
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX1, splY1, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX1+d, splY1, z);
    glColor4f(0, 0, 0, 0.0);
    glVertex3f(splX1+d, splY2, z);
    glColor4f(0, 0, 0, 1.0);
    glVertex3f(splX1, splY2, z);
    glEnd();

  }

  glLineWidth( lineW(4*displayScale) );

  glColor4f(1, 1, 1, 0.6);
  glBegin(GL_LINES);
  glVertex3f(splX1, splY1, z);
  glVertex3f(splX2, splY2, z);
  glEnd();

}


void overlayPointer::drawRotateArrows(float radius)
{
  float degInRad = 0.0;

  // half circle
  glBegin(GL_LINE_STRIP);
  for (int i=0; i < 150; i++)
  {
    degInRad = i*DEG2RAD;
    glVertex3f(cos(degInRad)*radius,sin(degInRad)*radius, z);
  }
  glEnd();

  // arrow
  glPushMatrix();
  glTranslatef(radius,0, 0);
  glBegin(GL_LINES);
  glVertex3f(0,0,z);
  glVertex3f(-radius/5, radius/3, z);
  glVertex3f(0,0,z);
  glVertex3f(radius/5, radius/3, z);
  glEnd();
  glPopMatrix();
}

void overlayPointer::drawCursor(double w, double h)
{
  // draw the outline of the cursor
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glBegin(GL_POLYGON);
  glVertex3f(0, 0, z);
  glVertex3f(-w/2, -h, z);
  glVertex3f(0, -h+h/5, z);
  glEnd();
  glBegin(GL_POLYGON);
  glVertex3f(0, 0, z);
  glVertex3f(0, -h+h/5, z);
  glVertex3f(w/2, -h, z);
  glEnd();

  // draw the filled polygon
  glTranslatef(0, -h*0.15/2, 0);
  glColor4f(color[0], color[1], color[2], 1.0);
  double sw = w - w*0.15;
  double sh = h - h*0.15;
  glBegin(GL_POLYGON);
  glVertex3f(0, 0, z);
  glVertex3f(-sw/2, -sh, z);
  glVertex3f(0, -sh+sh/5, z);
  glEnd();
  glBegin(GL_POLYGON);
  glVertex3f(0, 0, z);
  glVertex3f(0, -sh+sh/5, z);
  glVertex3f(sw/2, -sh, z);
  glEnd();
}



int overlayPointer::destroy()
{
  return 0;
}

void overlayPointer::parseGraphicsInfo(TiXmlElement *parent)
{
  tex = getTextureFromXml(parent, "image");
}

int overlayPointer::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  switch(code)
  {
  case MOVE: {
    int newx,newy;
    sscanf(msg, "%d %d %d %d", &id, &code, &newx, &newy);
    x = newx;
    y = newy;
    pos.x = newx;
    pos.y = newy;

    visible = true;
    alpha = 255;
    fadeAlpha = 255;
    doFade = true;
    lastTime = sage::getTime();
    //setDirty();
    //fade(FADE_OUT, 255, 0);

    //SAGE_PRINTLOG( "\nMOVE to %d %d", x, y);
    break;
  }
  case DRAG: {
    ptrShape = DRAG_SHAPE;
    break;
  }
  case RESIZE: {
    int cId;
    ptrShape = RESIZE_SHAPE;
    sscanf(msg, "%d %d %d", &id, &code, &cId);
    orientation = cId;
    break;
  }
  case DOWN: {
    color = down_color;
    break;
  }
  case UP: {
    color = up_color;
    ptrShape = NORMAL_SHAPE;
    break;
  }
  case RESET: {
    ptrShape = NORMAL_SHAPE;
    //color = up_color;
    orientation = 0;
    selection = false;
    capture = false;
    break;
  }
  case IN_APP: {
    int i;
    sscanf(msg, "%d %d %d", &id, &code, &i);
    if(i==0)
    {
      color = up_color;
      inApp = false;
    }
    else
    {
      color = in_app_color;
      inApp = true;
    }
    break;
  }
  case ANGLE: {
    int a;
    sscanf(msg, "%d %d %d", &id, &code, &a);
    angle = a;
    break;
  }
  case ZOOM: {
    ptrShape = ZOOM_SHAPE;
    break;
  }
  case ROTATE: {
    ptrShape = ROTATE_SHAPE;
    break;
  }
  case COLOR: {
    int r,g,b;
    sscanf(msg, "%d %d %d %d %d", &id, &code, &r, &g, &b);
    up_color[0] = r/255.0;
    up_color[1] = g/255.0;
    up_color[2] = b/255.0;
    break;
  }
  case SELECT: {
    sscanf(msg, "%d %d %d %d %d %d", &id, &code, &selL, &selR, &selT, &selB);
    selection = true;
    break;
  }

  case SHOW_SPLITTER: {
    int doDraw;
    sscanf(msg, "%d %d %d %d %d %d %d %d", &id, &code, &splX1, &splY1, &splX2, &splY2, &splW, &doDraw);
    doDrawSplitter = bool(doDraw);
    break;
  }
  case CAPTURE: {
    int doCapture;
    sscanf(msg, "%d %d %d %d %d %d %d", &id, &code, &doCapture, &selL, &selR, &selT, &selB);
    capture = bool(doCapture);
    break;
  }
  case CROSS: {
    ptrShape = CROSS_SHAPE;
    break;
  }
  }

  return 0;

}
