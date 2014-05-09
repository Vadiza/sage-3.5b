#include "capturewindow.h"

#if defined(__linux__)

#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xfixes.h>

struct x11_grab
{
  int frame_size;          /* Size in bytes of a grabbed frame */
  int height;              /* Height of the grab frame */
  int width;               /* Width of the grab frame */
  int x_off;               /* Horizontal top-left corner coordinate */
  int y_off;               /* Vertical top-left corner coordinate */

  Display *dpy;            /* X11 display from which x11grab grabs frames */
  XImage *image;           /* X11 image holding the grab */
  int use_shm;             /* !0 when using XShm extension */
  XShmSegmentInfo shminfo; /* When using XShm, keeps track of XShm infos */
  int nomouse;
};

struct x11_grab *x11grab;

#endif

#if defined(WIN32)
HWND    hDesktopWnd;
HDC     hDesktopDC;
HDC     hCaptureDC;
HBITMAP hCaptureBitmap;
void* pBits = NULL;
#endif


#if defined(__APPLE__)
#include <sys/utsname.h>
int IsLion = 0;  // Assume it's not MacOSX Lion, by default
CGDirectDisplayID *displays; // displays[] Quartz display ID's
long displaysIndex = 0; // default display

#include "pixfc-sse.h"
struct PixFcSSE *       pixfc;
PixFcPixelFormat        output_format = PixFcUYVY;
PixFcPixelFormat        input_format  = PixFcBGRA;
#endif

// headers for SAGE
#include "libsage.h"

// Compression
#include "libdxt.h"

// headers for SAGE
GLubyte *rgbBuffer = NULL;
GLubyte *yuvBuffer = NULL;
GLubyte *dxtBuffer = NULL;
sail *sageInf      = NULL;


