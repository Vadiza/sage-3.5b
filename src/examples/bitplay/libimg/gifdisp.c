/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
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
 * Direct questions, comments etc about SAGE to http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

/*-
 * gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun Raster image.
 *
 * Copyright (c) 1988 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *                     Patrick J. Naughton
 *                     Sun Microsystems, Inc.
 *                     2550 Garcia Ave, MS 14-40
 *                     Mountain View, CA 94043
 *                     (415) 336-1080
 *
 * Revision History:
 * 27-Jul-88: Updated to use libpixrect to fix 386i byteswapping problems.
 * 11-Apr-88: Converted to C and changed to write Sun rasterfiles.
 * 19-Jan-88: GIFSLOW.PAS posted to comp.graphics by Jim Briebel,
 *            a Turbo Pascal 4.0 program to painfully slowly display
 *            GIF images on an EGA equipped IBM-PC.
 * 13-Jul-90: changed to gif2raw for use by the Geometry Supercomputer Project
 *            by Cary Sandvig
 *
 * Description:
 *   This program takes a Compuserve "Graphics Interchange Format" or "GIF"
 * file as input and writes a file known as a Sun rasterfile.  This datafile
 * can be loaded by the NeWS "readcanvas" operator and is of the same format
 * as the files in /usr/NeWS/smi/...  Under X11R2 there is a program called
 * xraster to display these files.
 *
 * Portability:
 *   To make this program convert to some image format other than Sun's
 * Rasterfile format simply seach for the tag "SS:" in the source and
 * replace these simple mechanisms with the appropriate ones for the
 * other output format.  I have marked all (six) Sun Specific pieces
 * of code with this comment.
 *
 * SS: compile with "cc -o gif2raw -O gif2raw.c"
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "softpic.h"
#include "imginfo.h"

typedef int boolean;
#define True (1)
#define False (0)

#define NEXTSHORT (ptr += 2, ptr[-2] + 0x100 * ptr[-1])
#define NEXTBYTE (*ptr++)
#define IMAGESEP 0x2c
#define EXTENSION '!'
#define TRANSPARENCY 0xF9 /* transparency extension-code */
#define INTERLACEMASK 0x40
#define COLORMAPMASK 0x80

static int BitOffset = 0,   /* Bit Offset of next code */
  XC = 0, YC = 0,   /* Output X and Y coords of current pixel */
  Pass = 0,     /* Used by output routine if interlaced pic */
  OutCount = 0,   /* Decompressor output 'stack count' */
  RWidth, RHeight,    /* screen dimensions */
  Width, Height,    /* image dimensions */
  LeftOfs, TopOfs,    /* image offset */
  BitsPerPixel,   /* Bits per pixel, read from GIF header */
  ColorMapSize,   /* number of colors */
  CodeSize,     /* Code size, read from GIF header */
  InitCodeSize,   /* Starting code size, used during Clear */
  Code,     /* Value returned by ReadCode */
  MaxCode,      /* limiting value for current code size */
  ClearCode,      /* GIF clear code */
  EOFCode,      /* GIF end-of-information code */
  CurCode, OldCode, InCode, /* Decompressor variables */
  FirstFree,      /* First free code, generated per GIF spec */
  FreeCode,     /* Decompressor, next free slot in hash table */
  FinChar,      /* Decompressor variable */
  BitMask,      /* AND mask for data size */
  ReadMask;     /* Code AND mask for current code size */

static boolean Interlace, HasColormap;

static unsigned char *Image;      /* The result array */
static unsigned char *RawGIF;     /* The heap array to hold it, raw */
static unsigned char *Raster;     /* The raster data stream, unblocked */

/* The hash table used by the decompressor */
static int Prefix[4096];
static int Suffix[4096];

/* An output array used by the decompressor */
static int OutCode[1025];

/* The color map, read from the GIF header */
static unsigned char Red[256], Green[256], Blue[256];
static long abgr[256];

static char *id, *id2;

static char *pname;     /* program name (used for error messages) */

static int  ReadCode();
static void AddToPixel(char Index);

static void error( char *s1, char *s2 )
{
  fprintf(stderr, s1, pname, s2);
  exit(1);
}


static void gif_init()
{
  BitOffset = 0;
  XC = 0;
  YC = 0;
  Pass = 0;
  OutCount = 0;
  id = "GIF87a";
  id2 = "GIF89a";
}

