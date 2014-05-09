#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>


#if defined(USE_POPPLER)

#if SAGE_POPPLER_VERSION == 5
// Centos 5.5
#include "glib/poppler.h"
PopplerDocument *document;
PopplerBackend backend;
PopplerPage *page;
GEnumValue *enum_value;
GError *error;
GdkPixbuf *pixbuf;
#endif

#if SAGE_POPPLER_VERSION >= 12
#include "poppler/goo/GooString.h"
#include "poppler/GlobalParams.h"
#include "poppler/PDFDoc.h"
#include "poppler/UnicodeMap.h"
#include "poppler/PDFDocEncoding.h"
#include "poppler/DateInfo.h"
#include "poppler/splash/SplashBitmap.h"
#include "poppler/splash/Splash.h"
#include "poppler/SplashOutputDev.h"
// Rendering object
SplashOutputDev *splashOut;
PDFDoc *doc;
#endif

double x_resolution;
double pg_w, pg_h;

#else
#include <wand/magick-wand.h>
#endif


// headers for SAGE
#include "libsage.h"

#include "appWidgets.h"

// for dxt compression
#include "libdxt.h"

// Declarations
void makeWidgets(sail *sgi);

// One instance of the application
class App
{
public:
  int   pg_index;
  sail *sageInf;
  application_update_t up;
  volatile bool ready;

  // pixels
  byte *dxt, *rgba;

public:
  App()  { sageInf=NULL;pg_index=0;rgba=NULL;dxt=NULL;ready=false;};
  ~App() { if (sageInf) deleteSAIL(sageInf); };
};

volatile bool needApp = false;

// cache LRU: keeps a few pages indexed by page number

class Pixels
{
public:
  byte *p;
  int  idx;
public:
  Pixels() {       p=NULL;idx=-1;};
  Pixels(int n) {  p=new byte[n];};
  ~Pixels(){       delete []p;idx=-1;};
};

// Cache the pixels of 10 pages
#define PDF_CACHE_SIZE 10
Pixels *cache[PDF_CACHE_SIZE];

// Quit and sync functions
void qfunc(sail *sig);
void sfunc(sail *sig);

// sail object
App app1;
App app2;

// if true, it will show the original image and not the dxt compressed one
#if defined(USE_POPPLER)
bool useDXT = false;      // cannot yet since poppler returns RGB instead of RGBA
#else
bool useDXT = true;
#endif

int needSwap = 0;

unsigned int width, height;  // image size
string fileName;
float lastX = 0;
float dist = 0;
int numImages = 0;
int firstPage = 0;

// widgets
label *pageNum= new label;
char pagelabel[256];

#if ! defined(USE_POPPLER)
// use ImageMagick to read all other formats
MagickBooleanType status;
MagickWand *wand;
#endif


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

Pixels* inCache(int key)
{
  // Hash with the page number modulo cache size
  Pixels *p = cache[key%PDF_CACHE_SIZE];
  if (p && p->idx==key) {
    // Found a page with correct number
    return p;
  }
  else {
    // Key not found
    return NULL;
  }
}

void addCache(int idx, byte *rgbapixels)
{
  int pg_index = idx;
  int ds = 0;
  if (useDXT)
    ds = width*height*4/8;
  else
    ds = width*height*3;
  // If needed, allocate a pixel buffer
  if (cache[pg_index%PDF_CACHE_SIZE] == NULL)
    cache[pg_index%PDF_CACHE_SIZE] = new Pixels(ds);

  cache[pg_index%PDF_CACHE_SIZE]->idx = pg_index;

  // Copy the pixels in place
  memcpy( cache[pg_index%PDF_CACHE_SIZE]->p, rgbapixels, ds);
}


