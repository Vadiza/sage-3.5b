/*****************************************************************************************
 * VNCViewer for SAGE
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
 * Direct questions, comments etc about VNCViewer for SAGE to www.evl.uic.edu/cavern/forum
 *****************************************************************************************/

#if defined(USE_QT)
#include <QImage>
#include <QPainter>
#endif

#if defined(USE_LIBVNC)
#include <rfb/rfbclient.h>
#else
#include "sgVNCViewer.h"
#include <sys/fcntl.h>
#endif

// headers for SAGE
#include "sail.h"
#include "misc.h"
#include <time.h>
#include <unistd.h>

// Global variables
sail sageInf;        // sail object
char *username = NULL;
char *passwd = NULL;
char *server = NULL;
double rate = 10.0;  // by default, try to stream at 10fps
int winWidth, winHeight, display;

#if defined(USE_LIBVNC)
rfbClient *vnc;
#else
sgVNCViewer *vnc;
#endif

bool Done = false;

typedef struct ButtonMapping
{
  int sage;
  int rfb;
} ButtonMapping_t;

ButtonMapping_t buttonMapping[] =
  {
    {1, rfbButton1Mask},
    {2, rfbButton3Mask},
    {3, rfbButton2Mask},
    // {4, rfbButton4Mask},
    // {5, rfbButton5Mask},
    {0, 0}
  };
int buttonMask;

// Getopt variables
//extern char *optarg;
//extern int optind, optopt;


#if defined(USE_WIDGETS)
//SAGE widget library
#include "appWidgets.h"
#endif

#if defined(USE_LIBVNC)

static rfbBool got_data = FALSE;
static void signal_handler(int signal)
{
  rfbClientLog("Cleaning up.\n");
}

static rfbBool resize_func(rfbClient *client)
{
  static rfbBool first = TRUE;
  if (!first)
  {
    rfbClientLog("I don't know yet how to change resolutions!\n");
    exit(1);
  }
  signal(SIGINT, signal_handler);

  int width = client->width;
  int height = client->height;
  int  depth = client->format.bitsPerPixel;
  client->updateRect.x = client->updateRect.y = 0;
  client->updateRect.w = width; client->updateRect.h = height;

  client->frameBuffer = (uint8_t *)malloc(width * height * depth);
  memset(client->frameBuffer, 0, width * height * depth);

  rfbClientLog("Allocate %d bytes: %d x %d x %d\n", width * height * depth, width, height, depth);
  return TRUE;
}

static void frame_func(rfbClient *client)
{
  //rfbClientLog("Received a frame\n");
  static double lastUpdate = sage::getTime() - 1000000.0;
  // Wait to get the correct frame rate (only if needed)
  double now = sage::getTime();
  double fps = (1000000.0 / (now - lastUpdate));
  //SAGE_PRINTLOG("VNC> rate %g", fps);
  if (fps > rate)
  {
    unsigned long sl = (1000000.0 / rate) - (now - lastUpdate);
    //SAGE_PRINTLOG("VNC> have to delay [%g/%g]: %lu", fps, rate, sl);
    //sage::usleep( sl );
    // skip the frame
    return;
  }
  lastUpdate = sage::getTime();

  // Copy VNC buffer into SAGE buffer
  unsigned char *buffer = 0;
  unsigned char *vncpixels;
  buffer    = (unsigned char *) sageInf.getBuffer();
  vncpixels = (unsigned char *) client->frameBuffer;

  for (int k = 0 ; k < winWidth * winHeight; k++)
  {
    buffer[3 * k + 0] = vncpixels[ 4 * k + 0];
    buffer[3 * k + 1] = vncpixels[ 4 * k + 1];
    buffer[3 * k + 2] = vncpixels[ 4 * k + 2];
  }

  // SAGE Swap
  sageInf.swapBuffer( );


}

static rfbBool position_func(rfbClient *client, int x, int y)
{
  //rfbClientLog("Received a position for %d,%d\n",x,y);
  return TRUE;
}

static char *password_func(rfbClient *client)
{
  if (passwd)
  {
    char *str = (char *)malloc(64);
    memset(str, 0, 64);
    strncpy(str, passwd, 64);
    return str;
  }
  else
    return NULL;
}

