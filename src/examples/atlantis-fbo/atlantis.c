
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
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glut.h>
#include <GL/glu.h>
#endif


#include "atlantis.h"

// headers for SAGE
#include "sail.h"
#include "misc.h"
int winWidth, winHeight;
GLubyte *rgbBuffer = 0;
sail sageInf; // sail object


GLuint tname;
void *pixels;
GLint viewport[4];


GLuint g_dynamicTextureID;
GLuint g_frameBuffer;
GLuint g_depthRenderBuffer;
int xwin, ywin;



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
int fr = 0;
float t1,t2;
struct timeval tv_start;

double getTime()
{
  struct timeval tv;

  gettimeofday(&tv,0);
  return (double)(tv.tv_sec - tv_start.tv_sec) +
    (double)(tv.tv_usec - tv_start.tv_usec) / 1000000.0;
}

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

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glGenTextures(1,&tname);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tname);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  glDisable(GL_TEXTURE_RECTANGLE_ARB);
  glDisable(GL_TEXTURE_2D);

  glClearColor(0.0, 0.5, 0.9, 0.0);

  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  //    glPixelStorei(GL_PACK_ALIGNMENT, 1);


  //
  // Create a frame-buffer object and a render-buffer object...
  //

  glGenFramebuffersEXT( 1, &g_frameBuffer );
  glGenRenderbuffersEXT( 1, &g_depthRenderBuffer );

  // Initialize the render-buffer for usage as a depth buffer.
  // We don't really need this to render things into the frame-buffer object,
  // but without it the geometry will not be sorted properly.

  glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, g_depthRenderBuffer );
  glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, xwin, ywin);

  //
  // Check for errors...
  //

  GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );

  switch( status )
  {
  case GL_FRAMEBUFFER_COMPLETE_EXT:
    fprintf(stderr, "GL_FRAMEBUFFER_COMPLETE_EXT!\n");
    break;

  case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
    fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED_EXT!\n");
    exit(0);
    break;

  default:
    exit(0);
  }


  glGenTextures(1, &g_dynamicTextureID );
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_dynamicTextureID );

  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
               xwin, ywin, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );

  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}


extern "C"
{
#define GL_READ_PIXEL_DATA_RANGE_NV       0x8879
  void glPixelDataRangeNV(GLenum target, GLsizei length, const GLvoid *pointer);
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

  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = width;
  viewport[3] = height;

#if 0
  // Video Memory : 0,0,1
  // AGP memory   : 0,0,0.5
  pixels = glXAllocateMemoryNV(width * height * 4, 1.0,0.0,1.0);
  glEnable(GL_READ_PIXEL_DATA_RANGE_NV);
  glEnableClientState(GL_READ_PIXEL_DATA_RANGE_NV);
  glPixelDataRangeNV(GL_READ_PIXEL_DATA_RANGE_NV, width*height*3, pixels);
  int rs = glIsEnabled(GL_READ_PIXEL_DATA_RANGE_NV);
  fprintf(stderr, "Is Range enabled : %d\n", rs);
  //void glFlushPixelDataRangeNV(enum target);
#else
  pixels = malloc( width * height * 3 );
#endif

  assert( pixels );
  memset( pixels, 0, width * height * 3 );

  fprintf(stderr, "buffer there (%p): %d %d \n\n", pixels,width, height);
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

void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;
  switch (key)
  {
  case 27:           /* Esc will quit */
    exit(1);
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
  int x, y, width, height;
  double t1, t2;
  float p[4];
  GLboolean valid;
  int i, j, stepi, stepj, block;
  static int first = 1;

  x = viewport[0];
  y = viewport[1];
  width = viewport[2];
  height = viewport[3];

  t1 = getTime();
  glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels );
  //fprintf(stderr, "Return Read [%s]\n", gluErrorString(glGetError()));
  t2 = getTime();
  fprintf(stderr, "Read %12.4f -- %.2f Hz -- %.1f Mpixels/sec -- %.1f MB/sec\n",
          t2-t1, 1.0/(t2-t1), ((double)(width*height))/(1000000*(t2-t1)),
          ((double)(width*height*4))/(1000000*(t2-t1)));


  glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );


  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, 0, height, -1.0, 1.0);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  t1 = getTime();


#if 1
  glDrawPixels( width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels );
  //fprintf(stderr, "Return Draw [%s]\n", gluErrorString(glGetError()));
