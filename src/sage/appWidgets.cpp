/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appWidgets.cpp
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


#include "appWidgets.h"


//-------------------//
/*     BASE CLASS    */
//-------------------//

appWidget::appWidget()
{
  // we first fill in the absolute coords but mark them as relative
  size.wType = WIDGET_NORMALIZED;
  size.hType = WIDGET_NORMALIZED;
  size.w = 50;
  size.h = 50;

  pos.xType = WIDGET_NORMALIZED;
  pos.yType = WIDGET_NORMALIZED;
  pos.x = 0;
  pos.y = 0;
  pos.normX = 0.0;
  pos.normY = 0.0;
  pos.xMargin = 0;
  pos.yMargin = 0;

  zOffset = float(APP_WIDGET_Z);

  parentID = -1;
  parentOrder = -1;

  ftSize = 10;
  fontColor = -1;
  ttSize = 10;
  backgroundColor = -1;
  alpha = -1;
  labelAlignment = -1;

  winID = -1;  // will be filled in later

  ui = NULL;
}


void appWidget::destroy()
{
  char msg[32];
  sprintf(msg, "%d %d", getId(), winID);
  sailObj->sendMessage(REMOVE_OBJECT, msg);
}


// we only allow normalized size for app widgets
void appWidget::setSize(int w, int h) {
  size.wType = WIDGET_NORMALIZED;
  size.hType = WIDGET_NORMALIZED;
  size.w = w;
  size.h = h;
  labelAlignment = LEFT; // the default if the user specified size
}


// relative (will be converted to normalized later)
void appWidget::setPos(int x, int y) {
  pos.xType = WIDGET_NORMALIZED;
  pos.yType = WIDGET_NORMALIZED;
  pos.x = x;
  pos.y = y;
}
void appWidget::align(objAlignment xa, objAlignment ya, int xMargin, int yMargin)
{
  pos.xType = WIDGET_ALIGNED;
  pos.yType = WIDGET_ALIGNED;
  pos.xAlignment = xa;
  pos.yAlignment = ya;
  pos.xMargin = xMargin;
  pos.yMargin = yMargin;
}
void appWidget::setX(int x) {
  pos.xType = WIDGET_NORMALIZED;
  pos.x = x;
}
void appWidget::alignX(objAlignment xa, int xMargin)
{
  pos.xType = WIDGET_ALIGNED;
  pos.xAlignment = xa;
  pos.xMargin = xMargin;
}
void appWidget::setY(int y) {
  pos.yType = WIDGET_NORMALIZED;
  pos.y = y;
}
void appWidget::alignY(objAlignment ya, int yMargin)
{
  pos.yType = WIDGET_ALIGNED;
  pos.yAlignment = ya;
  pos.yMargin = yMargin;
}


void appWidget::setLabel(const char *labelText, int fontSize)
{
  label = labelText;
  ftSize = fontSize;

  // if we have winID SAGE already knows about the widget so
  // send a message about the updated label
  if (winID != -1) {
    char* msg = new char[label.length()+20];
    sprintf(msg, "%d %d %s", SET_LABEL, ftSize, label.c_str());
    sendWidgetMessage(msg);
    delete msg;
  }
  //else      // no need for this since the display side will calculate the size
  //fitAroundLabel();
}

void appWidget::setTooltip(const char *tooltipText, int fontSize)
{
  tooltip = tooltipText;
  ttSize = fontSize;
}


void appWidget::normalizeCoords(int bufW, int bufH) {

  // calculate what the font size is relative to the app
  relativeFontSize = float(ftSize) / float(bufH);
  relativeTooltipSize = float(ttSize) / float(bufH);

  // always normalize size because that's all we allow for app widgets
  size.normW = (float)size.w / (float)bufW;
  size.normH = (float)size.h / (float)bufH;

  // normalize pos if not aligned
  if (pos.xType == WIDGET_NORMALIZED)
    pos.normX = (float)pos.x / (float)bufW;
  if (pos.yType == WIDGET_NORMALIZED)
    pos.normY = (float)pos.y / (float)bufH;
}