void getRGBA(App *app)
{
  // Get the pixels to the SAGE buffer

  Pixels* val = inCache(app->pg_index);
  if ( val == NULL ) {

#if defined(USE_POPPLER)

#if SAGE_POPPLER_VERSION == 5
    poppler_page_render_to_pixbuf (page, 0, 0, width, height, x_resolution/72.0, 0, pixbuf);
    memcpy(app->rgba, gdk_pixbuf_get_pixels(pixbuf), width *height * 3);
#endif

#if SAGE_POPPLER_VERSION >= 12
    SplashBitmap *bitmap = splashOut->getBitmap();
    memcpy(app->rgba, bitmap->getDataPtr(), width *height * 3);
#endif

#else
    if (useDXT)
      MagickGetImagePixels(wand, 0, 0, width, height, "RGBA", CharPixel, app->rgba);
    else
      MagickGetImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, app->rgba);
#endif

    if (useDXT) {
      // compress into DXT
      unsigned int numBytes;
      memset(app->dxt, 0, width*height*4/8);
      numBytes = CompressDXT(app->rgba, app->dxt, width, height, FORMAT_DXT1, 1);
      addCache(app->pg_index, app->dxt);
    }
    else {
      addCache(app->pg_index, app->rgba);
    }
  } else {
    if (useDXT)
      memcpy(app->rgba, val->p, width*height*4/8);
    else
      memcpy(app->rgba, val->p, width*height*3);
  }

}

void swapPDF()
{
  if (useDXT) {
    swapWithBuffer(app1.sageInf, app1.dxt);
    swapWithBuffer(app1.sageInf, app1.dxt);

    if (app2.ready) {
      swapWithBuffer(app2.sageInf, app2.dxt);
      swapWithBuffer(app2.sageInf, app2.dxt);
    }
  }
  else {
    swapWithBuffer(app1.sageInf, app1.rgba);
    swapWithBuffer(app1.sageInf, app1.rgba);

    if (app2.ready) {
      SAGE_PRINTLOG("Swapping 2");
      swapWithBuffer(app2.sageInf, app2.rgba);
      swapWithBuffer(app2.sageInf, app2.rgba);
    }
  }

}

void updateSAGE()
{
  sprintf(pagelabel, "Page %d of %d", app1.pg_index+1, numImages);
  pageNum->setLabel(pagelabel, 20);
  getRGBA(&app1);
  if (app2.ready) getRGBA(&app2);
  needSwap = 1;
}

void renderCurrentPage(App *app)
{
#if defined(USE_POPPLER)
#if SAGE_POPPLER_VERSION == 5
  if (inCache(app->pg_index) == NULL)
    page = poppler_document_get_page(document, app->pg_index);
#endif
#if SAGE_POPPLER_VERSION >= 12
  if (inCache(app->pg_index) == NULL)
    doc->displayPageSlice(splashOut, app->pg_index+1, x_resolution, x_resolution, 0, gTrue, gFalse, gFalse, 0, 0, pg_w, pg_h);
#endif
#else
  if (inCache(app->pg_index) == NULL)
    MagickSetImageIndex(wand, app->pg_index);
#endif
}

void updatePageNumber(int pg)
{
  app1.pg_index = pg;
  if (app1.pg_index >= numImages) app1.pg_index = numImages-1;
  if (app1.pg_index < 0) app1.pg_index = 0;

  if (app2.ready) {
    app2.pg_index = app1.pg_index+1;
    if (app2.pg_index >= numImages) app2.pg_index = numImages-1;
    if (app2.pg_index < 0) app2.pg_index = 0;
  }
}

void onHomeBtn(int eventId, button *btnObj)
{
  updatePageNumber(0);
  renderCurrentPage(&app1);
  if (app2.ready) renderCurrentPage(&app2);
  updateSAGE();
}

void onEndBtn(int eventId, button *btnObj)
{
  updatePageNumber(numImages-1);
  renderCurrentPage(&app1);
  if (app2.ready) renderCurrentPage(&app2);
  updateSAGE();
}

void onPrevBtn(int eventId, button *btnObj)
{
  updatePageNumber(app1.pg_index - 1);
  renderCurrentPage(&app1);
  if (app2.ready) renderCurrentPage(&app2);
  updateSAGE();
}


void onNextBtn(int eventId, button *btnObj)
{
  updatePageNumber(app1.pg_index + 1);
  renderCurrentPage(&app1);
  if (app2.ready) renderCurrentPage(&app2);
  updateSAGE();
}

void onQuitBtn(int eventId, button *btnObj)
{
  // Quit the app, from the app pov
  if (qfunc) (*qfunc)(app1.sageInf);
  deleteSAIL(app1.sageInf);
  if (app2.ready) deleteSAIL(app2.sageInf);
  sleep(3);
  exit(1);
}