#else
  glColor4f(0.5f,0.5f,0.5f,1.0f);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tname);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);

  if (first)
  {
    //glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_COMPRESSED_RGB_S3TC_DXT1_ARB,width,height,0,GL_RGB, GL_UNSIGNED_BYTE,pixels);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGB8,width,height,0,GL_RGB, GL_UNSIGNED_BYTE,pixels);
    first = 0;
    //fprintf(stderr, "Return glTexImage2D [%s]\n", gluErrorString(glGetError()));
  }
  else
  {
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,width,height,GL_RGB, GL_UNSIGNED_BYTE,pixels);
    //fprintf(stderr, "Return glTexSubImage2D [%s]\n", gluErrorString(glGetError()));
  }

  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0);
  glVertex3f(10, 10, 0.2);

  glTexCoord2f(0.0, height);
  glVertex3f(10, height-10, 0.2);

  glTexCoord2f(width, height);
  glVertex3f( width-10,  height-10, 0.2);

  glTexCoord2f(width, 0.0);
  glVertex3f( width-10, 10, 0.2);

  glEnd();

  glDisable(GL_TEXTURE_RECTANGLE_ARB);
  glDisable(GL_TEXTURE_2D);

#endif

  t2 = getTime();
  fprintf(stderr, "Draw %12.4f -- %.2f Hz -- %.1f Mpixels/sec\n",
          t2-t1, 1.0/(t2-t1), ((double)(width*height))/(1000000*(t2-t1)));

  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
}

void
DisplayFrame( void )
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


  //
  // Bind the frame-buffer object and attach to it a render-buffer object
  // set up as a depth-buffer.
  //

  glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, g_frameBuffer );
  glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             GL_TEXTURE_RECTANGLE_ARB, g_dynamicTextureID, 0 );
  glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                GL_RENDERBUFFER_EXT, g_depthRenderBuffer );

  //
  // Set up the frame-buffer object just like you would set up a window.
  //
  glViewport( 0, 0, xwin, ywin);



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
  gluPerspective(40.0, 2.0, 10000.0, 400000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (i = 0; i < NUM_SHARKS; i++) {
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

  if (winWidth > 0) {
    glReadPixels(0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer);
    sageInf.swapBuffer();
    rgbBuffer = (GLubyte *)sageInf.getBuffer();
  }

  sageMessage msg;
  if (sageInf.checkMsg(msg, false) > 0) {
    switch (msg.getCode()) {
    case APP_QUIT : {
      sageInf.shutdown();
      exit(0);
      break;
    }
    }
  }


  //save_frame();

  glFinish();



  glutSwapBuffers( );
  /*
    if (fr> 1000)
    {
    t2 = getTime();
    printf("Time: %f, Framerate: %f\n", t2-t1, fr/(t2-t1));
    exit(0);
    }
    fr++;
  */
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
  switch (value) {
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
  int appID;
  if (argc < 2)
    appID = 0;
  else
    appID = atoi(argv[1]);

  int nodeId = 0;

  int resX = 300, resY = 300;
  if (argc > 3) {
    resX = atoi(argv[2]);
    resY = atoi(argv[3]);
  }

  sageRect atlantisImageMap;
  atlantisImageMap.left = 0.0;
  atlantisImageMap.right = 1.0;
  atlantisImageMap.bottom = 0.0;
  atlantisImageMap.top = 1.0;

  sailConfig scfg;
  scfg.init("atlantis-fbo.conf");
  scfg.setAppName("atlantis-fbo");
  scfg.rank = nodeId;
  scfg.appID = appID;
  scfg.resX = resX;
  scfg.resY = resY;
  scfg.imageMap = atlantisImageMap;
  scfg.pixFmt = PIXFMT_888;
  scfg.rowOrd = BOTTOM_TO_TOP;
  scfg.master = true;
  scfg.nwID = 1;

  if (argc > 4)
    scfg.nwID = atoi(argv[4]);

  sageInf.init(scfg);

  std::cout << "sail initialized " << std::endl;


  gettimeofday(&tv_start,0);

  glutInit(&argc, argv);
  xwin = resX;
  ywin = resY;
  glutInitWindowSize(xwin, ywin);
  glutInitWindowPosition(0,0);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("GLUT Atlantis Demo");
  //glutFullScreen();
  printf("sharks: %d\n",NUM_SHARKS);

  Init();

  glutDisplayFunc(DisplayFrame);
  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Key);
  moving = GL_TRUE;
  glutIdleFunc(Animate);
  // glutVisibilityFunc(Visible);
  glutCreateMenu(menuSelect);
  glutAddMenuEntry("Start motion", 1);
  glutAddMenuEntry("Stop motion", 2);
  glutAddMenuEntry("Quit", 3);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  if (rgbBuffer)
    delete [] rgbBuffer;
  rgbBuffer = (GLubyte *)sageInf.getBuffer();


  t1 = getTime();


  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
