/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageRect.h
 * Author : Byungil Jeong
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

#ifndef _SAGE_RECT_H
#define _SAGE_RECT_H

#include "sageBase.h"

#define LOWER_LEFT  0
#define LOWER_RIGHT 1
#define UPPER_RIGHT 2
#define UPPER_LEFT  3

#define LEFT_EDGE   0
#define RIGHT_EDGE  1
#define BOTTOM_EDGE 2
#define TOP_EDGE    3
#define ON_RECT     4

enum sageRotation {CCW_ZERO, CCW_90, CCW_180, CCW_270};

sageRotation operator+(sageRotation r1, sageRotation r2);
sageRotation operator-(sageRotation r1, sageRotation r2);

class sageRect {
public:
  int x, y, width, height;
  float left, right, bottom, top;
protected:
  sageRotation orientation;

public:
  sageRect() : x(0), y(0), width(0), height(0),
               left(0.0), right(1.0), bottom(0.0), top(1.0), orientation(CCW_ZERO) {}

  sageRect(float l, float r, float b, float t)
    : x(0), y(0), width(0), height(0),
      left(l), right(r), bottom(b), top(t), orientation(CCW_ZERO) {}

  sageRect(int rx, int ry, int rw, int rh)
    : x(rx), y(ry), width(rw), height(rh),
      left(0.0), right(1.0), bottom(0.0), top(1.0), orientation(CCW_ZERO) {}

  ~sageRect() {}

  //void operator= (sageRect &rect);
  void operator+= (sageRect &rect);
  sageRect operator+ (sageRect &rect);
  sageRect operator/ (float div);
  sageRect operator/ (sageRect &rect);
  void operator/= (sageRect &rect);
  sageRect operator* (sageRect &rect);
  void operator*= (sageRect &rect);

  void updateBoundary();
  void updateBoundary(sageRect &rect);
  void moveOrigin(sageRect &rect);
  void scale(float r);
  void scale(float sx, float sy);
  void translate(int x, int y);
  void translate(sageRect &rect);
  void rotate(sageRotation rot);
  void setOrientation(sageRotation ori) { orientation = ori; }
  inline sageRotation getOrientation() { return orientation; }
  void resetOrientation()  { rotate(CCW_ZERO - orientation); }
  void shrink(float x, float y);
  void locate();
  void normalize(sageRect &rect);
  int normalize(float x, float y);

  bool isOverLap(float l, float r, float b, float t, sageRect &com);
  bool isOverLapX(float l, float r, sageRect &com);
  bool isOverLapY(float b, float t, sageRect &com);

  bool isOverLap(sageRect &rect);
  int  isOverLap(sageRect &rect, float threshold);
  bool isOverLap(sageRect &rect, sageRect &com);
  bool isOverLapX(int x2, int w2, sageRect &com);
  bool isOverLapY(int y2, int h2, sageRect &com);

  bool isInRect(float x, float y);
  bool isInRect(int x, int y);
  bool isInRect(sageRect &rect);
  void genTexCoord(int corner, float &x, float &y);
  void sprintRect(char *str);
  void sscanRect(char *str);

  inline int halfWidth() { return width/2; }
  inline int halfHeight() { return height/2; }
  inline int centerX() { return x + halfWidth(); }
  inline int centerY() { return y + halfHeight(); }
  //int mask(sageRect &rect);  // crop by the rect which has normalized coordinates
  bool crop(sageRect &rect);
};

#endif
