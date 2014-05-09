/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlaySplitter.cpp
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

#include "overlaySplitter.h"



#define SET_STATE 0

#define MOUSE_OVER 0
#define MOUSE_NOT_OVER 1


overlaySplitter::overlaySplitter()
{
  strcpy(objectType, SPLITTER);
  z = TOP_Z;
  scaleMultiplier = 4.0;
  state = MOUSE_NOT_OVER;
  tex = 0;
  buffer = 0;
}


overlaySplitter::~overlaySplitter()
{
  // delete the textures we used
  glDeleteTextures(1, &tex);
}


void overlaySplitter::draw()
{
  glPushMatrix();
  glColor4ub(255,255,255, alpha);

  if (state == MOUSE_OVER)
    drawTexture(tex, scaleMultiplier);
  else
    drawTexture(tex, 1.0);

  glPopMatrix();
}


void overlaySplitter::drawTexture(GLuint tex, float s)
{
  glEnable(GL_TEXTURE_2D);

  float ww = width;
  float hh = height;
  if (direction == HORIZONTAL) {
    hh *= s;
    ww -= buffer;
    glTranslatef(x+buffer/2.0, y-hh/2.0, 0);
  }
  else {
    ww *= s;
    hh -= buffer;
    glTranslatef(x-ww/2.0, y+buffer/2.0, 0);
  }

  glBindTexture (GL_TEXTURE_2D, tex);
  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(0.0, 0.0, z);

  glTexCoord2f(0.0, 1.0);
  glVertex3f(0.0, hh, z);

  glTexCoord2f(1.0, 1.0);
  glVertex3f(ww, hh, z);

  glTexCoord2f(1.0, 0.0);
  glVertex3f(ww, 0, z);

  glEnd();
}


void overlaySplitter::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *s = parent->FirstChildElement("splitter");
  if (!s)
    return;

  s->QueryFloatAttribute("scaleMultiplier", &scaleMultiplier);
  s->QueryIntAttribute("direction", &direction);
  s->QueryIntAttribute("buffer", &buffer);
}



void overlaySplitter::parseGraphicsInfo(TiXmlElement *parent)
{
  tex = getTextureFromXml(parent, "image");
}


int overlaySplitter::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  switch(code) {

  case SET_STATE: {
    sscanf(msg, "%d %d %d %d", &id, &code, &state, &buffer);

    // if we are enlarged, draw it after the parent because
    // of the temporary overlap with the neighboring widgets
    if (state == MOUSE_OVER)  {
      delayDraw = true;
      //drawOrder = SAGE_INTER_DRAW;
    }
    else if (state == MOUSE_NOT_OVER) {
      delayDraw = false;
      //drawOrder = SAGE_PRE_DRAW;
    }
    break;
  }
  }

  return 0;
}
