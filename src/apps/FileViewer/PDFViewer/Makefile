TOP_DIR = ../../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/FileViewer/PDFViewer
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

# use the PDF Poppler library
#USE_POPPLER=1
# minimum poppler version 0.5.x
USE_POPPLER=$(shell pkg-config --atleast-version=0.5 poppler && echo 1)

ifeq ($(USE_POPPLER), 1)

USE_POPPLER12=$(shell pkg-config --atleast-version=0.12 poppler && echo 1)
USE_POPPLER20=$(shell pkg-config --atleast-version=0.20 poppler && echo 1)

ifeq ($(USE_POPPLER20), 1)
	PVERSION=20
else
	ifeq ($(USE_POPPLER12), 1)
		PVERSION=12
	else
		PVERSION=5
	endif
endif

CFLAGS = -DUSE_POPPLER -DSAGE_POPPLER_VERSION=$(PVERSION) `pkg-config --cflags poppler` `Wand-config --cflags --cppflags` -I$(SRC_DIR)/sage -fno-stack-protector -I$(SRC_DIR)/QUANTA $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = `pkg-config --libs poppler` $(MAGICK_LDFLAGS) -L/usr/lib64 -lfreetype -lfontconfig -lbz2 -lz -lm -lgomp -lpthread -L$(LIB_DIR) -lsail -L$(LIB_DIR) -lquanta $(PORTAUDIO_LDFLAGS) -lpthread

else

CFLAGS = `Wand-config --cflags --cppflags` -I$(SRC_DIR)/sage -fno-stack-protector -I$(SRC_DIR)/QUANTA $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = `Wand-config --ldflags --libs` -L$(LIB_DIR) -lsail -lquanta $(PORTAUDIO_LDFLAGS) -lpthread -lstdc++

endif


#----------------  BEGIN FastDXT stuff  ----------------#

OPTIM=1

ifeq ($(MACHINE), Darwin)

CFLAGS += -march=core2
LDFLAGS += -L/opt/local/lib

CFLAGS += -I/opt/local/include

ifeq ($(ARCHITECTURE), i386)
# Intel Mac
ifeq ($(OPTIM), 1)
CFLAGS += -DDXT_INTR
endif
endif

ifeq ($(ARCHITECTURE), powerpc)
# PowerPC Mac
echo "Disabling SSE instructions on PowerPC"
CFLAGS +=
endif
endif

ifeq ($(ARCHITECTURE), x86_64)
CFLAGS += -msse2 -DDXT_INTR
else
# everything considered i386/linux
CFLAGS += -msse2 -DDXT_INTR
endif

#------------------  END FastDXT stuff  -------------------#

SOURCES = pdfviewer.cpp dxt.cpp libdxt.cpp util.cpp intrinsic.cpp 
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

TARGETS = $(BIN_DIR)/pdfviewer

all: $(TARGETS)

$(BIN_DIR)/pdfviewer: $(OBJECTS)
	g++ -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(OBJ_DIR)
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *~ $(TARGETS) $(OBJ_DIR)

distclean: clean

-include $(DEPENDS)
