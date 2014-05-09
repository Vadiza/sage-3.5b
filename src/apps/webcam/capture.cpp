/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>
#include <libv4l2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

/////////////////////
// headers for SAGE
#include "pixfc-sse.h"
#include "libsage.h"
sail *sageInf; // sail object
int winWidth, winHeight;

PixFcPixelFormat                src_fmt = PixFcYUYV;
PixFcPixelFormat                dst_fmt = PixFcRGB24;
struct PixFcSSE *               pixfc = NULL;
////////////////////

static void stop_capturing(void);
static void uninit_device(void);
static void close_device(void);

typedef enum {
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
} io_method;

struct buffer {
  void *                  start;
  size_t                  length;
};

static char *           dev_name        = NULL;
static io_method  io    = IO_METHOD_MMAP;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;

static void
errno_exit                      (const char *           s)
{
  fprintf (stderr, "%s error %d, %s\n",
           s, errno, strerror (errno));

  exit (EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
  int r;

  do r = v4l2_ioctl(fd, request, arg);
  while (-1 == r && EINTR == errno);

  return r;
}

void appQuit(sail* si)
{
  stop_capturing ();
  uninit_device ();
  close_device ();
  destroy_pixfc(pixfc);
}

static void
process_image(const void *p)
{
#if 0
  void *buffer = nextBuffer(sageInf);
  pixfc->convert(pixfc, (void*)p, buffer);
  swapBuffer(sageInf);
#else
  swapWithBuffer(sageInf, (unsigned char*)p);
#endif

  processMessages(sageInf,NULL,NULL,NULL);
}

static int
read_frame      (void)
{
  struct v4l2_buffer buf;
  unsigned int i;

  switch (io) {
  case IO_METHOD_READ:
    if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
      switch (errno) {
      case EAGAIN:
        return 0;

      case EIO:
        /* Could ignore EIO, see spec. */

        /* fall through */

      default:
        errno_exit ("read");
      }
    }
    process_image (buffers[0].start);

    break;

  case IO_METHOD_MMAP:
    CLEAR (buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    xioctl (fd, VIDIOC_DQBUF, &buf);
    assert (buf.index < n_buffers);

    //SAGE_PRINTLOG("Got buffer: %d bytes", buffers[buf.index].length);
    process_image (buffers[buf.index].start);

    xioctl (fd, VIDIOC_QBUF, &buf);

    break;

  case IO_METHOD_USERPTR:
    CLEAR (buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
      switch (errno) {
      case EAGAIN:
        return 0;

      case EIO:
        /* Could ignore EIO, see spec. */

        /* fall through */

      default:
        errno_exit ("VIDIOC_DQBUF");
      }
    }

    for (i = 0; i < n_buffers; ++i)
      if (buf.m.userptr == (unsigned long) buffers[i].start
          && buf.length == buffers[i].length)
        break;

    assert (i < n_buffers);

    process_image ((void *) buf.m.userptr);

    if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
      errno_exit ("VIDIOC_QBUF");

    break;
  }

  return 1;
}

static void
mainloop                        (void)
{
  // unsigned int count;
  // count = 100;
  //while (count-- > 0) {
  while (true) {
    for (;;) {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO (&fds);
      FD_SET (fd, &fds);

      /* Timeout. */
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select (fd + 1, &fds, NULL, NULL, &tv);

      if (-1 == r) {
        if (EINTR == errno)
          continue;

        errno_exit ("select");
      }

      if (0 == r) {
        fprintf (stderr, "select timeout\n");
        //exit (EXIT_FAILURE);
      }

      if (read_frame ())
        break;

      /* EAGAIN - continue select loop. */
    }
  }
}

static void
stop_capturing                  (void)
{
  enum v4l2_buf_type type;

  switch (io) {
  case IO_METHOD_READ:
    /* Nothing to do. */
    break;

  case IO_METHOD_MMAP:
  case IO_METHOD_USERPTR:
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
      errno_exit ("VIDIOC_STREAMOFF");

    break;
  }
}

static void
start_capturing                 (void)
{
  unsigned int i;
  enum v4l2_buf_type type;

  switch (io) {
  case IO_METHOD_READ:
    /* Nothing to do. */
    break;

  case IO_METHOD_MMAP:
    for (i = 0; i < n_buffers; ++i) {
      struct v4l2_buffer buf;

      CLEAR (buf);

      buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory      = V4L2_MEMORY_MMAP;
      buf.index       = i;

      xioctl (fd, VIDIOC_QBUF, &buf);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl (fd, VIDIOC_STREAMON, &type);

    break;

  case IO_METHOD_USERPTR:
    for (i = 0; i < n_buffers; ++i) {
      struct v4l2_buffer buf;

      CLEAR (buf);

      buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory      = V4L2_MEMORY_USERPTR;
      buf.index       = i;
      buf.m.userptr = (unsigned long) buffers[i].start;
      buf.length      = buffers[i].length;

      if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
        errno_exit ("VIDIOC_QBUF");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
      errno_exit ("VIDIOC_STREAMON");

    break;
  }
}

static void
uninit_device                   (void)
{
  unsigned int i;

  switch (io) {
  case IO_METHOD_READ:
    free (buffers[0].start);
    break;

  case IO_METHOD_MMAP:
    for (i = 0; i < n_buffers; ++i)
      if (-1 == munmap (buffers[i].start, buffers[i].length))
        errno_exit ("munmap");
    break;

  case IO_METHOD_USERPTR:
    for (i = 0; i < n_buffers; ++i)
      free (buffers[i].start);
    break;
  }

  free (buffers);
}

static void
init_read     (unsigned int   buffer_size)
{
  buffers = (buffer*)calloc (1, sizeof (*buffers));

  if (!buffers) {
    fprintf (stderr, "Out of memory\n");
    exit (EXIT_FAILURE);
  }

  buffers[0].length = buffer_size;
  buffers[0].start = malloc (buffer_size);

  if (!buffers[0].start) {
    fprintf (stderr, "Out of memory\n");
    exit (EXIT_FAILURE);
  }
}

static void
init_mmap     (void)
{
  struct v4l2_requestbuffers req;

  CLEAR (req);

  req.count               = 2;
  req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory              = V4L2_MEMORY_MMAP;

  if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      fprintf (stderr, "%s does not support "
               "memory mapping\n", dev_name);
      exit (EXIT_FAILURE);
    } else {
      errno_exit ("VIDIOC_REQBUFS");
    }
  }

  if (req.count < 2) {
    fprintf (stderr, "Insufficient buffer memory on %s\n",
             dev_name);
    exit (EXIT_FAILURE);
  }

  buffers = (buffer*)calloc (req.count, sizeof (*buffers));

  if (!buffers) {
    fprintf (stderr, "Out of memory\n");
    exit (EXIT_FAILURE);
  }

  for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    struct v4l2_buffer buf;

    CLEAR (buf);

    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = n_buffers;

    if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
      errno_exit ("VIDIOC_QUERYBUF");

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start =
      v4l2_mmap (NULL /* start anywhere */,
                 buf.length,
                 PROT_READ | PROT_WRITE /* required */,
                 MAP_SHARED /* recommended */,
                 fd, buf.m.offset);

    if (MAP_FAILED == buffers[n_buffers].start)
      errno_exit ("mmap");
  }
}