void onReplicateBtn(int eventId, button *btnObj)
{
  // Set a flag to create a replica in the main event loop
  needApp = true;
}

void replicateApp()
{
  // Turn down the flag
  needApp = false;

  if (useDXT) {
    app2.sageInf = createSAIL("pdfviewer", width, height, PIXFMT_DXT, NULL, TOP_TO_BOTTOM, 10, NULL);
    app2.dxt  = (byte*) memalign(16, width*height*4/8);
    app2.rgba = (byte*) memalign(16, width*height*3);
  }
  else {
    app2.sageInf = createSAIL("pdfviewer", width, height, PIXFMT_888, NULL, TOP_TO_BOTTOM, 10, NULL);
    app2.rgba = (byte*) memalign(16, width*height*3);
  }
  app2.ready = true;
  // start at the same page
  app2.pg_index = app1.pg_index;
}


void makeWidgets(sail *sgi)
{
  float mult = 2.0;
  int bs = 75;

  sprintf(pagelabel, "Page %d of %d", 1, numImages);
  pageNum->setLabel(pagelabel, 20);
  pageNum->setSize(150, 50);
  pageNum->alignLabel(CENTER);
  pageNum->alignY(CENTER);
  pageNum->setBackgroundColor(255,255,255);
  pageNum->setFontColor(200,200,200);

  thumbnail *homeBtn = new thumbnail;
  homeBtn->setUpImage("images/skip-backward.png");
  homeBtn->setSize(bs,bs);
  homeBtn->setScaleMultiplier(mult);
  homeBtn->setTransparency(80);
  homeBtn->onUp(&onHomeBtn);

  thumbnail *endBtn = new thumbnail;
  endBtn->setUpImage("images/skip-forward.png");
  endBtn->setSize(bs,bs);
  endBtn->setScaleMultiplier(mult);
  endBtn->alignX(LEFT, 25);
  endBtn->setTransparency(80);
  endBtn->onUp(&onEndBtn);

  thumbnail *nextBtn = new thumbnail;
  nextBtn->setUpImage("images/fast-forward.png");
  nextBtn->setSize(bs,bs);
  nextBtn->setScaleMultiplier(mult);
  nextBtn->setTransparency(80);
  nextBtn->onUp(&onNextBtn);

  thumbnail *prevBtn = new thumbnail;
  prevBtn->setUpImage("images/rewind.png");
  prevBtn->setSize(bs,bs);
  prevBtn->setScaleMultiplier(mult);
  prevBtn->alignX(LEFT, 25);
  prevBtn->setTransparency(80);
  prevBtn->onUp(&onPrevBtn);

  sizer *s = new sizer(HORIZONTAL);
  s->align(CENTER, BOTTOM, 2,2);
  s->addChild(homeBtn);
  s->addChild(prevBtn);
  s->addChild(pageNum);
  s->addChild(nextBtn);
  s->addChild(endBtn);

  thumbnail *replicateBtn = new thumbnail;
  replicateBtn->setUpImage("images/vertical_divider_btn_up.png");
  replicateBtn->setSize(bs,bs);
  replicateBtn->setScaleMultiplier(mult);
  // put the button in the bottom left corner
  replicateBtn->setPos(width-80,0);
  replicateBtn->setTransparency(100);
  replicateBtn->onUp(&onReplicateBtn);


  thumbnail *quitBtn = new thumbnail;
  quitBtn->setUpImage("images/close_over.png");
  quitBtn->setSize(bs,bs);
  quitBtn->setScaleMultiplier(mult);
  // put the quit button in the bottom left corner
  quitBtn->setPos(0,0);
  quitBtn->setTransparency(100);
  quitBtn->onUp(&onQuitBtn);

  panel *p = new panel(37,37,37);
  p->align(CENTER, BOTTOM_OUTSIDE,0,4);
  p->setSize(width, 80);
  p->fitInWidth(false);
  p->fitInHeight(false);
  p->setTransparency(150);
  p->addChild(s);
  p->addChild(quitBtn);
  // Dont add the functionality yet: not finished
  // p->addChild(replicateBtn);
  sgi->addWidget(p);
}

