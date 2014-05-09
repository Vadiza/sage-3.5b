/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayApp.h
 * Author : Ratko Jagodic
 *
 *   Description: the super class of simple drawing objects
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

#ifndef SAGE_APP_OVERLAY_H
#define SAGE_APP_OVERLAY_H

#include "sageDrawObject.h"


class overlayApp : public sageDrawObject {

private:
  void drawOutline();
  void drawSelection();
  void drawCorners();
  void drawOneCorner();
  void drawXCorner();
  void drawCurtain();
  void drawFontBackground(float x, float y, float w, float h);
  //void update(double now);  // do any updates here for animation

  // this defines what gets drawn
  int state;     //resize or move
  int activeCorners[7];  //0=bottom, 1=top, 3=bottomleft, 4=topleft, 5=topright, 6=bottomright
  float aspectRatio;  //to preserve it during resizing
  float outline[4];  //left, right, top, bottom
  int cornerSize;
  bool dim;   // should the app be dimmed?
  bool mouseOver;  // for drawing the tooltip

  // color of pointer for up/down states
  float border_color[3], corner_color[4], *color;


public:
  bool closing;
  float curtainAlpha;

  // METHODS
  overlayApp();
  void draw();
  int parseMessage(char *msg);
  void onNewParentBounds();
};

#endif
