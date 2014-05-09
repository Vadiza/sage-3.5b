/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDrawObject.cpp
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

//#include <GL/glew.h>

#include "sageDrawObject.h"
namespace Magick {
#include <wand/magick-wand.h>
}

using namespace Magick;

// global font object so that we dont load them multiple times
std::map<const int, Font*> fonts;

// static method
void sageDrawObject::initFonts()
{
  // load the fonts for all the sizes...
  if (fonts.empty()) {
    for(int i=MIN_FONT_SIZE; i<=200; i++)
      fonts[i] = new Font(i);
  }
}

// static stuff
int sageDrawObject::selLineWidth = 120;
float sageDrawObject::selPulseDir = 7;
bool sageDrawObject::selUpdated = false;


sageDrawObject::sageDrawObject()
{
  objectID = 0;
  displayID = 0;
  winID = -1;
  parentID = -1;

  parentOrder = -1;
  drawOrder = SAGE_INTER_DRAW;
  global = true;
  visible = true;
  slideHidden = false;
  canSlide = false;
  pos.xMargin = pos.yMargin = 0;
  zOffset = 0.0;
  scale = 1.0;
  delayDraw = false;
  selected = false;

  fontColor = DEFAULT_FONT_COLOR;
  backgroundColor = DEFAULT_BACKGROUND_COLOR;
  label = "";

  doFade = true;
  initAlpha = DEFAULT_ALPHA;
  fadeAlpha = 0;
  alpha = 0;
  lastTime = sage::getTime();  // in microsecs
  fadeTime = 1.0;  // in seconds

  drawParent = NULL;
  drawDimBounds = false;
  dimBounds.x = dimBounds.y = 0;
  dimBounds.width = dimBounds.height = 100;
  dimBounds.updateBoundary();
  parentBoundsSet = false;
}

sageDrawObject::~sageDrawObject()
{
  // get rid of all children
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++) {
    // a little hack so that sizer.removeWidget doesn't get called
    // by sageDraw.removeObject
    (*it).second->parentID = -1;
    drawParent->removeObject((*it).second->getID());
  }
  children.clear();
}

void sageDrawObject::init(int id, sageDraw *dp, char *objType)
{
  // this is called AFTER parsing the XML data so be careful not to
  // overwrite the values that have been set during the XML data parsing
  objectID = id;
  drawParent = dp;
  lastTime = sage::getTime();  // in microsecs
  if (objType)
    strcpy(objectType, objType);
  init();   // initialize the subclass
  //fade(FADE_IN);
  //alpha = DEFAULT_ALPHA;
}

bool sageDrawObject::operator==(const char *wType)
{
  if (strcmp(wType, objectType) == 0)
    return true;
  else
    return false;
}

bool sageDrawObject::operator!=(const char *wType)
{
  if (strcmp(wType, objectType) != 0)
    return true;
  else
    return false;
}


// in case subclass doesn't supply its own method
void sageDrawObject::init()
{
}

/*
  int sageDrawObject::setGlobalView()
  {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(tileRect.left, tileRect.right, tileRect.bottom, tileRect.top, 0, 100000);
  glMatrixMode(GL_MODELVIEW);

  return 0;
  }

  int sageDrawObject::setLocalView()
  {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(tileRect.x, tileRect.x+tileRect.width, tileRect.y, tileRect.y+tileRect.height, 0, 100000);
  glMatrixMode(GL_MODELVIEW);

  return 0;
  }
*/

void sageDrawObject::showObject(bool doShow, bool directCall)
{
  if (directCall)
    visible = doShow;

  if (doShow) {
    //fade(FADE_IN); //, 0, 255);
    alpha = initAlpha;

    map<int, sageDrawObject*>::iterator it;
    for (it=children.begin(); it!=children.end(); it++) {
      (*it).second->showObject(doShow, false);
    }
  }
  else
    //fade(FADE_OUT, 255, 0);
    alpha = 0;

  refresh();
}

