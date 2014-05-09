/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appWidgets.h
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

#ifndef APPWIDGETS_H_
#define APPWIDGETS_H_


// where all the event IDs are defined
#include "drawObjects.h"
#include <vector>
#include "appUI.h"

//#include "base64.h"
#include "sail.h"

using namespace std;

class sail;
class appUI;

//-------------------//
/*     BASE CLASS    */
//-------------------//

class appWidget {
private:

protected:
  objectSize size;
  objectPos pos;
  char widgetType[30];

  void *userData;

  vector<appWidget*> children;  // a list of children

  // label stuff
  string label;
  int ftSize;
  string tooltip;
  int ttSize;
  float relativeTooltipSize;  // ratio of ttSize/appHeight
  int labelAlignment;   // LEFT, RIGHT, CENTER
  float relativeFontSize;  // ratio of ftSize/appHeight
  int fontColor, backgroundColor;
  int alpha;

  // when you register for events, they are automatically
  // added to this list of wanted events
  vector<int> wantedEvents;

  // for building the xml tree from widget info
  void addCommonInfo(TiXmlElement *root);
  void addEventInfo(TiXmlElement *root);
  virtual void addSpecificInfo(TiXmlElement *parent);
  virtual void addGraphicsInfo(TiXmlElement *parent);

  // helper methods
  string getFileAsBase64(string filename);
  void addGraphicsElement(TiXmlElement *parent,
                          const char *name, string b64ImageData);
  //void fitAroundLabel();


public:
  appWidget();
  virtual ~appWidget() {}
  sail *sailObj;
  appUI *ui;   // so that we can do ui->addWidget for children

  int winID;   //for associating widgets with apps
  int parentID;  //for associating widgets with parents
  int parentOrder;  // position of the widget inside a sizer
  int widgetID;
  float zOffset;  // from the app z

  // for storing user defined data
  void setUserData(void *d) {userData=d;}
  void * getUserData();

  char *getType() { return widgetType; };
  int getId() { return widgetID; };
  virtual void destroy();  // tells fsManager to remove overlay

  void setParentIDs(); // used internally
  bool hasChildren() { return (!children.empty()); }
  void addChildren();

  // for building the xml tree from widget info
  TiXmlDocument getXML();

  // this actually converts to normalized coords (if not aligned)
  void normalizeCoords(int bufW, int bufH);

  // called by appUI when a new event comes in
  virtual void processEvent(int eventId, char *eventMsg) = 0;

  // == operator for comparing types
  bool operator==(const char *wType);
  bool operator!=(const char *wType);

  // for sending messages to sail
  void sendWidgetMessage(char *msg);

  // for removing a widget from a parent
  virtual void removeWidget(appWidget *w);

  /***********  CAN BE CALLED BY THE APP PROGRAMMER  ***************/

  // size
  void setSize(int w, int h);
  int getW() {return size.w;}
  int getH() {return size.h;}

  // position
  void setPos(int x, int y);
  void setX(int x);
  void setY(int y);
  void alignX(objAlignment xa, int xMargin=0);
  void alignY(objAlignment ya, int yMargin=0);
  void align(objAlignment xa, objAlignment ya, int xMargin=0, int yMargin=0);

  // label (if necessary)
  void setLabel(const char *labelText, int fontSize=NORMAL);
  const char * getLabel() { return label.c_str(); }
  void alignLabel(int alignment) { labelAlignment = alignment; }
  void setFontColor(int r, int g, int b) {
    WIDGET_color tt;
    tt.r=r;
    tt.g=g;
    tt.b=b;
    fontColor=colorToInt( tt );
  }
  void setBackgroundColor(int r, int g, int b)
  {
    WIDGET_color tt;
    tt.r=r;
    tt.g=g;
    tt.b=b;
    backgroundColor=colorToInt( tt );  }
  void setTransparency(int a) { alpha=a; }

  void setTooltip(const char *tooltipText, int fontSize=NORMAL);
  const char * getTooltip() { return tooltip.c_str(); }

  // children (only makes sense for some widgets... panel, sizer)
  virtual void addChild(appWidget *w);

  /*****************************************************************/
};



//-----------------------------------------------------------//
/*                       WIDGET CLASSES                      */
//-----------------------------------------------------------//



/**********************   BUTTON   ***************************/

class button : public appWidget {

protected:
  // callbacks for events
  void (*cb_onDown)(int, button *);
  void (*cb_onUp)(int, button *);

  string upImage;
  string downImage;
  string overImage;
  bool isToggle;

  // internal methods
  virtual void addGraphicsInfo(TiXmlElement *parent);
  virtual void addSpecificInfo(TiXmlElement *parent);
  virtual void processEvent(int eventId, char *eventMsg);

public:
  button() { strcpy(widgetType, BUTTON); isToggle=false; }

  // register for event handlers
  void onUp( void (*callback)(int eventId, button *btnObj) );
  void onDown( void (*callback)(int eventId, button *btnObj) );

