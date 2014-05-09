/*
  A wiimote tracker that attempts to track up tor 4 independent dots and maps
  it to a position in a 2D plane as normalized coordinates. The coordinates and
  button presses are reported to SAGE (DIM).

  Ratko Jagodic
  Feb 2009
*/

// uncomment this to show an opengl debug window
#define SHOW_GL_DEBUG_WINDOW


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "wiiuse.h"
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <windows.h>
#include <winsock.h>
#endif



#ifdef SHOW_GL_DEBUG_WINDOW
#include <gl/gl.h>
#include <glut.h>
#endif



/*****************************************************************************/
// globals
/*****************************************************************************/

// for compatibility
#ifndef _WIN32
#define SOCKET_ERROR    -1
#endif

#define DEG2RAD 0.0174532925

#define round(fp) (int)((fp) >= 0 ? (fp) + 0.5 : (fp) - 0.5)


/////////  sage communication stuff  /////////
#define DIM_PORT 20005
char sageHost[100];
char myIP[16];
char *msg=NULL; // will hold queued up messages
int sock;
int use_sage = 1;

#define POS_MSG   1
#define BUTTON_MSG  2

#define BUTTON_UP   0
#define BUTTON_DOWN 1


/////////  wiimote structures  /////////
#define MAX_WIIMOTES 4
#define IR_SENSITIVITY 5   // 5=max, 1=min
wiimote** wiimotes;  // wiiuse structure holding all the wiimote data
int found;  // number of wiimotes actually found
struct ir_dot_t x[4], y[4];  // will store visible dots sorted by x and y

void connectToWiimotes();

/*****************************************************************************/
// SAGE connection
/*****************************************************************************/

void mySleep(unsigned long millis)
{
#ifdef _WIN32
  Sleep((DWORD) millis);
#else
  usleep(millis*1000);
#endif
}


void connectToSage(void)
{
  if (!use_sage)
    return;

  // socket stuff
  int error;
  struct sockaddr_in peer;
  struct hostent *myInfo;
  char myHostname[128];

#ifdef _WIN32
  WSADATA ws;
  WSAStartup(MAKEWORD(1,1), &ws);
#endif

  peer.sin_family = AF_INET;
  peer.sin_port = htons(DIM_PORT);
  peer.sin_addr.s_addr = inet_addr(sageHost);

  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0)
  {
    printf("\nERROR: Couldn't open socket on port %d!\n", DIM_PORT);
    exit(0);
  }

  // get my IP for the message header to SAGE
  gethostname(myHostname, 128);
  myInfo = gethostbyname(myHostname);
  sprintf(myIP, "%s\0", inet_ntoa(*(struct in_addr*) myInfo->h_addr));

  while ((error = connect(sock, (struct sockaddr*)&peer, sizeof(peer))) != 0)
  {
    printf("\nTrying to reconnect to sage on %s in a few seconds...", sageHost);
    mySleep(3000);
  }

  mySleep(3000);
  printf("\nConnected to sage on: %s\n", sageHost);
}


void disconnectFromSage(void)
{
  if (!use_sage) return;

#ifdef _WIN32
  closesocket(sock);
  WSACleanup();
#else
  close(sock);
#endif

  // clsoe the connection with all the wiimotes since we will reconnect them
  // when we connect to SAGE again
  wiiuse_cleanup(wiimotes, MAX_WIIMOTES);
}


void sendToSage()
{
  if (!use_sage) return;
  if (send(sock, msg, strlen(msg), 0) == SOCKET_ERROR)
  {
    printf("\nDisconnected from sage... reconnecting\n");
    disconnectFromSage();
    connectToSage();   // reconnect automatically
    connectToWiimotes();
  }
  free(msg);
  msg = NULL;
}

void queueMessage(char *newMsg) {
  if (!use_sage) return;
  if (msg == NULL) {
    msg = (char *) malloc( strlen(newMsg)+1 );
    strcpy(msg, newMsg);
  }
  else {
    msg = (char *) realloc( msg, strlen(newMsg)+strlen(msg)+1 );
    strcat(msg, newMsg);
  }
}

