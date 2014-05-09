/*****************************************************************************************
 * imgToDxt: converts a single image to a dxt-compressed image
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
 * Author: Ratko Jagodic
 *
 *****************************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <wand/magick-wand.h>

//#if MagickLibVersion >= 0x645
//#define MagickGetImagePixels MagickGetAuthenticPixels
//#endif

// for dxt compression
#include "libdxt.h"


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



void writeDXT(string fileName, byte *buffer, unsigned int width,
              unsigned int height, unsigned int numBytes)
{
  fprintf(stderr, "\nImgToDxt: Writing DXT to file... %u bytes", numBytes);

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
    fprintf(stderr, "\nImgToDxt: ERROR: Unable to write DXT file. Check dir permissions.");
}



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

  if (width%4 != 0 || height%4 != 0)
  {
    fprintf(stderr, "\nImgToDxt: Image cropped a few pixels to be a multiple of 4 for dxt");
    width -= width%4;
    height -= height%4;
  }

  // flip the image to have the correct orientation for dxt
  MagickFlipImage(wand);

  // get the pixels
  *rgba = (byte*) memalign(16, width*height*4);
  memset(*rgba, 0, width*height*4);
  MagickGetImagePixels(wand, 0, 0, width, height, "RGBA", CharPixel, *rgba);
  DestroyMagickWand(wand);
}


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
    fprintf(stderr, "\nImgToDxt: DXT file for %s doesn't exist yet.", fileName.data());
    return false;
  }
  else
  {
    fclose(dxtFile);
    return true;
  }
}


// -----------------------------------------------------------------------------



int main(int argc,char **argv)
{
  byte *dxt = NULL;
  byte *rgba = NULL;
  unsigned int width, height;  // image size


  // parse command line arguments
  if (argc < 2){
    fprintf(stderr, "\n\nUSAGE: imgToDxt filename\n");
    return 0;
  }

  // check file extension
  string fileName, fileExt;
  fileName = string(argv[1]);
  fileExt = fileName.substr(fileName.rfind("."));

  // if image is in DXT load it directly, otherwise compress and load
  if(fileExt.compare(".dxt") == 0 || dxtFileExists(fileName))   // DXT already exists
    return(0);
  else                               // all other image formats
  {
    getRGBA(fileName, &rgba, width, height);
    rgbaToDXT(fileName, &rgba, &dxt, width, height);
    free(dxt);
    free(rgba);
  }

  return(0);
}
