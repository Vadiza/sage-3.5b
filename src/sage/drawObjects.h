#ifndef DRAW_OBJ_H_
#define DRAW_OBJ_H_

#define DRAW_DELETE(p)      if ( (p) != NULL ) { delete p; p = NULL; }

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// for xml parsing
#include "tinyxml.h"

// for base64 conversion
#include "base64.h"

// for fonts
//#include "font.h"

// widget types
#define ICON   "icon"
#define BUTTON "button"
#define LABEL  "label"
#define MENU   "menu"
#define SIZER  "sizer"
#define PANEL  "panel"
#define POINTER "pointer"
#define APP    "app"
#define SPLITTER    "splitter"
#define THUMBNAIL "thumbnail"
#define ENDURANCE_THUMBNAIL "enduranceThumbnail"

// for specifying alignment of objects
enum objAlignment {CENTER, LEFT, RIGHT, TOP, BOTTOM,
                   LEFT_OUTSIDE, RIGHT_OUTSIDE, TOP_OUTSIDE, BOTTOM_OUTSIDE };

// z-ordering (not really z-values...)
#define TOP_Z_TYPE    0
#define APP_Z_TYPE    1
#define BOTTOM_Z_TYPE 2

#define TOP_Z -1.0
#define BOTTOM_Z  -9999.0

// z offsets relative to appZ value
#define APP_OVERLAY_Z -0.1
#define APP_WIDGET_Z -0.2  // widgets requested by the app
#define DIM_WIDGET_Z -0.3      // widgets tied to the app but requested by DIM

#define Z_STEP 0.001   // for drawing something with small offsets (like font above label background)
#define Z_CHILD_DIFF 0.01 // z difference between child and parent

// for specifying size and pos of objects
#define WIDGET_ABSOLUTE   0
#define WIDGET_NORMALIZED 1
#define WIDGET_ALIGNED    2

// sizer types
#define HORIZONTAL 0
#define VERTICAL   1

// font sizes
#define NORMAL 10
#define SMALLER  8
#define SMALLEST  6
#define LARGER  12
#define LARGEST  14

// fade directions
enum fadeDir {FADE_IN, FADE_OUT};


struct objectSize {
  int wType, hType;
  int w,h;
  float normW, normH;
};

struct objectPos {
  int xType, yType;
  int xMargin, yMargin;
  int x, y;
  float normX, normY;
  int xAlignment, yAlignment;
};


// different ways to specify graphics for widgets
#define IMAGE_FMT 0
#define FILE_FMT  1

// draw orders...
#define SAGE_POST_DRAW  0
#define SAGE_INTER_DRAW 1
#define SAGE_PRE_DRAW   2



/**************************/
/*         EVENTS         */
/**************************/

// widget events
#define BUTTON_DOWN 0
#define BUTTON_UP   1
#define MENU_ITEM_CLICKED 2
#define ENDURANCE_THUMBNAIL_RATING  3
#define MOVIE_MUTE  4


// common widget events (go to more than one widget)
#define COMMON_WIDGET_EVENTS 1000
#define SET_LABEL        COMMON_WIDGET_EVENTS + 1
#define SHOW_DIM_BOUNDS  COMMON_WIDGET_EVENTS + 2
#define TEMP_SCALE       COMMON_WIDGET_EVENTS + 3
#define SLIDE_DO_HIDE    COMMON_WIDGET_EVENTS + 4
#define SELECT_OVERLAY   COMMON_WIDGET_EVENTS + 5
#define SET_SIZE         COMMON_WIDGET_EVENTS + 6
#define SET_POS          COMMON_WIDGET_EVENTS + 7

// global display events (always negative)
// these change display parameters
#define SHOW_SIZERS    -1
#define SET_APP_BOUNDS -2
#define SET_INITIAL_BOUNDS -3


/**************************/


// colors
struct WIDGET_color {
  int r,g,b;
};

const WIDGET_color DEFAULT_FONT_COLOR = {255,255,255};
const WIDGET_color DEFAULT_BACKGROUND_COLOR = {55,55,55};
#define DEFAULT_ALPHA 200;
#define MIN_FONT_SIZE 8

#define DEFAULT_BUTTON "images/default_button_up.png"



/***************************/
/* commonly used functions */
/***************************/

inline int colorToInt(WIDGET_color c)
{
  return (c.r<<16) + (c.g<<8) + c.b;
}

inline WIDGET_color intToColor(int i)
{
  WIDGET_color c;
  c.r = (i & 0x00FF0000)>>16;
  c.g = (i & 0x0000FF00)>>8;
  c.b =  i & 0x000000FF;
  return c;
}


inline int round(float x) {
  if (x >= 0.0)
    return (int)(x+0.5);
  else
    return (int)(x-0.5);
}


inline std::string toString(float i)
{
  std::ostringstream buffer;
  buffer << i;
  return buffer.str();
}


inline std::string toString(int i)
{
  std::ostringstream buffer;
  buffer << i;
  return buffer.str();
}

#endif