#if defined(USE_POPPLER)
#if SAGE_POPPLER_VERSION == 5
static void
print_index (PopplerIndexIter *iter)
{
  do
  {
    PopplerAction *action;
    PopplerIndexIter *child;

    action = poppler_index_iter_get_action (iter);
    SAGE_PRINTLOG ("Action: %d", action->type);
    poppler_action_free (action);
    child = poppler_index_iter_get_child (iter);
    if (child)
      print_index (child);
    poppler_index_iter_free (child);
  }
  while (poppler_index_iter_next (iter));
}

static void
print_document_info (PopplerDocument *document)
{
  gchar *title, *format, *author, *subject, *keywords, *creator, *producer, *linearized;
  GTime creation_date, mod_date;
  PopplerPageLayout layout;
  PopplerPageMode mode;
  PopplerViewerPreferences view_prefs;
  PopplerFontInfo *font_info;
  PopplerFontsIter *fonts_iter;
  PopplerIndexIter *index_iter;
  GEnumValue *enum_value;

  g_object_get (document,
                "title", &title,
                "format", &format,
                "author", &author,
                "subject", &subject,
                "keywords", &keywords,
                "creation-date", &creation_date,
                "mod-date", &mod_date,
                "creator", &creator,
                "producer", &producer,
                "linearized", &linearized,
                "page-mode", &mode,
                "page-layout", &layout,
                "viewer-preferences", &view_prefs,
                NULL);

  SAGE_PRINTLOG ("\t---------------------------------------------------------");
  SAGE_PRINTLOG ("\tDocument Metadata");
  SAGE_PRINTLOG ("\t---------------------------------------------------------");
  if (title)  SAGE_PRINTLOG   ("\ttitle:\t\t%s", title);
  if (format) SAGE_PRINTLOG   ("\tformat:\t\t%s", format);
  if (author) SAGE_PRINTLOG   ("\tauthor:\t\t%s", author);
  if (subject) SAGE_PRINTLOG  ("\tsubject:\t%s", subject);
  if (keywords) SAGE_PRINTLOG ("\tkeywords:\t%s", keywords);
  if (creator) SAGE_PRINTLOG ("\tcreator:\t%s", creator);
  if (producer) SAGE_PRINTLOG ("\tproducer:\t%s", producer);
  if (linearized) SAGE_PRINTLOG ("\tlinearized:\t%s", linearized);

  enum_value = g_enum_get_value ((GEnumClass *) g_type_class_peek (POPPLER_TYPE_PAGE_MODE), mode);
  SAGE_PRINTLOG ("\tpage mode:\t%s", enum_value->value_name);
  enum_value = g_enum_get_value ((GEnumClass *) g_type_class_peek (POPPLER_TYPE_PAGE_LAYOUT), layout);
  SAGE_PRINTLOG ("\tpage layout:\t%s", enum_value->value_name);

  SAGE_PRINTLOG ("\tcreation date:\t%d", creation_date);
  SAGE_PRINTLOG ("\tmodified date:\t%d", mod_date);

  SAGE_PRINTLOG ("\tfonts:");
  font_info = poppler_font_info_new (document);
  while (poppler_font_info_scan (font_info, 20, &fonts_iter)) {
    if (fonts_iter) {
      do {
        SAGE_PRINTLOG ("\t\t\t%s", poppler_fonts_iter_get_name (fonts_iter));
      } while (poppler_fonts_iter_next (fonts_iter));
      poppler_fonts_iter_free (fonts_iter);
    }
  }
  poppler_font_info_free (font_info);

  index_iter = poppler_index_iter_new (document);
  if (index_iter)
  {
    SAGE_PRINTLOG ("\tindex:");
    print_index (index_iter);
    poppler_index_iter_free (index_iter);
  }

  g_free (title);
  g_free (format);
  g_free (author);
  g_free (subject);
  g_free (keywords);
  g_free (creator);
  g_free (producer);
  g_free (linearized);
}