/*
  int sageDrawObject::setViewport(sageRect &rect)
  {
  tileRect = rect;

  if (global)
  setGlobalView();
  else
  setLocalView();

  return 0;
  }
*/
void sageDrawObject::update(double now)
{
  float rate = 20.0;
  if (now - lastTime > 1000000.0/rate) {
    lastTime = now;
    doFade = false;
    if (doFade) {

      // change the alpha by a little bit each time
      if (fadeDirection == FADE_IN) {
        fadeAlpha += int(255.0 * (1.0/(rate*fadeTime)));  // make it run for 2 secs
        if (fadeAlpha > endAlpha) {
          fadeAlpha = endAlpha;
          doFade = false;
        }
      }
      else {
        fadeAlpha -= int(255.0 * (1.0/(rate*fadeTime)));  // make it run for 2 secs
        if(fadeAlpha < endAlpha) {
          fadeAlpha = endAlpha;
          doFade = false;
        }
      }

      alpha = fadeAlpha;

      // redraw
      setDirty();
    }

    if (selected && (*(this)) == APP && !selUpdated)
    {
      selUpdated = true;
      selLineWidth += selPulseDir;
      if (selLineWidth > 255) {
        selLineWidth = 255;
        selPulseDir *= -1;
      }
      else if(selLineWidth < 120) {
        selLineWidth = 120;
        selPulseDir *= -1;
      }

      // redraw
      setDirty();
    }


    /*if (canSlide && slideHidden && y > (2-height)) {
      y -= 100;
      setDirty();
      refresh();
      }
      else if(canSlide && !slideHidden && y < 0) {
      y += 100;
      setDirty();
      refresh();
      }*/
  }
}

void sageDrawObject::fade(fadeDir dir, int startAlpha, int endAlpha)
{
  alpha = endAlpha;
  setDirty();
  return;


  // different starting point depending on whether we are fading in or out
  if (startAlpha != -1)
    fadeAlpha = startAlpha;
  else if (dir == FADE_IN) {
    fadeAlpha = 0;
    alpha = fadeAlpha;
  }
  else
    fadeAlpha = alpha;

  if (endAlpha != -1)
    this->endAlpha = endAlpha;
  else if (dir == FADE_OUT)
    this->endAlpha = 0;
  else
    this->endAlpha = initAlpha;

  fadeDirection = dir;
  lastTime = sage::getTime();  // in microsecs
  doFade = true;
  setDirty();
}

void sageDrawObject::setDirty()
{
  drawParent->setDirty();
}


//----------------------------------------------------------------------//
/*                              XML STUFF                               */
//----------------------------------------------------------------------//

