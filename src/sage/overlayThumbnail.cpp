/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayButton.cpp
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

#include "overlayThumbnail.h"


overlayThumbnail::overlayThumbnail()
  :overlayButton()
{
  strcpy(objectType, THUMBNAIL);
  dragX = dragY = 0;
  scaleMultiplier = 1.4;
  outline = false;
  doScale = false;
  currScale = 1.0; // for smoothly resizing the thumbnail
  scaleTime = 0.2; // how long is this smooth scaling
}


void overlayThumbnail::draw()
{
  int a = alpha;
  float s = currScale; // temp thumbnail scale
  float drawX, drawY;

  /**********    DRAW THE STUFF   ***********/
  glPushMatrix();
  if (state == DRAG_STATE) {
    a = 150;
    drawX = dragX;
    drawY = dragY;
  }
  else
  {
    if (state == MOUSE_OVER)
      s = scaleMultiplier;
    drawX = x-(width*s-width)/2.0;
    drawY = y-(height*s-height)/2.0;
  }
  glTranslatef(drawX, drawY, 0);

  glColor4ub(255, 255, 255, a);
  drawTexture(current_tex, s);
  if (selected)
    drawSelection(s);
  else if (outline)
    drawOutline(s);
  glPopMatrix();

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  // draw the font with the correct size
  glPushMatrix();
  if (!label.empty() && ft) {
    if (state == DRAG_STATE)
      glTranslatef(drawX-x, drawY-y, 0);
    ft->drawText(label.c_str(),fx,fy,z+Z_STEP,fontColor, a);
  }
  else if(!tooltip.empty() && state == MOUSE_OVER) {  // draw the tooltip
    glTranslatef(drawX, drawY, 0);
    drawFontBackground(0,(height*scaleMultiplier)-ttSize, width*scaleMultiplier, ttSize);
    ft = getFont(ttSize);
    ft->drawText(tooltip.c_str(),3,(height*scaleMultiplier)-ttSize,z+Z_STEP, fontColor, a);
  }
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glPopMatrix();
}

void overlayThumbnail::drawOutline(float s)
{
  glDisable(GL_TEXTURE_2D);
  float c = 1.0;
  glLineWidth( lineW(2*displayScale) );
  glBegin(GL_LINE_LOOP);
  glColor4f(c, c, c, (alpha/255.0));
  glVertex3f(0.0, 0.0, z);

  glColor4f(c, c, c, (alpha/255.0));
  glVertex3f(0.0, height*s, z);

  glColor4f(c, c, c, (alpha/255.0));
  glVertex3f(width*s, height*s, z);

  glColor4f(c, c, c, (alpha/255.0));
  glVertex3f(width*s, 0, z);
  glEnd();
}


void overlayThumbnail::drawSelection(float s)
{
  glDisable(GL_TEXTURE_2D);
  float c = 0.8;
  glLineWidth( lineW(2*displayScale) );
  glBegin(GL_LINE_LOOP);
  glColor4f(c, c, 0.0, (alpha/255.0));
  glVertex3f(0.0, 0.0, z);

  glColor4f(c, c, 0.0, (alpha/255.0));
  glVertex3f(0.0, height*s, z);

  glColor4f(c, c, 0.0, (alpha/255.0));
  glVertex3f(width*s, height*s, z);

  glColor4f(c, c, 0.0, (alpha/255.0));
  glVertex3f(width*s, 0, z);
  glEnd();
}


void overlayThumbnail::drawFontBackground(float x, float y, float w, float h)
{
  glDisable(GL_TEXTURE_2D);
  glColor4ub(50,50,50,220);

  glBegin(GL_QUADS);
  glVertex3f(x, y, z);
  glVertex3f(x, y+h, z);
  glVertex3f(x+w, y+h, z);
  glVertex3f(x+w, y, z);
  glEnd();
}


void overlayThumbnail::drawTexture(GLuint tex, float s)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, tex);
  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(0.0, 0.0, z);

  glTexCoord2f(0.0, 1.0);
  glVertex3f(0.0, height*s, z);

  glTexCoord2f(1.0, 1.0);
  glVertex3f(width*s, height*s, z);

  glTexCoord2f(1.0, 0.0);
  glVertex3f(width*s, 0, z);

  glEnd();
}


void overlayThumbnail::update(double now)
{
  float rate = 10.0; // x times a sec

  if (now - lastTime > 1000000.0/rate) {
    lastTime = now;
    if (doScale) {
      currScale -= (scaleMultiplier-1.0) * (1.0/(rate*scaleTime));  // make it run for 2 secs
      if(currScale <= 1.0) {
        currScale = 1.0;
        doScale = false;
      }

      // redraw
      setDirty();
    }
  }
}



void overlayThumbnail::parseSpecificInfo(TiXmlElement *parent)
{
  overlayButton::parseSpecificInfo(parent);

  TiXmlElement *s = parent->FirstChildElement("thumbnail");
  if (!s)
    return;
  int d;
  s->QueryFloatAttribute("scaleMultiplier", &scaleMultiplier);
  s->QueryIntAttribute("drawOutline", &d);
  outline = bool(d);
}


int overlayThumbnail::parseMessage(char *msg)
{
  int id, code, animateScale, newState;
  animateScale == 0;
  sscanf(msg, "%d %d", &id, &code);

  switch(code) {
  case DRAG:
    sscanf(msg, "%d %d %d %d", &id, &code, &dragX, &dragY);
    state = DRAG_STATE;
    break;
  case SET_STATE:
    sscanf(msg, "%d %d %d", &id, &code, &newState);
    if (newState == MOUSE_NOT_OVER)
      sscanf(msg, "%d %d %d %d", &id, &code, &newState, &animateScale);
    break;
  }


  // so that we dont animate scaling when the mouse is simply over the button
  overlayButton::parseMessage(msg);

  // if we are enlarged, draw it after the parent because
  // of the temporary overlap with the neighboring widgets
  if (state == MOUSE_OVER || state == DRAG_STATE)
    delayDraw = true;
  else if (state == MOUSE_NOT_OVER) {
    if (animateScale==1 && !isToggle) {
      currScale = scaleMultiplier; // start here and slowly size down
      doScale = true;
    }
    else
      currScale = 1.0;

    delayDraw = false;
  }

  return 0;
}