#endif
#if SAGE_POPPLER_VERSION >= 12
// -----------------------------------------------------------------------------
static void printInfoString(Dict *infoDict, char *key, char *text,
                            UnicodeMap *uMap) {
  Object obj;
  GooString *s1;
  GBool isUnicode;
  Unicode u;
  char buf[8];
  int i, n;

  if (infoDict->lookup(key, &obj)->isString()) {
    fputs(text, stdout);
    s1 = obj.getString();
    if ((s1->getChar(0) & 0xff) == 0xfe &&
        (s1->getChar(1) & 0xff) == 0xff) {
      isUnicode = gTrue;
      i = 2;
    } else {
      isUnicode = gFalse;
      i = 0;
    }
    while (i < obj.getString()->getLength()) {
      if (isUnicode) {
        u = ((s1->getChar(i) & 0xff) << 8) |
          (s1->getChar(i+1) & 0xff);
        i += 2;
      } else {
        u = pdfDocEncoding[s1->getChar(i) & 0xff];
        ++i;
      }
      n = uMap->mapUnicode(u, buf, sizeof(buf));
      fwrite(buf, 1, n, stdout);
    }
    fputc('\n', stdout);
  }
  obj.free();
}


static void printInfoDate(Dict *infoDict, char *key, char *text) {
  Object obj;
  char *s;
  int year, mon, day, hour, min, sec, tz_hour, tz_minute;
  char tz;
  struct tm tmStruct;
  char buf[256];

  if (infoDict->lookup(key, &obj)->isString()) {
    fputs(text, stdout);
    s = obj.getString()->getCString();
    // TODO do something with the timezone info
    if ( parseDateString( s, &year, &mon, &day, &hour, &min, &sec, &tz, &tz_hour, &tz_minute ) ) {
      tmStruct.tm_year = year - 1900;
      tmStruct.tm_mon = mon - 1;
      tmStruct.tm_mday = day;
      tmStruct.tm_hour = hour;
      tmStruct.tm_min = min;
      tmStruct.tm_sec = sec;
      tmStruct.tm_wday = -1;
      tmStruct.tm_yday = -1;
      tmStruct.tm_isdst = -1;
      // compute the tm_wday and tm_yday fields
      if (mktime(&tmStruct) != (time_t)-1 &&
          strftime(buf, sizeof(buf), "%c", &tmStruct)) {
        fputs(buf, stdout);
      } else {
        fputs(s, stdout);
      }
    } else {
      fputs(s, stdout);
    }
    fputc('\n', stdout);
  }
  obj.free();
}
#endif
#endif


// -----------------------------------------------------------------------------

void qfunc(sail *sig)
{
  // release the memory
  if (useDXT) free(app1.dxt);
  free(app1.rgba);
#if ! defined(USE_POPPLER)
  DestroyMagickWand(wand);
#endif
}

void sfunc(sail *sig)
{
  SAGE_PRINTLOG("sfunc> Got to synchronize!");
  onHomeBtn(0, NULL);
}