void sageDrawObject::parseXml(TiXmlElement *root)
{
  // initialize some members
  int isGlobal=1;
  int dDraw=1;  //delay draw
  int isVisible=1;
  int isSlideHidden=0;
  pos.xMargin = pos.yMargin = 0;
  displayID = 0;
  //winID = -1;
  parentID = -1;
  parentOrder = -1;
  drawOrder = SAGE_INTER_DRAW;
  visible = true;
  slideHidden = false;
  zOffset = 0.0;
  scale = 1.0;
  displayScale = 1.0;
  fontColor = DEFAULT_FONT_COLOR;
  backgroundColor = DEFAULT_BACKGROUND_COLOR;
  initAlpha = DEFAULT_ALPHA;
  labelAlignment = -1;

  // read all the <widget> attributes that we care about
  //root->QueryIntAttribute("winID", &winID);   // XML FIELD NOT USED ANYMORE
  if (root->QueryIntAttribute("isGlobal", &isGlobal) == TIXML_SUCCESS)
    global = bool(isGlobal);
  if (root->QueryIntAttribute("delayDraw", &dDraw) == TIXML_SUCCESS)
    delayDraw = bool(dDraw);
  if (root->QueryIntAttribute("isVisible", &isVisible) == TIXML_SUCCESS)
    visible = bool(isVisible);
  if (root->QueryIntAttribute("isSlideHidden", &isSlideHidden) == TIXML_SUCCESS)
    slideHidden = bool(isSlideHidden);
  root->QueryIntAttribute("displayID", &displayID);
  root->QueryIntAttribute("parentID", &parentID);
  root->QueryIntAttribute("parentOrder", &parentOrder);
  root->QueryIntAttribute("drawOrder", &drawOrder);
  root->QueryFloatAttribute("displayScale", &displayScale);

  TiXmlElement *common = root->FirstChildElement("common");

  // read width
  TiXmlElement *w = common->FirstChildElement("w");
  w->QueryIntAttribute("type", &size.wType);
  if (size.wType == WIDGET_ABSOLUTE)  {
    size.w = atoi(w->GetText());
    width = size.w;
  }
  else if(size.wType == WIDGET_NORMALIZED)
    size.normW = atof(w->GetText());

  // read height
  TiXmlElement *h = common->FirstChildElement("h");
  h->QueryIntAttribute("type", &size.hType);
  if (size.hType == WIDGET_ABSOLUTE)  {
    size.h = atoi(h->GetText());
    height = size.h;
  }
  else if(size.hType == WIDGET_NORMALIZED)
    size.normH = atof(h->GetText());

  // read x
  TiXmlElement *x = common->FirstChildElement("x");
  x->QueryIntAttribute("type", &pos.xType);
  if (pos.xType == WIDGET_ABSOLUTE)  {
    pos.x = atoi(x->GetText());
    this->x = pos.x;
  }
  else if(pos.xType == WIDGET_NORMALIZED)
    pos.normX = atof(x->GetText());
  else if(pos.xType == WIDGET_ALIGNED)
    pos.xAlignment = (objAlignment)atoi(x->GetText());
  x->QueryIntAttribute("margin", &pos.xMargin);

  // read y
  TiXmlElement *y = common->FirstChildElement("y");
  y->QueryIntAttribute("type", &pos.yType);
  if (pos.yType == WIDGET_ABSOLUTE)  {
    pos.y = atoi(y->GetText());
    this->y = pos.y;
  }
  else if(pos.yType == WIDGET_NORMALIZED)
    pos.normY = atof(y->GetText());
  else if(pos.yType == WIDGET_ALIGNED)
    pos.yAlignment = (objAlignment)atoi(y->GetText());
  y->QueryIntAttribute("margin", &pos.yMargin);

  // read z
  TiXmlElement *z = common->FirstChildElement("z");
  z->QueryFloatAttribute("offset", &zOffset);
  int zType = atoi(z->GetText());
  if (zType == TOP_Z_TYPE)
    this->z = TOP_Z + zOffset;
  else if(zType == BOTTOM_Z_TYPE)
    this->z = BOTTOM_Z + zOffset;
  // else the z will be set in updateParentDepth

  // read scale (if it exists)
  TiXmlElement *s = common->FirstChildElement("scale");
  if (s)
    scale = atof(s->GetText());

  // read the parentBounds if they exist
  TiXmlElement *b = common->FirstChildElement("parentBounds");
  if (b) {
    b->QueryIntAttribute("x", &parentBounds.x);
    b->QueryIntAttribute("y", &parentBounds.y);
    b->QueryIntAttribute("w", &parentBounds.width);
    b->QueryIntAttribute("h", &parentBounds.height);
    parentBounds.updateBoundary();
  }

  // parse the graphics stuff
  TiXmlElement *graphics = root->FirstChildElement("graphics");
  if (graphics) {

    // font color if present
    TiXmlElement *fc = graphics->FirstChildElement("fontColor");
    if (fc)
      fontColor = intToColor(atoi(fc->GetText()));

    // background color if present
    TiXmlElement *bc = graphics->FirstChildElement("backgroundColor");
    if (bc)
      backgroundColor = intToColor(atoi(bc->GetText()));

    // transparency if present
    TiXmlElement *al = graphics->FirstChildElement("alpha");
    if (al)
      initAlpha = atoi(al->GetText());

    //alpha = 0;
    alpha = initAlpha;

    // the rest of the graphics info
    parseGraphicsInfo(graphics);
  }


  // parse the specific stuff
  TiXmlElement *specific = root->FirstChildElement("specific");
  if (specific)  {

    // parse the label first (if there)
    TiXmlElement *l = specific->FirstChildElement("label");
    if (l) {
      label = string(l->GetText());
      l->QueryIntAttribute("fontSize", &ftSize);
      l->QueryFloatAttribute("relativeFontSize", &relativeFontSize);
      l->QueryIntAttribute("labelAlignment", &labelAlignment);
    }

    TiXmlElement *tt = specific->FirstChildElement("tooltip");
    if (tt) {
      tooltip = string(tt->GetText());
      tt->QueryIntAttribute("fontSize", &ttSize);
      tt->QueryFloatAttribute("relativeTooltipSize", &relativeTooltipSize);
      ttSize = int(ttSize*scale);

      // store the size of the tooltip for later use
      ft = getFont(ttSize);
      SDL_Rect r;
      ft->textSize(tooltip.c_str(), &r);
      ttWidth = r.w;
    }

    // now call the subclass' method to parse widget specific info
    parseSpecificInfo(specific);
  }
}


