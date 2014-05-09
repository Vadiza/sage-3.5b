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

/*
 * cc -o txspeed{,.c} -L/usr/X11R6/lib -lglut -lGL -lX11
 */
#include <X11/Xlib.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#ifndef MAXTX
# define MAXTX 128
#endif

#ifndef TXROOM
# define TXROOM  64
#endif

unsigned char txbuf[ TXROOM * MAXTX * MAXTX * 4 ];
int width = MAXTX;
int height = MAXTX;
int internalfmt = GL_RGB;
int fmt = GL_RGB;
int type = GL_UNSIGNED_BYTE;

int newtex = 1;

int justone = 0;
int justload = 0;

#define TXSLOTS  1024

int winx, winy;
int nxtx, nytx, ntx;
GLuint txs[TXSLOTS];

void tick( int start );  /* tick(1) to start, tick(0) to continue, tick(-1) to clean up */

void reshape( int xsize, int ysize )
{
  int i;

  winx = xsize;  winy = ysize;

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho( 0, xsize, 0, ysize, -1, 1 );
  glViewport( 0, 0, xsize, ysize );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  glEnable( GL_TEXTURE_2D );

  /* How many textures do we need to fill the screen? */
  nxtx = winx / MAXTX;  if(nxtx == 0) nxtx = 1;
  nytx = winy / MAXTX;  if(nytx == 0) nytx = 1;
  if(nxtx*nytx > TXSLOTS) nytx = TXSLOTS / nxtx;

  /* ensure that that many texture-objects exist */
  for(i = 0; i < nxtx*nytx; i++) {
    if(txs[i] == 0) {
      glGenTextures( nxtx*nytx-i, &txs[i] );
      newtex = 1;
      break;
    }
  }

  for(i = 0; i < sizeof(txbuf); i+=4) {
    int v = i ^ (i/MAXTX) ^ (i/MAXTX/MAXTX);
    txbuf[i] = v;
    txbuf[i+1] = (v<<1) | (v>>7);
    txbuf[i+2] = (v<<3) | (v>>5);
    txbuf[i+3] = (v<<5) | (v>>3);
  }

  tick(1);
  printf("reshape %dx%d  %dx%d\n", xsize, ysize, nxtx, nytx);
}

int quitnow = 0;

void idler()
{
  if(quitnow) {
    tick(-1);
    exit(1);
  }
  glutPostRedisplay();
}

void catch(int sig) {
  quitnow = 1;
  }

int ntimes = 50;

void tick( int start )
{
  static struct timeval then;
  struct timeval now;
  static int count;
  double dt;

  if(start>0) {
    count = 0;
    return;
  }

  count++;

  if(count == 2) {
    gettimeofday( &then, NULL );
  } else if(count == ntimes+2 || start<0) {
    gettimeofday( &now, NULL );
    dt = (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec);
    if(count > 2)
      printf(" %g\n", 1000*dt/(count-2), now.tv_sec, then.tv_sec);
    else printf("tick done: count %d dt %g\n", count, dt);
    count = 0;
  }
}

void kb( unsigned char key, int x, int y )
{
  tick( -1 );
  exit(0);
}

void redraw()
{
  int i, j, k, start, e;
  int rowlength = 0;

  glClearColor( .3, newtex, 0, 1 );
  glClear( GL_COLOR_BUFFER_BIT );

  glEnable( GL_TEXTURE_2D );

  /* Load textures and draw */
  for(i = k = 0; i < nytx; i++) {
    for(j = 0; j < nxtx; j++, k++) {

      int x0 = j*MAXTX;
      int x1 = (j+1)*MAXTX;
      int y0 = i*MAXTX;
      int y1 = (i+1)*MAXTX;

      if(x1 > winx) x1 = winx;
      if(y1 > winy) y1 = winy;

      if(!justone) glBindTexture( GL_TEXTURE_2D, txs[k] );
      if(rowlength == 0)
        start = (int) ( (((TXROOM-1)*MAXTX*MAXTX)*4) * drand48() );
      else
        start = (int) ((MAXTX*MAXTX*4) * drand48());

      if(newtex) {

        glPixelStorei( GL_UNPACK_ROW_LENGTH, rowlength );
        /* glEnable( GL_TEXTURE_2D ); */
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

        glTexImage2D( GL_TEXTURE_2D, 0, internalfmt, width, height, 0, fmt, type,
                      &txbuf[(start / 16) * 16] );
      } else {
        glTexSubImage2D( GL_TEXTURE_2D, 0,  0,0,  width,height, fmt, type,
                         &txbuf[(start / 16) * 16] );
      }

      if(!justload) {
        glBegin( GL_QUADS );

        glTexCoord2f( 0, 0 );
        glVertex3i( x0, y0, 0 );

        glTexCoord2f( 0, 1 );
        glVertex3i( x0, y1, 0 );

        glTexCoord2f( 1, 1 );
        glVertex3i( x1, y1, 0 );

        glTexCoord2f( 1, 0 );
        glVertex3i( x1, y0, 0 );
        glEnd();
      }

      e = glGetError();
      if(e != 0)
        fprintf(stderr, "E%d ", e);
    }
  }

  glutSwapBuffers();

  newtex = 0;

  tick(0);

}

main(int argc, char *argv[])
{
  signal(SIGINT, catch);
  justone = (getenv("JUSTONE") != NULL);
  justload = (getenv("JUSTLOAD") != NULL);

  glutInit( &argc, argv );
  glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

  glutCreateWindow( "eep" );
  glutFullScreen();

  glutIdleFunc( idler );
  glutReshapeFunc( reshape );
  glutDisplayFunc( redraw );
  glutKeyboardFunc( kb );

  glutMainLoop();
}
