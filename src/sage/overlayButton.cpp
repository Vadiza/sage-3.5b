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

#include "overlayButton.h"

// messages
#define SET_STATE 0

// STATES for drawing buttons
#define DOWN_STATE 0
#define UP_STATE   1
#define MOUSE_OVER 2
#define MOUSE_NOT_OVER 3


// one default texture for buttons
GLuint overlayButton::default_tex = 0;



overlayButton::overlayButton()
{
  strcpy(objectType, BUTTON);
  state = MOUSE_NOT_OVER;
  z = TOP_Z;
  isToggle = false;
  isDown = false;

  // load the default texture and font
  if (default_tex == 0)
    default_tex = getTextureFromFile(DEFAULT_BUTTON);

  up_tex = down_tex = over_tex = current_tex = prev_tex = default_tex;
}


overlayButton::~overlayButton()
{
  // delete the textures we used
  deleteTex(up_tex);
  deleteTex(down_tex);
  deleteTex(over_tex);
}


// this just makes sure that the default_tex is not deleted
void overlayButton::deleteTex(GLuint tex)
{
  if (tex != default_tex)
    glDeleteTextures(1, &tex);
}


void overlayButton::draw()
{
  /**********    DRAW THE STUFF   ***********/
  glPushMatrix();
  glColor4ub(255, 255, 255, alpha);
  drawTexture(current_tex);
  glPopMatrix();

  // draw the font with the correct size
  if (!label.empty() && ft)
    ft->drawText(label.c_str(),fx,fy,z+Z_STEP,fontColor, alpha);
}


void overlayButton::drawTexture(GLuint tex)
{
  glEnable(GL_TEXTURE_2D);
  glTranslatef(x, y, 0);

  glBindTexture (GL_TEXTURE_2D, tex);
  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(0.0, 0.0, z);

  glTexCoord2f(0.0, 1.0);
  glVertex3f(0.0, height, z);

  glTexCoord2f(1.0, 1.0);
  glVertex3f(width, height, z);

  glTexCoord2f(1.0, 0.0);
  glVertex3f(width, 0, z);

  glEnd();
}


int overlayButton::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  switch(code)
  {
  case SET_STATE:
    sscanf(msg, "%d %d %d", &id, &code, &state);
    if (state == UP_STATE) {
      current_tex = up_tex;
      prev_tex = up_tex;
      isDown = false;
    }
    else if(state == DOWN_STATE) {
      isDown = true;
      current_tex = down_tex;
      prev_tex = down_tex;
    }
    else if (state == MOUSE_OVER) {
      if (isToggle)
        current_tex = down_tex;
      else {
        prev_tex = current_tex;
        current_tex = over_tex;
      }
      fadeTime = 0.5;
      fade(FADE_IN, alpha, 255);
    }
    else if (state == MOUSE_NOT_OVER) {
      current_tex = prev_tex;
      fadeTime = 1.0;
      fade(FADE_OUT, 255, initAlpha);
    }
    break;
  }

  return 0;
}

void overlayButton::parseGraphicsInfo(TiXmlElement *parent)
{
  up_tex = getTextureFromXml(parent, "up");
  if (up_tex == 0)
    up_tex = default_tex;

  down_tex = getTextureFromXml(parent, "down");
  if (down_tex == 0)
    down_tex = up_tex;

  over_tex = getTextureFromXml(parent, "over");
  if (over_tex == 0)
    over_tex = up_tex;

  current_tex = up_tex;
  prev_tex = up_tex;
}


void overlayButton::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *s = parent->FirstChildElement("toggle");
  if (s)
    isToggle = true;
  else
    isToggle = false;
}
