#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "DeckLinkAPI.h"
#include "decklinkcapture.h"
#include "ring.h"

// headers for SAGE
#include "libsage.h"

// SAGE UI API
#include "suil.h"
suil uiLib;

// FFMPEG
#if defined(DEINTERLACING)
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
}
AVPicture *picture_prev, *picture_next, *picture_curr;
struct SwsContext *img_convert_topacked;
struct SwsContext *img_convert_toplanar;
#endif

// SAGE
sail *sageInf; // sail object
int  useSAGE = 0;
int  useDEINT = 0; //de-interlacing
int  useSAM  = 0;
int  sageW, sageH;
double sageFPS;

// SAM
Ring_Buffer *audio_ring;
#define AUDIO_TEMP 16384
int audio_temp[AUDIO_TEMP]; //32bit
sam::StreamingAudioClient* m_sac = NULL;
int physicalInputs = 0;
char *samIP;

// if achieving less than threshold frame rate
//     then drop frames
#define DROP_THRESHOLD 0.95

// Decklink
pthread_mutex_t     sleepMutex;
pthread_cond_t      sleepCond;
int       videoOutputFile = -1;
int       audioOutputFile = -1;
int       card = 0;
IDeckLink       *deckLink;
IDeckLinkInput      *deckLinkInput;
IDeckLinkDisplayModeIterator  *displayModeIterator;

static BMDTimecodeFormat  g_timecodeFormat = 0;
static int      g_videoModeIndex = -1;
static int      g_audioChannels = 2;
static int      g_audioSampleDepth = 32;
const char *      g_videoOutputFile = NULL;
const char *      g_audioOutputFile = NULL;
static int      g_maxFrames = -1;
static unsigned long    frameCount = 0;
BMDTimeValue frameRateDuration, frameRateScale;

#if defined(DEINTERLACING)
////////////////////////////////////////////////////////////////////////////////
// FFMPEG deinterlacing
////////////////////////////////////////////////////////////////////////////////

// ==================================
AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
  AVFrame *picture;
  uint8_t *picture_buf;
  int size;

  picture = avcodec_alloc_frame();
  if (!picture)
    return NULL;
  size = avpicture_get_size(pix_fmt, width, height);
  picture_buf = (uint8_t*)av_malloc(size);
  fprintf(stderr, "Allocating picture: %d bytes [%dx%d]\n", size, width, height);
  if (!picture_buf) {
    av_free(picture);
    return NULL;
  }
  avpicture_fill((AVPicture *)picture, picture_buf, pix_fmt, width, height);
  return picture;
}

//< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
#define SAGE_YUV   PIX_FMT_YUYV422

//< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
//#define SAGE_YUV   PIX_FMT_UYVY422

////////////////////////////////////////////////////////////////////////////////
#endif


void* readThread(void *args)
{
  suil *uiLib = (suil *)args;

  while(1) {
    sageMessage msg;
    msg.init(READ_BUF_SIZE);
    uiLib->rcvMessageBlk(msg);
    if ( msg.getCode() == (APP_INFO_RETURN) ) {
      //SAGE_PRINTLOG("UI Message> %d --- %s", msg.getCode(), (char *)msg.getData());
      // format:   decklinkcapture 13 1972 5172 390 2190 1045 0 0 0 0 none 1280 720 1
      char appname[256], cmd[256];
      int ret;
      int appID, left, right, bottom, top, sailID, zValue;
      ret = sscanf((char*)msg.getData(), "%s %d %d %d %d %d %d %d",
                   appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);
      if (ret == 8) {
        if (sageInf->getWinID() == appID) {
          int x,y,w,h;
          x = left;
          y = top;
          w = right-left;
          h = top-bottom;
          if (useSAM && m_sac) {
            int ret;
            SAGE_PRINTLOG("\t ME ME ME: %d x %d - %d x %d - %d", x,y,w,h, zValue);
            ret = m_sac->setPosition(x,y,w,h,zValue);
          }
        }
      }
    }
  }

  return NULL;
}


DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() : m_refCount(0)
{
  pthread_mutex_init(&m_mutex, NULL);
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
  pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
  pthread_mutex_lock(&m_mutex);
  m_refCount++;
  pthread_mutex_unlock(&m_mutex);

  return (ULONG)m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
  pthread_mutex_lock(&m_mutex);
  m_refCount--;
  pthread_mutex_unlock(&m_mutex);

  if (m_refCount == 0)
  {
    delete this;
    return 0;
  }

  return (ULONG)m_refCount;
}

