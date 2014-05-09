/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageRect.cpp - a few geometrical operations for rectangles used in SAGE
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

#include "sageRect.h"

sageRotation operator+(sageRotation r1, sageRotation r2)
{
  int sum = ((int)r1 + (int)r2)%4;
  return (sageRotation)sum;
}

sageRotation operator-(sageRotation r1, sageRotation r2)
{
  int diff = ((int)r1 - (int)r2 + 4)%4;
  return (sageRotation)diff;
}

/*
  void sageRect::operator= (sageRect &rect)
  {
  x = rect.x;
  y = rect.y;
  width = rect.width;
  height = rect.height;
  left = rect.left;
  right = rect.right;
  bottom = rect.bottom;
  top = rect.top;
  }
*/

sageRect sageRect::operator+ (sageRect &rect)
{
  sageRect res = *this;
  res += rect;

  return res;
}

void sageRect::operator+= (sageRect &rect)
{
  x += rect.x;
  y += rect.y;
  width += rect.width;
  height += rect.height;
}


sageRect sageRect::operator/ (float val)
{
  sageRect res;

  res.x = (int)floor(x/val+0.5);
  res.y = (int)floor(y/val+0.5);
  res.width = (int)floor(width/val+0.5);
  res.height = (int)floor(height/val+0.5);

  return res;
}

sageRect sageRect::operator/ (sageRect &rect)
{
  sageRect res;

  res.left = (x - rect.x)/(float)rect.width;
  res.right = (x + width - rect.x)/(float)rect.width;
  res.bottom = (y - rect.y)/(float)rect.height;
  res.top = (y + height - rect.y)/(float)rect.height;

  return res;
}

void sageRect::operator/= (sageRect &rect)
{
  left = (x - rect.x)/(float)rect.width;
  right = (x + width - rect.x)/(float)rect.width;
  bottom = (y - rect.y)/(float)rect.height;
  top = (y + height - rect.y)/(float)rect.height;
}

sageRect sageRect::operator* (sageRect &rect)
{
  sageRect res;

  res.x = (int)floor(width*rect.left+0.5);
  res.y = (int)floor(height*rect.bottom+0.5);
  res.width = (int)floor(width*rect.right+0.5) - res.x;
  res.height = (int)floor(height*rect.top+0.5) - res.y;
  res.translate(x, y);

  return res;
}

void sageRect::operator*= (sageRect &rect)
{
  sageRect res;

  res.x = (int)floor(width*rect.left+0.5);
  res.y = (int)floor(height*rect.bottom+0.5);
  res.width = (int)floor(width*rect.right+0.5) - res.x;
  res.height = (int)floor(height*rect.top+0.5) - res.y;
  res.translate(x, y);

  *this = res;
}

void sageRect::updateBoundary()
{
  left = (float)x;
  right = (float)(x + width);
  bottom = (float)y;
  top = (float)(y + height);
}

void sageRect::updateBoundary(sageRect &rect)
{
  left = (float)rect.x;
  right = (float)(rect.x + rect.width);
  bottom = (float)rect.y;
  top = (float)(rect.y + rect.height);
}

// move the origin to the lowerleft corner of rect
void sageRect::moveOrigin(sageRect &rect)
{
  translate(-rect.x, -rect.y);
}

void sageRect::scale(float r)
{
  int oldHalfWidth = halfWidth();
  int oldHalfHeight = halfHeight();

  width = (int)floor(width*r + 0.5);
  height = (int)floor(height*r + 0.5);

  x = x + oldHalfWidth - halfWidth();
  y = y + oldHalfHeight - halfHeight();
}

void sageRect::scale(float sx, float sy)
{
  left *= sx;
  right *= sx;
  bottom *= sy;
  top *= sy;
}

void sageRect::translate(int dx, int dy)
{
  x += dx;
  y += dy;
}

void sageRect::translate(sageRect &rect)
{
  translate(rect.x, rect.y);
}

void sageRect::shrink(float dx, float dy)
{
  left += dx;
  right -= dx;
  bottom += dy;
  top -= dy;
}

void sageRect::locate()
{
  x = (int)floor(left*width/(right-left)+0.5);
  y = (int)floor(bottom*height/(top-bottom)+0.5);
}