// override this if you need to parse some graphics or widget-specific info
void sageDrawObject::parseGraphicsInfo(TiXmlElement *parent)
{
}

// override this if you need to parse some graphics or widget-specific info
void sageDrawObject::parseSpecificInfo(TiXmlElement *parent)
{
}


//----------------------------------------------------------------------//
/*                        APPLICATION UPDATES                           */
//----------------------------------------------------------------------//

void sageDrawObject::updateParentBounds(int newX, int newY, int newW, int newH, sageRotation o)
{
  // change the parentBounds
  parentBounds.x = newX;
  parentBounds.y = newY;
  parentBounds.width = newW;
  parentBounds.height = newH;
  parentBounds.setOrientation(o);
  parentBounds.updateBoundary();

  parentBoundsSet = true;

  // only update the parents and they will update their children
  if (parentID == -1)
    updateBounds();
}


// update the bounds based on the parent bounds
void sageDrawObject::updateBounds()
{
  // children size
  updateChildrenBounds();

  // update font size first
  if (!label.empty() && isAppWidget())  // only change the font size if we are tied to app
    ftSize = int(floor(parentBounds.height * relativeFontSize));

  // update font size first
  if (!tooltip.empty() && isAppWidget())  // only change the font size if we are tied to app
    ttSize = int(floor(parentBounds.height * relativeTooltipSize));

  // our size
  if (children.size() > 0)
    fitAroundChildren();

  else if (!label.empty() && labelAlignment == -1)
    fitAroundLabel();

  else {
    if (size.wType == WIDGET_NORMALIZED)
      width = int(floor(parentBounds.width * size.normW)); // * scale
    else
      width = int(size.w * scale);
    if (size.hType == WIDGET_NORMALIZED)
      height = int(floor(parentBounds.height * size.normH)); // * scale
    else
      height = int(size.h * scale);
  }

  // pos
  sageRect pb;
  if (parentID != -1) {
    sageDrawObject *parent = drawParent->getDrawObject(parentID);
    if (parent)
      pb = *parent;
  }
  else
    pb = parentBounds;

  updateX(pb);
  updateY(pb);
  updateBoundary();

  // so that subclasses can do something on update if needed
  onNewParentBounds();

  // position children if wanted (if subclass overrides this method)
  positionChildren();

  // if a label is used, update the position of the text inside the widget
  if (!label.empty())
    updateFontPosition();
}


// calculates x relative to pb
void sageDrawObject::updateX(sageRect pb)
{
  int slideGap = -1;
  // x
  if (pos.xType == WIDGET_ALIGNED) {
    switch(pos.xAlignment) {
    case CENTER:
      x = pb.x + int(floor(pb.width/2.0)) - int(floor(width/2.0));
      break;
    case LEFT_OUTSIDE:
      x = pb.x - width - pos.xMargin;
      break;
    case LEFT:
      if (slideHidden)
        x = pb.x - width + slideGap;
      else
        x = pb.x + pos.xMargin;
      break;
    case RIGHT:
      if (slideHidden)
        x = int(pb.right) - slideGap;
      else
        x = int(pb.right) - width - pos.xMargin;
      break;
    case RIGHT_OUTSIDE:
      x = int(pb.right) + pos.xMargin;
      break;
    default:
      x = pb.x;
      break;
    }
  }
  else if (pos.xType == WIDGET_NORMALIZED)
    x = int(floor(pb.width * pos.normX)) + pb.x;
  else
    x = pos.x;
}