void sendPosition(int wiimoteId, double x, double y)
{
  char msgData[256];
  sprintf(msgData, "%s:wiimote%d wiimote %d %f %f\n\0", myIP, wiimoteId, POS_MSG, x, y);
  queueMessage(msgData);
}

void sendButton(int wiimoteId, int btnId, int state)
{
  char msgData[256];
  sprintf(msgData, "%s:wiimote%d wiimote %d %d %d\n\0", myIP, wiimoteId, BUTTON_MSG, btnId, state);
  //fprintf(stderr, "\n%s", msgData);
  queueMessage(msgData);
}


/*****************************************************************************/
// tracking dots
/*****************************************************************************/

// maps num from range [fromLow, fromHi] to [toLow, toHi]
double mapRange(double num, double fromLow, double fromHi, double toLow, double toHi)
{
  double norm;

  // ensure the number is in range
  if (fromLow < fromHi) {
    if (num < fromLow) num = fromLow;
    else if (num > fromHi) num = fromHi;
  }
  else {
    if (num > fromLow) num = fromLow;
    else if (num < fromHi) num = fromHi;
  }

  norm = fabs(num - fromLow) / fabs(fromHi-fromLow);
  return toLow + norm*(toHi - toLow);
}


/*void jumpToEdge(struct wiimote_t* wm)
  {
  float pitch = wm->orient.pitch;
  int j = 0;
  wm->ir.jumped = 1;

  if (pitch > 10 && wm->ir.cursorY > 0.8) {
  wm->ir.cursorY = 1.0;
  j = 1;
  }
  else if (pitch < 10 && wm->ir.cursorY < 0.2) {
  wm->ir.cursorY = 0.0;
  j = 1;
  }

  // inform sage of the new position
  if (j == 1) {
  sendPosition(wm->unid, wm->ir.cursorX, wm->ir.cursorY);
  fprintf(stderr, "\n\nJUMPED to %f %f\n", wm->ir.cursorX, wm->ir.cursorY);
  }
  }*/


// rotate all the dots around the center of the wiimote frame
void rotatePoints(struct ir_t *ir, double deg)
{
  int i, x, y, num=0;
  double rx, ry;
  double rad = deg*DEG2RAD;

  for(i=0; i<4; i++) {
    if (ir->dot[i].visible) {

      // translate to center of the frame
      x = ir->dot[i].x - 512;
      y = ir->dot[i].y - 384;

      // rotate the points and translate back
      rx = cos(rad)*x - sin(rad)*y + 512;
      ry = sin(rad)*x + cos(rad)*y + 384;

      // did the rotation push the point out of bounds?
      if(rx>1024 || rx<0 || ry>768 || ry<0) {
        ir->dot[i].visible = 0;
      }
      else {
        num++;
        ir->dot[i].x = round(rx);
        ir->dot[i].y = round(ry);
      }
    }
  }
  ir->num_dots = num;
}


void straightenPoints(int numVisible, struct ir_dot_t x[4], struct ir_dot_t y[4])
{
  int a,i;

  if (numVisible == 4) {
    for (i=0; i<4; i+=2) {
      a = round((x[i].x + x[i+1].x)/2.0);
      x[i].x = x[i+1].x = a;

      a = round((y[i].y + y[i+1].y)/2.0);
      y[i].y = y[i+1].y = a;
    }
  }
  else if(numVisible == 3) {

    if ((x[1].x-x[0].x) < (x[2].x-x[1].x)) {
      a = round((x[0].x + x[1].x)/2.0);
      x[0].x = x[1].x = a;
    }
    else {
      a = round((x[1].x + x[2].x)/2.0);
      x[1].x = x[2].x = a;
    }

    if ((y[1].y-y[0].y) < (y[2].y-y[1].y)) {
      a = round((y[0].y + y[1].y)/2.0);
      y[0].y = y[1].y = a;
    }
    else {
      a = round((y[1].y + y[2].y)/2.0);
      y[1].y = y[2].y = a;
    }
  }
  else if(numVisible == 2) {
    if ((x[1].x-x[0].x) < (y[1].y-y[0].y)) {
      a = round((x[0].x + x[1].x)/2.0);
      x[0].x = x[1].x = a;
      y[0].x = y[1].x = a;
    }
    else {
      a = round((y[0].y + y[1].y)/2.0);
      y[0].y = y[1].y = a;
      x[0].y = x[1].y = a;
    }
  }
}


