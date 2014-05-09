/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu
 *****************************************************************************/

//Shalini Venkataraman
//A simple glut program that draws spheres and cones, reads the frame buffer
//and sends it across to a display client
//Feb 2004
//
//modified by Byungil Jeong
//July 2004

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <time.h>
#ifdef WIN32
#include "gettimeofday.hxx"
#else
#include <sys/timeb.h>
#include <sys/time.h>
#endif

// headers for SAGE
#include "sail.h"
#include "misc.h"

#include <mpi.h>

#define PI 3.1415926535898f
#define DEG_TO_RAD PI/180.0f
#define TRANSLATION_RATE 0.2f
#define ROTATION_RATE 0.01f

int rank = 0;  // node rank
int size = 0;  // number of nodes

enum {SPHERE = 1, CONE};
enum {X, Y, Z};

int winWidth, winHeight;
float rgbReadTime;
GLubyte *rgbBuffer = 0;
GLfloat transformMatrix[] = {1.0, 0.0, 0.0, 0.0,
                             0.0, 1.0, 0.0, 0.0,
                             0.0, 0.0, 1.0, 0.0,
                             0.0, 0.0, 0.0, 1.0};

float eyeX, eyeY, eyeZ; //for the camera position for glulookat
float centreX, centreY, centreZ; //the point the camera looks at in glulookat
float rot = 0.0f;
sail sageInf; // sail object


double getTimeInSecs() {
  const double oneMicroSec = 0.000001;
  struct timeval timeSec;
  gettimeofday(&timeSec, NULL);
  return (((double) timeSec.tv_sec) +
          (((double) timeSec.tv_usec) * oneMicroSec));

}

void rotateCameraY(float rot_inc) {
  rot += rot_inc;
  centreX = eyeX - sin(rot);
  centreZ = eyeZ - cos(rot);
}

void moveCameraZ(float trans_inc) {
  float inc_x = -trans_inc*sin(rot); //increment in camera X for the given translation
  float inc_z = -trans_inc*cos(rot); //increment in camera Y for the given translation
  eyeX += inc_x;
  eyeZ += inc_z;
  centreX += inc_x;
  centreZ += inc_z;
}

void moveCameraY(float trans_inc) {
  eyeY += trans_inc;
  centreY += trans_inc;
}

GLfloat *make_texture(int maxs, int maxt)
{
  int s, t;
  static GLfloat *texture;

  texture = (GLfloat *)malloc(3*maxs * maxt * sizeof(GLfloat));
  for(t = 0; t < maxt; t++) {
    for(s = 0; s < maxs; s++) {
      texture[s*3 + 3*maxs * t] = 0.0f;
      texture[s*3 + 3*maxs * t+1] = 0.4f*(((s >> 4) & 0x1) ^ ((t >> 4) & 0x1));
      texture[s*3 + 3*maxs * t+2] = 0.0f*(((s >> 4) & 0x1) ^ ((t >> 4) & 0x1));
    }
  }
  return texture;
}

//writes the grabbed framebuffer to a ppm file
void writePPM(const char* filename) {
  if (!rgbBuffer)
    return;
  FILE* fp = fopen(filename, "wb");
  if (fp==NULL) { printf("PPM ERROR (WritePPM) : unable to open %s!\n",filename); return; }
  fprintf(fp, "P6\n%d %d\n255\n", winWidth, winHeight);
  fwrite(rgbBuffer,sizeof(GLubyte),winWidth*winHeight*3,fp);
  fclose(fp);
  printf("RGB data written to file %s\n",filename);
}

//key handler
//'s' - will save the grabbed image to a ppm file
void key(unsigned char key, int x, int y)
{
  switch(key) {
  case 's': //save the read pixels to out.ppm
    writePPM("out.ppm");
    break;
  case 't': //display time taken for readpixels
    printf("Frame Buffer Readback Time: %dx%d %f \n",winWidth, winHeight, rgbReadTime);
    break;
  case '\033':
    // finalize
    MPI_Finalize();

    exit(0);
    break;
  }
  glutPostRedisplay();
}

// The function called whenever a "special" key is pressed.
void special(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_UP:
    moveCameraZ(TRANSLATION_RATE);
    break;
  case GLUT_KEY_DOWN:
    moveCameraZ(-TRANSLATION_RATE);
    break;
  case GLUT_KEY_LEFT:
    rotateCameraY(ROTATION_RATE);
    break;
  case GLUT_KEY_RIGHT:
    rotateCameraY(-ROTATION_RATE);
    break;
  case GLUT_KEY_PAGE_UP:
    moveCameraY(TRANSLATION_RATE);
    break;
  case GLUT_KEY_PAGE_DOWN:
    moveCameraY(-TRANSLATION_RATE);
    break;
  }
  glutPostRedisplay();
}

void idle()
{
  glutPostRedisplay();
}

void
motion(int x, int y)
{
  y = winHeight - y;
  glutPostRedisplay();

}

//reallocate the rgbBuffer depending on the new window dimesions
void
reshape(int width, int height)
{
  glViewport(0, 0, width, height);
  winWidth = width;
  winHeight = height;
  if (rgbBuffer)
    delete [] rgbBuffer;
  rgbBuffer = new GLubyte[width*height*3];
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0,(GLfloat)width/ (GLfloat)height, 0.5, 100.0);
  glMatrixMode(GL_MODELVIEW);
}


void
mouse(int button, int state, int x, int y)
{
  y = winHeight - y;
  if(state == GLUT_DOWN) {
    switch(button) {
    case GLUT_LEFT_BUTTON: //rotate
      break;
    case GLUT_MIDDLE_BUTTON: //zoom
      break;
    case GLUT_RIGHT_BUTTON:
      break;
    }
  }
  else {//GLUT_UP
    switch(button) {
    case GLUT_LEFT_BUTTON:
      break;
    case GLUT_MIDDLE_BUTTON:
      break;
    case GLUT_RIGHT_BUTTON:
      break;
    }
  }
  glutPostRedisplay();
}