static void
init_userp      (unsigned int   buffer_size)
{
  struct v4l2_requestbuffers req;
  unsigned int page_size;

  page_size = getpagesize ();
  buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

  CLEAR (req);

  req.count               = 4;
  req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory              = V4L2_MEMORY_USERPTR;

  if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      fprintf (stderr, "%s does not support "
               "user pointer i/o\n", dev_name);
      exit (EXIT_FAILURE);
    } else {
      errno_exit ("VIDIOC_REQBUFS");
    }
  }

  buffers = (buffer*)calloc (4, sizeof (*buffers));

  if (!buffers) {
    fprintf (stderr, "Out of memory\n");
    exit (EXIT_FAILURE);
  }

  for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
    buffers[n_buffers].length = buffer_size;
    buffers[n_buffers].start = memalign (/* boundary */ page_size,
                                         buffer_size);

    if (!buffers[n_buffers].start) {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }
  }
}

static int float_to_fraction_recursive(double f, double p, int *num, int *den)
{
  int whole = (int)f;
  f = fabs(f - whole);

  if(f > p) {
    int n, d;
    int a = float_to_fraction_recursive(1 / f, p + p / f, &n, &d);
    *num = d;
    *den = d * a + n;
  }
  else {
    *num = 0;
    *den = 1;
  }
  return whole;
}
static void float_to_fraction(float f, int *num, int *den)
{
  int whole = float_to_fraction_recursive(f, FLT_EPSILON, num, den);
  *num += whole * *den;
}

