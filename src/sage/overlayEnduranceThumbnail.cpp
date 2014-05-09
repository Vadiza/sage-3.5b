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

#include "overlayEnduranceThumbnail.h"


overlayEnduranceThumbnail::overlayEnduranceThumbnail()
  : overlayThumbnail()
{
  strcpy(objectType, ENDURANCE_THUMBNAIL);
  rating = 0;
  scaleMultiplier = 1.4;
}


void overlayEnduranceThumbnail::draw()
{
  int a = alpha;
  float s = 1.0; // temp thumbnail scale
  float drawX, drawY;

  //overlayThumbnail::draw();

  /**********    DRAW THE THUMBNAIL BASE CLASS STUFF   ***********/

  glPushMatrix();
  if (state == DRAG_STATE) {
    a = 255;
    s = scaleMultiplier;
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
  glPopMatrix();


  // draw the font with the correct size
  glPushMatrix();
  if (!label.empty() && ft) {
    if (state == DRAG_STATE)
      glTranslatef(drawX-x, drawY-y, 0);
    ft->drawText(label.c_str(),fx,fy,z+Z_STEP,fontColor, a);
  }
  else if(!tooltip.empty() && (state == MOUSE_OVER || state == DRAG_STATE)) {  // draw the tooltip
    glTranslatef(drawX, drawY, 0);
    overlayThumbnail::drawFontBackground(0,(height*scaleMultiplier)-ttSize, width*scaleMultiplier, ttSize);
    ft = getFont(ttSize);
    ft->drawText(tooltip.c_str(),3,(height*scaleMultiplier)-ttSize,z+Z_STEP, fontColor, a);
  }
  glPopMatrix();
  /// end thumbnail draw code


  /**********    DRAW THE STUFF   ***********/
  if (state == DRAG_STATE) {
    a = 255;//150;
    s = scaleMultiplier;
    drawX = dragX;
    drawY = dragY;
  }
  else
  {
    if (state == MOUSE_OVER)
      s = scaleMultiplier;
    drawX = x - (width * s - width) / 2.0;
    drawY = y - (height * s - height) / 2.0;
  }
  glPushMatrix();
  glTranslatef(drawX, drawY, 0);
  drawRating(s);
  glPopMatrix();
}


void overlayEnduranceThumbnail::drawRating(float s)
{
  if (rating > 0) {
    glDisable(GL_TEXTURE_2D);

    int c = 255; // color
    if(rating == 1) c=0;
    else if(rating == 2) c=50;
    else if(rating == 3) c=120;
    else if(rating == 4) c=200;
    else if(rating == 5) c=255;

    glColor4ub(c, 255 - c, 0, 255);
    int lineWidth = lineW(5*displayScale);
    glLineWidth(lineWidth * 2);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-lineWidth, -lineWidth, z);
    glVertex3f(-lineWidth, height * s + lineWidth, z);
    glVertex3f(width * s + lineWidth, height * s + lineWidth, z);
    glVertex3f(width * s + lineWidth, -lineWidth, z);
    glEnd();

    // draw the rating number
    ft = getFont(ttSize);
    char rt[2];
    sprintf(rt, "%d", rating);
    ft->drawText(rt,2,2,z+Z_STEP,fontColor, 255);
  }
}

void overlayEnduranceThumbnail::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *s = parent->FirstChildElement("enduranceThumbnail");
  if (!s)
    return;

  s->QueryIntAttribute("rating", &rating);
  overlayThumbnail::parseSpecificInfo(parent);
}


int overlayEnduranceThumbnail::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  switch(code) {
  case SET_RATING:
    sscanf(msg, "%d %d %d", &id, &code, &rating);
    break;
  }

  overlayThumbnail::parseMessage(msg);

  return 0;
}