IMG *gifmakedisp( char *inname )
{
  FILE *fpin;
  int filesize;
  register unsigned char ch, ch1;
  register unsigned char *ptr, *ptr1;
  IMG *out;
  int transindex = -1;
  int i, j;

  gif_init();
  setbuf(stderr, NULL);

  fpin = fopen(inname, "rb");
  /* find the size of the file */

  fseek(fpin, 0L, 2);
  filesize = ftell(fpin);
  fseek(fpin, 0L, 0);

  if (!(ptr = RawGIF = (unsigned char *) malloc(filesize)))
    error("%s: not enough memory to read gif file.\n", NULL);

  if (!(Raster = (unsigned char *) malloc(filesize)))
    error("%s: not enough memory to read gif file.\n", NULL);

  fread(ptr, filesize, 1, fpin);

  if (memcmp(ptr, id, 6) && memcmp(ptr, id2, 6))
    error("%s: %s is not a GIF file.\n", inname);
  ptr += 6;

  /* Get variables from the GIF screen descriptor */

  RWidth = NEXTSHORT;   /* screen dimensions... not used. */
  RHeight = NEXTSHORT;

  ch = NEXTBYTE;
  HasColormap = ((ch & COLORMAPMASK) ? True : False);

  BitsPerPixel = (ch & 7) + 1;
  ColorMapSize = 1 << BitsPerPixel;
  BitMask = ColorMapSize - 1;

  ch = NEXTBYTE;    /* background color... not used. */

  (void) NEXTBYTE;    /* what's this? */

  /* Read in global colormap. */

  if (HasColormap) {
    /*
      fprintf(stderr, "%s is %d bits per pixel, (%d colors).\n",
      inname, BitsPerPixel, ColorMapSize);
    */
    for (i = 0; i < ColorMapSize; i++) {
      Red[i] = NEXTBYTE;
      Green[i] = NEXTBYTE;
      Blue[i] = NEXTBYTE;
      abgr[i] = 0xFF000000 | (Blue[i]<<16) | (Green[i]<<8) | Red[i];
    }

  }
  else error("gifdisp: %s does not have a colormap.\n", inname);


  /* Skip any GIF89 extensions */
  while((i = NEXTBYTE) == EXTENSION) {
    int extno = NEXTBYTE & 0xFF;
    int len;
    unsigned char block[256];

    /* Skip <lengthbyte><data> segments until we see lengthbyte==0. */
    while((len = NEXTBYTE) != 0) {
      for(i = 0; i < len; i++)
        block[i] = NEXTBYTE;
    }
    switch(extno) {
    case 0xF9:    /* Transparency */
      if(block[0] & 0x01) {
        transindex = block[3];
        if(transindex >= 0 && transindex < ColorMapSize)
          abgr[transindex] &= ~0xFF000000;
      }
      break;
    }
  }

  /* Check for image separator */
  if (i != IMAGESEP)
    fprintf(stderr, "gifdisp: %s is a corrupt GIF file.\n", inname);

  /* Now read in values from the image descriptor */

  LeftOfs = NEXTSHORT;
  TopOfs = NEXTSHORT;
  Width = NEXTSHORT;
  Height = NEXTSHORT;
  Interlace = ((NEXTBYTE & INTERLACEMASK) ? True : False);

  /*
    fprintf(stderr, "Reading a %d by %d %sinterlaced image...",
    Width, Height, (Interlace) ? "" : "non-");
  */


  /* Note that I ignore the possible existence of a local color map.
   * I'm told there aren't many files around that use them, and the spec
   * says it's defined for future use.  This could lead to an error
   * reading some files.
   */

  /* Start reading the raster data. First we get the intial code size
   * and compute decompressor constant values, based on this code size.
   */

  CodeSize = NEXTBYTE;
  ClearCode = (1 << CodeSize);
  EOFCode = ClearCode + 1;
  FreeCode = FirstFree = ClearCode + 2;

  /* The GIF spec has it that the code size is the code size used to
   * compute the above values is the code size given in the file, but the
   * code size used in compression/decompression is the code size given in
   * the file plus one. (thus the ++).
   */

  CodeSize++;
  InitCodeSize = CodeSize;
  MaxCode = (1 << CodeSize);
  ReadMask = MaxCode - 1;

  /* Read the raster data.  Here we just transpose it from the GIF array
   * to the Raster array, turning it from a series of blocks into one long
   * data stream, which makes life much easier for ReadCode().
   */

  ptr1 = Raster;
  do {
    ch = ch1 = NEXTBYTE;
    while (ch--) *ptr1++ = NEXTBYTE;
  } while(ch1);

  free(RawGIF);   /* We're done with the raw data now... */

  /*
    fprintf(stderr, "done.\n");
    fprintf(stderr, "Decompressing...");
  */

  Image = (unsigned char *)calloc((Width*Height), sizeof(char));


  /* Decompress the file, continuing until you see the GIF EOF code.
   * One obvious enhancement is to add checking for corrupt files here.
   */

  Code = ReadCode();
  while (Code != EOFCode) {

    /* Clear code sets everything back to its initial value, then reads the
     * immediately subsequent code as uncompressed data.
     */

    if (Code == ClearCode) {
      CodeSize = InitCodeSize;
      MaxCode = (1 << CodeSize);
      ReadMask = MaxCode - 1;
      FreeCode = FirstFree;
      CurCode = OldCode = Code = ReadCode();
      FinChar = CurCode & BitMask;
      AddToPixel(FinChar);
    }
    else {

      /* If not a clear code, then must be data: save same as CurCode and InCode */

      CurCode = InCode = Code;

      /* If greater or equal to FreeCode, not in the hash table yet;
       * repeat the last character decoded
       */

      if (CurCode >= FreeCode) {
        CurCode = OldCode;
        OutCode[OutCount++] = FinChar;
      }

      /* Unless this code is raw data, pursue the chain pointed to by CurCode
       * through the hash table to its end; each code in the chain puts its
       * associated output code on the output queue.
       */

      while (CurCode > BitMask) {
        OutCode[OutCount++] = Suffix[CurCode];
        CurCode = Prefix[CurCode];
      }

      /* The last code in the chain is treated as raw data. */

      FinChar = CurCode & BitMask;
      OutCode[OutCount++] = FinChar;

      /* Now we put the data out to the Output routine.
       * It's been stacked LIFO, so deal with it that way...
       */

      for (i = OutCount - 1; i >= 0; i--)
        AddToPixel(OutCode[i]);
      OutCount = 0;

      /* Build the hash table on-the-fly. No table is stored in the file. */

      Prefix[FreeCode] = OldCode;
      Suffix[FreeCode] = FinChar;
      OldCode = InCode;

      /* Point to the next slot in the table.  If we exceed the current
       * MaxCode value, increment the code size unless it's already 12.  If it
       * is, do nothing: the next code decompressed better be CLEAR
       */

      FreeCode++;
      if (FreeCode >= MaxCode) {
        if (CodeSize < 12) {
          CodeSize++;
          MaxCode *= 2;
          ReadMask = (1 << CodeSize) - 1;
        }
      }
    }
    Code = ReadCode();
  }

  free(Raster);

  /*
    fprintf(stderr, "done.\n");

    fprintf(stderr, "outputting image.\n");
  */

  out = (IMG *)malloc(sizeof(IMG));
  out->type = IT_LONG;
  out->rowbytes = Width * sizeof(long);
  out->xsize = Width;
  out->ysize = Height;
  out->data = (unsigned char *)malloc(4*(Width*Height)*sizeof(char));

  for(j = 0; j<Height; j++)
    for (i=0; i<Width; i++) {
      *(long *)&out->data[4*(((Height - j - 1)*Width)+i)] =
        abgr[Image[j*Width + i]];
    }

  free(Image);
  fclose(fpin);
  return (out);

  /*
    fprintf(stderr, "done.\n");
  */
}