/*
  int sageRect::mask(sageRect &rect)
  {
  float width = right - left;
  float height = top - bottom;

  sageRect tmpRect = rect;
  tmpRect.scale(width, height);
  tmpRect.translate(left, bottom);
  *this = tmpRect;

  return 0;
  }

  int sageRect::normalize(sageRect &rect)
  {
  if (rect.left == rect.right || rect.bottom == rect.top) {
  std::cout << "Can't divide by 0 : fail to normalize rect" << std::endl;
  return -1;
  }

  float width = rect.right - rect.left;
  float height = rect.top - rect.bottom;

  moveOrigin(rect);

  left /= width;
  right /= width;
  bottom /= height;
  top /= height;

  return 0;
  }
*/

void sageRect::normalize(sageRect &rect)
{
  switch(rect.orientation) {
  case CCW_ZERO: {
    left = (x - rect.x)/(float)rect.width;
    right = (x + width - rect.x)/(float)rect.width;
    bottom = (y - rect.y)/(float)rect.height;
    top = (y + height - rect.y)/(float)rect.height;
    break;
  }
  case CCW_90: {
    left = (y - rect.y)/(float)rect.height;
    right = (y + height - rect.y)/(float)rect.height;
    bottom = (rect.x + rect.width - x - width)/(float)rect.width;
    top = (rect.x + rect.width - x)/(float)rect.width;
    break;
  }
  case CCW_180: {
    left = (rect.x + rect.width - x - width)/(float)rect.width;
    right = (rect.x + rect.width - x)/(float)rect.width;
    bottom = (rect.y + rect.height - y - height)/(float)rect.height;
    top = (rect.y + rect.height - y)/(float)rect.height;
    break;
  }
  case CCW_270: {
    left = (rect.y + rect.height - y - height)/(float)rect.height;
    right = (rect.y + rect.height - y)/(float)rect.height;
    bottom = (x - rect.x)/(float)rect.width;
    top = (x + width - rect.x)/(float)rect.width;
    break;
  }
  }
}

int sageRect::normalize(float x, float y)
{
  if (x == 0 || y == 0) {
    std::cout << "Can't divide by 0 : fail to normalize rect" << std::endl;
    return -1;
  }

  left /= x;
  right /= x;
  bottom /= y;
  top /= y;

  return 0;
}

void sageRect::rotate(sageRotation rot)
{
  int halfW = halfWidth();
  int halfH = halfHeight();
  int centerX = x + halfW;
  int centerY = y + halfH;
  orientation = orientation + rot;

  if (rot == CCW_90 || rot == CCW_270) {
    x = centerX - halfH;
    y = centerY - halfW;
    int oldWidth = width;
    width = height;
    height = oldWidth;
  }
}

bool sageRect::isOverLapX(float l, float r, sageRect &common)
{
  if (l <= right && r >= left) {
    common.left = MAX(l, left);
    common.right = MIN(r, right);
    return true;
  }
  else
    return false;
}

bool sageRect::isOverLapY(float b, float t, sageRect &common)
{
  if (b <= top && t >= bottom) {
    common.bottom = MAX(b, bottom);
    common.top = MIN(t, top);
    return true;
  }
  else
    return false;
}

bool sageRect::isOverLap(float l, float r, float b, float t, sageRect &common)
{
  return (isOverLapX(l, r, common) && isOverLapY(b, t, common));
}

bool sageRect::isOverLapX(int x2, int w2, sageRect &common)
{
  int r1 = x+width-1;
  int r2 = x2+w2-1;

  if (x2 <= r1 && r2 >= x) {
    common.x = MAX(x2, x);
    int rCommon = MIN(r1, r2);
    common.width = rCommon-common.x+1;
    return true;
  }
  else
    return false;
}

bool sageRect::isOverLapY(int y2, int h2, sageRect &common)
{
  int t1 = y+height-1;
  int t2 = y2+h2-1;

  if (y2 <= t1 && t2 >= y) {
    common.y = MAX(y2, y);
    int tCommon = MIN(t1, t2);
    common.height = tCommon-common.y+1;
    return true;
  }
  else
    return false;
}

bool sageRect::isOverLap(sageRect &rect, sageRect &com)
{
  return (isOverLapX(rect.x, rect.width, com) && isOverLapY(rect.y, rect.height, com));
}

