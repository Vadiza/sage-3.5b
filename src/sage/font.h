/** BTW, it is covered by the LGPL  ** Bob Pendleton **/
/** http://www.devolution.com/pipermail/sdl/2004-December/066119.html **/

/**

   +--------------------------------------+
   + Bob Pendleton: writer and programmer +
   + email: Bob@xxxxxxxxxxxxx             +
   + blog:  www.Stonewolf.net             +
   + web:   www.GameProgrammer.com        +
   +--------------------------------------+

   http://www.oreillynet.com/pub/au/1205

   Edits by Kaolin Fire ( kfire@xxxxxxxx; web: erif.org )
   now takes into account glyph minx so glyphs can overlap as intended
   now takes into account newline character

**/

#ifndef FONT_H
#define FONT_H



#if defined(__APPLE__)
#include <SDL/SDL.h>
//#include <SDL_ttf/SDL_ttf.h>
#include <SDL/SDL_ttf.h>
#include <ApplicationServices/ApplicationServices.h>
#include <AGL/agl.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>   // Header File For The OpenGL Library
#include <OpenGL/glu.h>   // Header File For The GLu Library
#else

#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <GL/gl.h>   // Header File For The OpenGL Library
#include <GL/glu.h>   // Header File For The GLu Library
#endif

struct color;

class Font
{
private:
  static const int minGlyph = ' ';
  static const int maxGlyph = 126;

  static int initCounter;

  typedef struct
  {
    int minx, maxx;
    int miny, maxy;
    int advance;
    SDL_Surface *pic;
    GLuint tex;
    GLfloat texMinX, texMinY;
    GLfloat texMaxX, texMaxY;
  } glyph;

  int height;
  int ascent;
  int descent;
  int lineSkip;
  glyph glyphs[maxGlyph + 1];

  const char *fontName;
  int pointSize;
  float fgRed, fgGreen, fgBlue;

  TTF_Font *ttfFont;

  SDL_Color foreground;

  void loadChar(char c);
  int power_of_two(int input);
  GLuint SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord);

public:

  Font(int pointSize);
  //float fgRed, float fgGreen, float fgBlue);

  ~Font();

  void initFont();
  int getLineSkip();
  int getHeight();
  void textSize(const char *text, SDL_Rect *r);
  void drawText(const char *text, int x, int y, float z, WIDGET_color c, int alpha=255);
};


#endif