CaptureWindow::CaptureWindow(QWidget *parent) : QMainWindow(parent)
{
  // just making sure
  rgbBuffer = NULL;
  yuvBuffer = NULL;
  dxtBuffer = NULL;
  sageInf   = NULL;

  qDebug() << "Current path: " << QDir::currentPath();
  qDebug() << "App path : " << qApp->applicationDirPath();
  qDebug() << "File path : " << qApp->applicationFilePath();

  dxt_aInitialize();

  // Build the UI
  ui.setupUi(this);
  ui.statusBar->showMessage(QString("Desktop capture"), 0);

  // Disable DXT for now
  ui.checkBox->setEnabled(false);

  // Get dimensions
  QDesktopWidget *desktop = QApplication::desktop();
  WW = desktop->width();
  HH = desktop->height();
  SAGE_PRINTLOG("QSHARE> Desktop width %d height %d", WW, HH);

  // Set private variable
  fps = 25;
  started = false;
  dxt = false;
  count = 0;
  showcursor = true;
  //showcursor = false;
  ui.checkBox_2->setChecked(showcursor);

#if defined(__APPLE__)
  // Determine the OS version
  /////////////////////////
  int ret = 0;
  struct utsname uname_info;

  ret = uname(&uname_info);

  if (ret == 0)
  {
    SAGE_PRINTLOG("Uname Info");
    SAGE_PRINTLOG("Sysname  : %s", uname_info.sysname);
    SAGE_PRINTLOG("Nodename : %s", uname_info.nodename);
    int release = atoi(uname_info.release);
    if (release == 10) { // Snow Leopard
      IsLion = 0;
      SAGE_PRINTLOG("Release  : %s - %d - Snow Leopard", uname_info.release, release);
    }
    if (release > 10) { // Lion or greater
      IsLion = 1;
      switch (release) {
      case 11:
        SAGE_PRINTLOG("Release  : %s - %d - Lion", uname_info.release, release);
        break;
      case 12:
        SAGE_PRINTLOG("Release  : %s - %d - Mountain Lion", uname_info.release, release);
        break;
      case 13:
        SAGE_PRINTLOG("Release  : %s - %d - Mavericks", uname_info.release, release);
        break;
      default:
        SAGE_PRINTLOG("Release  : %s - %d - Unknown", uname_info.release, release);
        break;
      }
    }
    SAGE_PRINTLOG("Version  : %s", uname_info.version);
    SAGE_PRINTLOG("Machine  : %s", uname_info.machine);

    if (strcmp(uname_info.machine, "x86_64") == 0)
      SAGE_PRINTLOG("You are running in 64-bit mode");
    else
      SAGE_PRINTLOG("You are running in 32-bit mode");
  }
  /////////////////////////

  /////////////////////////
  // Get the OpenGL context
  CGLPixelFormatObj pixelFormatObj ;
  GLint numPixelFormats ;
  CGDirectDisplayID displayId = CGMainDisplayID();
  CGRect dRect = CGDisplayBounds( displayId );
  WW = dRect.size.width;
  HH = dRect.size.height;
  qDebug() << "W " << WW << "  H" << HH;

  if (! IsLion) {
    CGOpenGLDisplayMask displayMask = CGDisplayIDToOpenGLDisplayMask(displayId);
    CGLPixelFormatAttribute attribs[] =
      {
        (CGLPixelFormatAttribute)kCGLPFAFullScreen,
        (CGLPixelFormatAttribute)kCGLPFADisplayMask,
        (CGLPixelFormatAttribute)displayMask,
        (CGLPixelFormatAttribute)0
      };
    CGLChoosePixelFormat( attribs, &pixelFormatObj, &numPixelFormats );
    CGLCreateContext( pixelFormatObj, NULL, &glContextObj ) ;
    CGLDestroyPixelFormat( pixelFormatObj ) ;
    CGLSetCurrentContext( glContextObj ) ;
    glReadBuffer(GL_FRONT);
    CGLSetFullScreen( glContextObj ) ;///UUUUUUUUUUnbelievable
    CGLSetCurrentContext( NULL );
  } else {

    CGError     err = CGDisplayNoErr;
    CGDisplayCount    dspCount = 0;

    /* How many active displays do we have? */
    err = CGGetActiveDisplayList(0, NULL, &dspCount);

    displays = nil;

    /* Allocate enough memory to hold all the display IDs we have. */
    displays = (CGDirectDisplayID*)calloc((size_t)dspCount, sizeof(CGDirectDisplayID));

    // Get the list of active displays
    err = CGGetActiveDisplayList(dspCount, displays, &dspCount);

    /* More error-checking here. */
    if(err != CGDisplayNoErr)
    {
      fprintf(stderr, "Could not get active display list (%d)\n", err);
    }

    displaysIndex = 0;
    fprintf(stderr, "display number: %d - %d\n", dspCount, displays[displaysIndex]);

    // Create struct pixfc
    //if (create_pixfc(&pixfc, input_format, output_format, WW, HH, PixFcFlag_BT709Conversion) != 0) {
    if (create_pixfc(&pixfc, input_format, output_format, WW, HH, WW*4, PixFcFlag_BT709Conversion) != 0) {
      fprintf(stderr, "Error creating struct pixfc\n");
    }

  }
  /////////////////////////
#endif

#if defined(__linux__)
  //
  // Initialize x11 frame grabber
  //
  Display *dpy;
  XImage *image;
  int use_shm;

  x11grab = (struct x11_grab*)malloc(sizeof(struct x11_grab));
  memset(x11grab, 0, sizeof(struct x11_grab));

  dpy = XOpenDisplay(0);
  if(!dpy) {
    SAGE_PRINTLOG("QSHARE> Could not open X display");
  }

  use_shm = XShmQueryExtension(dpy);
  SAGE_PRINTLOG("QSHARE> shared memory extension %s found", use_shm ? "" : "not");

  if(use_shm) {
    int scr = XDefaultScreen(dpy);
    image = XShmCreateImage(dpy,
                            DefaultVisual(dpy, scr),
                            DefaultDepth(dpy, scr),
                            ZPixmap,
                            NULL,
                            &x11grab->shminfo,
                            WW, HH);
    SAGE_PRINTLOG("QSHARE> Image: widht %d  height %d bytes_per_line %d, depth %d", image->width,image->height,image->bytes_per_line, DefaultDepth(dpy, scr));
    x11grab->shminfo.shmid = shmget(IPC_PRIVATE,
                                    image->bytes_per_line * image->height,
                                    IPC_CREAT|0777);
    if (x11grab->shminfo.shmid == -1) {
      SAGE_PRINTLOG("QSHARE> Fatal: Can't get shared memory!");
    }
    x11grab->shminfo.shmaddr = image->data = (char*)shmat(x11grab->shminfo.shmid, 0, 0);
    x11grab->shminfo.readOnly = False;

    if (!XShmAttach(dpy, &x11grab->shminfo)) {
      SAGE_PRINTLOG("QSHARE> Fatal: Failed to attach shared memory!");
    }
  } else {
    image = XGetImage(dpy, RootWindow(dpy, DefaultScreen(dpy)),
                      0, 0, WW, HH,
                      AllPlanes, ZPixmap);
  }

  switch (image->bits_per_pixel) {
  case 8:
    SAGE_PRINTLOG("QSHARE> 8 bit palette");
    //input_pixfmt = PIX_FMT_PAL8;
    break;
  case 16:
    if (       image->red_mask   == 0xf800 &&
               image->green_mask == 0x07e0 &&
               image->blue_mask  == 0x001f ) {
      SAGE_PRINTLOG("QSHARE> 16 bit RGB565");
      //input_pixfmt = PIX_FMT_RGB565;
    } else if (image->red_mask   == 0x7c00 &&
               image->green_mask == 0x03e0 &&
               image->blue_mask  == 0x001f ) {
      SAGE_PRINTLOG("QSHARE> 16 bit RGB555");
      //input_pixfmt = PIX_FMT_RGB555;
    } else {
      SAGE_PRINTLOG("QSHARE> RGB ordering at image depth %i not supported ... aborting\n", image->bits_per_pixel);
      SAGE_PRINTLOG("QSHARE> color masks: r 0x%.6lx g 0x%.6lx b 0x%.6lx\n", image->red_mask, image->green_mask, image->blue_mask);
    }
    break;
  case 24:
    if (        image->red_mask   == 0xff0000 &&
                image->green_mask == 0x00ff00 &&
                image->blue_mask  == 0x0000ff ) {
      SAGE_PRINTLOG("QSHARE> 24 bit BGR24");
      //input_pixfmt = PIX_FMT_BGR24;
    } else if ( image->red_mask   == 0x0000ff &&
                image->green_mask == 0x00ff00 &&
                image->blue_mask  == 0xff0000 ) {
      SAGE_PRINTLOG("QSHARE> 24 bit RGB24");
      //input_pixfmt = PIX_FMT_RGB24;
    } else {
      SAGE_PRINTLOG("QSHARE> rgb ordering at image depth %i not supported ... aborting", image->bits_per_pixel);
      SAGE_PRINTLOG("QSHARE> color masks: r 0x%.6lx g 0x%.6lx b 0x%.6lx", image->red_mask, image->green_mask, image->blue_mask);
    }
    break;
  case 32:
    SAGE_PRINTLOG("QSHARE> 32 bit RGB32");
    //input_pixfmt = PIX_FMT_RGB32;
    break;
  default:
    SAGE_PRINTLOG("QSHARE> image depth %i not supported ... aborting", image->bits_per_pixel);
  }
  x11grab->nomouse = 1;
  x11grab->frame_size = WW * HH * image->bits_per_pixel/8;
  x11grab->dpy = dpy;
  x11grab->width = WW;
  x11grab->height = HH;
  x11grab->x_off = 0;
  x11grab->y_off = 0;
  x11grab->image = image;
  x11grab->use_shm = use_shm;
#endif

#if defined(WIN32)
  BITMAPINFO  bmpInfo;
  ZeroMemory(&bmpInfo,sizeof(BITMAPINFO));
  bmpInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  bmpInfo.bmiHeader.biBitCount=24;//BITSPERPIXEL;
  bmpInfo.bmiHeader.biCompression = BI_RGB;
  bmpInfo.bmiHeader.biWidth=WW;
  bmpInfo.bmiHeader.biHeight=HH;
  bmpInfo.bmiHeader.biPlanes=1;
  bmpInfo.bmiHeader.biSizeImage=abs(bmpInfo.bmiHeader.biHeight)*bmpInfo.bmiHeader.biWidth*bmpInfo.bmiHeader.biBitCount/8;

  hDesktopWnd = GetDesktopWindow();
  hDesktopDC = GetDC(hDesktopWnd);
  hCaptureDC = CreateCompatibleDC(hDesktopDC);
  hCaptureBitmap = CreateDIBSection(hDesktopDC,&bmpInfo,DIB_RGB_COLORS,&pBits,NULL,0);
  SelectObject(hCaptureDC,hCaptureBitmap);
#endif

  // Enables buttons
  ui.pushButton->setEnabled(true);
  ui.pushButton_2->setEnabled(false);
  ui.pushButton_3->setEnabled(false);

  // Load the pointer
#if defined(__APPLE__)
  if ( IsLion )
    cursor_icon = new QImage(":/arrow3.png");
  else
    cursor_icon = new QImage(":/arrow2.png");
#else
  cursor_icon = new QImage(":/arrow2.png");
#endif
  if (cursor_icon->isNull())
    SAGE_PRINTLOG("QSHARE> Couldn't load cursor_icon image");

  // Load SAIL config
  sailConfig scfg;
  scfg.init((char*)"qshare.conf");

  fsip = QString(scfg.fsIP);
  qDebug() << "FSIP " << fsip;
  ui.lineEdit->setText(fsip);

  // Timer setup
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));