// sort Xs from "ir" into x[4]
// sort Ys from "ir" into y[4]
void sort(struct ir_t ir, struct ir_dot_t x[4], struct ir_dot_t y[4])
{
  int i,j,c,minX,minY;
  struct ir_dot_t temp;

  // first copy dots to new arrays
  for(i=0, c=0; i<4; i++) {
    if (ir.dot[i].visible) {
      x[c] = ir.dot[i];
      y[c] = ir.dot[i];
      c++;
    }
  }

  // now sort
  for(i=0; i<c-1; i++)
  {
    minX = i;
    minY = i;
    for(j=i+1; j<c; j++) {
      if(x[j].x < x[minX].x)
        minX = j;
      if(y[j].y < y[minY].y)
        minY = j;
    }
    temp = x[i];
    x[i] = x[minX];
    x[minX] = temp;

    temp = y[i];
    y[i] = y[minY];
    y[minY] = temp;
  }
}


// flip the coords to make (0,0) in upper left corner
void flipCoords(struct ir_t *ir)
{
  int i;
  for(i=0; i<4; i++) {
    if (ir->dot[i].visible) {
      ir->dot[i].x = 1024 - ir->dot[i].x;
      ir->dot[i].y = 768 - ir->dot[i].y;
    }
  }
}



/*int getYDist(int yId, int yi, struct ir_dot_t y[4])
  {
  int yDist = 1;
  int xx = y[yi].x;
  int yy = y[yi].y;

  if (yId == TOP) {
  if( abs((int)y[2].x - xx) <= 50)
  yDist = y[2].y - yy;
  else
  yDist = y[3].y - yy;
  }
  else {
  if( abs((int)y[0].x - xx) <= 50)
  yDist = yy - y[0].y;
  else
  yDist = yy - y[1].y;
  }
  return yDist;
  }*/


// goes through all the visible dots and finds the ones that are closest to
// the edges in x and y directions... those are used for tracking
// the found indices are stored in xi and yi
void findDotsToTrack4(struct ir_t *ir, struct ir_dot_t x[4], struct ir_dot_t y[4])
{
  int i=0, minX=513, minY=385, minXi, minYi, xi, yi, t;

  for (i; i<4; ++i) {
    t = abs(512-(int)x[i].x);
    if(t < minX) {
      minX = t;
      minXi = i;
    }
    t = abs(384-(int)y[i].y);
    if(t < minY) {
      minY = t;
      minYi = i;
    }
  }
  xi = minXi;
  yi = minYi;


  // so these are the coords we will use for tracking this round
  ir->dotX = x[xi].x;
  ir->dotY = y[yi].y;

  // determine the relative position of these dots and distance
  if(xi < 2) {
    ir->xId = LEFT;
    ir->xDist = x[xi+2].x - x[xi].x;
  }
  else {
    ir->xId = RIGHT;
    ir->xDist = x[xi].x - x[xi-2].x;
  }
  if(yi < 2) {
    ir->yId = TOP;
    ir->yDist = y[yi+2].y - y[yi].y;
  }
  else {
    ir->yId = BOTTOM;
    ir->yDist = y[yi].y - y[yi-2].y;
  }

  // recalculate the distance aspect ratio
  ir->distAr = (double)ir->xDist / (double)ir->yDist;
}



