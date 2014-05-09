#include "drawObjects.h"
#include "font.h"
#include "misc.h"


int Font::initCounter = 0;



void Font::loadChar(char c)
{
  GLfloat texcoord[4];
  char letter[2] = {0, 0};

  if ((minGlyph <= c) &&
      (c <= maxGlyph) &&
      (NULL == glyphs[((int)c)].pic))
  {
    SDL_Surface *g0 = NULL;
    SDL_Surface *g1 = NULL;

    letter[0] = c;
    TTF_GlyphMetrics(ttfFont,
                     (Uint16)c,
                     &glyphs[((int)c)].minx,
                     &glyphs[((int)c)].maxx,
                     &glyphs[((int)c)].miny,
                     &glyphs[((int)c)].maxy,
                     &glyphs[((int)c)].advance);

    g0 = TTF_RenderText_Blended(ttfFont, letter, foreground);

    if (NULL != g0) {
      g1 = SDL_DisplayFormatAlpha(g0);
      SDL_FreeSurface(g0);
    }

    if (NULL != g1) {
      glyphs[((int)c)].pic = g1;
      glyphs[((int)c)].tex = SDL_GL_LoadTexture(g1, texcoord);
      glyphs[((int)c)].texMinX = texcoord[0];
      glyphs[((int)c)].texMinY = texcoord[1];
      glyphs[((int)c)].texMaxX = texcoord[2];
      glyphs[((int)c)].texMaxY = texcoord[3];
    }
  }
}



Font::Font(int pointSize):

  pointSize(pointSize),
  //fgRed(fgRed), fgGreen(fgGreen), fgBlue(fgBlue),
  ttfFont(NULL)
{
  // force the font here...
  char fontPath[256];
  char *sageDir = getenv("SAGE_DIRECTORY");
  sprintf(fontPath, "%s/fonts/Vera.ttf", sageDir);
  fontName = fontPath;

  if (0 == initCounter) {
    if (TTF_Init() < 0) {
      //TODO :: errorExit("Can't init SDL_ttf");
      SAGE_PRINTLOG( "\nCan't init sdl_ttf !?\n");
    }
  }
  initCounter++;
  initFont();
}



Font::~Font()
{
  initCounter--;
  if (0 == initCounter)
    TTF_Quit();
}



void Font::initFont()
{
  int i;
  ttfFont = TTF_OpenFont(fontName, pointSize);
  if (NULL == ttfFont) {
    SAGE_PRINTLOG("\nCan't open font file\n");
    //TODO :: errorExit("Can't open font file");
  }

  foreground.r = (Uint8)(DEFAULT_FONT_COLOR.r);
  foreground.g = (Uint8)(DEFAULT_FONT_COLOR.g);
  foreground.b = (Uint8)(DEFAULT_FONT_COLOR.b);

  height = TTF_FontHeight(ttfFont);
  ascent = TTF_FontAscent(ttfFont);
  descent = TTF_FontDescent(ttfFont);
  lineSkip = TTF_FontLineSkip(ttfFont);

  for (i = minGlyph; i <= maxGlyph; i++) {
    glyphs[i].pic = NULL;
    glyphs[i].tex = 0;
  }
}



int Font::getLineSkip()
{
  return lineSkip;
}



int Font::getHeight()
{
  return height;
}



void Font::textSize(const char *text, SDL_Rect *r)
{
  int maxx = 0;
  int advance = 0;
  int minx = 0;
  int w_largest = 0;
  char lastchar = 0;

  r->x = 0;
  r->y = 0;
  r->w = 0;
  r->h = height;

  while (0 != *text)
  {
    if ((minGlyph <= *text) && (*text <= maxGlyph))
    {
      lastchar = *text;
      if (*text == '\n') {
        r->h += lineSkip;
        r->w = r->w - advance + maxx;
        if (r->w > w_largest) w_largest = r->w;
        r->w = 0;
      } else {
        loadChar(*text);

        maxx = glyphs[((int)*text)].maxx;
        advance = glyphs[((int)*text)].advance;
        r->w += advance;
      }
    }

    text++;
  }

  if (lastchar != '\n') {
    r->w = r->w - advance + maxx;
    if (r->w > w_largest) w_largest = r->w;
  } else
    r->h -= lineSkip;

  if (w_largest > r->w)
    r->w = w_largest;
}