  // set the graphics if you want or the label
  void setUpImage(const char *filename) { upImage=filename; }
  void setDownImage(const char *filename) { downImage=filename; }
  void setOverImage(const char *filename) { overImage=filename; }

  // make a toggle button
  void setToggle(bool t) { isToggle=t; }
};


/**********************   THUMBNAIL   ***************************/

class thumbnail : public button {

protected:
  float scale; // how much to enlarge the thumbnail when mouse is over
  bool outline;

  // internal methods
  virtual void addSpecificInfo(TiXmlElement *parent);

public:
  thumbnail() {
    strcpy(widgetType, THUMBNAIL);
    scale = 1.4f;
    outline = false;
  }

  // set scale multiplier
  void setScaleMultiplier(float scaleMultiplier) { scale=scaleMultiplier; }

  // should outline be shown?
  void showOutline(bool showOutline) { outline=showOutline; }
};


/**********************   ENDURANCE THUMBNAIL   ***************************/

class enduranceThumbnail : public thumbnail {

protected:
  // thumbnail rating
  int rating;

  // callbacks for events
  void (*cb_onRatingChange)(int, enduranceThumbnail *);

  // internal methods
  virtual void addSpecificInfo(TiXmlElement *parent);
  virtual void processEvent(int eventId, char *eventMsg);

public:
  enduranceThumbnail() {
    strcpy(widgetType, ENDURANCE_THUMBNAIL);
    rating=0;
  }

  // register for event handlers
  void onRatingChange( void (*callback)(int eventId, enduranceThumbnail *btnObj) );

  // thumbnail rating
  void setRating(int newRating) { rating=newRating; }
  int getRating() { return rating; }
};



/**********************   ICON   ***************************/

class icon : public appWidget {

private:
  string image;

  // internal methods
  void addGraphicsInfo(TiXmlElement *parent);
  void processEvent(int eventId, char *eventMsg) {}

public:
  icon() { strcpy(widgetType,ICON); }
  icon(const char * imageFile) { strcpy(widgetType,ICON); image=imageFile; }
  void setImage(const char *imageFile) { image=imageFile; }
};



/**********************   LABEL   ***************************/

class label : public appWidget {

private:
  // internal methods
  void processEvent(int eventId, char *eventMsg) {}

public:
  label() { strcpy(widgetType,LABEL); }
  label(const char * text, int fontSize)
  {
    strcpy(widgetType,LABEL);
    setLabel(text, fontSize);
  }
};




/**********************   MENU   ***************************/

class menuItem {
private:
  char label[256];
  int itemId;
  void * userData;

public:

  void (*cb_onItem) (menuItem*);  // callback

  menuItem(const char * itemText, int id, void (*callback)(menuItem * miObj), void * data)
  {
    strcpy(label, itemText);
    cb_onItem = callback;
    itemId = id;
    userData = data;
  }
  char * getLabel() { return label; }
  int getItemId() { return itemId; }
  void *getUserData() {return userData;}
};


class menu : public button {

private:
  // menu items as strings
  vector<menuItem> menuItems;

  // internal methods
  void addSpecificInfo(TiXmlElement *parent);
  void processEvent(int eventId, char *eventMsg);

public:
  menu()
  {
    strcpy(widgetType,MENU);
    wantedEvents.push_back(MENU_ITEM_CLICKED);
  }

  int addMenuItem(const char * item, void (*callback)(menuItem * miObj), void * data=NULL);
};


/**********************   SIZER   ***************************/


// Only does positioning for now... the sizes of its items are still
// defined relative to the app window itself.
// You only need to setPos for the sizer since size is automatically
// determined from its children.

class sizer : public appWidget {

private:
  int dir;

  // internal methods
  void addSpecificInfo(TiXmlElement *parent);
  void processEvent(int eventId, char *eventMsg) {}
  //void fitAroundChildren();  // resizes the sizer to fit around the children

public:

  sizer(int direction)   // HORIZONTAL - or - VERTICAL
  {
    strcpy(widgetType,SIZER);
    dir=direction;
  }
  void addChild(appWidget *w);
};



/**********************   PANEL   ***************************/

// panel is just for visual purposes, basically a background color

class panel : public appWidget {

private:

  // internal methods
  void processEvent(int eventId, char *eventMsg) {}
  void addSpecificInfo(TiXmlElement *parent);
  bool fitWidth;
  bool fitHeight;

public:

  panel(int r, int g, int b)   // color of the panel
  {
    strcpy(widgetType,PANEL);
    setBackgroundColor(r,g,b);
    //zOffset = APP_OVERLAY_Z + 0.01;
    fitWidth = true;
    fitHeight = true;
  }

  // do we automatically resize the panel to fit around
  // the widgets (children) in width or height or neither or both
  // otherwise you set the size manually
  void fitInWidth(bool doFit) { fitWidth=doFit; }
  void fitInHeight(bool doFit){ fitHeight=doFit; }
};

#endif