void findDotsToTrack3(struct ir_t *ir, struct ir_dot_t x[4], struct ir_dot_t y[4])
{
  int x1,x2,y1,y2;

  ////////  X  ///////////
  // (we check in y array because we want to find two dots
  // that are roughly on the same horizontal plane)
  if ((y[2].y-y[1].y) > (y[1].y-y[0].y)) {
    x1 = y[0].x;
    x2 = y[1].x;
  }
  else {
    x1 = y[1].x;
    x2 = y[2].x;
  }

  // distance
  ir->xDist = abs(x1-x2);

  // find which one is closer to the center
  if (abs(512-x1) < abs(512-x2)) ir->dotX = x1;
  else  ir->dotX = x2;

  // find the relative position
  if(ir->dotX > x1 || ir->dotX > x2)  ir->xId = RIGHT;
  else  ir->xId = LEFT;


  ////////  Y  /////////
  if ((x[2].x-x[1].x) > (x[1].x-x[0].x)) {
    y1 = x[0].y;
    y2 = x[1].y;
  }
  else {
    y1 = x[1].y;
    y2 = x[2].y;
  }

  // distance
  ir->yDist = abs(y1-y2);

  // recalculate the distance aspect ratio
  ir->distAr = (double)ir->xDist / (double)ir->yDist;

  // find which one is closer to the center
  if (abs(384-y1) < abs(384-y2)) ir->dotY = y1;
  else  ir->dotY = y2;

  // find the relative position
  if(ir->dotY > y1 || ir->dotY > y2)  ir->yId = BOTTOM;
  else  ir->yId = TOP;
}



void findDotsToTrack2(struct ir_t *ir, struct ir_dot_t x[4], struct ir_dot_t y[4])
{
  int x1=x[0].x, x2 = x[1].x, y1 = y[0].y, y2 = y[1].y;

  // find which one is closer to the center in x
  if ( abs(512-x1) < abs(512-x2)) ir->dotX = x1;
  else  ir->dotX = x2;

  // find which one is closer to the center in y
  if (abs(384-y1) < abs(384-y2)) ir->dotY = y1;
  else  ir->dotY = y2;


  // find the relative position and distance depending on
  if ((x2-x1) > (y2-y1)) {  // HORIZONTAL

    // distance, approximate one distance based on the one we know
    ir->xDist = x2-x1;
    ir->yDist = (int) (ir->xDist / ir->distAr);

    // find the relative position
    if(ir->dotX > x1 || ir->dotX > x2)  ir->xId = RIGHT;
    else  ir->xId = LEFT;
    // don't change the yId... just use the previous one
  }

  else {             // VERTICAL

    // distance, approximate one distance based on the one we know
    ir->yDist = y2-y1;
    ir->xDist = (int) (ir->yDist * ir->distAr);

    // find the relative position
    if(ir->dotY > y1 || ir->dotY > y2)  ir->yId = BOTTOM;
    else  ir->yId = TOP;
  }
}



/*****************************************************************************/
// OpenGL debug window
/*****************************************************************************/

#ifdef SHOW_GL_DEBUG_WINDOW
void output(int x, int y, char *string)
{
  int len, i;

  glRasterPos2f(x, y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
  }
}