// calculates y relative to pb
void sageDrawObject::updateY(sageRect pb)
{
  int slideGap = -1;

  if (pos.yType == WIDGET_ALIGNED) {
    switch(pos.yAlignment) {
    case CENTER:
      y = pb.y + int(floor(pb.height/2.0)) - int(floor(height/2.0));
      break;
    case BOTTOM_OUTSIDE:
      y = pb.y - height - pos.yMargin;
      break;
    case BOTTOM:
      if (slideHidden) /*{

if (canSlide && slideHidden)
slideY = int(pb.top) - 2;
else if(canSlide && !slideHidden)
slideY = int(pb.top) - height - pos.yMargin;*/
        y = pb.y - height + slideGap;
      else
        y = pb.y + pos.yMargin;

      break;
    case TOP:
      if(slideHidden)
        y = int(pb.top) - slideGap;
      else
        y = int(pb.top) - height - pos.yMargin;
      break;
    case TOP_OUTSIDE:
      y = int(pb.top) + pos.yMargin;
      break;
    default:
      y = pb.y;
      break;
    }
  }
  else if (pos.yType == WIDGET_NORMALIZED)
    y = int(floor(pb.height * pos.normY)) + pb.y;
  else
    y = pos.y;
}

void sageDrawObject::updateParentDepth(float depth)
{
  appZ = depth;
  z = -(appZ + zOffset);

  // so that subclasses can do something on update if needed
  onNewParentDepth();

  //updateChildrenZ();
}


// you can override these 2 if your subclass needs to do something
// special when the app bounds change
//
// (size, position and depth of your draw object is automatically
// updated by this base class based on then restrictions specified
// in the original xml description (commonInfo))
void sageDrawObject::onNewParentBounds()
{
}

void sageDrawObject::onNewParentDepth()
{
}

void sageDrawObject::updateFontPosition() {
  // this resizes the font based on the new widget size
  // and figures out the position so that it's centered
  if (!label.empty()) {
    int fs = int(ftSize*scale);
    if (fs < MIN_FONT_SIZE)
      fs = MIN_FONT_SIZE;
    ft = getFont(fs);
    SDL_Rect r;
    ft->textSize(label.c_str(), &r);

    int padding = int(fs*0.4);

    if (labelAlignment == CENTER)
      fx = int(x + (width/2.0)-(r.w/2.0));
    else if(labelAlignment == RIGHT) // 5% from the right
      fx = int(x + width - r.w - padding);
    else                            // left by default
      fx = int(x + padding);
    fy = int(y + (height/2.0)-(r.h/2.0));
  }
}


void sageDrawObject::fitAroundLabel()
{
  if (!label.empty() && labelAlignment == -1) {
    int fs = int(ftSize*scale);
    if (fs < MIN_FONT_SIZE)
      fs = MIN_FONT_SIZE;
    ft = getFont(fs);
    SDL_Rect r;
    ft->textSize(label.c_str(), &r);

    // fit the object around the label
    int wPadding = int(fs*0.8);
    int hPadding = int(fs*0.4);
    width = r.w + wPadding;
    height = r.h + hPadding;
  }
}


// recalculates objects position and size but only if there is no parent
// otherwise propagate the request to the parent which will recalculate
void sageDrawObject::refresh()
{
  if (parentID != -1) {
    sageDrawObject *parentObj = drawParent->getDrawObject(parentID);
    if (parentObj)
      parentObj->refresh();
  }
  else {
    updateBounds();
    //updateChildrenZ();
  }
}


void sageDrawObject::redraw(bool delayedCall)
{
  if (!visible && (*(this)) != POINTER )
    return;

  // don't do anything if the parent info hasn't come in for
  if (isAppWidget() && !parentBoundsSet)
    return;

  // draw the widget now or delay it
  if (delayDraw && !delayedCall)
    drawParent->delayDraw(this);
  else
    draw();

  if (drawDimBounds) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    glColor4f(1,1,1,1);

    glBegin(GL_LINE_LOOP);
    glVertex3f(dimBounds.left, dimBounds.bottom, z);
    glVertex3f(dimBounds.left, dimBounds.top, z);
    glVertex3f(dimBounds.right, dimBounds.top, z);
    glVertex3f(dimBounds.right, dimBounds.bottom, z);
    glEnd();

    glPopAttrib();
  }

  // then draw all my children... to draw by z order
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++)
    (*it).second->redraw();
}


