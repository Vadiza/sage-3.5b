/*****************************************************************************************
 * imageflip: loads an image sequence and sends it to SAGE for display
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
#include "appWidgets.h"
#include "misc.h"
#include "util.h"

typedef unsigned char byte;
#define memalign(x,y) malloc((y))


// Set of filenames
std::set<std::string> Names;
// Iterator for the names
std::set<std::string>::iterator iter;
int current = 1;
// image size
unsigned int width, height;
// Number of images to ne processed
int Limit;

int isPlaying = 0;

// SAGE object
sail sageInf; // sail object
byte *rgbBuffer = NULL;

// widgets
label *pageNum= new label;
//char wlabel[256];



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

void getRGB(string fileName, byte *rgb, unsigned int &w, unsigned int &h)
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
  w = MagickGetImageWidth(wand);
  h = MagickGetImageHeight(wand);

  // get the pixels
  //MagickGetImagePixels(wand, 0, 0, w, h, "RGB", CharPixel, rgb);
  MagickExportImagePixels(wand, 0, 0, w, h, "RGB", CharPixel, rgb);
  DestroyMagickWand(wand);
}

// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

void getDimensions(string fileName, unsigned int &w, unsigned int &h)
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
  w = MagickGetImageWidth(wand);
  h = MagickGetImageHeight(wand);

  DestroyMagickWand(wand);
}

// -----------------------------------------------------------------------------

void swapBuffer()
{
  byte* sageBuffer;
  // get buffer from SAGE and fill it with data
  sageBuffer = (byte*)sageInf.getBuffer();
  memcpy(sageBuffer, rgbBuffer, width*height*3);
  sageInf.swapBuffer();
}

// -----------------------------------------------------------------------------
// --  SAGE UI Callbacks -------------------------------------------------------
// -----------------------------------------------------------------------------

void onHomeBtn(int eventId, button *btnObj)
{
  isPlaying = 0;
  // Rewind
  iter = Names.begin();
  current = 1;

  // Get some pixels
  cerr << "Getting " << *iter << "\n";  // endl;
  getRGB(*iter, rgbBuffer, width, height);

  // Send the picture
  swapBuffer();

  // Update UI
  //memset(wlabel, 0, 256);
  //sprintf(wlabel, "Image %d of %d", current, (int)Names.size() );
  //pageNum->setLabel(wlabel, 20);
}


void onEndBtn(int eventId, button *btnObj)
{
  isPlaying = 0;
  // Rewind
  iter = Names.end();
  iter--;
  current = Names.size();

  // Get some pixels
  cerr << "Getting " << *iter << "\n";  // endl;
  getRGB(*iter, rgbBuffer, width, height);

  // Send the picture
  swapBuffer();

  // Update UI
  //memset(wlabel, 0, 256);
  //sprintf(wlabel, "Image %d of %d", current, (int)Names.size() );
  //pageNum->setLabel(wlabel, 20);
}

void onPrevBtn(int eventId, button *btnObj)
{
  isPlaying = 0;
  fprintf(stderr, "onPrevBtn: current %d\n", current);

  // Next step
  iter--;

  if (current == 1) {
    // Rewind
    iter = Names.end();
    iter--;
    current = Names.size();
  }
  else {
    current--;
  }
  // Get some pixels
  cerr << "Getting " << *iter << "\n";  // endl;
  getRGB(*iter, rgbBuffer, width, height);

  // Send the picture
  swapBuffer();

  // Update UI
  //memset(wlabel, 0, 256);
  //sprintf(wlabel, "Image %d of %d", current, (int)Names.size() );
  //pageNum->setLabel(wlabel, 20);
}

void onNextBtn(int eventId, button *btnObj)
{
  isPlaying = 0;
  fprintf(stderr, "onNextBtn: current %d\n", current);

  // Next step
  iter++;

  if (current == (int)Names.size()) {
    // Rewind
    iter = Names.begin();
    current = 1;
  }
  else {
    current++;
  }
  // Get some pixels
  cerr << "Getting " << *iter << "\n";  // endl;
  getRGB(*iter, rgbBuffer, width, height);

  // Send the picture
  swapBuffer();

  // Update UI
  //memset(wlabel, 0, 256);
  //sprintf(wlabel, "Image %d of %d", current, (int)Names.size() );
  //pageNum->setLabel(wlabel, 20);
}

void onPlayBtn(int eventId, button *btnObj)
{
  isPlaying = 1;
}

void onPauseBtn(int eventId, button *btnObj)
{
  isPlaying = 0;
}

// -----------------------------------------------------------------------------
// --  SAGE User Interface  ----------------------------------------------------
// -----------------------------------------------------------------------------

void makeWidgets()
{
  float mult = 2.0;
  int bs = 85;

  //memset(wlabel, 0, 256);
  //sprintf(wlabel, "Image %d of %d", 1, (int)Names.size() );
  //pageNum->setLabel(wlabel, 20);
  //pageNum->setSize(bs, bs);
  //pageNum->alignLabel(CENTER);
  //pageNum->alignY(CENTER);
  //pageNum->setBackgroundColor(255,255,255);
  //pageNum->setFontColor(0,0,0);

  thumbnail *homeBtn = new thumbnail;
  homeBtn->setUpImage("images/skip-backward.png");
  homeBtn->setSize(bs, bs);
  homeBtn->setScaleMultiplier(mult);
  homeBtn->onUp(&onHomeBtn);

  thumbnail *endBtn = new thumbnail;
  endBtn->setUpImage("images/skip-forward.png");
  endBtn->setSize(bs, bs);
  endBtn->setScaleMultiplier(mult);
  endBtn->alignX(LEFT, 25);
  endBtn->onUp(&onEndBtn);

  thumbnail *nextBtn = new thumbnail;
  nextBtn->setUpImage("images/fast-forward.png");
  nextBtn->setSize(bs, bs);
  nextBtn->setScaleMultiplier(mult);
  nextBtn->onUp(&onNextBtn);

  thumbnail *prevBtn = new thumbnail;
  prevBtn->setUpImage("images/rewind.png");
  prevBtn->setSize(bs, bs);
  prevBtn->setScaleMultiplier(mult);
  prevBtn->alignX(LEFT, 25);
  prevBtn->onUp(&onPrevBtn);

  //thumbnail *playBtn = new thumbnail;
  button *playBtn = new button;
  playBtn->setToggle(true);
  playBtn->setUpImage("images/play2.png");
  playBtn->setDownImage("images/pause2.png");
  playBtn->alignX(LEFT, 50);
  playBtn->setSize(bs, bs);
  //playBtn->setScaleMultiplier(mult);
  playBtn->onUp(&onPauseBtn);
  playBtn->onDown(&onPlayBtn);

  sizer *s = new sizer(HORIZONTAL);
  s->align(CENTER, BOTTOM);
  s->addChild(homeBtn);
  s->addChild(prevBtn);
  s->addChild(playBtn);
  s->addChild(pageNum);
  s->addChild(nextBtn);
  s->addChild(endBtn);

  //sizer *t = new sizer(HORIZONTAL);
  //t->addChild(playBtn);
  //t->addChild(pauseBtn);

  panel *p = new panel(255,255,255);
  p->setTransparency(100);
  p->align(CENTER, BOTTOM_OUTSIDE);
  p->addChild(s);
  sageInf.addWidget(p);

  //panel *q = new panel(255,255,255);
  //q->setTransparency(100);
  //q->align(CENTER, TOP);
  //q->addChild(t);
  //sageInf.addWidget(q);
}


// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

void RefreshList(string dir)
{
  DIR *dirh;
  struct dirent *dirp;
  char pathname[1024];
  char linkname[1024];
  struct stat statbuf;

  const char *dn = dir.c_str();


  if ((dirh = opendir(dn)) == NULL)
  {
    perror("opendir");
    exit(1);
  }


  for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
  {
    if (strcmp(".",dirp->d_name) == 0 || strcmp("..",dirp->d_name) == 0)
    {
      continue;
    }

    sprintf(pathname,"%s/%s",dn,dirp->d_name);

    if (lstat(pathname,&statbuf) == -1)                /* see man stat */
    {
      perror("stat");
      continue;
    }

    if (S_ISREG(statbuf.st_mode))
    {
      //printf("%s is a regular file\n",pathname);
      string pn = string(pathname);
      string fileExt = pn.substr(pn.rfind("."));
      if ( (fileExt.compare(".JPG") == 0)  || (fileExt.compare(".jpg") == 0)
           || (fileExt.compare(".PNG") == 0)  || (fileExt.compare(".png") == 0)
           || (fileExt.compare(".TIFF") == 0) || (fileExt.compare(".tiff") == 0)
           || (fileExt.compare(".TIF")  == 0) || (fileExt.compare(".tif") == 0)
           )
        if (Names.insert(pathname).second)
        {
          cout << pathname << " added successfully" << endl;
        }
        else
        {
          //cout << pathname << " rejected" << endl;
        }

    }

    if (S_ISDIR(statbuf.st_mode))
    {
      //printf("%s is a directory\n",pathname);
    }

    if (S_ISLNK(statbuf.st_mode))
    {
      bzero(linkname,1024);                         /* clear string */
      readlink(pathname,linkname,1024);
      //printf("%s is a link to %s\n",pathname,linkname);
    }

    //printf("The mode of %s is %o\n\n",pathname,statbuf.st_mode & 07777);
  }

  closedir(dirh);

  // Will flip through the whole list
  Limit = Names.size();

}