//display function
void redraw(void)
{
  /* material properties for objects in scene */
  static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};
  static GLfloat sphere_mat[] = {0.5f, 0.0f, 1.f, 1.f};
  static GLfloat cone_mat[] = {1.0f, 0.5f, 0.0f, 1.f};
  static GLfloat sphere_mat2[] = {1.0f, 0.0f, 0.5f, 1.f};
  static GLfloat cone_mat2[] = {0.0f, 0.5f, 1.0f, 1.f};
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0, 0.0, 0.2, 0.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(eyeX, eyeY, eyeZ,
            centreX, centreY, centreZ,
            0.0,1.0,0.0);
  /*
  ** Note: wall verticies are ordered so they are all front facing
  ** this lets me do back face culling to speed things up.
  */

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glNormal3f(0.f, 1.f, 0.f);
  glTexCoord2i(0, 0);
  glVertex3f(-6.f, -4.f, -2.f);
  glTexCoord2i(1, 0);
  glVertex3f( 6.f, -4.f, -2.f);
  glTexCoord2i(1, 1);
  glVertex3f( 6.f, -4.f, -15.f);
  glTexCoord2i(0, 1);
  glVertex3f(-6.f, -4.f, -15.f);
  glEnd();
  glDisable(GL_TEXTURE_2D);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
  glPushMatrix();
  glTranslatef(-3.f, 0.5f, -10.f);
  glScalef(1, 1, 1);
  glCallList(SPHERE);
  glPopMatrix();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
  glPushMatrix();
  glTranslatef(0.f, 0.3f, -3.f);
  glRotatef(30.f, 2.f, 0.0, -1.0f);
  glScalef(1.0, 1.0, 1.0);
  glCallList(CONE);
  glPopMatrix();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat2);
  glPushMatrix();
  glTranslatef(-2.f, -2.f, -7.f);
  glScalef(1, 1, 1);
  glCallList(SPHERE);
  glPopMatrix();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat2);
  glPushMatrix();
  glTranslatef(4.f, -2.f, -13.f);
  glRotatef(-45.f, 1.f, 0.0f, 0.0f);
  glScalef(0.8, 0.8, 0.8);
  glCallList(CONE);
  glPopMatrix();

  //finished drawing. At this point capture the frame buffer
  //expensive new!! TODO: put this in reshape

  if (winWidth > 0) {
    if (sageInf.getBufSize() == 0)
      sageInf.resizeBuffer(winWidth, winHeight, 24);

    //cout << "read pixels" << endl;
    double prevtime = getTimeInSecs();
    glReadPixels(0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer);
    rgbReadTime = getTimeInSecs()-prevtime;
    //TODO-send it to QUANTA

    cout << rank << " swap buffer " << winWidth << "," << winHeight << endl;

    MPI_Barrier(MPI_COMM_WORLD);

    sageInf.swapBuffer(rgbBuffer);
    sageInf.mainLoop();

  }

  glutSwapBuffers();
}

void update(int value) {

  //sageInf.swapBuffer((unsigned char *)rgbBuffer);
  sageInf.mainLoop();

  //glutTimerFunc(1000, update, 1);
}

main(int argc, char *argv[])
{

  // MPI Initialization
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Barrier(MPI_COMM_WORLD);

  GLfloat *tex;
  static GLfloat lightpos[] = {10.f, 10.f, -20.0f, 1.f};
  GLUquadricObj *sphere, *cone, *base;

  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
  glutInitWindowSize(1024,1024);
  glutCreateWindow("sage render");
  glutDisplayFunc(redraw);
  glutIdleFunc(idle);
  glutKeyboardFunc(key);
  glutMouseFunc(mouse);
  //glutMotionFunc(motion);
  glutSpecialFunc(special);
  glutReshapeFunc(reshape);
  //glutFullScreen();

  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  glCullFace(GL_BACK);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  //create the display list to draw a sphere
  glNewList(SPHERE, GL_COMPILE);
  sphere = gluNewQuadric();
  gluSphere(sphere, 1.f, 20, 20);
  gluDeleteQuadric(sphere);
  glEndList();

  //create the display list to draw a cone
  glNewList(CONE, GL_COMPILE);
  cone = gluNewQuadric();
  base = gluNewQuadric();
  glRotatef(-90.f, 1.f, 0.f, 0.f);
  gluQuadricOrientation(base, GLU_INSIDE);
  gluDisk(base, 0., 1., 20, 1);
  gluCylinder(cone, 1., 0., 2., 20, 20);
  gluDeleteQuadric(cone);
  gluDeleteQuadric(base);
  glEndList();

  //create a 2D texture for our floor
  tex = make_texture(256, 256);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, 256, 256, 0, GL_RGB, GL_FLOAT, tex);
  free(tex);

  eyeX = eyeY = centreX = centreY = 0.0f;
  eyeZ = 10.0f; centreZ = 9.0f;

  //glutTimerFunc(100, update, 1);

  MPI_Barrier(MPI_COMM_WORLD);

  char procname[MPI_MAX_PROCESSOR_NAME];
  int lenproc;
  MPI_Get_processor_name(procname, &lenproc);
  fprintf(stderr, "Processor %2d is machine [%s]\n", rank, procname);

  sageInf.init("sail.conf", "render", rank, 0, 0, 24, TVPIXFMT_888, BOTTOM_TO_TOP);

  MPI_Barrier(MPI_COMM_WORLD);

  glutMainLoop();

  // finalize
  MPI_Finalize();

  // exit
  return EXIT_SUCCESS;
}