// draws the tooltip at (0,0)... if any
void sageDrawObject::drawTooltip()
{

  if(!tooltip.empty()) {
    float degInRad = 0.0;
    float radius = ttSize/2.0+ttSize/8.0;

    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    glTranslatef(0.0, ttSize/2.0+ttSize/16.0, 0);
    glColor4f(0.0, 0.0, 0.0, 0.5);

    // left half circle
    glBegin(GL_POLYGON);
    for (int i=90; i <= 270; i+=10)
    {
      degInRad = i*DEG2RAD;
      glVertex3f(cos(degInRad)*radius,sin(degInRad)*radius, z);
    }

    // right half circle
    for (int i=270; i <= 450; i+=10)
    {
      degInRad = i*DEG2RAD;
      glVertex3f(cos(degInRad)*radius+ttWidth,sin(degInRad)*radius, z);
    }
    glEnd();

    glPopMatrix();

    // draw the label
    ft = getFont(ttSize);
    ft->drawText(tooltip.c_str(), 0, 0, z, fontColor, 255);
  }
}


// attach a child to this object
void sageDrawObject::addChild(sageDrawObject *widget)
{
  children[widget->parentOrder] = widget;
  refresh();
}

void sageDrawObject::removeChild(int id)
{
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++) {
    if ((*it).second->getID() == id) {
      children.erase(it);
      break;
    }
  }
  refresh();
}


void sageDrawObject::updateChildrenBounds()
{
  // update the sizes of all children based on their parentBounds
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++)  {
    sageRect pb = parentBounds;
    (*it).second->updateParentBounds(pb.x, pb.y, pb.width, pb.height, pb.getOrientation());
    (*it).second->updateBounds();
  }
}


/*void sageDrawObject::updateChildrenZ()
  {
  // update the sizes of all children based on their parentBounds
  map<int, sageDrawObject*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++) {
  (*it).second->z = z + Z_CHILD_DIFF;
  (*it).second->updateChildrenZ();
  }
  }*/


void sageDrawObject::positionChildren()
{
  // override this method if you want to position children yourself (e.g. sizers)
}


void sageDrawObject::fitAroundChildren()
{
  // override this method if you want to resize this parent object around it's
  // children (e.g. sizers do that)
}


//----------------------------------------------------------------------//
/*                             TEXTURE STUFF                            */
//----------------------------------------------------------------------//


// read image from imgData of size len and store it in **rgba
// MAKE SURE YOU FREE THE **rgba BUFFER!!
// return 1 on success, 0 otherwise
int getRGBA(const void *imgData, int len, GLubyte **rgba,
            unsigned int &width, unsigned int &height)
{
  // use ImageMagick to read images
  MagickBooleanType status;
  MagickWand *wand;

  // read file
  wand=NewMagickWand();
  if (MagickReadImageBlob(wand, imgData, len) == MagickFalse)
    return 0;

  // get the image size
  width = MagickGetImageWidth(wand);
  height = MagickGetImageHeight(wand);

  MagickFlipImage(wand);

  // get the pixels
  *rgba = (GLubyte*) malloc(width*height*4);
  memset(*rgba, 0, width*height*4);
  if (MagickGetImagePixels(wand, 0, 0, width, height, "RGBA", CharPixel, *rgba) == MagickFalse) {
    DestroyMagickWand(wand);
    return 0;
  }
  DestroyMagickWand(wand);
  return 1;
}

// read image from filename and store it in **rgba
// MAKE SURE YOU FREE THE **rgba BUFFER!!
// return 1 on success, 0 otherwise
int getRGBA(char *filename, GLubyte **rgba,
            unsigned int &width, unsigned int &height)
{
  // use ImageMagick to read images
  MagickBooleanType status;
  MagickWand *wand;

  // read file
  wand=NewMagickWand();
  if (MagickReadImage(wand, filename) == MagickFalse)
    return 0;

  // get the image size
  width = MagickGetImageWidth(wand);
  height = MagickGetImageHeight(wand);

  MagickFlipImage(wand);

  // get the pixels
  *rgba = (GLubyte *) malloc(width*height*4);
  memset(*rgba, 0, width*height*4);
  if (MagickGetImagePixels(wand, 0, 0, width, height, "RGBA", CharPixel, *rgba) == MagickFalse) {
    SAGE_PRINTLOG("\n\n======>> getRGBA ERROR in MagickGetImagePixels.... %s\n", filename);
    DestroyMagickWand(wand);
    return 0;
  }
  DestroyMagickWand(wand);
  return 1;
}