void Font::drawText(const char *text, int x, int y, float z, WIDGET_color c, int alpha)
{
  GLfloat left, right;
  GLfloat top, bottom;
  GLfloat texMinX, texMinY;
  GLfloat texMaxX, texMaxY;
  GLfloat minx;
  GLfloat baseleft = x;

  /*foreground.r = 255;
    foreground.g = 255;
    foreground.b = 255; */

  glEnable(GL_TEXTURE_2D);

  // set the color of the font
  glColor4ub(c.r, c.g, c.b, alpha);

  while (0 != *text)
  {
    if (*text == '\n') {
      x = (int)baseleft;
      y += lineSkip;
    }
    else if ((minGlyph <= *text) && (*text <= maxGlyph)) {
      loadChar(*text);

      texMinX = glyphs[((int)*text)].texMinX;
      texMinY = glyphs[((int)*text)].texMinY;
      texMaxX = glyphs[((int)*text)].texMaxX;
      texMaxY = glyphs[((int)*text)].texMaxY;

      minx = glyphs[((int)*text)].minx;

      left   = x + minx;
      right  = x + glyphs[((int)*text)].pic->w + minx;
      //top    = y;
      //bottom = y + glyphs[((int)*text)].pic->h;
      bottom = y;
      top    = y + glyphs[((int)*text)].pic->h;

      glBindTexture(GL_TEXTURE_2D, glyphs[((int)*text)].tex);

      glBegin(GL_TRIANGLE_STRIP);

      glTexCoord2f(texMinX, texMinY); glVertex3f( left,    top, z);
      glTexCoord2f(texMaxX, texMinY); glVertex3f(right,    top, z);
      glTexCoord2f(texMinX, texMaxY); glVertex3f( left, bottom, z);
      glTexCoord2f(texMaxX, texMaxY); glVertex3f(right, bottom, z);

      glEnd();

      x += glyphs[((int)*text)].advance;
    }

    text++;
  }
}


/* Quick utility function for texture creation */
int Font::power_of_two(int input)
{
  int value = 1;

  while ( value < input ) {
    value <<= 1;
  }
  return value;
}


GLuint Font::SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord)
{
  GLuint texture;
  int w, h;
  SDL_Surface *image;
  SDL_Rect area;
  Uint32 saved_flags;
  Uint8  saved_alpha;

  /* Use the surface width and height expanded to powers of 2 */
  w = power_of_two(surface->w);
  h = power_of_two(surface->h);
  texcoord[0] = 0.0f;                        /* Min X */
  texcoord[1] = 0.0f;                        /* Min Y */
  texcoord[2] = (GLfloat)surface->w / w;        /* Max X */
  texcoord[3] = (GLfloat)surface->h / h;        /* Max Y */

  image = SDL_CreateRGBSurface(
                               SDL_SWSURFACE,
                               w, h,
                               32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
                               0x000000FF,
                               0x0000FF00,
                               0x00FF0000,
                               0xFF000000
#else
                               0xFF000000,
                               0x00FF0000,
                               0x0000FF00,
                               0x000000FF
#endif
                               );

  if ( image == NULL ) {
    return 0;
  }

  /* Save the alpha blending attributes */
  saved_flags = surface->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
  saved_alpha = surface->format->alpha;
  if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA )
    SDL_SetAlpha(surface, 0, 0);

  /* Copy the surface into the GL texture image */
  area.x = 0;
  area.y = 0;
  area.w = surface->w;
  area.h = surface->h;
  SDL_BlitSurface(surface, &area, image, &area);

  /* Restore the alpha blending attributes */
  if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA )
    SDL_SetAlpha(surface, saved_flags, saved_alpha);

  /* Create an OpenGL texture for the image */
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               w, h,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               image->pixels);
  SDL_FreeSurface(image); /* No longer needed */

  return texture;
}