void decklinkQuit(sail *si)
{
  pthread_cond_signal(&sleepCond);
  if (m_sac) delete m_sac;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
  IDeckLinkVideoFrame*                  rightEyeFrame = NULL;
  IDeckLinkVideoFrame3DExtensions*        threeDExtensions = NULL;
  void*         frameBytes;
  void*         audioFrameBytes;
  static int warmup = (int)sageFPS; // 1 sec of frames
  static BMDTimeValue startoftime;
  static int skipf = 0;
  static double lastframetime = 0.0;

  // Handle Video Frame
  if(videoFrame)
  {

#if 0
    if (warmup > 0) {
      warmup--;
      fprintf(stderr, "Warmup: frame %03d\r", warmup);
      return S_OK;
    }
    if (warmup == 0) {
      fprintf(stderr, "\nWarmup done\n");
      warmup--;
    }
#endif

    if (frameCount==0) {
      BMDTimeValue hframedur;
      HRESULT h = videoFrame->GetHardwareReferenceTimestamp(frameRateScale, &startoftime, &hframedur);
      lastframetime = sage::getTime();
    }

    if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
    {
      fprintf(stderr, "Frame received (#%lu) - No input signal detected\n", frameCount);
    }
    else
    {
      if (useSAGE) {
        BMDTimeScale htimeScale = frameRateScale;
        BMDTimeValue hframeTime;
        BMDTimeValue hframeDuration;
        HRESULT h = videoFrame->GetHardwareReferenceTimestamp(htimeScale, &hframeTime, &hframeDuration);
        if (h == S_OK) {
          double frametime = (double)(hframeTime-startoftime) / (double)hframeDuration;
          double instantfps = 1000000.0/(sage::getTime()-lastframetime);
          if ( ((int)frametime) > frameCount) { skipf = 1; frameCount++; }
          if ( instantfps <= (sageFPS*DROP_THRESHOLD) ) { skipf = 1; frameCount++; }   // if lower than 95% of target FPS
          //if (skipf)
          //SAGE_PRINTLOG("HARD Time: %10ld %10ld %10ld: %f %d %f --", hframeTime, hframeDuration, htimeScale, frametime, frameCount, instantfps );
          //else
          //SAGE_PRINTLOG("HARD Time: %10ld %10ld %10ld: %f %d %f", hframeTime, hframeDuration, htimeScale, frametime, frameCount, instantfps );
          lastframetime = sage::getTime();
        } else {
          BMDTimeValue frameTime, frameDuration;
          videoFrame->GetStreamTime(&frameTime, &frameDuration, frameRateScale);

          SAGE_PRINTLOG("SOFT Time: %10ld %10ld %10ld", frameTime, frameDuration, frameRateScale);
        }

        //double actualFPS = (double)frameRateScale / (double)frameRateDuration;

        if (!skipf) {
          // Get the pixels from the capture
          videoFrame->GetBytes(&frameBytes);

          if (useDEINT) {
#if defined(DEINTERLACING)
            int ret;

            // Put data into frame
            memcpy(picture_curr->data[0], frameBytes, sageW*sageH*2);

            // Convert to planar format
            sws_scale(img_convert_toplanar, picture_curr->data, picture_curr->linesize, 0,
                      sageH, picture_next->data, picture_next->linesize);

            // Deinterlacing (in place)
            ret = avpicture_deinterlace(picture_next, picture_next, PIX_FMT_YUV422P, sageW, sageH);
            if (ret!=0)
              SAGE_PRINTLOG("Deinterlace error %d", ret);

            // Convert to packed format
            sws_scale(img_convert_topacked, picture_next->data, picture_next->linesize, 0,
                      sageH, picture_prev->data, picture_prev->linesize);

            // Pass the pixels to SAGE
            swapWithBuffer(sageInf, (unsigned char *)picture_prev->data[0]);
#else
            // Pass the pixels to SAGE
            swapWithBuffer(sageInf, (unsigned char *)frameBytes);
#endif
          } else {
            // Pass the pixels to SAGE

            int localskip = 0;
            if (sageFPS >= 50.0) {                // if 50p or 60p rate
              if ( (frameCount%2) == 0 ) {  // skip every other frame
                localskip = 1;
              }
            }
            if (!localskip)
              swapWithBuffer(sageInf, (unsigned char *)frameBytes);
          }

          processMessages(sageInf, NULL,decklinkQuit,NULL);
        } else {
          SAGE_PRINTLOGNL("#");
        }

      }
      else if (videoOutputFile != -1)
      {
        videoFrame->GetBytes(&frameBytes);
        write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());

        if (rightEyeFrame)
        {
          rightEyeFrame->GetBytes(&frameBytes);
          write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
        }
      }
    }
    frameCount++;

    if (g_maxFrames > 0 && frameCount >= g_maxFrames)
    {
      pthread_cond_signal(&sleepCond);
    }
  }

  // Handle Audio Frame
  if (audioFrame)
  {
    if (useSAM && ! physicalInputs ) {
      int gotFrames = audioFrame->GetSampleFrameCount();
      audioFrame->GetBytes(&audioFrameBytes);
      //fprintf(stderr, "Audio Frame received: %d bytes - Samples %d\n",
      //gotFrames * g_audioChannels * (g_audioSampleDepth / 8), gotFrames );
      unsigned int len =  gotFrames * g_audioChannels * (g_audioSampleDepth / 8);
      unsigned int putin =  len;
      audio_ring->Buffer_Data(audioFrameBytes, putin);
      //fprintf(stderr, "Added: %6u / %6u -- Buffered: %6u -- Free %6u\n", len, putin, audio_ring->Buffered_Bytes(), audio_ring->Free_Space());
    }
    else {
      if (audioOutputFile != -1)
      {
        audioFrame->GetBytes(&audioFrameBytes);
        write(audioOutputFile, audioFrameBytes, audioFrame->GetSampleFrameCount() * g_audioChannels * (g_audioSampleDepth / 8));
      }
    }
  }


  //frameCount++;
  skipf = 0;

  return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
  fprintf(stderr, "VideoInputFormatChanged VideoInputFormatChanged VideoInputFormatChanged\n");
  fprintf(stderr, "VideoInputFormatChanged VideoInputFormatChanged VideoInputFormatChanged\n");
  fprintf(stderr, "VideoInputFormatChanged VideoInputFormatChanged VideoInputFormatChanged\n");
  return S_OK;
}