void display(void)
{
  int i=0, xx, yy, s;
  char tmp[50] = "";
  glClear(GL_COLOR_BUFFER_BIT);

  // draw center lines
  glColor3f(0.3,0.3,0.3);
  glBegin(GL_LINES);
  glVertex2f(0, 384);
  glVertex2f(1024, 384);
  glVertex2f(512, 0);
  glVertex2f(512, 768);
  glEnd();

  // draw all the visible dots
  /*for(i=0; i<wiimotes[0]->ir.num_dots; i++) {
    s = 8;
    xx = x[i].x; //wiimotes[0]->ir.dot[i].x;
    yy = x[i].y; //wiimotes[0]->ir.dot[i].y;

    if (x == wiimotes[0]->ir.dotX || y == wiimotes[0]->ir.dotY) {
    s = 10;
    glColor3f(1.0, 1.0, 1.0);
    }
    else
    glColor3f(0.5, 0.5, 0.5);

    glBegin(GL_POLYGON);
    glVertex2f(xx+s, yy+s);
    glVertex2f(xx+s, yy-s);
    glVertex2f(xx-s, yy-s);
    glVertex2f(xx-s, yy+s);
    glEnd();

    }*/

  glColor3f(0.5, 0.5, 0.5);
  // draw all the visible dots
  for(i=0; i<4; i++) {
    if (wiimotes[0]->ir.dot[i].visible) {
      s = 8;
      xx = wiimotes[0]->ir.dot[i].x;
      yy = wiimotes[0]->ir.dot[i].y;

      /*if (x == wiimotes[0]->ir.dotX || y == wiimotes[0]->ir.dotY) {
        s = 10;*/
      //glColor3f(1.0, 1.0, 1.0);
      /*}
        else
        glColor3f(0.5, 0.5, 0.5);
      */
      glBegin(GL_POLYGON);
      glVertex2f(xx+s, yy+s);
      glVertex2f(xx+s, yy-s);
      glVertex2f(xx-s, yy-s);
      glVertex2f(xx-s, yy+s);
      glEnd();
    }
  }


  // draw the dot we report
  xx = round(1024*(1-wiimotes[0]->ir.cursorX));
  yy = round(768*wiimotes[0]->ir.cursorY);

  glColor3f(1.0, 1.0, 0.0);
  glBegin(GL_POLYGON);
  glVertex2f(xx+3, yy+3);
  glVertex2f(xx+3, yy-3);
  glVertex2f(xx-3, yy-3);
  glVertex2f(xx-3, yy+3);
  glEnd();

  // draw some debug variables on top
  glColor3f(1.0, 1.0, 1.0);
  if (wiimotes[0]->ir.xId == LEFT) output(0, 20, "LEFT");
  else output(0, 20, "RIGHT");
  if (wiimotes[0]->ir.yId == TOP) output(0, 40, "TOP");
  else output(0, 40, "BOTTOM");
  sprintf(tmp, "%d dots", wiimotes[0]->ir.num_dots);
  output(0, 60, tmp);
  sprintf(tmp, "cursorx = %.4f", wiimotes[0]->ir.cursorX);
  output(0, 80, tmp);
  sprintf(tmp, "cursory = %.4f", wiimotes[0]->ir.cursorY);
  output(0, 100, tmp);
  sprintf(tmp, "distx = %d", wiimotes[0]->ir.xDist);
  output(0, 120, tmp);
  sprintf(tmp, "disty = %d", wiimotes[0]->ir.yDist);
  output(0, 140, tmp);
  //sprintf(tmp, "cx = %.4f", wiimotes[0]->ir.lastCenterX);
  //output(0, 120, tmp);
  //sprintf(tmp, "cy = %.4f", wiimotes[0]->ir.lastCenterY);
  //output(0, 140, tmp);

  glutSwapBuffers();
}
#endif



