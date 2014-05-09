
/* Copyright (c) Mark J. Kilgard, 1994. */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <sys\timeb.h>
int gettimeofday(struct timeval *tp, void *tzp);
#else
#include <sys/time.h>
#endif

#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include "atlantis.h"

#include <mpi.h>
int rank   = 0;  // node rank
int nprocs = 0;  // number of nodes
int dimX, dimY, Xidx, Yidx;

// headers for SAGE
#include "sail.h"
#include "misc.h"
int winWidth, winHeight;
GLubyte *rgbBuffer = 0;
sail sageInf; // sail object

#if defined(WIN32)
int
gettimeofday(struct timeval *tp, void *tzp)
{
  struct _timeb t;

  _ftime(&t);
  tp->tv_sec = t.time;
  tp->tv_usec = t.millitm * 1000;
  return 0;
}
#endif

fishRec sharks[NUM_SHARKS];
fishRec momWhale;
fishRec babyWhale;
fishRec dolph;

GLboolean moving;
int screenshot = 0;
float t1,t2;
struct timeval tv_start;

double getTime()
{
  struct timeval tv;

  gettimeofday(&tv,0);
  return (double)(tv.tv_sec - tv_start.tv_sec) + (double)(tv.tv_usec - tv_start.tv_usec) / 1000000.0;
}
/*
  float abs(float f)
  {
  if (f >= 0.0f) return f;
  else return -f;
  }
*/
void
InitFishs(void)
{
  int i;

  for (i = 0; i < NUM_SHARKS; i++) {
    sharks[i].x = 15000.0 + rand() % 6000;
    sharks[i].y = (rand() % 20000) - 10000;
    sharks[i].z = (rand() % 6000) - 5000;
    sharks[i].psi = rand() % 360 - 180.0;
    sharks[i].v = 1.0;
  }

  dolph.x = 30000.0;
  dolph.y = 0.0;
  dolph.z = 6000.0;
  dolph.psi = 90.0;
  dolph.theta = 0.0;
  dolph.v = 3.0;

  momWhale.x = 70000.0;
  momWhale.y = 0.0;
  momWhale.z = 0.0;
  momWhale.psi = 90.0;
  momWhale.theta = 0.0;
  momWhale.v = 3.0;

  babyWhale.x = 60000.0;
  babyWhale.y = -2000.0;
  babyWhale.z = -2000.0;
  babyWhale.psi = 90.0;
  babyWhale.theta = 0.0;
  babyWhale.v = 3.0;
}

void
Init(void)
{
  static float ambient[] =
    {0.1, 0.1, 0.1, 1.0};
  static float diffuse[] =
    {1.0, 1.0, 1.0, 1.0};
  static float position[] =
    {0.0, 1.0, 0.0, 0.0};
  static float mat_shininess[] =
    {90.0};
  static float mat_specular[] =
    {0.8, 0.8, 0.8, 1.0};
  static float mat_diffuse[] =
    {0.46, 0.66, 0.795, 1.0};
  static float mat_ambient[] =
    {0.0, 0.1, 0.2, 1.0};
  static float lmodel_ambient[] =
    {0.4, 0.4, 0.4, 1.0};
  static float lmodel_localviewer[] =
    {1.0};

  glFrontFace(GL_CW);

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);

  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

  InitFishs();

  glClearColor(0.0, 0.5, 0.9, 0.0);
}

void
Reshape(int width, int height)
{
  winWidth = width;
  winHeight = height;

  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(4.0, 2.0, 10000.0, 400000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

}

void
Animate(void)
{
  int i;

  for (i = 0; i < NUM_SHARKS; i++) {
    SharkPilot(&sharks[i]);
    SharkMiss(i);
  }
  WhalePilot(&dolph);
  dolph.phi++;
  glutPostRedisplay();
  WhalePilot(&momWhale);
  momWhale.phi++;
  WhalePilot(&babyWhale);
  babyWhale.phi++;
}

/* ARGSUSED1 */
void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;
  switch (key)
  {
  case 27:           /* Esc will quit */
    exit(1);
    MPI_Finalize();
    break;
  case 's':
    screenshot++;
    glutPostRedisplay( );
    break;
  case ' ':          /* space will advance frame */
    if (!moving) {
      Animate();
    }
  }
}