rfbCredential *getcred(struct _rfbClient *client, int credentialType)
{
  rfbCredential *res = (rfbCredential *)malloc(sizeof(rfbCredential));
  if (username)
    res->userCredential.username = strdup(username);
  else
    res->userCredential.username = NULL;
  if (passwd)
    res->userCredential.password = strdup(passwd);
  else
    res->userCredential.password = NULL;

  return res;
}


static void update_func(rfbClient *client, int x, int y, int w, int h)
{
  rfbPixelFormat *pf = &client->format;
  int bpp = pf->bitsPerPixel / 8;
  int row_stride = client->width * bpp;

  got_data = TRUE;

  //rfbClientLog("Received an update for %d,%d,%d,%d.\n",x,y,w,h);
}


#endif

#if defined(USE_WIDGETS)
void forwardFunc(int eventId, button *btnObj)
{
  if (eventId == BUTTON_UP)
  {
    SAGE_PRINTLOG("SAGE: forwardFunc");
    SendKeyEvent(vnc, XK_Right, TRUE); // true: on key down
    SendKeyEvent(vnc, XK_Right, FALSE);
  }
}
void backwardFunc(int eventId, button *btnObj)
{
  if (eventId == BUTTON_UP)
  {
    SAGE_PRINTLOG("SAGE: backwardFunc");
    SendKeyEvent(vnc, XK_Left, TRUE); // true: on key down
    SendKeyEvent(vnc, XK_Left, FALSE);
  }
}

void pauseFunc(int eventId, button *btnObj)
{
  if (eventId == BUTTON_UP)
  {
    SAGE_PRINTLOG("SAGE: SPACE");
    SendKeyEvent(vnc, XK_space, TRUE);
    SendKeyEvent(vnc, XK_space, FALSE); // false: on key up
  }
}
#endif