TiXmlDocument appWidget::getXML()
{
  // make the xml document
  TiXmlDocument xml;
  TiXmlElement *root = new TiXmlElement("widget");
  xml.LinkEndChild( root );

  // make the xml file
  addCommonInfo(root);
  addEventInfo(root);

  // label
  TiXmlElement *specific = new TiXmlElement("specific");
  if (!label.empty())  {
    TiXmlElement *l = new TiXmlElement("label");
    l->SetAttribute("fontSize", toString(ftSize));
    l->SetAttribute("relativeFontSize", toString(relativeFontSize));

    if (labelAlignment != -1)
      l->SetAttribute("labelAlignment", toString(labelAlignment));

    specific->LinkEndChild(l);
    l->LinkEndChild(new TiXmlText(label.data()));
  }
  if (!tooltip.empty())  {
    TiXmlElement *t = new TiXmlElement("tooltip");
    t->SetAttribute("fontSize", toString(ttSize));
    t->SetAttribute("relativeTooltipSize", toString(relativeTooltipSize));
    specific->LinkEndChild(t);
    t->LinkEndChild(new TiXmlText(tooltip.data()));
  }

  root->LinkEndChild(specific);
  addSpecificInfo(specific);


  // elements related to graphics
  TiXmlElement *graphics = new TiXmlElement("graphics");

  // font color
  if (fontColor != -1) {
    TiXmlElement *fc = new TiXmlElement("fontColor");
    fc->LinkEndChild(new TiXmlText(toString(fontColor)));
    graphics->LinkEndChild(fc);
  }

  // background color
  if (backgroundColor != -1) {
    TiXmlElement *bc = new TiXmlElement("backgroundColor");
    bc->LinkEndChild(new TiXmlText(toString(backgroundColor)));
    graphics->LinkEndChild(bc);
  }

  // transparency
  if (alpha != -1) {
    TiXmlElement *al = new TiXmlElement("alpha");
    al->LinkEndChild(new TiXmlText(toString(alpha)));
    graphics->LinkEndChild(al);
  }

  root->LinkEndChild(graphics);
  addGraphicsInfo(graphics);

  return xml;
}

void appWidget::addCommonInfo(TiXmlElement *root)
{
  // add the initial stuff like various IDs
  root->SetAttribute("widgetType", widgetType);
  //root->SetAttribute("winID", winID);  // XML FIELD NOT USED ANYMORE!!!
  root->SetAttribute("isGlobal", 1);
  root->SetAttribute("drawOrder", SAGE_INTER_DRAW);
  root->SetAttribute("displayID", 0);
  root->SetAttribute("parentID", parentID);
  root->SetAttribute("parentOrder", parentOrder);

  // add the common stuff
  TiXmlElement *common = new TiXmlElement("common");
  root->LinkEndChild(common);

  // width
  TiXmlElement *w = new TiXmlElement("w");
  common->LinkEndChild(w);
  w->SetAttribute("type", size.wType);
  w->LinkEndChild(new TiXmlText(toString(size.normW)));

  // height
  TiXmlElement *h = new TiXmlElement("h");
  common->LinkEndChild(h);
  h->SetAttribute("type", size.hType);
  h->LinkEndChild(new TiXmlText(toString(size.normH)));

  // x
  TiXmlElement *x = new TiXmlElement("x");
  common->LinkEndChild(x);
  x->SetAttribute("type", pos.xType);
  if (pos.xMargin != 0)
    x->SetAttribute("margin", pos.xMargin);
  if (pos.xType == WIDGET_ALIGNED)
    x->LinkEndChild(new TiXmlText(toString(pos.xAlignment)));
  else if(pos.xType == WIDGET_NORMALIZED)
    x->LinkEndChild(new TiXmlText(toString(pos.normX)));

  // y
  TiXmlElement *y = new TiXmlElement("y");
  common->LinkEndChild(y);
  y->SetAttribute("type", pos.yType);
  if (pos.yMargin != 0)
    y->SetAttribute("margin", pos.yMargin);
  if (pos.yType == WIDGET_ALIGNED)
    y->LinkEndChild(new TiXmlText(toString(pos.yAlignment)));
  else if(pos.yType == WIDGET_NORMALIZED)
    y->LinkEndChild(new TiXmlText(toString(pos.normY)));

  // z
  TiXmlElement *z = new TiXmlElement("z");
  common->LinkEndChild(z);
  z->SetAttribute("offset", toString(zOffset));  //app widget z offset
  z->LinkEndChild(new TiXmlText(toString(APP_Z_TYPE)));
}


void appWidget::addEventInfo(TiXmlElement *root)
{
  // add the event stuff
  TiXmlElement *events = new TiXmlElement("events");
  root->LinkEndChild(events);

  // go through each event and make an entry for it
  for (int i=0; i<wantedEvents.size(); i++)  {
    TiXmlElement *e = new TiXmlElement("event");
    events->LinkEndChild(e);
    e->LinkEndChild(new TiXmlText(toString(wantedEvents[i])));
  }
}