int usage(int status)
{
  HRESULT result;
  IDeckLinkDisplayMode *displayMode;
  int displayModeCount = 0;

  fprintf(stderr,
          "Usage: declinkcapture -d <card id> -m <mode id> [OPTIONS]\n"
          "\n"
          "    -d <card id>:\n");

  IDeckLinkIterator *di = CreateDeckLinkIteratorInstance();
  IDeckLink *dl;
  int dnum = 0;
  while (di->Next(&dl) == S_OK)
  {
    const char *deviceNameString = NULL;
    int  result = dl->GetModelName(&deviceNameString);
    if (result == S_OK)
    {
      fprintf(stderr, "\t device %d [%s]\n", dnum, deviceNameString);
    }
    dl->Release();
    dnum++;
  }


  fprintf(stderr,
          "    -m <mode id>:\n"
          );

  while (displayModeIterator->Next(&displayMode) == S_OK)
  {
    char *displayModeString = NULL;

    result = displayMode->GetName((const char **) &displayModeString);
    if (result == S_OK)
    {
      BMDTimeValue iframeRateDuration, iframeRateScale;
      displayMode->GetFrameRate(&iframeRateDuration, &iframeRateScale);

      fprintf(stderr, "        %2d:  %-20s \t %li x %li \t %g FPS\n",
              displayModeCount, displayModeString, displayMode->GetWidth(), displayMode->GetHeight(), (double)iframeRateScale / (double)iframeRateDuration);

      free(displayModeString);
      displayModeCount++;
    }

    // Release the IDeckLinkDisplayMode object to prevent a leak
    displayMode->Release();
  }

  fprintf(stderr,
          "    -i <video input number>\n"
          "    -p <pixelformat>\n"
          "         0:  8 bit YUV (4:2:2) (default)\n"
          "         1:  10 bit YUV (4:2:2)\n"
          "         2:  10 bit RGB (4:4:4)\n"
          "    -t <format>          Print timecode\n"
          "     rp188:  RP 188\n"
          "      vitc:  VITC\n"
          "    serial:  Serial Timecode\n"
          "    -f <filename>        Filename raw video will be written to\n"
          "    -a <filename>        Filename raw audio will be written to\n"
          "    -c <channels>        Audio Channels (2, 8 or 16 - default is 2)\n"
          "    -s <depth>           Audio Sample Depth (16 or 32 - default is 32)\n"
          "    -n <frames>          Number of frames to capture (default is unlimited)\n"
          "    -3                   Capture Stereoscopic 3D (Requires 3D Hardware support)\n"
          "    -u <SAM IP>          stream using SAM (audio), 127.0.0.1 by default \n"
          "    -j                   physical inputs for SAM inputs (audio)\n"
          "    -v                   stream using SAGE (video)\n"
          "    -y                   apply deinterlacing filter\n"
          "\n"
          "Capture video and/or audio to a file. Raw video and/or audio can be viewed with mplayer eg:\n"
          "\n"
          "    Capture -m2 -n 50 -f video.raw -a audio.raw\n"
          "    mplayer video.raw -demuxer rawvideo -rawvideo pal:uyvy -audiofile audio.raw -audio-demuxer 20 -rawaudio rate=48000\n"
          );

  exit(status);
}




