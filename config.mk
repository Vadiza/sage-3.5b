MACHINE = $(shell uname -s)
   #Darwin  MacOSX
   #Linux   linux
   #Solaris SunOS

ARCHITECTURE = $(shell uname -m)
   #i386    MacOSX
   #i686    Linux 32bit
   #x86_64  Linux 64bit
   #ia64    Linux Itanium 64bit
   #ppc64   Linux PPC PS3

ifneq ($(MACHINE), Darwin)
GLSL_YUV = 1
endif

# To enable audio, uncomment the following line
#AUDIO = 1

# Compilers
cc = gcc
CC = g++

SAGE_CFLAGS = -Wno-deprecated -Wno-deprecated-declarations -fPIC
SAGE_LDFLAGS =

# SDL
SDL_CFLAGS = `sdl-config --cflags`
SDL_LDFLAGS = `sdl-config --libs`

# GLUT
GLUT_CFLAGS =
GLUT_LDFLAGS = -lglut

# READLINE settings
READLINE_CFLAGS =
READLINE_LDFLAGS = -lreadline

ifdef AUDIO
#PORTAUDIO
PORTAUDIO_DIR = /usr/local
PORTAUDIO_CFLAGS = -I${PORTAUDIO_DIR}/include -DSAGE_AUDIO
PORTAUDIO_LDFLAGS = -L${PORTAUDIO_DIR}/lib -lportaudio -lasound
endif

# SDL_ttf (freetype font library)
FONT_LDFLAGS = -lSDL_ttf

# imagemagick
MAGICK_CFLAGS = `Wand-config --cflags --cppflags`
MAGICK_LDFLAGS = -L/usr/lib64 -lMagickWand -lMagickCore -llcms -ltiff -lfreetype -ljpeg -lfontconfig -lICE -lX11 -lXt -lbz2 -lz -lm -lgomp -lpthread  -lpng -ltiff

# SAIL library name
SAIL_LIB = libsail.so

# QUANTA library name	
QUANTA_LIB = libquanta.so

# how to build a shared library
SHARED_LDFLAGS = -shared 

# GPU programming setting
ifeq ($(GLSL_YUV), 1)
  GLEW_CFLAGS =
  GLEW_LDFLAGS = -lGLEW
  GLSL_YUV_DEFINE = -DGLSL_YUV
else
  GLEW_CFLAGS =
  GLEW_LDFLAGS = -lGLEW
  GLSL_YUV_DEFINE =
endif

ifeq ($(MACHINE), Darwin)
  # SAIL library name
  SAIL_LIB = libsail.dylib

ifdef AUDIO
  PORTAUDIO_DIR = /opt/local
  PORTAUDIO_CFLAGS = -I${PORTAUDIO_DIR}/include -DSAGE_AUDIO
  PORTAUDIO_LDFLAGS = -L${PORTAUDIO_DIR}/lib -lportaudio
endif

  SDL_CFLAGS = `sdl-config --cflags`
  SDL_LDFLAGS = `sdl-config --libs`
  FONT_LDFLAGS = `pkg-config --libs SDL_ttf`

  SAGE_CFLAGS = -fPIC
  SAGE_LDFLAGS =

  MAGICK_CFLAGS = `pkg-config --cflags Wand`
  MAGICK_LDFLAGS = `pkg-config --libs Wand` -ljpeg -lpng -ltiff

  # QUANTA library name
  QUANTA_LIB = libquanta.dylib

  # READLINE settings
  READLINE_CFLAGS = -I/usr/local/Cellar/readline/6.2.4/include
  READLINE_LDFLAGS = -lreadline

  # Lower-level graphics library
  XLIBS = -framework OpenGL -lobjc -framework AGL -framework Carbon -framework QuartzCore

  # how to build a shared library
  SHARED_LDFLAGS=-dynamiclib -flat_namespace -undefined suppress

else

ifeq ($(ARCHITECTURE), x86_64)
  # Lower-level graphics library
  XLIBS = -L/usr/X11R6/lib64 -lGLU -lGL -lXmu -lXi -lXext -lX11
else

ifeq ($(ARCHITECTURE), ppc64)
  # SDL
  SDL_CFLAGS = `sdl-config --cflags`
  SDL_LDFLAGS = -lSDL

  # Lower-level graphics library
  XLIBS = -L/usr/X11R6/lib -lGLU -lGL -lXmu -lXi -lXext -lX11
else

ifeq ($(ARCHITECTURE), ia64)
  # Lower-level graphics library
  XLIBS = -L/usr/X11R6/lib64 -lGLU -lGL -lXmu -lXi -lXext -lX11
else

  # anything else is 32bit 

  # READLINE settings
  READLINE_LDFLAGS = -lreadline

  # Lower-level graphics library
  XLIBS = -L/usr/X11R6/lib -lGLU -lGL -lXmu -lXi -lXext -lX11

ifdef AUDIO
  PORTAUDIO_LDFLAGS = -L${PORTAUDIO_DIR}/lib -lportaudio -lasound
endif

endif
endif
endif
endif

ifeq ($(MACHINE), SunOS)

ifndef SUN_GCC
cc = cc
CC = CC
SAGE_CFLAGS = -Kpic -m64
SUN_LDFLAGS = -lrt -lnsl -lsocket -lCrun -lCstd
SHARED_LDFLAGS = -G -Kpic -m64 
else
cc = cc
CC = c++
SAGE_CFLAGS = -m64 -Wno-deprecated -fPIC 
SUN_LDFLAGS = -lrt -lnsl -lsocket 
SHARED_LDFLAGS = -shared -m64 
endif

SAGE_LDFLAGS = -m64
SUN_INCLUDE = -I/usr/local/include 

GLUT_CFLAGS = -I/opt/SUNWfreeglut/include
GLUT_LDFLAGS = -L/opt/SUNWfreeglut/lib/amd64 -lglut

# READLINE settings
READLINE_CFLAGS = -I/usr/local/include 
READLINE_LDFLAGS = -L/usr/local/lib/64 -lreadline -lcurses

endif
