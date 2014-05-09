TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/dxt
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = -fPIC -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = -lpthread -lm -ldl $(GLEW_LDFLAGS) -L$(LIB_DIR) -lquanta -lsail

SOURCES = app2.cpp
#dxt.o app.o
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

TARGETS = $(BIN_DIR)/dxt

all: $(TARGETS)

$(BIN_DIR)/dxt: $(OBJECTS)
	g++ -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(OBJ_DIR)
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
