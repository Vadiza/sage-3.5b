/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayMenu.cpp
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

#include "overlayMenu.h"

// states
#define SHOWN 0
#define HIDDEN 1

// messages
#define SHOW_MENU 10
#define HIDE_MENU 11
#define SELECT_MENU_ITEM 12



overlayMenu::overlayMenu()
{
  strcpy(objectType, MENU);
  shownZ = TOP_Z + 0.5; // above everything else
  menuState = HIDDEN;
  currSelection = -1;  // nothing
  shownBounds = sageRect();
  ft = NULL;
}

overlayMenu::~overlayMenu()
{
  menuItems.clear();
}


void overlayMenu::init()
{
  for (int i=0; i<menuItems.size(); i++)
    menuItems[i]->init(-1, drawParent, (char*)LABEL);
}


void overlayMenu::update(double now)
{
  overlayButton::update(now);
  for (int i=0; i<menuItems.size(); i++)
    menuItems[i]->update(now);
}


void overlayMenu::draw()
{
  // draw the menu button first
  overlayButton::draw();
  //SAGE_PRINTLOG( "\nFont color = %d %d %d %d", fontColor.r, fontColor.g, fontColor.b, fontColor.a );
  // the menu itself will be drawn by sageDraw::draw meethod
  // drawMenu();
}

void overlayMenu::drawMenu()
{
  if (menuState == SHOWN || (menuItems.size()>0 && menuItems[0]->doFade)) {
    float hiddenZ = z;  // preserve the z
    z = shownZ;   // draw the menu on top of everything else

    // draw the menu items
    //glColor4f(1.0, 1.0, 1.0, 1.0);
    for (int i=0; i<menuItems.size(); i++)
      menuItems[i]->draw();

    glDisable(GL_TEXTURE_2D);

    if (currSelection != -1)
      drawSelection();

    drawOutline();
    z = hiddenZ;  // restore z
  }
}

void overlayMenu::drawSelection()
{
  glColor4f(1,1,1,0.3);

  int w = shownBounds.width;
  int h = menuItems[currSelection]->height;
  int x = shownBounds.x;
  int y = int(shownBounds.top - currSelection*h - h);

  glBegin(GL_QUADS);
  glVertex3f(x, y, z+Z_STEP*3);
  glVertex3f(x, y+h, z+Z_STEP*3);
  glVertex3f(x+w, y+h, z+Z_STEP*3);
  glVertex3f(x+w, y, z+Z_STEP*3);
  glEnd();
}

void overlayMenu::drawOutline()
{
  glColor4ub(255,255,255,menuItems[0]->fadeAlpha);

  int w = shownBounds.width + 2;
  int h = shownBounds.height + 2;
  int x = shownBounds.x - 1;
  int y = shownBounds.y - 1;

  glBegin(GL_LINE_LOOP);
  glVertex3f(x, y, z+Z_STEP*2);
  glVertex3f(x, y+h, z+Z_STEP*2);
  glVertex3f(x+w, y+h, z+Z_STEP*2);
  glVertex3f(x+w, y, z+Z_STEP*2);
  glEnd();
}


int overlayMenu::parseMessage(char *msg)
{
  int id, code;
  sscanf(msg, "%d %d", &id, &code);

  switch(code)
  {
  case SHOW_MENU:
    int l, r, t, b;
    sscanf(msg, "%d %d %d %d %d %d", &id, &code, &l, &r, &t, &b);
    shownBounds.x = l;
    shownBounds.y = b;
    shownBounds.width = r-l;
    shownBounds.height = t-b;
    shownBounds.updateBoundary();
    updateMenuItems();
    menuState = SHOWN;
    break;

  case HIDE_MENU:
    menuState = HIDDEN;
    currSelection = -1;
    for (int i=0; i<menuItems.size(); i++)
      menuItems[i]->fade(FADE_OUT);
    break;

  case SELECT_MENU_ITEM:
    sscanf(msg, "%d %d %d", &id, &code, &currSelection);
    break;

  default:
    overlayButton::parseMessage(msg);
    break;
  }
  return 0;
}


void overlayMenu::updateMenuItems()
{
  sageRect r = shownBounds;  // for less typing
  labelMenuItem *mi;
  int h = int(r.height / menuItems.size());

  for (int i=0; i<menuItems.size(); i++) {
    mi = (labelMenuItem*)menuItems[i];
    mi->scale = scale;
    mi->setFontSize(ftSize);

    // position
    mi->x = r.x;
    mi->y = int(r.top - mi->itemId*h - h);

    // size
    mi->width = r.width;
    mi->height = h;
    mi->z = shownZ;

    // label
    mi->fontColor = fontColor;
    mi->backgroundColor = backgroundColor;
    mi->updateFontPosition();
    mi->initAlpha = initAlpha;
    mi->alpha = 0;
    mi->fadeTime = 0.2;
    mi->fade(FADE_IN);
  }
}

void overlayMenu::parseSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *m = parent->FirstChildElement("menu");
  if (!m)
    return;

  TiXmlElement *child = NULL;
  while( child = (TiXmlElement*)m->IterateChildren( child ) )  {
    labelMenuItem *mi = new labelMenuItem();
    child->QueryIntAttribute("id", &mi->itemId);
    mi->setLabel(string(child->GetText()), ftSize);
    menuItems.push_back(mi);
  }

  overlayButton::parseSpecificInfo(parent);
}
