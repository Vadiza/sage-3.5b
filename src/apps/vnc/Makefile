TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/vnc
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

#USE_QT=1

ifeq ($(MACHINE), Darwin)
VNCTEST=/usr/local/bin/libvncserver-config
else
VNCTEST=/usr/bin/libvncserver-config
endif

# Test for installation of libvncserver/libvncclient
#   otherwise, it's using the old code
ifneq "$(wildcard $(VNCTEST) )" ""
SOURCES = main.cpp
CFLAGS = $(SAGE_CFLAGS) -DUSE_LIBVNC -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = $(SAGE_LDFLAGS) -lvncclient -O3 -L$(LIB_DIR) -lquanta -lsail -lz -ljpeg $(PORTAUDIO_LDFLAGS) -pthread
else
SOURCES = d3des.cpp vncauth.cpp argsresources.cpp colour.cpp rfbproto.cpp sockets.cpp sgVNCViewer.cpp main.cpp
CFLAGS = $(SAGE_CFLAGS) -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = $(SAGE_LDFLAGS) -O3 -L$(LIB_DIR) -lquanta -lsail -lz -ljpeg $(PORTAUDIO_LDFLAGS) -pthread
endif

ifeq ($(USE_QT), 1)
CFLAGS += -DUSE_QT -I/usr/include/QtGui
LDFLAGS += -lQtGui
endif

OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

TARGETS = $(BIN_DIR)/VNCViewer

all: $(TARGETS)

$(BIN_DIR)/VNCViewer: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJ_DIR)/%.o : %.cpp
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