int main(int argc,char **argv)
{
  sage::initUtil();

  int wallw = 4096; // Assume a 4K by 4K wall by default
  int wallh = 4096; //    used for rasterization below
  if ( getWallSize(wallw, wallh) ) {
    SAGE_PRINTLOG("PDF> Got wall size: [%d x %d]",wallw, wallh);
    if (wallw < 4096) wallw = getMax2n(wallw);
    if (wallh < 4096) wallh = getMax2n(wallh);
  }

  // parse command line arguments
  if (argc < 2){
    SAGE_PRINTLOG("PDF> pdfviewer filename [-show_original] [-page num]");
    return 0;
  }

  for (int argNum=2; argNum<argc; argNum++)
  {
    if (strcmp(argv[argNum], "-show_original") == 0) {
      useDXT = false;
    }
    else if(strcmp(argv[argNum], "-page") == 0) {
      int p = atoi(argv[argNum+1]);
      if (p != 0)
        firstPage = p-1;
      argNum++;
    }
    else if(atoi(argv[argNum]) != 0 && atoi(argv[argNum+1]) != 0) {
      // pass
    }
  }


  fileName = string(argv[1]);

#if defined(USE_POPPLER)
#if SAGE_POPPLER_VERSION == 5
  g_type_init();
  SAGE_PRINTLOG("PDF> Poppler version %s", poppler_get_version());
  backend = poppler_get_backend ();
  enum_value = g_enum_get_value ((GEnumClass *) g_type_class_ref (POPPLER_TYPE_BACKEND), backend);
  SAGE_PRINTLOG ("PDF> Backend is %s", enum_value->value_name);

  error = NULL;
  const string filename = string((const char*)"file://") + string(argv[1]);
  document = poppler_document_new_from_file (filename.c_str(), NULL, &error);
  if (document == NULL) {
    SAGE_PRINTLOG("PDF> error for [%s]: %s", filename.c_str(), error->message);
    exit(0);
  }
  // get the number of pages
  numImages = poppler_document_get_n_pages(document);

  app1.pg_index = 0; // index starts at 0
  page = poppler_document_get_page(document, app1.pg_index);
  if (page == NULL) {
    SAGE_PRINTLOG("PDF> error for [%s]: %s", filename.c_str());
    exit(0);
  }

  // page size
  double mwidth, mheight;
  poppler_page_get_size (page, &mwidth, &mheight);
  SAGE_PRINTLOG ("PDF> page size: %g by %g ", mwidth, mheight);

  x_resolution = 300.0;
  pg_w = mwidth  * (x_resolution / 72.0);
  pg_h = mheight * (x_resolution / 72.0);

  if (pg_w > wallw) {
    pg_w = (double)wallw;
    pg_h = (double)wallw / (mwidth/mheight);
    x_resolution = pg_w * 72.0 / mwidth;
  }
  if (pg_h > wallh) {
    pg_w = (double)wallh / (mheight/mwidth);
    pg_h = (double)wallh;
    x_resolution = pg_h * 72.0 / mheight;
  }

  width  = (unsigned int)pg_w;
  height = (unsigned int)pg_h;
  SAGE_PRINTLOG("PDF> Crop @ %d DPI: width %d height %d", int(x_resolution), int(pg_w), int(pg_h));

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
  gdk_pixbuf_fill (pixbuf, 0xffffffff);

  // print some information
  print_document_info (document);

#endif
#if SAGE_POPPLER_VERSION >= 12
  GooString *fileName;
  GooString *ownerPW, *userPW;
  Object info;
  UnicodeMap *uMap;

  // read config file
  globalParams = new GlobalParams();

  // get mapping to output encoding
  if (!(uMap = globalParams->getTextEncoding())) {
    SAGE_PRINTLOG("PDF> Couldn't get text encoding");
  }

  fileName = new GooString(argv[1]);
  ownerPW = NULL;
  userPW = NULL;
  doc = new PDFDoc(fileName, ownerPW, userPW, NULL);
  SAGE_PRINTLOG("PDF> file [%s] is OK: %d", argv[1], doc->isOk() );
  numImages = doc->getNumPages();
  SAGE_PRINTLOG("PDF> num pages: %d", numImages );

  SplashColor paperColor;
  paperColor[0] = 255;
  paperColor[1] = 255;
  paperColor[2] = 255;
  paperColor[3] = 0;

  splashOut = new SplashOutputDev(splashModeRGB8, 3, gFalse, paperColor);
#if SAGE_POPPLER_VERSION >= 20
  splashOut->startDoc(doc);
#else
  splashOut->startDoc(doc->getXRef());
#endif

  SAGE_PRINTLOG("PDF> Starting at page %d", firstPage);
  app1.pg_index = firstPage;
  double mwidth  = doc->getPageMediaWidth(app1.pg_index+1);
  double mheight = doc->getPageMediaHeight(app1.pg_index+1);
  SAGE_PRINTLOG("PDF> Media width %d height %d", int(mwidth), int(mheight));

  x_resolution = 300.0;
  pg_w = mwidth  * (x_resolution / 72.0);
  pg_h = mheight * (x_resolution / 72.0);

  if (pg_w > wallw) {
    pg_w = (double)wallw;
    pg_h = (double)wallw / (mwidth/mheight);
    x_resolution = pg_w * 72.0 / mwidth;
  }
  if (pg_h > wallh) {
    pg_w = (double)wallh / (mheight/mwidth);
    pg_h = (double)wallh;
    x_resolution = pg_h * 72.0 / mheight;
  }

  width  = pg_w;
  height = pg_h;
  SAGE_PRINTLOG("PDF> Crop @ %d DPI: width %d height %d", int(x_resolution), int(pg_w), int(pg_h));

  doc->displayPageSlice(splashOut, 1, x_resolution, x_resolution, 0, gTrue, gFalse, gFalse, 0, 0, pg_w, pg_h);

  // print doc info
  doc->getDocInfo(&info);
  if (info.isDict()) {
    printInfoString(info.getDict(), (char*)"Title",       (char*)"Title:          ", uMap);
    printInfoString(info.getDict(), (char*)"Subject",     (char*)"Subject:        ", uMap);
    printInfoString(info.getDict(), (char*)"Keywords",    (char*)"Keywords:       ", uMap);
    printInfoString(info.getDict(), (char*)"Author",      (char*)"Author:         ", uMap);
    printInfoString(info.getDict(), (char*)"Creator",     (char*)"Creator:        ", uMap);
    printInfoString(info.getDict(), (char*)"Producer",    (char*)"Producer:       ", uMap);
    printInfoDate  (info.getDict(), (char*)"CreationDate",(char*)"CreationDate:   ");
    printInfoDate  (info.getDict(), (char*)"ModDate",     (char*)"ModDate:        ");
  }
  info.free();
#endif
#else
  // read file
  wand=NewMagickWand();

  // set the resolution (pixel density)
  MagickSetResolution(wand, 200,200);

  status=MagickReadImage(wand, fileName.data());
  if (status == MagickFalse)
    ThrowWandException(wand);
  numImages = MagickGetNumberImages(wand);
  MagickSetImageIndex(wand, firstPage);

  // get the image size
  width = MagickGetImageWidth(wand);
  height = MagickGetImageHeight(wand);

  // crop the image if necessary to make sure it's a multiple of 4
  if (useDXT)
  {
    if (width%4 != 0 || height%4 != 0)
    {
      fprintf(stderr, "\n**** Image cropped a few pixels to be a multiple of 4 for dxt");
      width -= width%4;
      height -= height%4;
    }

    // flip the image to have the correct orientation for dxt
    MagickFlipImage(wand);
  }
#endif

  // allocate buffers
  if (useDXT) {
    app1.rgba = (byte*) memalign(16, width*height*4);
    app1.dxt  = (byte*) memalign(16, width*height*4/8);
  }
  else
    app1.rgba = (byte*) memalign(16, width*height*3);

  // get the first page
  getRGBA(&app1);

  // initialize SAIL

  if (useDXT)
  {
    app1.sageInf = createSAIL("pdfviewer", width, height, PIXFMT_DXT, NULL, BOTTOM_TO_TOP, 10, makeWidgets);
  }
  else
  {
    app1.sageInf = createSAIL("pdfviewer", width, height, PIXFMT_888, NULL, TOP_TO_BOTTOM, 10, makeWidgets);
  }

  // finally swap the first buffer
  app1.ready = true;
  needSwap = 1;


  // Wait the end
  while (1) {
    if (needSwap) {
      needSwap = 0;
      swapPDF();
    }
    usleep(500000);  // so that we don't keep spinning too frequently (0.5s)


    processMessages(app1.sageInf, &app1.up, qfunc, sfunc);
    if (app1.up.updated) {
      SAGE_PRINTLOG("PDF> App location and size: (%d,%d) - %d x %d [wall %d x %d]",
                    app1.up.app_x,app1.up.app_y,app1.up.app_w,app1.up.app_h, app1.up.wall_width, app1.up.wall_height);
      app1.up.updated = 0;
    }

    if (needApp) {
      replicateApp();
      renderCurrentPage(&app2);
      updateSAGE();
    }

    if (app2.ready) {
      processMessages(app2.sageInf, &app2.up, qfunc, sfunc);
      if (app2.up.updated) {
        SAGE_PRINTLOG("PDF> App2 location and size: (%d,%d) - %d x %d [wall %d x %d]",
                      app2.up.app_x,app2.up.app_y,app2.up.app_w,app2.up.app_h, app2.up.wall_width, app2.up.wall_height);
        app2.up.updated = 0;
      }
    }

  } // end while

  return 0;
}