void *msgThread(void *args)
{
  int Gmx = 0;
  int Gmy = 0;

  while (! Done )
  {
    // Process SAGE messages
    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0)
    {

      // Click event x and y location normalized to size of window
      float clickX, clickY;
      float dX, dY;
      int mx, my, buttonMask, i, state;
      // Ckick device Id, button Id, and is down flag
      int clickDeviceId, clickButtonId, clickIsDown, clickEvent;
      int keycode;

      // SAGE_PRINTLOG("VNC> message %d", msg.getCode());

      // Get SAGE message data
      char *data = (char *) msg.getData();
      int code   = msg.getCode();
      // Determine message
      switch (code)
      {
      case EVT_KEY:
        sscanf(data, "%d %f %f %d", &clickDeviceId, &clickX, &clickY, &keycode);
        SAGE_PRINTLOG("VNC> EVT_KEY %d", keycode);
        SendKeyEvent(vnc, keycode, TRUE);
        break;

      case EVT_DOUBLE_CLICK:
        SAGE_PRINTLOG("VNC> EVT_DOUBLE_CLICK");
        sscanf(data, "%d %f %f %d %d %d",
               &clickDeviceId, &clickX, &clickY,
               &clickButtonId, &clickIsDown, &clickEvent);
        mx = int ( clickX * vnc->width );
        my = int ( (1 - clickY) * vnc->height );

        // Simulate a double click (with left button)
        buttonMask = rfbButton1Mask;
        SendPointerEvent(vnc, mx, my, buttonMask); // down
        buttonMask = 0;
        buttonMask &= ~rfbButton1Mask;
        SendPointerEvent(vnc, mx, my, buttonMask); // up

        buttonMask = rfbButton1Mask;
        SendPointerEvent(vnc, mx, my, buttonMask); // down
        buttonMask = 0;
        buttonMask &= ~rfbButton1Mask;
        SendPointerEvent(vnc, mx, my, buttonMask); // up
        break;

        // Mouse move event
      case EVT_MOVE:
        // Click event
      case EVT_CLICK:
      case EVT_PAN:     // drag + button1
      case EVT_ROTATE:  // drag + button2
      case EVT_ZOOM:    // drag + button3

        if (code == EVT_MOVE)
        {
          //SAGE_PRINTLOG("VNC> move");
          sscanf(data, "%d %f %f %f %f",
                 &clickDeviceId, &clickX, &clickY, &dX, &dY);
        }
        else if (code == EVT_PAN || code == EVT_ROTATE || code == EVT_ZOOM)
        {
          // Pan event x and y location and change in x, y and z direction
          // normalized to size of window
          float panDX, panDY, panDZ;
          sscanf(data,
                 "%d %f %f %f %f %f",
                 &clickDeviceId, &clickX, &clickY,
                 &panDX, &panDY, &panDZ);

        }
        else if (code == EVT_CLICK)
        {
          sscanf(data, "%d %f %f %d %d %d",
                 &clickDeviceId, &clickX, &clickY,
                 &clickButtonId, &clickIsDown, &clickEvent);
          SAGE_PRINTLOG("VNC> EVT_CLICK %d", clickButtonId);
          for (i = 0; buttonMapping[i].sage; i++)
          {
            if (clickButtonId == buttonMapping[i].sage)
            {
              SAGE_PRINTLOG("VNC> clickButtonId %d", clickButtonId);
              state = buttonMapping[i].rfb;
              if (clickIsDown)
                buttonMask |= state;
              else
                buttonMask &= ~state;
              break;
            }
          }
        }

        // Print out the message
        // SAGE_PRINTLOG("EVT_MOVE> Device %d X %f Y %f dX %f dY %f",
        //     clickDeviceId, clickX, clickY, dX, dY);
        mx = int ( clickX * vnc->width );
        my = int ( (1 - clickY) * vnc->height );

        SendPointerEvent(vnc, mx, my, buttonMask);
        buttonMask &= ~(rfbButton4Mask | rfbButton5Mask);


        // if (mx != Gmx || my != Gmy)
        // {
        //     buttonMask = 0;
        //     SAGE_PRINTLOG("EVT_MOVE: %d x %d ", mx, my);
        //     SendPointerEvent(vnc, mx, my, buttonMask);
        //     Gmx = mx;
        //     Gmy = my;
        // }

        // Print out the message
        // SAGE_PRINTLOG("EVT_CLICK> Device %d X %f Y %f Button %d isDown %d event %d",
        //     clickDeviceId, clickX, clickY,
        //     clickButtonId, clickIsDown, clickEvent);
        // EVT_CLICK> Device -1 X 0.536449 Y 0.466600 Button 1 isDown 0 event 31003

        // if (clickIsDown)
        // {
        //     mx = int ( clickX * vnc->width );
        //     my = int ( (1 - clickY) * vnc->height );
        //     buttonMask = rfbButton1Mask;
        //     SAGE_PRINTLOG("EVT_CLICK: %d x %d mask %d", mx, my, buttonMask);
        //     SendPointerEvent(vnc, mx, my, buttonMask);
        // }

        // // If up event
        // if (clickIsDown == 0)
        // {
        // }

        // Done with EVT_CLICK case
        // Done with EVT_MOVE case
        break;

      case APP_QUIT:
        SAGE_PRINTLOG("VNC> APP_QUIT");
        Done = true;
        return NULL;
        break;

      default:
        SAGE_PRINTLOG("Event: %d", code);
        break;
      }
    }
  }

  return NULL;
}