/* Fetch the next code from the raster data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location in the Raster array as a BIT Offset.  We compute
 * the byte Offset into the raster array by dividing this by 8, pick up
 * three bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it.
 */
static int ReadCode()
{
  int RawCode, ByteOffset;

  ByteOffset = BitOffset / 8;
  RawCode = Raster[ByteOffset] + (0x100 * Raster[ByteOffset + 1]);
  if (CodeSize >= 8)
    RawCode += (0x10000 * Raster[ByteOffset + 2]);
  RawCode >>= (BitOffset % 8);
  BitOffset += CodeSize;
  return(RawCode & ReadMask);
}

static void AddToPixel(char Index)
{
  if((unsigned int)XC >= Width || (unsigned int)YC >= Height) {
    fprintf(stderr, "[BP] ");
  } else {
    *(Image + YC * Width + XC) = Index;
  }

  /* Update the X-coordinate, and if it overflows, update the Y-coordinate */

  if (++XC == Width) {

    /* If a non-interlaced picture, just increment YC to the next scan line.
     * If it's interlaced, deal with the interlace as described in the GIF
     * spec.  Put the decoded scan line out to the screen if we haven't gone
     * past the bottom of it
     */

    XC = 0;
    if (!Interlace) YC++;
    else {
      switch (Pass) {
      case 0:
        YC += 8;
        if (YC >= Height) {
          Pass++;
          YC = 4;
        }
        break;
      case 1:
        YC += 8;
        if (YC >= Height) {
          Pass++;
          YC = 2;
        }
        break;
      case 2:
        YC += 4;
        if (YC >= Height) {
          Pass++;
          YC = 1;
        }
        break;
      case 3:
        YC += 2;
        if(YC >= Height) {
          fprintf(stderr, "[BG] ");
          YC = Height-1;  /* eh?? */
        }
        break;
      default:
        break;
      }
    }
  }
}