/*****************************************************************************/
// wiiuse library interface & main
/*****************************************************************************/
int numFrames = 0;
void handle_event(struct wiimote_t* wm)
{
  int w=1024, h=768;
  double c=-1;
  int xo=100, yo=75;

  if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_B))
    sendButton(wm->unid, 0, BUTTON_DOWN);
  if (IS_RELEASED(wm, WIIMOTE_BUTTON_B))
    sendButton(wm->unid, 0, BUTTON_UP);

  if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_A))
    sendButton(wm->unid, 0, BUTTON_DOWN);
  if (IS_RELEASED(wm, WIIMOTE_BUTTON_A))
    sendButton(wm->unid, 0, BUTTON_UP);

  if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_PLUS))
    sendButton(wm->unid, 4, BUTTON_DOWN);
  if (IS_RELEASED(wm, WIIMOTE_BUTTON_PLUS))
    sendButton(wm->unid, 4, BUTTON_UP);

  if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_MINUS))
    sendButton(wm->unid, 5, BUTTON_DOWN);
  if (IS_RELEASED(wm, WIIMOTE_BUTTON_MINUS))
    sendButton(wm->unid, 5, BUTTON_UP);

  if (IS_HELD(wm, WIIMOTE_BUTTON_PLUS))
    sendButton(wm->unid, 4, BUTTON_DOWN);
  if (IS_HELD(wm, WIIMOTE_BUTTON_MINUS))
    sendButton(wm->unid, 5, BUTTON_DOWN);


  // record the last time we got IR info
  if (wm->ir.num_dots > 0) {
    numFrames++;

    if (((double)clock() - wm->ir.lastUpdateTime)/CLOCKS_PER_SEC > 1) {
      //fprintf(stderr, "\nNUM FRAMES %d", numFrames);
      numFrames = 0;
      wm->ir.lastUpdateTime = clock();
    }
  }

  // flip all the coords so that 0,0 is top-left corner
  flipCoords(&wm->ir);

  // account for tilt
  rotatePoints(&wm->ir, wm->orient.roll);

  // if at least one dot is visible, track it
  if (wm->ir.num_dots > 0) {

    // sort the dots into two arrays... one sorted by x and one sorted by y
    sort(wm->ir, x, y);

    // account for the last few pixel differences between the dots in one line
    straightenPoints(wm->ir.num_dots, x, y);

    // find the dots to track depending on how many are visible
    if (wm->ir.num_dots == 4)
      findDotsToTrack4(&wm->ir, x, y);
    else if(wm->ir.num_dots == 3)
      findDotsToTrack3(&wm->ir, x, y);
    else if(wm->ir.num_dots == 2)
      findDotsToTrack2(&wm->ir, x, y);
    else {
      // if we changed ids, just return because we would be using the wrong dot
      if (abs(wm->ir.dotX - (int)x[0].x) > (wm->ir.xDist*0.8)) {
        return;
        //if (wm->ir.xId == LEFT)  wm->ir.xId = RIGHT;
        //else wm->ir.xId = LEFT;
      }

      if (abs(wm->ir.dotY - (int)y[0].y) > (wm->ir.yDist*0.8)) {
        return;
        //if (wm->ir.yId == TOP)  wm->ir.yId = BOTTOM;
        //else wm->ir.yId = TOP;
      }
      wm->ir.dotX = x[0].x;
      wm->ir.dotY = y[0].y;
    }

    // adjust the centerpoint if we changed the dots
    if (wm->ir.lastXid != -1 && wm->ir.xId != wm->ir.lastXid)
      wm->ir.lastCenterX = wm->ir.cursorX;
    if (wm->ir.lastYid != -1 && wm->ir.yId != wm->ir.lastYid)
      wm->ir.lastCenterY = wm->ir.cursorY;

    // finally figure out where the cursor is in normalized coords
    if (wm->ir.xId == LEFT)
      wm->ir.cursorX = mapRange(wm->ir.dotX, w-xo, (w-wm->ir.xDist)/2.0, 0, wm->ir.lastCenterX);
    else
      wm->ir.cursorX = mapRange(wm->ir.dotX, wm->ir.xDist+(w-wm->ir.xDist)/2.0, xo, wm->ir.lastCenterX, 1);

    if (wm->ir.yId == BOTTOM)
      wm->ir.cursorY = mapRange(wm->ir.dotY, yo, wm->ir.yDist+(h-wm->ir.yDist)/2.0, 0, wm->ir.lastCenterY);
    else
      wm->ir.cursorY = mapRange(wm->ir.dotY, (h-wm->ir.yDist)/2.0, h-yo,  wm->ir.lastCenterY, 1);

    // inform sage of the new position
    //sendPosition(wm->unid, wm->ir.cursorX, wm->ir.cursorY);

    wm->ir.changedXid = 0;
    wm->ir.changedYid = 0;
    wm->ir.lastXid = wm->ir.xId;
    wm->ir.lastYid = wm->ir.yId;
  }
  else {
    wm->ir.lastCenterX = 0.5;
    wm->ir.lastCenterY = 0.5;
  }
}