void
save_frame( void )
{
  /*  GLint viewport[4];
      int x, y, width, height;
      void *pixels;
      FILE *f;

      glGetIntegerv( GL_VIEWPORT, viewport );

      x = viewport[0];
      y = viewport[1];
      width = viewport[2];
      height = viewport[3];

      printf( "saving %dx%d screenshot\n", width, height );

      pixels = malloc( width * height * 3 );
      assert( pixels );
      memset( pixels, 0, width * height * 3 );

      glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels );


      f = fopen( "screenshot.ppm", "wb" );
      fprintf( f, "P6\n%d %d\n255\n", width, height );
      for ( y = height - 1; y >= 0; y-- )
      fwrite( (char *) pixels + y * width * 3, width, 3, f );
      fclose( f );

      free( pixels );

      printf( "done\n" );*/
}

void
print_performance( void )
{
  /*  static int first = 1;
      = 0
      static int start, num_frames;
      int current;

      if ( first )
      {
      start = glutGet( GLUT_ELAPSED_TIME );
      num_frames = 0;
      first = 0;
      }

      num_frames++;
      current = glutGet( GLUT_ELAPSED_TIME );

      if ( current - start > 1000 )
      {
      double elapsed = 1e-3 * (double) ( current - start );
      double rate = (double) num_frames / elapsed;
      printf( "%5.1f fps\n", rate );

      num_frames = 0;
      start = current;
      }*/
}

