/*****************************************************************************************
 * imageviewer: loads an image and sends it to SAGE for display
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
 * Direct questions, comments etc to www.evl.uic.edu/cavern/forum
 *
 * Author: Ratko Jagodic, Luc Renambot
 *
 *****************************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <wand/magick-wand.h>

// headers for SAGE
#include "sail.h"
#include "misc.h"



// #if MagickLibVersion >= 0x645
// #define MagickGetImagePixels MagickGetAuthenticPixels
// #endif


#ifdef USE_CAIRO
#include <cairo/cairo.h>
#endif


#ifdef USE_DXT

// for dxt compression
#include "libdxt.h"
// if true, it will show the original image and not the dxt compressed one
bool loadDXT = true;

#else

bool loadDXT = false;
typedef unsigned char byte;
#define memalign(x,y) malloc((y))

#endif


using namespace std;


#define ThrowWandException(wand)                                        \
  {                                                                     \
    char                                                                \
      *description;                                                     \
                                                                        \
    ExceptionType                                                       \
      severity;                                                         \
                                                                        \
    description=MagickGetException(wand,&severity);                     \
    (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description);           \
    exit(-1);                                                           \
  }


// -----------------------------------------------------------------------------
#ifdef USE_CAIRO

#define CAIRO_BORDERS 0.02  // percent

int counter = 1;

void curved_rectangle(cairo_t *cr, cairo_surface_t *image)
{
  double x1,y1;
  double x0, y0, rect_width, rect_height, radius;
  int w, h;
  double ww, hh;
  double ratio;

  cairo_identity_matrix (cr);

  w = cairo_image_surface_get_width (image);
  h = cairo_image_surface_get_height (image);
  ratio = (double)w / (double)h;

  radius = w * 0.025;

  if (ratio > 1.0) {
    rect_width  = ( w * (1.0-2.0*CAIRO_BORDERS) );
    rect_height = h * (rect_width / w);
    x0 = w * CAIRO_BORDERS;
    y0 = (h - rect_height ) / 2.0;
  }
  else {
    rect_height = (h * 0.95);
    rect_width  = w * (rect_height / h);
    x0 = (w - rect_width ) / 2.0;
    y0 = h * 0.05;
  }

  x1=x0+rect_width;
  y1=y0+rect_height;

  if (ratio > 1.0) {
    ww = rect_width / w;
    hh = ww;
  }
  else {
    ww = rect_height / h;
    hh = ww;
  }

  if (!rect_width || !rect_height)
    return;
  if (rect_width/2<radius) {
    if (rect_height/2<radius) {
      cairo_move_to  (cr, x0, (y0 + y1)/2);
      cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
      cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
    } else {
      cairo_move_to  (cr, x0, y0 + radius);
      cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
      cairo_line_to (cr, x1 , y1 - radius);
      cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
    }
  } else {
    if (rect_height/2<radius) {
      cairo_move_to  (cr, x0, (y0 + y1)/2);
      cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
      cairo_line_to (cr, x1 - radius, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
      cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
      cairo_line_to (cr, x0 + radius, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
    } else {
      cairo_move_to  (cr, x0, y0 + radius);
      cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
      cairo_line_to (cr, x1 - radius, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
      cairo_line_to (cr, x1 , y1 - radius);
      cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
      cairo_line_to (cr, x0 + radius, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
    }
  }
  cairo_close_path (cr);

  // Draw Text
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 15.0);
  cairo_move_to (cr, 8.0, 15.0);
  char msg[256];
  memset(msg, 0, 256);
  sprintf(msg, "%06d", counter++);
  cairo_set_source_rgb (cr, 0.0, 0.0, 1);
  cairo_show_text (cr, msg);


  cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
  cairo_set_line_width (cr, (0.8*w/100.0) );
  cairo_stroke_preserve (cr);

  // Draw Image
  cairo_clip (cr);
  cairo_new_path (cr);

  cairo_identity_matrix (cr);
  cairo_translate (cr, x0, y0);
  cairo_scale (cr, ww, hh);

  cairo_set_source_surface (cr, image, 0,0);
  cairo_paint (cr);

  // Extra
  double xc = w/2.0; //128.0;
  double yc = h/2.0; //128.0;
  double rradius = 0.8*h/2.0;
  static double init = 0.0;
  double angle1 = init + 45.0  * (M_PI/180.0);  /* angles are specified */
  double angle2 = init + 180.0 * (M_PI/180.0);  /* in radians           */

  // arc
  cairo_set_source_rgba (cr, 0.1, 0.8, 0.1, 0.5);
  cairo_set_line_width (cr, 20.0);
  cairo_arc (cr, xc, yc, rradius, angle1, angle2);
  cairo_stroke (cr);

  // draw helping lines
  cairo_set_source_rgba (cr, 1, 0.2, 0.2, 0.8);
  cairo_set_line_width (cr, 10.0);

  cairo_arc (cr, xc, yc, 10.0, 0, 2*M_PI);
  cairo_fill (cr);

  cairo_arc (cr, xc, yc, rradius, angle1, angle1);
  cairo_line_to (cr, xc, yc);
  cairo_arc (cr, xc, yc, rradius, angle2, angle2);
  cairo_line_to (cr, xc, yc);
  cairo_stroke (cr);
  init += 4.0 * (M_PI/180.0);
}

