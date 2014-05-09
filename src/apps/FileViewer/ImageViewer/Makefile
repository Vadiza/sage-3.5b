TOP_DIR = ../../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/FileViewer/ImageViewer
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

DXT=1     # comment out for no DXT support
#CAIRO=1  # comment out for no CAIRO support

CFLAGS = $(SAGE_CFLAGS) -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS += $(SAGE_LDFLAGS) -L$(LIB_DIR) -lsail -lquanta -lz -ljpeg $(PORTAUDIO_LDFLAGS)

ifneq ($(MACHINE), Darwin)
	CFLAGS += `Wand-config --cflags --cppflags` -fno-stack-protector
	LDFLAGS += -L/usr/lib64 -lMagickWand -lMagickCore -llcms -ltiff -lfreetype -ljpeg -lfontconfig -lXext -lSM -lICE -lX11 -lXt -lbz2 -lz -lm -lgomp -lpthread -lpthread
else
	CFLAGS += $(MAGICK_CFLAGS)
	LDFLAGS += $(MAGICK_LDFLAGS)
endif

SOURCES = imageviewer.cpp
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

TARGETS = $(BIN_DIR)/imageviewer

#----------------  BEGIN CAIRO stuff  ----------------#
ifdef CAIRO
CFLAGS += -DUSE_CAIRO
LDFLAGS += -lcairo
endif

#----------------  BEGIN FastDXT stuff  ----------------#
ifdef DXT
SOURCES = imageviewer.cpp dxt.cpp libdxt.cpp util.cpp intrinsic.cpp
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

DXT_SOURCES = imgToDxt.cpp dxt.cpp libdxt.cpp util.cpp intrinsic.cpp
DXT_OBJECTS = $(addprefix $(OBJ_DIR)/,${DXT_SOURCES:.cpp=.o})
DXT_DEPENDS = $(addprefix $(OBJ_DIR)/,${DXT_SOURCES:.cpp=.d})

TARGETS += $(BIN_DIR)/fileServer/imgToDxt $(BIN_DIR)/fileServer/dirToDxt.pyc
CFLAGS += -DUSE_DXT

OPTIM=1

MACHINE=$(shell uname -s)
ARCHITECTURE=$(shell uname -p)

ifeq ($(MACHINE), Darwin)

CFLAGS += -I/opt/local/include

ifeq ($(ARCHITECTURE), i386)
# Intel Mac
ifeq ($(OPTIM), 1)
# MacPro
# CXXFLAGS += -march=nocona -msse3 -DDXT_INTR

# MacbookPro
#CFLAGS += -march=i686 -msse2 -DDXT_INTR 
CFLAGS += -march=core2 -DDXT_INTR
endif
endif

ifeq ($(ARCHITECTURE), powerpc)
# PowerPC Mac
#echo "Disabling SSE instructions on PowerPC"
CFLAGS +=
endif

endif

ifeq ($(ARCHITECTURE), x86_64)
CFLAGS += -msse2 -DDXT_INTR

else

# everything considered i386/linux
CFLAGS += -msse2 -DDXT_INTR

endif

endif
#------------------  END FastDXT stuff  -------------------#

all: $(TARGETS)

$(BIN_DIR)/imageviewer: $(OBJECTS)
	g++ -o $@ $(OBJECTS) $(LDFLAGS)

$(BIN_DIR)/fileServer/imgToDxt: $(DXT_OBJECTS)
	mkdir -p $(BIN_DIR)/fileServer
	g++ -o $@ $(DXT_OBJECTS) $(LDFLAGS)

$(BIN_DIR)/fileServer/dirToDxt.pyc: dirToDxt.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(OBJ_DIR) 
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *~ $(TARGETS) $(OBJ_DIR)

distclean: clean

-include $(DEPENDS) $(DXT_DEPENDS)