#if defined(__linux__)
  // temporary solution to auto-start and hide on linux
  onStart();
#endif
}

void CaptureWindow::closeEvent(QCloseEvent *event)
{
#if defined(__linux__)
  //
  // Close x11 frame grabber
  //

  // Detach cleanly from shared mem
  // if (x11grab && x11grab->use_shm) {
  //  XShmDetach(x11grab->dpy, &x11grab->shminfo);
  //  shmdt(x11grab->shminfo.shmaddr);
  //  shmctl(x11grab->shminfo.shmid, IPC_RMID, NULL);
  // }

  // Destroy X11 image
  // if (x11grab && x11grab->image) {
  //  XDestroyImage(x11grab->image);
  //  x11grab->image = NULL;
  // }

  // Free X11 display
  // if (x11grab)
  //  XCloseDisplay(x11grab->dpy);
#endif
  onStop();
  event->accept();
}

void CaptureWindow::frameRate(int f)
{
  fps = f;
  qDebug() << "framerate " << fps;
  timer->setInterval(1000/fps);
}


void CaptureWindow::fsIP(QString f)
{
  fsip = f;
  qDebug() << "FSIP changed" << fsip;
}

void CaptureWindow::update()
{
  if (sageInf && started) {

    // Capture the desktop pixels
#if defined(__APPLE__)
    if ( IsLion ) {
      yuvBuffer = nextBuffer(sageInf);
      capture((char*)yuvBuffer,0,0,WW,HH);
    }
    else {
      rgbBuffer = nextBuffer(sageInf);
      capture((char*)rgbBuffer,0,0,WW,HH);
    }
#else
    rgbBuffer = nextBuffer(sageInf);
    capture((char*)rgbBuffer,0,0,WW,HH);
#endif

    if (dxt)
    {
      dxtBuffer = nextBuffer(sageInf);
      CompressDXT(rgbBuffer, dxtBuffer, WW, HH, FORMAT_DXT1, 1);
    }

    swapBuffer(sageInf);


    processMessages(sageInf,NULL,NULL,NULL);

    QString str;
    double nowt = dxt_aTime();
    //qDebug() << "now " << nowt << "   startt" << startt;
    double dfps = 1.0 / (nowt - startt);
    str = QString("%1 ms / %2 fps/ # %3 / DXT %4 / %5x%6").arg(nowt-startt,5,'f',1).arg(dfps,5,'f',1).arg(count++).arg(dxt).arg(WW).arg(HH);
    ui.statusBar->showMessage( str, 0 );

#if 1
    if (dfps < fps) {
      timer->start( MAX(0, (2*(1000/fps)-(1000/dfps) ) ) );
    }
    else
      timer->start(1000/fps);
#else
    timer->start(0);
#endif
    startt = nowt;
  }
}