// returns new texture object on success, 0 otherwise
GLuint getTextureFromXml(TiXmlElement *parent, const char *elementName)
{
  GLuint t = 0;
  int format = 0;

  TiXmlElement *el = parent->FirstChildElement(elementName);
  if (el) {
    el->QueryIntAttribute("format", &format);
    if (format == FILE_FMT) {
      t = getTextureFromFile(el->GetText());
    }
    else if(format == IMAGE_FMT) {
      std::string b64 (el->GetText());
      std::string decoded (Base64::decode(b64));
      t = getTextureFromBuffer(decoded.data(), decoded.size());
    }
  }

  return t;
}

// returns new texture object on success, 0 otherwise
GLuint getTextureFromBuffer(const char *imgData, int len)
{
  unsigned int w_tex, h_tex;
  GLubyte *rgba = NULL;
  GLuint t = 0;

  if (getRGBA((void *)imgData, len, &rgba, w_tex, h_tex)) {
    glGenTextures (1, &t);
    glBindTexture (GL_TEXTURE_2D, t);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w_tex, h_tex, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte*) rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //glGenTextures(1, &t);
    //glBindTexture(GL_TEXTURE_2D, t);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w_tex, h_tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte*)rgba);
    //glGenerateMipmap(GL_TEXTURE_2D);  //Generate mipmaps now!!!


    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  }
  free(rgba);
  return t;
}

// returns new texture number on success, 0 otherwise
GLuint getTextureFromFile(const char *filename)
{
  unsigned int w_tex, h_tex;
  GLubyte *rgba = NULL;
  GLuint t = 0;

  // the filename parameter is relative to the "images"
  // directory in the current sage directory
  char *sageDir = getenv("SAGE_DIRECTORY");
  char filePath[256];
  sprintf(filePath, "%s/%s",sageDir, filename);

  if (getRGBA(filePath, &rgba, w_tex, h_tex)) {
    glGenTextures (1, &t);
    glBindTexture (GL_TEXTURE_2D, t);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w_tex, h_tex, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
    //glGenTextures(1, &t);
    //glBindTexture(GL_TEXTURE_2D, t);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w_tex, h_tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte*)rgba);
    //glGenerateMipmap(GL_TEXTURE_2D);  //Generate mipmaps now!!!

  }
  free(rgba);
  return t;
}



//----------------------------------------------------------------------//
/*                               MISC STUFF                             */
//----------------------------------------------------------------------//


// returns a pointer to the font object of the correct size
// so that it fits in height pixels (finds the closest match)
Font * sageDrawObject::getFont(int h)
{
  if (h < MIN_FONT_SIZE)
    return fonts[MIN_FONT_SIZE];
  else if (h > 200)
    return fonts[200];
  else
    return fonts[h];//[h - h%2];
}


void sageDrawObject::parseCommonMessage(char *msg)
{
  int objId, msgCode;
  sscanf(msg, "%d %d", &objId, &msgCode);

  switch(msgCode)
  {
  case SET_LABEL: {
    sscanf(sage::tokenSeek(msg, 2), "%d", &ftSize);
    label = string(sage::tokenSeek(msg, 3));
    refresh();
    break;
  }
  case TEMP_SCALE: {
    sscanf(sage::tokenSeek(msg, 2), "%f", &scale);
    refresh();
    break;
  }
  case SLIDE_DO_HIDE: {
    bool sh;
    sscanf(sage::tokenSeek(msg, 2), "%d", &sh);
    slideHidden = bool(sh);
    canSlide = true;
    slideY = 500;
    refresh();
    break;
  }
  case SHOW_DIM_BOUNDS: {
    int l,r,t,b;
    drawDimBounds = true;
    sscanf(sage::tokenSeek(msg, 2), "%d %d %d %d", &l,&r,&t,&b);
    dimBounds.right = r;
    dimBounds.left = l;
    dimBounds.top = t;
    dimBounds.bottom = b;
    refresh();
    break;
  }
  case SELECT_OVERLAY: {
    int sel;
    sscanf(sage::tokenSeek(msg, 2), "%d", &sel);
    selected = bool(sel);
    break;
  }
  case SET_SIZE: {
    int w, h;
    sscanf(sage::tokenSeek(msg, 2), "%d %d", &w, &h);
    size.w = w;
    size.h = h;
    refresh();
    break;
  }
  case SET_POS: {
    int x, y;
    sscanf(sage::tokenSeek(msg, 2), "%d %d", &x, &y);
    pos.x = x;
    pos.y = y;
    refresh();
    break;
  }
  }
}