void
Display( void )
{
  float ambient[] = {0.1, 0.1, 0.1, 1.0};
  float diffuse[] = {1.0, 1.0, 1.0, 1.0};
  float position[] = {0.0, 1.0, 0.0, 0.0};
  float mat_shininess[] = {90.0};
  float mat_specular[] = {0.8, 0.8, 0.8, 1.0};
  float mat_diffuse[] = {0.46, 0.66, 0.795, 1.0};
  float mat_ambient[] = {0.0, 0.1, 0.2, 1.0};
  float lmodel_ambient[] = {0.4, 0.4, 0.4, 1.0};
  float lmodel_localviewer[] = {0.0};

  int i;

  glFrontFace(GL_CCW);

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
  glClearColor(0.0, 0.5, 0.9, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();


  //gluPerspective(40.0, 2.0, 10000.0, 400000.0);

  GLfloat fov = 40.0;
  GLfloat aspect = 2.0;
  GLfloat near = 10000.0;
  GLfloat far = 400000.0;
  GLfloat range = near*tan(M_PI * (fov/2) / 180.0);

#if 0
  // standard view
  glFrustum(-range*aspect,range*aspect,-range,range,near,far);
#else
  // split view
  GLfloat Xstep = 2.0*range*aspect / (float)dimX;
  GLfloat Ystep = 2.0*range / (float)dimY;

  glFrustum(-range*aspect + Xidx*Xstep, -range*aspect + (Xidx+1)*Xstep,
            -range + Yidx*Ystep, -range + (Yidx+1)*Ystep, near, far);
#endif

  glMatrixMode(GL_MODELVIEW);

  glLoadIdentity();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (i = 0; i < NUM_SHARKS; i++)
  {
    glPushMatrix();
    FishTransform(&sharks[i]);
    DrawShark(&sharks[i]);
    glPopMatrix();
  }

  glPushMatrix();
  FishTransform(&dolph);
  DrawDolphin(&dolph);
  glPopMatrix();

  glPushMatrix();
  FishTransform(&momWhale);
  DrawWhale(&momWhale);
  glPopMatrix();

  glPushMatrix();
  FishTransform(&babyWhale);
  glScalef(0.45, 0.45, 0.3);
  DrawWhale(&babyWhale);
  glPopMatrix();
  glFinish();


  if (winWidth > 0) {
    glReadPixels(0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  sageInf.swapBuffer();
  rgbBuffer = (GLubyte *)sageInf.getBuffer();

  glutSwapBuffers( );
}

void
Visible(int state)
{
  if (state == GLUT_VISIBLE) {
    if (moving)
      glutIdleFunc(Animate);
  } else {
    if (moving)
      glutIdleFunc(NULL);
  }
}

void
menuSelect(int value)
{
  switch (value)
  {
  case 1:
    moving = GL_TRUE;
    glutIdleFunc(Animate);
    break;
  case 2:
    moving = GL_FALSE;;
    glutIdleFunc(NULL);
    break;
  case 3:
    exit(0);
    break;
  }
}

int
main(int argc, char **argv)
{
  // MPI Initialization
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Barrier(MPI_COMM_WORLD);

  int resX = 1024, resY = 1024;
  if (argc > 2) {
    resX = atoi(argv[1]);
    resY = atoi(argv[2]);
  }

  dimX = nprocs-1;
  dimY = 1;

  if (argc > 4) {
    dimX = atoi(argv[3]);
    dimY = atoi(argv[4]);
  }

  char procname[MPI_MAX_PROCESSOR_NAME];
  int lenproc;
  MPI_Get_processor_name(procname, &lenproc);
  fprintf(stderr, "Processor %2d is machine [%s]\n", rank, procname);

  // control master
  if (rank == 0) {
    sailConfig scfg;
    scfg.init("atlantis-mpi.conf");
    scfg.setAppName("atlantis-mpi");
    scfg.rank = rank;
    scfg.nodeNum = nprocs-1;
    scfg.master = true;
    scfg.rendering = false;

    sageInf.init(scfg);

    std::cerr << "sail initialized " << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);

    std::cerr << "entering message loop" << std::endl;
    while(1) {
      sageMessage msg;
      if (sageInf.checkMsg(msg, false) > 0) {
        switch (msg.getCode()) {
        case APP_QUIT : {
          //sageInf.shutdown();
          int err;
          MPI_Abort(MPI_COMM_WORLD, err);
          // finalize
          MPI_Finalize();
          exit(0);
          break;
        }
        }
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }
  }
  // rendering slaves
  else {
    Xidx = (rank-1)%dimX;
    Yidx = (rank-1)/dimX;

    printf("Index x=%d y=%d\n", Xidx, Yidx);

    sageRect atlantisImageMap;
    atlantisImageMap.left = ((float)Xidx) / ((float)dimX);
    atlantisImageMap.right = ((float)Xidx+1.0) / ((float)dimX);
    atlantisImageMap.bottom = ((float)Yidx) / ((float)dimY);
    atlantisImageMap.top = ((float)Yidx+1.0) / ((float)dimY);;

    sailConfig scfg;
    scfg.init("atlantis-mpi.conf");
    scfg.setAppName("atlantis-mpi");
    scfg.rank = rank;
    scfg.resX = resX;
    scfg.resY = resY;
    scfg.imageMap = atlantisImageMap;
    scfg.pixFmt = PIXFMT_888;
    scfg.rowOrd = BOTTOM_TO_TOP;
    scfg.nodeNum = nprocs-1;
    scfg.master = false;
    scfg.rendering = true;

    sageInf.init(scfg);

    std::cerr << "sail initialized " << std::endl;

    //glutInitWindowSize(512, 192);
    gettimeofday(&tv_start,0);

    glutInitWindowSize(resX, resY);
    glutInitWindowPosition(0,0);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("GLUT Atlantis Demo");
    printf("sharks: %d\n",NUM_SHARKS);
    Init();
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    moving = GL_TRUE;
    glutIdleFunc(Animate);
    glutVisibilityFunc(Visible);
    glutCreateMenu(menuSelect);
    glutAddMenuEntry("Start motion", 1);
    glutAddMenuEntry("Stop motion", 2);
    glutAddMenuEntry("Quit", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    //glutFullScreen();

    t1 = getTime();

    if (rgbBuffer)
      delete [] rgbBuffer;
    rgbBuffer = (GLubyte *)sageInf.getBuffer();

    MPI_Barrier(MPI_COMM_WORLD);
    std::cerr << "entering rendering loop" << std::endl;

    glutMainLoop();

    // finalize
    MPI_Finalize();
  }

  // exit
  return EXIT_SUCCESS;
}