// -----------------------------------------------------------------------------



int main(int argc,char **argv)
{
  unsigned int window_width=0, window_height=0;  // sage window size


  // parse command line arguments
  if (argc < 2){
    fprintf(stderr, "\n\nUSAGE: imageflip directory [fps]\n");
    return 0;
  }

  // check file extension
  string dirName;
  dirName = string(argv[1]);

  float fps = 1.0f;
  if (argc==3)
    fps = atof(argv[2]);


  // Populate the filename list
  RefreshList(dirName);

  cout << Names.size() << " elements in the set" << endl;


  // List the names
  int k =0;
  for ( iter = Names.begin() ; k < Limit ; iter++ ) {k++;}
  for ( ; iter != Names.end(); iter++ )
    cout << " " << *iter << endl;

  // last element
  cout << "Last element: " << *(Names.rbegin()) << endl;

  string firstname = *( Names.begin() );

  // if the user didn't specify the window size, use the image size
  if (window_height == 0 && window_width == 0)
  {
    // Get the dimensions of the first file
    getDimensions(firstname, width, height);
    window_width = width;
    window_height = height;
    cout << "Dimensions of image series: " << window_width << "x" << window_height << endl;
  }

  // initialize SAIL
  sailConfig scfg;
  scfg.init("imageflip.conf");
  scfg.setAppName("imageflip");

  scfg.resX = width;
  scfg.resY = height;

  if (scfg.winWidth == -1 || scfg.winHeight == -1)
  {
    scfg.winWidth = window_width;
    scfg.winHeight = window_height;
  }

  scfg.pixFmt = PIXFMT_888;
  scfg.rowOrd = TOP_TO_BOTTOM;

  rgbBuffer = (byte*)malloc(width*height*3);
  memset(rgbBuffer, 0, width*height*3);

  makeWidgets();

  // Initialization
  sageInf.init(scfg);


  // Get some pixels
  current = 1;
  getRGB(firstname, rgbBuffer, width, height);
  swapBuffer();

  // Rewind
  iter = Names.begin();

  // Wait the end
  sageMessage msg;
  double lt = 0.0;
  while (1)
  {
    // Process SAGE messages
    if (sageInf.checkMsg(msg, false) > 0) {
      switch (msg.getCode()) {
      case APP_QUIT:
        cout << "\nDone\n\n";
        sageInf.shutdown();
        exit(1);
        break;
      }
    }
    // Automatic playback
    if (isPlaying) {
      if ( ( aTime() - lt ) > (1.0f/fps) ) {

        // Next step
        iter++;

        if (current == (int)Names.size()) {
          // Rewind
          iter = Names.begin();
          current = 1;
        }
        else {
          current++;
        }
        // Get some pixels
        cerr << "Getting " << *iter << "\r";  // endl;
        getRGB(*iter, rgbBuffer, width, height);
        // Send the picture
        swapBuffer();

        lt = aTime();
      }
    }

  }

  return 0;
}