void draw(cairo_t *cr, cairo_pattern_t *pat, cairo_surface_t *surface, cairo_surface_t *image)
{
  cairo_save (cr);

  int w = cairo_image_surface_get_width (surface);
  int h = cairo_image_surface_get_height(surface);

  cairo_identity_matrix (cr);

  // Draw background
  cairo_rectangle (cr, 0, 0, w, h);
  cairo_set_source (cr, pat);
  cairo_fill (cr);

  // Draw image
  curved_rectangle(cr, image);

  // Render
  cairo_show_page (cr);

  cairo_restore (cr);
}


#endif


// -----------------------------------------------------------------------------

#ifdef USE_DXT
void readDXT(string fileName, byte **buffer, unsigned int &width,
             unsigned int &height)
{
  unsigned long r;
  unsigned int numBytes;
  FILE *f = fopen(fileName.data(), "rb");

  // read the size of the image from the first 8 bytes
  r = fread(&width, sizeof(unsigned int), 1, f);
  r = fread(&height, sizeof(unsigned int), 1, f);
  r = fread(&numBytes, sizeof(unsigned int), 1, f);

  // allocate buffer size and read the rest of the file into it
  *buffer = (byte*) malloc(width*height*4/8);
  memset(*buffer, 0, width*height*4/8);
  r = fread(*buffer, 1, width*height*4/8, f);
  fclose(f);
}


void writeDXT(string fileName, byte *buffer, unsigned int width,
              unsigned int height, unsigned int numBytes)
{
  fprintf(stderr, "\n**** Writing DXT to file... %u bytes", numBytes);

  unsigned long r;
  FILE *f = fopen(fileName.data(), "wb");

  if (f != NULL)
  {
    // write the size of the image in the first 8 bytes
    r = fwrite(&width, sizeof(unsigned int), 1, f);
    r = fwrite(&height, sizeof(unsigned int), 1, f);
    r = fwrite(&numBytes, sizeof(unsigned int), 1, f);

    // write the buffer out to the file
    r = fwrite(buffer, 1, numBytes, f);
    fclose(f);
  }
  else
    fprintf(stderr, "\n**** Imageviewer ERROR: Unable to write DXT file. Check dir permissions.\n");
}
#endif


