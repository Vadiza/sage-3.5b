TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/ImageS3D
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = -g `Wand-config --cflags --cppflags` -I$(SRC_DIR)/QUANTA -I$(SRC_DIR)/sage -fno-stack-protector $(SDL_CFLAGS) -MMD
LDFLAGS = `Wand-config --ldflags --libs` -lpthread -lstdc++ -lltdl -L$(LIB_DIR) -lsail -lquanta

SOURCES = images3d.cpp
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

TARGETS = $(BIN_DIR)/images3d

all: $(TARGETS)

$(BIN_DIR)/images3d: $(OBJECTS)
	g++ -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(OBJ_DIR)
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
