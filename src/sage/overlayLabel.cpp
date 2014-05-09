/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayLabel.cpp
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

#include "overlayLabel.h"



overlayLabel::overlayLabel()
{
  strcpy(objectType, LABEL);
  z = TOP_Z;
  ft = NULL;
  background = false;
}


void overlayLabel::draw()
{
  /**********    DRAW THE LABEL   ***********/

  if (!label.empty() && ft) {
    drawBackground();
    ft->drawText(label.c_str(),fx,fy,z+Z_STEP, fontColor, alpha);
  }
}


void overlayLabel::drawBackground()
{
  if (!background)
    return;

  glDisable(GL_TEXTURE_2D);
  glColor4ub(backgroundColor.r, backgroundColor.g, backgroundColor.b, alpha);

  glBegin(GL_QUADS);
  glVertex3f(x, y, z);
  glVertex3f(x, y+height, z);
  glVertex3f(x+width, y+height, z);
  glVertex3f(x+width, y, z);
  glEnd();
}


void overlayLabel::drawTexture(GLuint tex)
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

void overlayLabel::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *f = parent->FirstChildElement("background");
  if (f)
    background = true;
}


int overlayLabel::parseMessage(char *msg)
{
  return 0;
}