bool sageRect::isOverLap(sageRect &rect)
{
  sageRect dummy;
  return isOverLap(rect, dummy);
}

bool sageRect::crop(sageRect &rect)
{
  int r1 = x+width-1;
  int r2 = rect.x+rect.width-1;
  int t1 = y+height-1;
  int t2 = rect.y+rect.height-1;

  if (rect.y <= t1 && y <= t2 && rect.x <= r1 && x <= r2) {
    y = MAX(rect.y, y);
    int tCommon = MIN(t1, t2);
    height = tCommon-y+1;

    x = MAX(rect.x, x);
    int rCommon = MIN(r1, r2);
    width = rCommon-x+1;
    return true;
  }
  else
    return false;
}

int sageRect::isOverLap(sageRect &rect, float threshold)
{
  float outX = 0.0, outY = 0.0;
  bool leftOut = false, bottomOut = false;

  if (rect.x < x) {
    outX = (float)(x - rect.x)/rect.width;
    leftOut = true;
  }

  if (rect.x + rect.width > x + width) {
    float tmpOutX = (float)(rect.x + rect.width - x - width)/rect.width;
    if (tmpOutX > outX) {
      leftOut = false;
      outX = tmpOutX;
    }
  }

  if (rect.y < y) {
    outY = (float)(y - rect.y)/rect.height;
    bottomOut = true;
  }

  if (rect.y + rect.height > y + height) {
    float tmpOutY = (float)(rect.y + rect.height - y - height)/rect.height;
    if (tmpOutY > outY) {
      bottomOut = false;
      outY = tmpOutY;
    }
  }

  int result = ON_RECT;

  if (outX > threshold) {
    if (leftOut)
      result = LEFT_EDGE;
    else
      result = RIGHT_EDGE;
  }

  if (outY > threshold && (outX <= threshold || outY > outX)) {
    if (bottomOut)
      result = BOTTOM_EDGE;
    else
      result = TOP_EDGE;
  }

  return result;
}

bool sageRect::isInRect(int px, int py)
{
  if (x <= px && x+width > px && y <= py && y+height > py)
    return true;
  else
    return false;
}

bool sageRect::isInRect(float x, float y)
{
  updateBoundary();

  if (left <= x && right >= x && bottom <= y && top >= y)
    return true;
  else
    return false;
}

bool sageRect::isInRect(sageRect &rect)
{
  updateBoundary();
  rect.updateBoundary();

  if (left <= rect.left && right >= rect.right && bottom <= rect.bottom && top >= rect.top)
    return true;
  else
    return false;
}

void sageRect::sprintRect(char *str)
{
  sprintf(str, "%d %d %d %d %f %f %f %f", x, y, width, height, left, right, bottom, top);
}

void sageRect::sscanRect(char *str)
{
  sscanf(str, "%d %d %d %d %f %f %f %f", &x, &y, &width, &height, &left, &right, &bottom, &top);
}

void sageRect::genTexCoord(int corner, float &x, float &y)
{
  bool lowerCorner, leftCorner;

  switch(corner) {
  case LOWER_LEFT: {
    lowerCorner = true;
    leftCorner = true;
    break;
  }
  case LOWER_RIGHT: {
    lowerCorner = true;
    leftCorner = false;
    break;
  }
  case UPPER_RIGHT: {
    lowerCorner = false;
    leftCorner = false;
    break;
  }
  case UPPER_LEFT: {
    lowerCorner = false;
    leftCorner = true;
    break;
  }
  }

  switch(orientation) {
  case CCW_ZERO: {
    if (leftCorner)
      x = left;
    else
      x = right;

    if (lowerCorner)
      y = bottom;
    else
      y = top;

    break;
  }

  case CCW_90: {
    if (leftCorner)
      y = top;
    else
      y = bottom;

    if (lowerCorner)
      x = left;
    else
      x = right;

    break;
  }

  case CCW_180: {
    if (leftCorner)
      x = right;
    else
      x = left;

    if (lowerCorner)
      y = top;
    else
      y = bottom;

    break;
  }

  case CCW_270: {
    if (leftCorner)
      y = bottom;
    else
      y = top;

    if (lowerCorner)
      x = right;
    else
      x = left;

    break;
  }
  }
}