void appWidget::addSpecificInfo(TiXmlElement *parent)
{
}


void appWidget::addGraphicsInfo(TiXmlElement *parent)
{
}


string appWidget::getFileAsBase64(string filename)
{
  FILE * f;
  int size;
  char * buffer = NULL;
  string contentsInBase64;

  // filename is relative to SAGE_DIRECTORY
  char *sageDir = getenv("SAGE_DIRECTORY");
  string filePath (sageDir);
  filePath += "/"+filename;

  f = fopen (filePath.c_str() , "rb");
  if (f) {
    // obtain file size:
    fseek (f , 0 , SEEK_END);
    size = ftell (f);
    rewind (f);

    // allocate the buffer and read the file into it
    buffer = (char*)malloc(size);
    if (fread(buffer, 1, size, f) != size) {
      SAGE_PRINTLOG( "\n\nSAIL: Error while reading widget image file: %s\n\n",filePath.c_str());
      free(buffer);
      return contentsInBase64;
    }
    fclose(f);

    // encode it as base64
    contentsInBase64 = Base64::encode( string(buffer, size) );
    free(buffer);
  }
  else
    SAGE_PRINTLOG( "\nSAIL: Couldn't open widget image file: %s\n\n",filePath.c_str());

  return contentsInBase64;
}


// if imageFileName is empty or the file can't be opened or there was
// another error, it won't add anything
void appWidget::addGraphicsElement(TiXmlElement *parent,
                                   const char *elementName,
                                   string imageFileName)
{
  if (!imageFileName.empty())  {
    TiXmlElement *el = new TiXmlElement(elementName);
    parent->LinkEndChild(el);
    el->SetAttribute("format", toString(IMAGE_FMT));
    string b64(getFileAsBase64(imageFileName));
    if (!b64.empty())
      el->LinkEndChild(new TiXmlText(b64.data()));
    else
      parent->RemoveChild(el);  // something went wrong so remove it
  }
}



void appWidget::setParentIDs()
{
  // set all childrens' parentID to this sizer
  for (int i=0; i<children.size(); i++)
    children[i]->parentID = getId();
}


void appWidget::removeWidget(appWidget *w)
{
  vector<appWidget*>::iterator it;
  for (it=children.begin(); it!=children.end(); it++) {
    if ((*it)->getId() == w->getId()) {
      children.erase(it);
      break;
    }
  }
}

void appWidget::addChild(appWidget *w)
{
  // find the last position in the sizer and use that for the new widget
  int maxPos = -1;
  for (int i=0; i<children.size(); i++)
    if (children[i]->parentOrder > maxPos)
      maxPos = children[i]->parentOrder;

  // if we know the widgetID of the sizer, give it to the widget
  if (getId() != -1)
    w->parentID = getId();

  children.push_back(w);
  w->parentOrder = maxPos+1;

  // if this parent has been added already, its *ui pointer will be set
  // so just use it to add this child to SAGE... otherwise, this child
  // will be added when the parent is added to SAGE by the app programmer
  if (ui)
    ui->addWidget(w);
}


void appWidget::addChildren()
{
  for (int i=0; i<children.size(); i++)
    ui->addWidget(children[i]);
}


// resizes the widget to fit around the label of specified fontSize
/*void appWidget::fitAroundLabel()
  {
  if (!label.empty() && labelAlignment == -1) {
  Font ft (ftSize);

  // get the size of the text so that we can figure out the
  // size of the widget that fits around the label
  SDL_Rect r;
  ft.textSize(label.c_str(), &r);
  setSize(r.w + int(r.w*0.2), r.h + int(r.h*0.2));
  }
  }*/

bool appWidget::operator==(const char *wType)
{
  if (strcmp(wType, getType()) == 0)
    return true;
  else
    return false;
}

bool appWidget::operator!=(const char *wType)
{
  if (strcmp(wType, getType()) != 0)
    return true;
  else
    return false;
}

void appWidget::sendWidgetMessage(char *msg)
{
  char m[1024];
  sprintf(m, "%d %d %s", widgetID, winID, msg);
  sailObj->sendMessage(OBJECT_MESSAGE, m);
}


//-----------------------------------------------------------//
/*                       WIDGET CLASSES                      */
//-----------------------------------------------------------//


/**********************   BUTTON   ***************************/

void button::onUp(void (*callback)(int eventID, button *btnObj))
{
  cb_onUp = callback;
  wantedEvents.push_back(BUTTON_UP);
}

void button::onDown(void (*callback)(int eventID, button *btnObj))
{
  cb_onDown = callback;
  wantedEvents.push_back(BUTTON_DOWN);
}

