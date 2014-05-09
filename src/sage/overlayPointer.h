/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: overlayPointer.h
 * Author : Ratko Jagodic, Byungil Jeong
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

#ifndef SAGE_POINTER_H
#define SAGE_POINTER_H

#include "sageDrawObject.h"



class overlayPointer : public sageDrawObject {

private:
  //double lastUpdateTime; // when was the last time a message came in for touch
  GLuint tex;
  void drawCursor(double w, double h);
  void drawRotateArrows(float radius);
  void drawTexture(GLuint tex);

protected:

public:
  // this defines what gets drawn
  int ptrShape;   // influences the shape of the pointer
  int orientation;  //0=none, 1=bottomleft, 2=topleft, 3=topright, 4=bottomright
  int angle;   // where the down side of the pointer is
  bool inApp;  // whether the pointer is in the app mode or not

  // color of pointer for up/down states
  float up_color[3], down_color[3], in_app_color[3], *color;

  // for drawing the selection and capture rectangle
  int selL, selR, selT, selB;
  bool selection;
  bool capture;

  // draw temporary splitter
  bool doDrawSplitter;
  int splX1, splY1, splX2, splY2, splW;


  // METHODS
  overlayPointer();
  void update(double now);
  void draw();
  int destroy();
  int parseMessage(char *msg);
  void parseGraphicsInfo(TiXmlElement *parent);
  void drawSelection();
  void drawCapture();
  void drawSplitter();
};

#endif