static void
init_device                     (void)
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;
  unsigned int min;

  if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
    if (EINVAL == errno) {
      fprintf (stderr, "%s is no V4L2 device\n",
               dev_name);
      exit (EXIT_FAILURE);
    } else {
      errno_exit ("VIDIOC_QUERYCAP");
    }
  }

  fprintf(stderr, "Detected v4l2 device: %s\n", cap.card);
  uint32_t u32 = cap.version;
  fprintf(stderr, "Driver: %s, version: %d.%d.%d\n", cap.driver,
          (u32 >> 16) & 0xff,
          (u32 >>  8) & 0xff,
          u32         & 0xff);
  fprintf(stderr, "Capabilities: 0x%08X\n", cap.capabilities);

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf (stderr, "%s is no video capture device\n",
             dev_name);
    exit (EXIT_FAILURE);
  }
  else {
    struct v4l2_fmtdesc     fmtdesc;
    struct v4l2_format      format;
    fprintf (stderr, "%s is video capture device\n", dev_name);
    for (int i = 0;; i++) {
      CLEAR(fmtdesc);
      fmtdesc.index = i;
      fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc))
        break;
      fprintf (stderr, "    VIDIOC_ENUM_FMT(%d,VIDEO_CAPTURE)\n",i);
      fprintf (stderr, "    \tdescription: %s\n", fmtdesc.description);
      u32 = fmtdesc.pixelformat;
      unsigned char *ptr = (unsigned char*)&fmtdesc.pixelformat;
      fprintf (stderr, "    \tpixel format: 0x%08x [%c%c%c%c]\n", u32,
               isprint(ptr[0]) ? ptr[0] : '.',
               isprint(ptr[1]) ? ptr[1] : '.',
               isprint(ptr[2]) ? ptr[2] : '.',
               isprint(ptr[3]) ? ptr[3] : '.');
    }
    CLEAR(format);
    format.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd,VIDIOC_G_FMT,&format)) {
      perror("VIDIOC_G_FMT(VIDEO_CAPTURE)");
    } else {
      fprintf(stderr, "    VIDIOC_G_FMT(VIDEO_CAPTURE)\n");
      fprintf(stderr, "    \tsize %dx%d\n", format.fmt.pix.width, format.fmt.pix.height);
      fprintf(stderr, "    \tformat %d\n", format.fmt.pix.pixelformat);
      fprintf(stderr,     "\tfield %d\n", format.fmt.pix.field);
      winWidth  = format.fmt.pix.width;
      winHeight = format.fmt.pix.height;
    }

#if 0
    struct v4l2_streamparm setfps;
    CLEAR(setfps);
    // Convert the frame rate into a fraction for V4L2
    int n = 0, d = 0;
    float_to_fraction(25.0, &n, &d);
    setfps.parm.capture.timeperframe.numerator = d;
    setfps.parm.capture.timeperframe.denominator = n;

    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd,VIDIOC_S_PARM,&setfps)) {
      perror("VIDIOC_S_PARM(VIDEO_CAPTURE)");
    } else {
      float confirmed_fps = (float)setfps.parm.capture.timeperframe.denominator /
        (float)setfps.parm.capture.timeperframe.numerator;
      fprintf(stderr, "    VIDIOC_S_PARM(VIDEO_CAPTURE): %.f\n", confirmed_fps);
    }
#endif

    struct v4l2_streamparm getfps;
    CLEAR(getfps);
    getfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd,VIDIOC_G_PARM,&getfps)) {
      perror("VIDIOC_G_PARM(VIDEO_CAPTURE)");
    } else {
      float confirmed_fps = (float)getfps.parm.capture.timeperframe.denominator /
        (float)getfps.parm.capture.timeperframe.numerator;
      fprintf(stderr, "    VIDIOC_G_PARM(VIDEO_CAPTURE): %.f\n", confirmed_fps);
    }
    fprintf (stderr, "\n");


  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT))
    fprintf (stderr, "%s doesn't support single-planar API \n", dev_name);
  else
    fprintf (stderr, "%s supports  single-planar API \n", dev_name);

  if (!(cap.capabilities &V4L2_CAP_VIDEO_OVERLAY))
    fprintf (stderr, "%s doesn't support overlay \n", dev_name);
  else
    fprintf (stderr, "%s supports overlay \n", dev_name);

  switch (io) {
  case IO_METHOD_READ:
    if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
      fprintf (stderr, "%s does not support read i/o\n", dev_name);
      exit (EXIT_FAILURE);
    } else
      fprintf (stderr, "%s does support read i/o\n", dev_name);

    break;

  case IO_METHOD_MMAP:
  case IO_METHOD_USERPTR:
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
      fprintf (stderr, "%s does not support streaming i/o\n", dev_name);
      exit (EXIT_FAILURE);
    }
    else
      fprintf (stderr, "%s does support streaming i/o\n", dev_name);

    break;
  }


  /* Select video input, video standard and tune here. */


  CLEAR (cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
      switch (errno) {
      case EINVAL:
        /* Cropping not supported. */
        break;
      default:
        /* Errors ignored. */
        break;
      }
    }
  } else {
    /* Errors ignored. */
  }

  CLEAR (fmt);

  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = winWidth;
  fmt.fmt.pix.height      = winHeight;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24; // V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field       = V4L2_FIELD_NONE;    // V4L2_FIELD_ANY;

  // if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
  //   errno_exit ("VIDIOC_S_FMT");
  xioctl (fd, VIDIOC_S_FMT, &fmt);

  /* Note VIDIOC_S_FMT may change width and height. */

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  switch (io) {
  case IO_METHOD_READ:
    init_read (fmt.fmt.pix.sizeimage);
    break;

  case IO_METHOD_MMAP:
    init_mmap ();
    break;

  case IO_METHOD_USERPTR:
    init_userp (fmt.fmt.pix.sizeimage);
    break;
  }
}