bool CaptureWindow::screenHasCursor()
{
#if defined(__linux__)
  int root_x, root_y;
  int win_x, win_y;
  unsigned int mask;
  Window root, child;

  return  XQueryPointer(x11grab->dpy,
                        RootWindow(x11grab->dpy, DefaultScreen(x11grab->dpy)),
                        &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
#else
  return true;
#endif
}

void CaptureWindow::drawCursor(char *pixels, int nbytes, int flip)
{
  if (showcursor && screenHasCursor()) {
    // Get mouse position
    QPoint pt = QCursor::pos();
    int startx, starty;
    //SAGE_PRINTLOG("Cursor at: %dx%d", pt.x(), pt.y());

    if (flip) {
      // Flip the Y value
      int adjust = HH - pt.y();
      if (adjust < 0) adjust = 0;
      pt.setY(adjust);

      // Compute boundaries
      startx = MAX(0,pt.x());
      // int starty = MAX(0, pt.y()-cursor_icon->height());
      starty = pt.y()-cursor_icon->height();
    } else {
      startx = pt.x();
      starty = pt.y();
    }

    // Compose desktop with cursor
    QImage resultImage;
    if (nbytes == 4)
      resultImage = QImage((uchar*)pixels, WW, HH, QImage::Format_ARGB32);
    else
      resultImage = QImage((uchar*)pixels, WW, HH, QImage::Format_RGB888);
    QPainter painter(&resultImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(startx, starty, *cursor_icon);
    painter.end();
    // pixels buffer is filled after that point
  }
}

void CaptureWindow::capture(char* m_pFrameRGB,int x,int y,int cx,int cy)
{
#if defined(__APPLE__)
  if ( IsLion ) {
    // Get an Image from display
    CGImageRef image = CGDisplayCreateImage(displays[displaysIndex]);
    // Get the data
    CFDataRef dref = CGDataProviderCopyData(CGImageGetDataProvider(image));
    // Copy everything
    CFDataGetBytes (dref, CFRangeMake(0,CFDataGetLength(dref)), (UInt8*)rgbBuffer);

    drawCursor((char*)rgbBuffer, 4, 0);

    // Do conversion
    pixfc->convert(pixfc, rgbBuffer, m_pFrameRGB);

    // Free stuff
    CFRelease(dref);
    CFRelease(image);
  } else {
    CGLSetCurrentContext( glContextObj ) ;
    //CGLSetFullScreen( glContextObj ) ;
    //glReadBuffer(GL_FRONT);

    if (dxt) {
      glReadPixels(x,y,cx,cy,GL_RGBA,GL_UNSIGNED_BYTE,m_pFrameRGB);
      drawCursor(m_pFrameRGB, 4);
    }
    else {
      glReadPixels(x,y,cx,cy,GL_RGB,GL_UNSIGNED_BYTE,m_pFrameRGB);
      drawCursor(m_pFrameRGB, 3);
    }

    CGLSetCurrentContext( NULL );
  }
#endif

#if defined(__linux__)
  if (x11grab->use_shm) {
    if (!XShmGetImage(x11grab->dpy, RootWindow(x11grab->dpy, DefaultScreen(x11grab->dpy)),
                      x11grab->image, 0, 0, AllPlanes)) {
      SAGE_PRINTLOG("QSHARE> XShmGetImage() failed");
    }
    else {
      // Upside-down and RGBA-to-RGB conversion
      for (int i = 0 ; i < cy; i++) {
        for (int j = 0 ; j < cx; j++) {
          m_pFrameRGB [ (i*cx+j) * 3 + 0 ] = x11grab->image->data [ ((cy-i-1)*cx+j) * 4 + 2 ];
          m_pFrameRGB [ (i*cx+j) * 3 + 1 ] = x11grab->image->data [ ((cy-i-1)*cx+j) * 4 + 1 ];
          m_pFrameRGB [ (i*cx+j) * 3 + 2 ] = x11grab->image->data [ ((cy-i-1)*cx+j) * 4 + 0 ];
        }
      }
      drawCursor(m_pFrameRGB, 3);
    }
  }
#endif

#if defined(WIN32)
  BitBlt(hCaptureDC,0,0,WW,HH,hDesktopDC,0,0,SRCCOPY|CAPTUREBLT);
  memcpy(m_pFrameRGB, pBits, WW*HH*3);
  drawCursor(m_pFrameRGB, 3);
#endif
}

void CaptureWindow::onStart()
{
  if (! started) {
    ui.pushButton->setEnabled(false);
    ui.pushButton_2->setEnabled(true);
    ui.pushButton_3->setEnabled(true);
    ui.lineEdit->setEnabled(false);
    ui.spinBox->setEnabled(true);
    ui.checkBox->setEnabled(false);

    qDebug() << "On Start";

    int roworder;
    enum sagePixFmt pformat;
    pformat  = PIXFMT_888;
    roworder = BOTTOM_TO_TOP;

    if (dxt)
      pformat = PIXFMT_DXT;
    else
#if defined(WIN32)
      pformat = PIXFMT_888_INV; // for some reasons, win32 is BGR
#else
    pformat = PIXFMT_888;
#endif

#if defined(__APPLE__)
    if ( IsLion ) {
      pformat = PIXFMT_YUV;
      roworder = TOP_TO_BOTTOM;
    }
    else
      pformat = PIXFMT_888;
#endif

    // Copy back the text box into the SAGE variable
    char fsIP[SAGE_IP_LEN];
    memset(fsIP, 0, SAGE_IP_LEN);
    //strncpy(fsIP, fsip.toAscii().constData(), SAGE_IP_LEN);
    strncpy(fsIP, fsip.toLatin1().constData(), SAGE_IP_LEN);

    // Create the SAIL object
    sageInf = createSAIL("qshare", WW, HH, pformat, fsIP, roworder);

    if (dxt) {
      if (rgbBuffer) delete [] rgbBuffer;
      rgbBuffer = (byte*)memalign(16, WW*HH*4);
      memset(rgbBuffer, 0,  WW*HH*4);
    }
    else {

#if defined(__APPLE__)
      if ( IsLion ) {
        rgbBuffer = (byte*)memalign(16, WW*HH*4);
        memset(rgbBuffer, 0,  WW*HH*4);
      }
#endif
    }

    timer->setSingleShot(true);
    timer->start(1000/fps);
    //timer->start();

    startt = dxt_aTime();

    started = true;
  }
}

void CaptureWindow::compression(int c)
{
  dxt = c;
}

void CaptureWindow::cursor(int c)
{
  showcursor = c;
}

void CaptureWindow::onStop()
{
  qDebug() << "On Stop";
  started = false;
  timer->stop();
  ui.pushButton->setEnabled(true);
  ui.pushButton_2->setEnabled(false);
  ui.pushButton_3->setEnabled(false);
  ui.lineEdit->setEnabled(true);
  ui.spinBox->setEnabled(true);
  ui.checkBox->setEnabled(true);

  if (sageInf && started) {
    //deleteSAIL(sageInf);
    sageInf->shutdown();
  }
}

void CaptureWindow::onPause()
{
  if (started) {
    qDebug() << "Pause";
    started = false;
    timer->stop();
  }
  else {
    qDebug() << "UnPause";
    started = true;
    timer->start(1000/fps);
  }
}


/////////////////////////////////////////////////////////////////////////