bool handle_sac_audio_callback(unsigned int numChannels, unsigned int nframes, float** out, void*obj)
{
  unsigned int alen = nframes  * g_audioChannels * (g_audioSampleDepth / 8);
  unsigned int datalen = nframes  * g_audioChannels * 4;

  for (unsigned int k = 0; k < nframes*numChannels; k++) {
    audio_temp[k] = 0;
  }
  for (unsigned int ch = 0; ch < 1; ch++)
    for (unsigned int n = 0; n < nframes; n++)
      out[ch][n] = 0.0f;

  //fprintf(stderr, "Callback: asking %d frames (%d bytes)\n", nframes, datalen);

#if 1
  unsigned int got = nframes  * g_audioChannels * (g_audioSampleDepth / 8);
  audio_ring->Get_Data(audio_temp, got);
  int numsamples = got / (g_audioChannels * (g_audioSampleDepth / 8));
  if (got == 0) {
    //fprintf(stderr, "No data queue\n");
    fprintf(stderr, "#");
  }
  if ((got > 0) && (got != alen)) {
    fprintf(stderr, ".");
    //fprintf(stderr, "seconds: asked %u got %u -- Buffered: %6u -- Free %6u\n",
    //datalen, got, audio_ring->Buffered_Bytes(), audio_ring->Free_Space());
  }
  if (got == alen) {
    //fprintf(stderr, "Good: asked %u got %u -- Buffered: %6u -- Free %6u\n",
    //datalen, got, audio_ring->Buffered_Bytes(), audio_ring->Free_Space());
  }

  float *ptr = *out;
  int value; // for 32bit capture
  float fvalue;
  for (int k = 0; k < numsamples; k++) {
    for (int ch = 0; ch < g_audioChannels; ch++) {
      value = audio_temp[g_audioChannels*k+ch];
      fvalue = value / (float)(RAND_MAX); // for 32bit sound data
      out[ch][k] = fvalue;
    }
  }



  //write(audioOutputFile, ptr+numsamples, numsamples* 4);
#else
  for (unsigned int ch = 0; ch < 2; ch++)
    for (unsigned int n = 0; n < nframes; n++)
      out[ch][n] = rand() / (float)RAND_MAX;
#endif

  return true;
}


void myQuit(int sig)
{
  fprintf(stderr, "Caught signal [%d]\n", sig);

  // Quit SAM
  if (m_sac) delete m_sac;

  if (sageInf) deleteSAIL(sageInf);
}

char *connectionName(int64_t vport)
{
  char *name;
  switch (vport) {
  case bmdVideoConnectionSDI:
    name = strdup("SDI");
    break;
  case bmdVideoConnectionHDMI:
    name = strdup("HDMI");
    break;
  case bmdVideoConnectionComponent:
    name = strdup("Component");
    break;
  case bmdVideoConnectionComposite:
    name = strdup("Composite");
    break;
  case bmdVideoConnectionSVideo:
    name = strdup("SVideo");
    break;
  case bmdVideoConnectionOpticalSDI:
    name = strdup("OpticalSDI");
    break;
  default:
    name = strdup("UNKNOWN");
    break;
  }
}