void handle_disconnect(wiimote* wm)
{
  printf("\n\n--- WIIMOTE DISCONNECTED [wiimote id %i] ---\n", wm->unid);
  //disconnectFromSage();
}

int e=0;
void update(void)
{
  int i=0;
  float timeDiff;
  if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) {
    for (i=0; i < MAX_WIIMOTES; ++i) {
      switch (wiimotes[i]->event) {
      case WIIUSE_EVENT:
        handle_event(wiimotes[i]);
        break;

      case WIIUSE_DISCONNECT:
      case WIIUSE_UNEXPECTED_DISCONNECT:
        handle_disconnect(wiimotes[i]);
        break;

      default:
        break;
      }
    }
  }
  for (i=0; i<found; ++i) {
    sendPosition(wiimotes[i]->unid, wiimotes[i]->ir.cursorX, wiimotes[i]->ir.cursorY);
    if (wiimotes[i]->ir.lastUpdateTime > 0) {
      timeDiff = ((double)clock() - wiimotes[i]->ir.lastUpdateTime)/CLOCKS_PER_SEC;
      if (timeDiff > 0.02) {
        wiimotes[i]->ir.lastCenterX = 0.5;
        wiimotes[i]->ir.lastCenterY = 0.5;
      }
    }
  }

  if (msg) sendToSage(); // send all queued up messages in one
#ifdef SHOW_GL_DEBUG_WINDOW
  display();
#endif
}


void connectToWiimotes()
{
  int connected, i;

  // find and connect to wiimotes
  wiimotes = wiiuse_init(MAX_WIIMOTES);
  found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
  if (!found) {
    printf ("No wiimotes found.");
    return;
  }

  connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
  if (connected)
    printf("Connected to %i wiimotes (of %i found).\n", connected, found);
  else {
    printf("Failed to connect to any wiimote.\n");
    return;
  }

  // enable IR tracking on all the wiimotes
  for (i=0; i<MAX_WIIMOTES; ++i) {
    wiiuse_set_ir(wiimotes[i], 1);
    wiiuse_set_ir_sensitivity(wiimotes[i], IR_SENSITIVITY);
    wiiuse_set_orient_threshold(wiimotes[i], 1);

    wiimotes[i]->ir.cursorX = wiimotes[i]->ir.cursorY = 0.0;
    wiimotes[i]->ir.dotX = wiimotes[i]->ir.dotY = 0;
    wiimotes[i]->ir.distAr = 1.0;
    wiimotes[i]->ir.xDist = wiimotes[i]->ir.yDist = 100;
    wiimotes[i]->ir.lastCenterX = 0.5;
    wiimotes[i]->ir.lastCenterY = 0.5;
    wiimotes[i]->ir.lastXid = (dot_position) -1;
    wiimotes[i]->ir.lastYid = (dot_position) -1;
    wiimotes[i]->ir.changedXid = 0;
    wiimotes[i]->ir.changedYid = 0;
    wiimotes[i]->ir.lastUpdateTime = 0;
    wiimotes[i]->ir.jumped = 0;
  }
  wiiuse_set_timeout(wiimotes, 1, 15, 10);
}



int main(int argc, char** argv)
{
  msg = NULL;

  // check the arguments
  if (argc < 2)
  {
    printf("\nUSAGE: wiimote.exe sage_master_ip\n");
    exit(0);
  }
  else if(argc > 2)
    use_sage = 0;  // if another argument is passed, don't connect to sage

  strcpy(sageHost, argv[1]);

  // initiate connections
  connectToSage();
  connectToWiimotes();


#ifdef SHOW_GL_DEBUG_WINDOW
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(640, 480);
  glutCreateWindow("Wiimote 4 dot tracking");
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 1024, 768, 0);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glutDisplayFunc(display);
  glutIdleFunc(update);
  glutMainLoop();
#else
  while(1) {
    update();
  }
#endif

  wiiuse_cleanup(wiimotes, MAX_WIIMOTES);

  return 0;
}
