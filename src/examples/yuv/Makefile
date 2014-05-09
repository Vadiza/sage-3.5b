TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/yuv
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = -fPIC -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = -lpthread -lm -ldl -L$(LIB_DIR) -lsail -lquanta $(GLEW_LDFLAGS) -L$(LIB_DIR)

SOURCES = yuv.cxx
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cxx=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cxx=.d})

TARGETS = $(BIN_DIR)/yuv

all: $(TARGETS)

$(BIN_DIR)/yuv: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJ_DIR)/%.o : %.cxx
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