int
main(int argc, char **argv)
{
  double lastcursorupdate = 0.0;
  int cx = -1;
  int cy = -1;
  int listenmode = 0;

  ////////////////////////
  int errflg = 0;
  int c;
  while ((c = getopt(argc, argv, "lhs:d:p:u:r:")) != -1)
  {
    switch (c)
    {
    case 's': // hostname of server
      server = optarg;
      break;
    case 'd': // display number of server
      display = atoi(optarg);
      break;
    case 'r': // update frame rate
      rate = atof(optarg);
      break;
    case 'p': // password (vnc or login)
      passwd = optarg;
      break;
    case 'u': // username (login)
      username = optarg;
      break;
    case 'l':
      listenmode = 1;
      break;
    case 'h':
      errflg++;
      break;
    case '?':
      fprintf(stderr, "Unrecognized option: -%c\n", optopt);
      errflg++;
      break;
    default:
      errflg++;
      break;
    }
  }
  if (errflg)
  {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s\n", argv[0]);
    fprintf(stderr, "\t -l : listen mode (server initiates the connection\n");
    fprintf(stderr, "\t -s [hostname]: machine runnign the VNC server (required)\n");
    fprintf(stderr, "\t -d [display number]: X11 display number (default: 0)\n");
    fprintf(stderr, "\t -r [frame rate]: maximum frame rate (default: 10.0)\n");
    fprintf(stderr, "\t -p [password]: vnc password or login password with username] (required)\n");
    fprintf(stderr, "\t -u [username]: user name (for server requiring login) (optional)\n");
    fprintf(stderr, "\t -h : this help\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "example:\t %s -s 127.0.0.1 -p mypasswd (vnc usage)\n", argv[0]);
    fprintf(stderr, "\t\t %s -s 127.0.0.1 -u toto -p mypasswd (login usage, like MacOSX Lion)\n", argv[0]);
    fprintf(stderr, "\n");
    exit(2);
  }
  ////////////////////////

  /*
  // Old command line parameters
  // VNC Init
  if (argc < 4)
  {
  SAGE_PRINTLOG("Usage> VNCviewer <hostname> <display#> <password> [fps]\n");
  exit(0);
  }
  else if (argc  == 4) {
  // VNCviewer hostname display# password
  server = strdup(argv[1]);
  display = atoi(argv[2]);
  passwd  = argv[3];
  }
  else if (argc == 5) {
  // VNCviewer hostname display# password fps
  server = strdup(argv[1]);
  display = atoi(argv[2]);
  passwd  = argv[3];
  rate = atof(argv[4]);
  }
  else if (argc == 6) {
  // VNCviewer hostname display# width height password
  //     ignore width and height
  server = strdup(argv[1]);
  display = atoi(argv[2]);
  passwd  = argv[5];
  }
  else if (argc == 7) {
  // VNCviewer hostname display# width height password fps
  //     ignore width and height
  server = strdup(argv[1]);
  display = atoi(argv[2]);
  passwd  = argv[5];
  rate = atof(argv[6]);
  }
  */


  // SAGE Init
  sage::initUtil();

#if defined(USE_LIBVNC)
  // get a vnc client structure (don't connect yet).
  // bits, channels, bytes
  vnc = rfbGetClient(8, 3, 4);
  // to get position update callbacks
  vnc->appData.useRemoteCursor = TRUE;

  //vnc->appData.compressLevel=8;
  //vnc->appData.qualityLevel=2;

  /* open VNC connection */
  vnc->canHandleNewFBSize = FALSE;
  vnc->MallocFrameBuffer = resize_func;
  vnc->GotFrameBufferUpdate = update_func;
  vnc->HandleCursorPos = position_func;
  vnc->GetPassword = password_func;


  if (username && passwd)
  {
    vnc->GetCredential = getcred;
    const uint32_t authSchemes[] = { rfbARD, rfbVncAuth, rfbNoAuth};
    int num_schemes = 3;
    SetClientAuthSchemes(vnc, authSchemes, num_schemes);
  }
  else
  {
    const uint32_t authSchemes[] = { rfbVncAuth, rfbNoAuth};
    int num_schemes = 2;
    SetClientAuthSchemes(vnc, authSchemes, num_schemes);
  }

  vnc->FinishedFrameBufferUpdate = frame_func;

  if (listenmode)
  {
    vnc->listenPort = LISTEN_PORT_OFFSET;
    SAGE_PRINTLOG("VNC listening at port [%d]", vnc->listenPort);
    listenForIncomingConnections(vnc);
  }

  int margc = 2;
  char *margv[2];
  margv[0] = strdup("vnc");
  margv[1] = (char *)malloc(256);
  memset(margv[1], 0, 256);
  sprintf(margv[1], "%s:%d", server, display);
  if (!rfbInitClient(vnc, &margc, margv))
  {
    printf("usage: %s server:port password\n"
           "VNC client.\n", argv[0]);
    exit(1);
  }
  if (vnc->serverPort == -1)
    vnc->vncRec->doNotSleep = TRUE; /* vncrec playback */

  winWidth  = vnc->width;
  winHeight = vnc->height;
#else

  // Connection to VNC server:
  //   host, display number, passwd, width, height
  //   width and height are return with the size of the desktop
  vnc = new sgVNCViewer(server, display, passwd, winWidth, winHeight);
#endif

  SAGE_PRINTLOG("VNCViewer> Server [%s] display %d resolution %dx%d",
                server, display, winWidth, winHeight);
  SAGE_PRINTLOG("VNCViewer> polling at %.0f fps", rate);


#if defined(USE_WIDGETS)
  // SAGE Widgets
  ///////////////
  double mult = 1.5;
  double bw = 80 * winWidth / 1920;  // proportional to image width

  thumbnail *pauseBtn = new thumbnail;
  pauseBtn->setUpImage("images/mplayer-icons/mplayer-pause.png");
  pauseBtn->setSize(bw, bw);
  pauseBtn->setScaleMultiplier(mult);
  pauseBtn->onUp(&pauseFunc);
  pauseBtn->setTransparency(140);
  pauseBtn->align(CENTER, BOTTOM);
  // pauseBtn->setToggle(true);

  thumbnail *stepfBtn = new thumbnail;
  stepfBtn->setUpImage("images/mplayer-icons/mplayer-step.png");
  stepfBtn->setSize(bw, bw);
  stepfBtn->setScaleMultiplier(mult);
  stepfBtn->onUp(&forwardFunc);
  stepfBtn->setTransparency(140);

  thumbnail *stepbBtn = new thumbnail;
  stepbBtn->setUpImage("images/mplayer-icons/mplayer-stepback.png");
  stepbBtn->setSize(bw, bw);
  stepbBtn->setScaleMultiplier(mult);
  stepbBtn->onUp(&backwardFunc);
  stepbBtn->setTransparency(140);

  sizer *asizer = new sizer(HORIZONTAL);
  asizer->align(CENTER, BOTTOM);
  asizer->addChild(stepbBtn);
  asizer->addChild(pauseBtn);
  asizer->addChild(stepfBtn);
  sageInf.addWidget(asizer);
#endif

  // Sage Init
  sailConfig scfg;


  // Search for a configuration file
  char *tmpconf = getenv("SAGE_APP_CONFIG");
  if (tmpconf)
  {
    SAGE_PRINTLOG("VNCViewer> found SAGE_APP_CONFIG variable: [%s]", tmpconf);
    scfg.init(tmpconf);
  }
  else
  {
    SAGE_PRINTLOG("VNCViewer> using default VNCViewer.conf");
    scfg.init((char *)"VNCViewer.conf");
  }

  scfg.setAppName((char *)"VNCViewer");
  scfg.rank = 0;

  // Rendering resolution
  scfg.resX = winWidth;
  scfg.resY = winHeight;

  // Display resolution
  // if it hasn't been specified by the config file, use the app-determined size
  if (scfg.winWidth == -1 || scfg.winHeight == -1)
  {
    scfg.winWidth = winWidth;
    scfg.winHeight = winHeight;
  }

  sageRect renderImageMap;
  renderImageMap.left = 0.0;
  renderImageMap.right = 1.0;
  renderImageMap.bottom = 0.0;
  renderImageMap.top = 1.0;

  scfg.imageMap = renderImageMap;
#if defined(USE_QT)
  scfg.pixFmt   = PIXFMT_888_INV;
#else
  scfg.pixFmt   = PIXFMT_888;
#endif
  scfg.rowOrd   = TOP_TO_BOTTOM;
  scfg.master   = true;

  sageInf.init(scfg);


  // data pointer
  //unsigned char *buffer = 0;
  //unsigned char *vncpixels;

  double lastUpdate, now, fps;
  lastUpdate = sage::getTime();

#if defined(USE_QT)
  data_path path;
  std::string found = path.get_file("arrow.png");
  if (found.empty())
  {
    SAGE_PRINTLOG("VNCViewer: cannot find the image [%s]", "arrow.png");
  }
  const char *imPath = found.c_str();

  QImage *load_icon = new QImage(imPath);
  QImage cursor_icon = load_icon->mirrored(false, true);
#endif


  // Threaded message processing
  pthread_t thId;
  if (pthread_create(&thId, 0, msgThread, NULL) != 0)
  {
    SAGE_PRINTLOG("VNC> Cannot create message thread");
    return -1;
  }


  // Main loop
  while (!Done)
  {
#if defined(USE_LIBVNC)

    /*
      now = sage::getTime();
      while ( (sage::getTime() - now) < (1000000/rate)) {
      int i=WaitForMessage(vnc,100000);
      if(i<0) {
      rfbClientLog("VNC error, quit\n");
      sageInf.shutdown();
      exit(0);
      }
      if(i) {
      if(!HandleRFBServerMessage(vnc)) {
      rfbClientLog("HandleRFBServerMessage quit\n");
      sageInf.shutdown();
      exit(0);
      }
      }
      }
    */
    int i = WaitForMessage(vnc, 100);
    if (i < 0)
    {
      rfbClientCleanup(vnc);
      break;
    }
    if (i)
      if (!HandleRFBServerMessage(vnc))
      {
        rfbClientCleanup(vnc);
        break;
      }


    /*
    // Copy VNC buffer into SAGE buffer
    buffer    = (unsigned char *) sageInf.getBuffer();
    vncpixels = (unsigned char *) vnc->frameBuffer;

    for (int k =0 ; k<winWidth*winHeight; k++) {
    buffer[3*k + 0] = vncpixels[ 4*k + 0];
    buffer[3*k + 1] = vncpixels[ 4*k + 1];
    buffer[3*k + 2] = vncpixels[ 4*k + 2];
    }

    // SAGE Swap
    sageInf.swapBuffer( );

    // Process SAGE messages
    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0) {
    switch (msg.getCode()) {
    case APP_QUIT:
    Done = true;
    sageInf.shutdown();
    exit(0);
    break;
    }
    }
    */

    // Wait to get the correct frame rate (only if needed)
    //now = sage::getTime();
    //fps = (1000000.0/(now-lastUpdate));
    //SAGE_PRINTLOG("VNC> rate %g", fps);
    //if (fps > rate) {
    //unsigned long sl = (1000000.0/rate) - (now-lastUpdate);
    //SAGE_PRINTLOG("VNC> have to delay [%g/%g]: %lu\n", fps, rate, sl);
    //sage::usleep( sl );
    //}
    //lastUpdate = sage::getTime();

#else

    // Process VNC messages
    if (!vnc->Step())
    {
      sageInf.shutdown();
      exit(0);
    }

    // Copy VNC buffer into SAGE buffer
    buffer    = (unsigned char *) sageInf.getBuffer();
    vncpixels = (unsigned char *) vnc->Data();


#if defined(USE_QT)
    // Compose desktop with cursor
    QImage resultImage = QImage((uchar *)buffer, winWidth, winHeight, QImage::Format_RGB888);
    QImage vncImage = QImage((uchar *)vncpixels, winWidth, winHeight, QImage::Format_RGB32);

    QPainter painter(&resultImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(0, 0, vncImage);
    if ( (vnc->CursorX() >= 0) &&
         (vnc->CursorY() >= 0) )
    {
      // If position different, draw and update last update time
      if ( (cx != vnc->CursorX()) || (cy != vnc->CursorY()) )
      {
        cx = vnc->CursorX();
        cy = vnc->CursorY();
        lastcursorupdate = sage::getTime();
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(vnc->CursorX(), vnc->CursorY(), cursor_icon);
      }
      else
      {
        // Disable cursor after 3 sec of non-movement
        if ( (sage::getTime() - lastcursorupdate) < 3000000 )
        {
          painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
          painter.drawImage(vnc->CursorX(), vnc->CursorY(), cursor_icon);
        }
      }
    }
    painter.end();
#else
    for (int k = 0 ; k < winWidth * winHeight; k++)
    {
      buffer[3 * k + 0] = vncpixels[ 4 * k + 0];
      buffer[3 * k + 1] = vncpixels[ 4 * k + 1];
      buffer[3 * k + 2] = vncpixels[ 4 * k + 2];
    }
#endif


    // SAGE Swap
    sageInf.swapBuffer( );

    // Process SAGE messages
    sageMessage msg;
    if (sageInf.checkMsg(msg, false) > 0)
    {
      switch (msg.getCode())
      {
      case APP_QUIT:
        Done = true;
        sageInf.shutdown();
        exit(0);
        break;
      }
    }

    // Wait to get the correct frame rate (only if needed)
    now = sage::getTime();
    fps = (1000000.0 / (now - lastUpdate));
    if (fps > rate)
    {
      sage::usleep( (1000000.0 / rate) - (now - lastUpdate)  );
    }
    lastUpdate = sage::getTime();

#endif


  }

  sageInf.shutdown();
  _exit(0);

  return 1;
}