void getRGBA(string fileName, byte **rgba, unsigned int &width,
             unsigned int &height)
{
  // use ImageMagick to read all other formats
  MagickBooleanType status;
  MagickWand *wand;

  // read file
  wand=NewMagickWand();
  status=MagickReadImage(wand, fileName.data());
  if (status == MagickFalse)
    ThrowWandException(wand);

  // get the image size
  width = MagickGetImageWidth(wand);
  height = MagickGetImageHeight(wand);

  // crop the image if necessary to make sure it's a multiple of 4
  if (loadDXT)
  {
    if (width%4 != 0 || height%4 != 0)
    {
      fprintf(stderr, "\n**** Image cropped a few pixels to be a multiple of 4 for dxt");
      width -= width%4;
      height -= height%4;
    }

    // flip the image to have the correct orientation for dxt
    MagickFlipImage(wand);
  }

  // get the pixels
#ifdef USE_DXT
  *rgba = (byte*) memalign(16, width*height*4);
  memset(*rgba, 0, width*height*4);
#if MagickLibVersion >= 0x645
  MagickExportImagePixels(wand, 0, 0, width, height, "RGBA", CharPixel, *rgba);
#else
  MagickGetImagePixels(wand, 0, 0, width, height, "RGBA", CharPixel, *rgba);
#endif

#else
  *rgba = (byte*) malloc(width*height*3);
  memset(*rgba, 0, width*height*3);
#if MagickLibVersion >= 0x645
  MagickExportImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, *rgba);
#else
  MagickGetImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, *rgba);
#endif
#endif
  DestroyMagickWand(wand);
}

#ifdef USE_DXT
void rgbaToDXT(string fileName, byte **rgba, byte **dxt, unsigned int width,
               unsigned int height)
{
  unsigned int numBytes;

  // compress into DXT
  *dxt = (byte*) memalign(16, width*height*4/8);
  memset(*dxt, 0, width*height*4/8);
  numBytes = CompressDXT(*rgba, *dxt, width, height, FORMAT_DXT1, 1);

  // write this DXT out to a file (change extension to .dxt)
  string dxtFileName = string(fileName);
  dxtFileName.resize(fileName.rfind("."));
  dxtFileName += ".dxt";
  writeDXT(dxtFileName, *dxt, width, height, numBytes);
}


bool dxtFileExists(string fileName)
{
  // replace the extension with .dxt
  string dxtFileName = string(fileName);
  dxtFileName.resize(fileName.rfind("."));
  dxtFileName += ".dxt";

  // check whether the file exists by trying to open it
  FILE *dxtFile = fopen(dxtFileName.data(), "r");
  if ( dxtFile == NULL)
  {
    fprintf(stderr, "\nDXT file for %s doesn't exist yet.", fileName.data());
    return false;
  }
  else
  {
    fclose(dxtFile);
    return true;
  }
}
#endif

// -----------------------------------------------------------------------------



