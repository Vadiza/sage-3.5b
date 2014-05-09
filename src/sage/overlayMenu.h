/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayMenu.h
 * Author : Ratko Jagodic
 *
 *   Description: widget overlays
 *
 * Copyright (C) 2007 Electronic Visualization Laboratory,
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
 *********************************************************************************/

#ifndef MENU_OVERLAY_H
#define MENU_OVERLAY_H

#include "overlayButton.h"
#include "overlayLabel.h"
#include <vector>
#include <string>

using namespace std;



class labelMenuItem : public overlayLabel {
public:
  int itemId;

  labelMenuItem() { labelAlignment=LEFT; }
  virtual ~labelMenuItem() {}
  void setLabel(string l, int fontSize) { label=l;ftSize=fontSize; }
  void setFontSize(int fontSize) { ftSize = fontSize; }
};



class overlayMenu : public overlayButton {

private:
  void drawOutline();
  void drawSelection();
  void updateMenuItems();

  // this defines what gets drawn
  float shownZ; // the base z value at which the menu will be shown

  // menu items
  vector<sageDrawObject*> menuItems;

  sageRect shownBounds;
  int menuState;
  int currSelection;  // itemId

public:

  // METHODS
  overlayMenu();
  virtual ~overlayMenu();

  void init();
  void update(double now);
  void draw();
  void drawMenu();
  int parseMessage(char *msg);
  void parseSpecificInfo(TiXmlElement *parent);
};

#endif