static void
close_device                    (void)
{
  if (-1 == close (fd))
    errno_exit ("close");

  fd = -1;
}

static void
open_device                     (void)
{
  struct stat st;

  if (-1 == stat (dev_name, &st)) {
    fprintf (stderr, "Cannot identify '%s': %d, %s\n",
             dev_name, errno, strerror (errno));
    exit (EXIT_FAILURE);
  }

  if (!S_ISCHR (st.st_mode)) {
    fprintf (stderr, "%s is no device\n", dev_name);
    exit (EXIT_FAILURE);
  }

  fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);

  if (-1 == fd) {
    fprintf (stderr, "Cannot open '%s': %d, %s\n",
             dev_name, errno, strerror (errno));
    exit (EXIT_FAILURE);
  }
}

static void
usage                           (FILE *                 fp,
                                 int                    argc,
                                 char **                argv)
{
  fprintf (fp,
           "Usage: %s [options]\n\n"
           "Options:\n"
           "-d | --device name   Video device name [/dev/video0]\n"
           "-h | --help          Print this message\n"
           "-m | --mmap          Use memory mapped buffers\n"
           "-r | --read          Use read() calls\n"
           "-u | --userp         Use application allocated buffers\n"
           "",
           argv[0]);
}

static const char short_options [] = "d:mruw:h:";

static const struct option
long_options [] = {
  { "device",     required_argument,      NULL,           'd' },
  { "mmap",       no_argument,            NULL,           'm' },
  { "read",       no_argument,            NULL,           'r' },
  { "userp",      no_argument,            NULL,           'u' },
  { "w",      required_argument,          NULL,           'w' },
  { "h",      required_argument,          NULL,           'h' },
  { 0, 0, 0, 0 }
};

int
main (int                    argc,
      char **                argv)
{
  dev_name = (char*)"/dev/video0";

  //winWidth  = 864;
  //winHeight = 480;
  // winWidth  = 640;
  // winHeight = 480;
  //winWidth  = 800;
  //winHeight = 504;


  for (;;) {
    int index;
    int c;

    c = getopt_long (argc, argv,
                     short_options, long_options,
                     &index);

    if (-1 == c)
      break;

    switch (c) {
    case 0: /* getopt_long() flag */
      break;

    case 'd':
      dev_name = optarg;
      break;

    case 'm':
      io = IO_METHOD_MMAP;
      break;

    case 'r':
      io = IO_METHOD_READ;
      break;

    case 'u':
      io = IO_METHOD_USERPTR;
      break;

    case 'w':
      winWidth = atoi(optarg);
      break;

    case 'h':
      winHeight = atoi(optarg);
      break;

    default:
      usage (stderr, argc, argv);
      exit (EXIT_FAILURE);
    }
  }

  open_device ();

  init_device ();

  /////////////////////////////
  // SAGE
  /////////////////////////////
  sageInf = createSAIL("webcam", winWidth, winHeight, PIXFMT_888, NULL, TOP_TO_BOTTOM);
  SAGE_PRINTLOG("SAGE Initialized: %dx%d", winWidth, winHeight);

  uint32_t code;
  code = create_pixfc(&pixfc, src_fmt, dst_fmt, winWidth, winHeight, PixFcFlag_Default);
  if (code !=0) {
    fprintf (stderr, "Error create_pixfc: %d\n", code);
    exit (EXIT_FAILURE);
  }
  /////////////////////////////

  start_capturing ();

  mainloop ();

  stop_capturing ();

  uninit_device ();

  close_device ();

  destroy_pixfc(pixfc);

  exit (EXIT_SUCCESS);

  return 0;
}