void button::processEvent(int eventId, char *eventMsg)
{
  if (eventId == BUTTON_UP)
    cb_onUp(BUTTON_UP, this);
  else if(eventId == BUTTON_DOWN)
    cb_onDown(BUTTON_DOWN, this);
}

void button::addGraphicsInfo(TiXmlElement *parent)
{
  addGraphicsElement(parent, "up", upImage);
  addGraphicsElement(parent, "down", downImage);
  addGraphicsElement(parent, "over", overImage);
}

void button::addSpecificInfo(TiXmlElement *parent)
{
  if (isToggle) {
    TiXmlElement *t = new TiXmlElement("toggle");
    parent->LinkEndChild(t);
  }
}


/**********************   THUMBNAIL   ***************************/

void thumbnail::addSpecificInfo(TiXmlElement *parent)
{
  button::addSpecificInfo(parent);
  TiXmlElement *t = new TiXmlElement("thumbnail");
  t->SetAttribute("scaleMultiplier", toString(scale));
  t->SetAttribute("drawOutline", toString((int)outline));
  parent->LinkEndChild(t);
}



/**********************  EDNURANCE THUMBNAIL   ***************************/

void enduranceThumbnail::onRatingChange(void (*callback)(int eventID, enduranceThumbnail *btnObj))
{
  cb_onRatingChange = callback;
  wantedEvents.push_back(ENDURANCE_THUMBNAIL_RATING);
}

void enduranceThumbnail::processEvent(int eventId, char *eventMsg)
{
  if (eventId == ENDURANCE_THUMBNAIL_RATING) {
    rating = atoi(eventMsg);
    cb_onRatingChange(ENDURANCE_THUMBNAIL_RATING, this);
  }
  else
    thumbnail::processEvent(eventId, eventMsg);
}

void enduranceThumbnail::addSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *t = new TiXmlElement("enduranceThumbnail");
  t->SetAttribute("rating", toString(rating));
  parent->LinkEndChild(t);
  thumbnail::addSpecificInfo(parent);
}



/**********************   ICON   ***************************/

void icon::addGraphicsInfo(TiXmlElement *parent)
{
  addGraphicsElement(parent, "image", image);
}





/**********************   LABEL   ***************************/
// nothing here



/**********************   MENU   ***************************/
void menu::addSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *m = new TiXmlElement("menu");
  parent->LinkEndChild(m);

  // go through all the menu items and add them as xml nodes
  for (int i=0; i<menuItems.size(); i++) {
    TiXmlElement *mi = new TiXmlElement("menuItem");
    m->LinkEndChild(mi);
    mi->SetAttribute("id", toString(i));
    mi->LinkEndChild(new TiXmlText(menuItems[i].getLabel()));
  }

  button::addSpecificInfo(parent);
}

int menu::addMenuItem(const char * itemText, void (*callback)(menuItem * miObj), void * data)
{
  int mId = menuItems.size();
  menuItems.push_back( menuItem(itemText, mId, callback, data) );
  return mId;  // return menuItem id
}

void menu::processEvent(int eventId, char *eventMsg)
{
  int itemId = atoi(eventMsg);
  menuItems[itemId].cb_onItem( &menuItems[itemId] );
}







/**********************   SIZER   ***************************/

// the order in which you add the widgets matters because that's
// how they will be lined up in the end (top-to-bottom and left-to-right)


void sizer::addChild(appWidget *w)
{
  appWidget::addChild(w);

  // may not be necessary
  //fitAroundChildren();
}

void sizer::addSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *s = new TiXmlElement("sizer");
  s->SetAttribute("direction", toString(dir));
  parent->LinkEndChild(s);
}


// resizes the sizer to fit around the children
/*void sizer::fitAroundChildren()
  {
  return;

  size.w = size.h = 0;

  if (dir == HORIZONTAL) {
  for (int i=0; i<children.size(); i++) {
  size.w += children[i]->getW();
  if (children[i]->getH() > size.h)
  size.h = children[i]->getH();
  }
  }
  else {
  for (int i=0; i<children.size(); i++) {
  size.h += children[i]->getH();
  if (children[i]->getW() > size.w)
  size.w = children[i]->getW();
  }
  }
  }*/



/**********************   PANEL   ***************************/
void panel::addSpecificInfo(TiXmlElement *parent)
{
  TiXmlElement *f = new TiXmlElement("fit");
  f->SetAttribute("width", toString(int(fitWidth)));
  f->SetAttribute("height", toString(int(fitHeight)));
  parent->LinkEndChild(f);
}