int main(int argc, char *argv[])
{
  signal(SIGABRT,myQuit);//If program aborts go to assigned function
  signal(SIGTERM,myQuit);//If program terminates go to assigned function
  signal(SIGINT, myQuit);//If program terminates go to assigned function

  IDeckLinkIterator   *deckLinkIterator = CreateDeckLinkIteratorInstance();
  IDeckLinkAttributes*                            deckLinkAttributes = NULL;
  DeckLinkCaptureDelegate   *delegate;
  IDeckLinkDisplayMode    *displayMode;
  BMDVideoInputFlags    inputFlags = 0;
  BMDDisplayMode      selectedDisplayMode = bmdModeNTSC;
  BMDPixelFormat      pixelFormat = bmdFormat8BitYUV;
  int       displayModeCount = 0;
  int       exitStatus = 1;
  int       ch;
  bool        foundDisplayMode = false;
  HRESULT       result;
  int       dnum = 0;
  IDeckLink             *tempLink = NULL;
  int       found = 0;
  bool        supported = 0;
  int64_t                                         ports;
  int                                                     itemCount;
  int                                                     vinput = 0;
  int64_t                                                     vport = 0;
  IDeckLinkConfiguration       *deckLinkConfiguration = NULL;
  bool flickerremoval = true;
  //bool pnotpsf = false;
  bool pnotpsf = true;

  // Default IP address for SAM
  samIP = strdup("127.0.0.1");

  pthread_mutex_init(&sleepMutex, NULL);
  pthread_cond_init(&sleepCond, NULL);

  // Parse command line options
  while ((ch = getopt(argc, argv, "?h3c:d:s:f:a:m:n:p:t:u::vi:jy")) != -1)
  {
    switch (ch)
    {
    case 'i':
      vinput = atoi(optarg);
      break;
    case 'u':
      useSAM  = 1;
      SAGE_PRINTLOG("Set useSAM to 1: [%s]", optarg);
      if (optarg)
        samIP = strdup(optarg);
      break;
    case 'y':
      useDEINT = 1;
      break;
    case 'v':
      useSAGE = 1;
      break;
    case 'j':
      physicalInputs = 1;
      break;
    case 'd':
      card = atoi(optarg);
      break;
    case 'm':
      g_videoModeIndex = atoi(optarg);
      break;
    case 'c':
      g_audioChannels = atoi(optarg);
      if (g_audioChannels != 2 &&
          g_audioChannels != 8 &&
          g_audioChannels != 16)
      {
        fprintf(stderr, "Invalid argument: Audio Channels must be either 2, 8 or 16\n");
        goto bail;
      }
      break;
    case 's':
      g_audioSampleDepth = atoi(optarg);
      if (g_audioSampleDepth != 16 && g_audioSampleDepth != 32)
      {
        fprintf(stderr, "Invalid argument: Audio Sample Depth must be either 16 bits or 32 bits\n");
        goto bail;
      }
      break;
    case 'f':
      g_videoOutputFile = optarg;
      break;
    case 'a':
      g_audioOutputFile = optarg;
      break;
    case 'n':
      g_maxFrames = atoi(optarg);
      break;
    case '3':
      inputFlags |= bmdVideoInputDualStream3D;
      break;
    case 'p':
      switch(atoi(optarg))
      {
      case 0: pixelFormat = bmdFormat8BitYUV; break;
      case 1: pixelFormat = bmdFormat10BitYUV; break;
      case 2: pixelFormat = bmdFormat10BitRGB; break;
      default:
        fprintf(stderr, "Invalid argument: Pixel format %d is not valid", atoi(optarg));
        goto bail;
      }
      break;
    case 't':
      if (!strcmp(optarg, "rp188"))
        g_timecodeFormat = bmdTimecodeRP188Any;
      else if (!strcmp(optarg, "vitc"))
        g_timecodeFormat = bmdTimecodeVITC;
      else if (!strcmp(optarg, "serial"))
        g_timecodeFormat = bmdTimecodeSerial;
      else
      {
        fprintf(stderr, "Invalid argument: Timecode format \"%s\" is invalid\n", optarg);
        goto bail;
      }
      break;
    case '?':
    case 'h':
      usage(0);
    }
  }

  if (useSAGE && (pixelFormat != bmdFormat8BitYUV)) {
    SAGE_PRINTLOG("SAGE works only with pixelFormat 1 (bmdFormat8BitYUV)");
    pixelFormat = bmdFormat8BitYUV;
  }

  if (!deckLinkIterator)
  {
    fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
    goto bail;
  }

  /* Connect to the first DeckLink instance */
  while (deckLinkIterator->Next(&tempLink) == S_OK)
  {
    if (card != dnum) {
      dnum++;
      // Release the IDeckLink instance when we've finished with it to prevent leaks
      tempLink->Release();
      continue;
    }
    else {
      deckLink = tempLink;
      found = 1;
    }
    dnum++;
  }

  if (! found ) {
    fprintf(stderr, "No DeckLink PCI cards found.\n");
    goto bail;
  }
  if (deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)
    goto bail;

  {
    // Query the DeckLink for its attributes interface
    result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
    if (result != S_OK)
    {
      fprintf(stderr, "Could not obtain the IDeckLinkAttributes interface - result = %08x\n", result);
    }
  }

  result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supported);
  if (result == S_OK)
  {
    fprintf(stderr, " %-40s %s\n", "Input mode detection supported ?", (supported == true) ? "Yes" : "No");
  }
  else
  {
    fprintf(stderr, "Could not query the input mode detection attribute- result = %08x\n", result);
  }

  fprintf(stderr, "Supported video input connections (-i [input #]:\n  ");
  itemCount = 0;
  result = deckLinkAttributes->GetInt(BMDDeckLinkVideoInputConnections, &ports);
  if (result == S_OK)
  {
    if (ports & bmdVideoConnectionSDI)
    {
      fprintf(stderr, "%d: SDI, ", bmdVideoConnectionSDI);
      itemCount++;
    }

    if (ports & bmdVideoConnectionHDMI)
    {
      fprintf(stderr, "%d: HDMI, ", bmdVideoConnectionHDMI);
      itemCount++;
    }

    if (ports & bmdVideoConnectionOpticalSDI)
    {
      fprintf(stderr, "%d: Optical SDI, ", bmdVideoConnectionOpticalSDI);
      itemCount++;
    }

    if (ports & bmdVideoConnectionComponent)
    {
      fprintf(stderr, "%d: Component, ", bmdVideoConnectionComponent);
      itemCount++;
    }

    if (ports & bmdVideoConnectionComposite)
    {
      fprintf(stderr, "%d: Composite, ", bmdVideoConnectionComposite);
      itemCount++;
    }

    if (ports & bmdVideoConnectionSVideo)
    {
      fprintf(stderr, "%d: S-Video, ", bmdVideoConnectionSVideo);
      itemCount++;
    }
  }
  fprintf(stderr, "\n");



  delegate = new DeckLinkCaptureDelegate();
  deckLinkInput->SetCallback(delegate);

  // Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
  result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
  if (result != S_OK)
  {
    fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
    goto bail;
  }


  if (g_videoModeIndex < 0)
  {
    fprintf(stderr, "No video mode specified\n");
    usage(0);
  }

  if (g_videoOutputFile != NULL)
  {
    videoOutputFile = open(g_videoOutputFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
    if (videoOutputFile < 0)
    {
      fprintf(stderr, "Could not open video output file \"%s\"\n", g_videoOutputFile);
      goto bail;
    }
  }
  if (g_audioOutputFile != NULL)
  {
    audioOutputFile = open(g_audioOutputFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
    if (audioOutputFile < 0)
    {
      fprintf(stderr, "Could not open audio output file \"%s\"\n", g_audioOutputFile);
      goto bail;
    }
  }

  while (displayModeIterator->Next(&displayMode) == S_OK)
  {
    if (g_videoModeIndex == displayModeCount)
    {
      BMDDisplayModeSupport result;
      const char *displayModeName;

      foundDisplayMode = true;
      displayMode->GetName(&displayModeName);
      selectedDisplayMode = displayMode->GetDisplayMode();

      deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);

      if (result == bmdDisplayModeNotSupported)
      {
        fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
        goto bail;
      }

      if (inputFlags & bmdVideoInputDualStream3D)
      {
        if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D))
        {
          fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
          goto bail;
        }
      }
      fprintf(stderr, "Selecting mode: %s\n", displayModeName);

      break;
    }
    displayModeCount++;
    displayMode->Release();
  }

  if (!foundDisplayMode)
  {
    fprintf(stderr, "Invalid mode %d specified\n", g_videoModeIndex);
    goto bail;
  }

  {
    // Query the DeckLink for its configuration interface
    result = deckLinkInput->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
    if (result != S_OK)
    {
      fprintf(stderr, "Could not obtain the IDeckLinkConfiguration interface: %08x\n", result);
    }
  }

  BMDVideoConnection conn;
  switch (vinput) {
  case 0:
    conn = bmdVideoConnectionSDI;
    break;
  case 1:
    conn = bmdVideoConnectionHDMI;
    break;
  case 2:
    conn = bmdVideoConnectionComponent;
    break;
  case 3:
    conn = bmdVideoConnectionComposite;
    break;
  case 4:
    conn = bmdVideoConnectionSVideo;
    break;
  case 5:
    conn = bmdVideoConnectionOpticalSDI;
    break;
  default:
    break;
  }
  conn = vinput;
  // Set the input desired
  result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoInputConnection, conn);
  if(result != S_OK) {
    fprintf(stderr, "Cannot set the input to [%d]\n", conn);
    goto bail;
  }

  // check input
  result = deckLinkConfiguration->GetInt(bmdDeckLinkConfigVideoInputConnection, &vport);
  if (vport == bmdVideoConnectionSDI)
    fprintf(stderr, "Before Input configured for SDI\n");
  if (vport == bmdVideoConnectionHDMI)
    fprintf(stderr, "Before Input configured for HDMI\n");


  if (deckLinkConfiguration->SetFlag(bmdDeckLinkConfigFieldFlickerRemoval, flickerremoval) == S_OK) {
    fprintf(stderr, "Flicker removal set : %d\n", flickerremoval);
  }
  else {
    fprintf(stderr, "Flicker removal NOT set\n");
  }

  if (deckLinkConfiguration->SetFlag(bmdDeckLinkConfigUse1080pNotPsF, pnotpsf) == S_OK) {
    fprintf(stderr, "bmdDeckLinkConfigUse1080pNotPsF: %d\n", pnotpsf);
  }
  else {
    fprintf(stderr, "bmdDeckLinkConfigUse1080pNotPsF NOT set\n");
  }

  //if (deckLinkConfiguration->SetFlag(bmdDeckLinkConfigVideoInputConnection, conn) == S_OK) {
  //fprintf(stderr, "Input set to: %d\n", vinput);
  //}

  result = deckLinkConfiguration->GetInt(bmdDeckLinkConfigVideoInputConnection, &vport);
  if (vport == bmdVideoConnectionSDI)
    fprintf(stderr, "After Input configured for SDI\n");
  if (vport == bmdVideoConnectionHDMI)
    fprintf(stderr, "After Input configured for HDMI\n");



  result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
  if(result != S_OK)
  {
    fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
    goto bail;
  }


  result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
  if(result != S_OK)
  {
    fprintf(stderr, "Failed to enable audio input with 48kHz, depth %d, %d channels\n",
            g_audioSampleDepth, g_audioChannels);
    goto bail;
  }

  if (useSAGE) {
    // Get capture size
    sageW = displayMode->GetWidth();
    sageH = displayMode->GetHeight();

    // Compute the actual framerate
    displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);
    sageFPS = (double)frameRateScale / (double)frameRateDuration;
    fprintf(stderr, "GetFrameRate: %10ld %10ld --> fps %g\n", (long)frameRateScale, (long)frameRateDuration, sageFPS);

    // SAGE setup
    fprintf(stderr, "SAIL configuration was initialized by decklinkcapture.conf\n");
    fprintf(stderr, "\twidth %d height %d fps %g\n", sageW, sageH, sageFPS);

    if (pixelFormat != bmdFormat8BitYUV)
      fprintf(stderr, "SAGE works only with pixelFormat 1 (bmdFormat8BitYUV)\n");

    //cfg.rowOrd = TOP_TO_BOTTOM;
    sageInf = createSAIL("decklinkcapture", sageW, sageH, PIXFMT_YUV, NULL, TOP_TO_BOTTOM);
    SAGE_PRINTLOG("SAIL initialized");