int main(int argc,char **argv)
{
  byte *sageBuffer = NULL;  // buffers for sage and dxt data
  byte *dxt = NULL;
  byte *rgba = NULL;
  unsigned int width, height;  // image size
  unsigned int window_width=-1, window_height=-1;  // sage window size

#ifdef USE_CAIRO
  cairo_surface_t *surface;
  cairo_surface_t *image;
  cairo_t *cr;
  loadDXT = false;
#endif


  // parse command line arguments
  if (argc < 2){
    fprintf(stderr, "\n\nUSAGE: imageviewer filename [width] [height] [-show_original]");
    return 0;
  }
  for (int argNum=2; argNum<argc; argNum++)
  {
    if (strcmp(argv[argNum], "-show_original") == 0) {
      loadDXT = false;
    }
    else if(atoi(argv[argNum]) != 0 && atoi(argv[argNum+1]) != 0) {
      window_width = atoi( argv[argNum] );
      window_height = atoi( argv[argNum+1] );
      argNum++;  // increment because we read two args here
    }
  }


  // check file extension
  string fileName, fileExt;
  fileName = string(argv[1]);
  fileExt = fileName.substr(fileName.rfind("."));

  // if image is in DXT load it directly, otherwise compress and load
  if(fileExt.compare(".dxt") == 0)   // DXT
#ifndef USE_DXT
  {
    fprintf(stderr, "\n\nImageviewer was not compiled with DXT option\n");
    exit(0);
  }
#else
  readDXT(fileName, &dxt, width, height);

  else if(loadDXT && dxtFileExists(fileName))
  {
    // replace the extension with .dxt and read the file
    string dxtFileName = string(fileName);
    dxtFileName.resize(fileName.rfind("."));
    dxtFileName += ".dxt";
    readDXT(dxtFileName, &dxt, width, height);
  }
#endif
  else                               // all other image formats
  {
    getRGBA(fileName, &rgba, width, height);
#ifdef USE_DXT
    if (loadDXT)
    {
      rgbaToDXT(fileName, &rgba, &dxt, width, height);
      free(rgba);
      rgba = NULL;
    }
#endif
  }

  // if the user didn't specify the window size, use the image size
  if (window_height == -1 && window_width == -1)
  {
    window_width = width;
    window_height = height;
  }

  // initialize SAIL
  sail sageInf; // sail object
  sailConfig scfg;

  // Search for a configuration file
  char *tmpconf = getenv("SAGE_APP_CONFIG");
  if (tmpconf) {
    SAGE_PRINTLOG("ImageViewer> found SAGE_APP_CONFIG variable: [%s]", tmpconf);
    scfg.init(tmpconf);
  }
  else {
    SAGE_PRINTLOG("ImageViewer> using default imageviewer.conf");
    scfg.init((char*)"imageviewer.conf");
  }

  scfg.setAppName((char*)"imageviewer");

  scfg.resX = width;
  scfg.resY = height;

  // if it hasn't been specified by the config file, use the app-determined size
  if (scfg.winWidth == -1 || scfg.winHeight == -1)
  {
    scfg.winWidth = window_width;
    scfg.winHeight = window_height;
  }

  if (loadDXT)
  {
    scfg.pixFmt = PIXFMT_DXT;
    scfg.rowOrd = BOTTOM_TO_TOP;
  }
  else
  {
    scfg.pixFmt = PIXFMT_888;
    scfg.rowOrd = TOP_TO_BOTTOM;
  }
  sageInf.init(scfg);


#ifdef USE_CAIRO
  // Load image
  image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  unsigned char *img_data = cairo_image_surface_get_data(image);
  memcpy(img_data, rgba, width*height*4);

  // Gradient background
  cairo_pattern_t *pat;
  pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, height);
  cairo_pattern_add_color_stop_rgba (pat, 0, 0.8, 0.8, 0.8, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0.1, 0.1, 0.1, 1);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  // cairo_surface_set_fallback_resolution(surface, 72, 72);
  cr = cairo_create (surface);
  unsigned char *surface_data = cairo_image_surface_get_data(surface);

  // Draw the vector graphics
  draw(cr, pat, surface, image);

  // Copy back pixels
  sageBuffer = (byte*)sageInf.getBuffer();
  memcpy(sageBuffer, surface_data, width*height*4);
  sageInf.swapBuffer();
#else

  // get buffer from SAGE and fill it with dxt data
  sageBuffer = (byte*)sageInf.getBuffer();
  if (loadDXT)
    memcpy(sageBuffer, dxt, width*height*4/8);
  else
    memcpy(sageBuffer, rgba, width*height*3);
  sageInf.swapBuffer();

  sageBuffer = (byte*)sageInf.getBuffer();
  if (loadDXT)
    memcpy(sageBuffer, dxt, width*height*4/8);
  else
    memcpy(sageBuffer, rgba, width*height*3);
  sageInf.swapBuffer();
#endif


  // release the memory
  free(dxt);
  free(rgba);


  // Wait the end
  while (1)
  {
    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0) {
      switch (msg.getCode()) {
      case APP_QUIT:
        sageInf.shutdown();
        exit(1);
        break;
      }
    }

#ifdef USE_CAIRO
    // Draw the vector graphics
    draw(cr, pat, surface, image);

    // Copy back pixels
    sageBuffer = (byte*)sageInf.getBuffer();
    memcpy(sageBuffer, surface_data, width*height*4);
    sageInf.swapBuffer();
#else
    usleep(100000);
    //if (loadDXT)
    //memcpy(sageBuffer, dxt, width*height*4/8);
    //else
    //memcpy(sageBuffer, rgba, width*height*4);
    //sageInf.swapBuffer();
#endif

  }

#ifdef USE_CAIRO
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_pattern_destroy (pat);
  cairo_surface_destroy (image);
#endif

  return 0;
}