#if defined(DEINTERLACING)
    //////////////////////////////////
    // FFMPEG for de-interlacing
    //////////////////////////////////
    if (useDEINT) {
      picture_prev = (AVPicture*) alloc_picture(SAGE_YUV, sageW, sageH);
      picture_curr = (AVPicture*) alloc_picture(SAGE_YUV, sageW, sageH);
      picture_next = (AVPicture*) alloc_picture(PIX_FMT_YUV422P, sageW, sageH);

      img_convert_toplanar = sws_getContext(sageW, sageH, SAGE_YUV,
                                            sageW, sageH, PIX_FMT_YUV422P, SWS_BICUBIC, NULL, NULL, NULL);
      if (img_convert_toplanar == NULL) {
        SAGE_PRINTLOG("Cannot img_convert_toplanar converter");
        exit(1);
      }
      img_convert_topacked = sws_getContext(sageW, sageH, PIX_FMT_YUV422P,
                                            sageW, sageH, SAGE_YUV, SWS_BICUBIC, NULL, NULL, NULL);
      if (img_convert_topacked == NULL) {
        SAGE_PRINTLOG("Cannot img_convert_topacked converter");
        exit(1);
      }
    }
    //////////////////////////////////
#endif

    //////////////////////////////////
#define BIG_AUDIO_BUFFER (10*1024*1024)
    char *audio_buffer;
    int buffer_length = BIG_AUDIO_BUFFER * g_audioChannels * (g_audioSampleDepth / 8);
    audio_buffer = (char*)malloc(buffer_length);
    memset(audio_buffer, 0, buffer_length);
    audio_ring = new Ring_Buffer(audio_buffer, buffer_length);
    fprintf(stderr, "Audio ring created\n");
    /////////////////////////////////

    //////////////////////////////////
    // SAM
    //////////////////////////////////
    if (useSAM) {
      char *displayModeString = NULL;
      char *inputString = NULL;
      const char *modelName = NULL;
      // Sadly, jack uses short names
      char samName[32];
      memset(samName, 0, 32);
      displayMode->GetName((const char **) &displayModeString);
      deckLink->GetModelName(&modelName);
      inputString = connectionName(vport);
      snprintf(samName, 32, "[%s-%s] [%d-%d]", inputString, displayModeString, g_audioChannels, g_audioSampleDepth);
      free(displayModeString);

      SAGE_PRINTLOG("Using SAM IP: %s", samIP);

      m_sac = new sam::StreamingAudioClient();
      m_sac->init(g_audioChannels, sam::TYPE_BASIC, samName, samIP, 7770);
      int returnCode = m_sac->start(0, 0, sageW, sageH, 0);

      if (physicalInputs)
      {
        unsigned int* inputs = new unsigned int[g_audioChannels];
        for (unsigned int n = 0; n < g_audioChannels; n++)
        {
          inputs[n] = n + 1;
        }
        SAGE_PRINTLOG("Calling SetPhysicalInputs");
        m_sac->setPhysicalInputs(inputs);
        delete[] inputs;
      }
      else {
        // register SAC callback
        m_sac->setAudioCallback(handle_sac_audio_callback, NULL);
      }

    }
    //////////////////////////////////
    uiLib.init((char*)"fsManager.conf");
    uiLib.connect(sageInf->Config().fsIP);
    uiLib.sendMessage(SAGE_UI_REG,(char*)" ");
    pthread_t thId;
    if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0){
      SAGE_PRINTLOG("Cannot create a UI thread\n");
    }

  }


  result = deckLinkInput->StartStreams();
  if(result != S_OK)
  {
    goto bail;
  }

  // All Okay.
  exitStatus = 0;

  // Block main thread until signal occurs
  pthread_mutex_lock(&sleepMutex);
  pthread_cond_wait(&sleepCond, &sleepMutex);
  pthread_mutex_unlock(&sleepMutex);
  fprintf(stderr, "Stopping Capture\n");

  bail:

  if (videoOutputFile)
    close(videoOutputFile);
  if (audioOutputFile)
    close(audioOutputFile);

  if (displayModeIterator != NULL)
  {
    displayModeIterator->Release();
    displayModeIterator = NULL;
  }

  if (deckLinkInput != NULL)
  {
    deckLinkInput->Release();
    deckLinkInput = NULL;
  }

  if (deckLink != NULL)
  {
    deckLink->Release();
    deckLink = NULL;
  }

  if (deckLinkIterator != NULL)
    deckLinkIterator->Release();

  if (m_sac) delete m_sac;
  if (sageInf) deleteSAIL(sageInf);

  return exitStatus;
}
