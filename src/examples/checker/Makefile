TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/checker
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = $(SAGE_CFLAGS) -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA -DQUANTA_USE_PTHREADS -DQUANTA_THREAD_SAFE $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = -lpthread -lm -ldl $(SAGE_LDFLAGS) -L$(LIB_DIR) -lsail -lquanta $(PORTAUDIO_LDFLAGS) $(GLEW_LDFLAGS)

SOURCES = checker.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

TARGETS = $(BIN_DIR)/checker

all: $(TARGETS)

$(BIN_DIR)/checker: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(SAGE_LDFLAGS) $(CFLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.o : %.cpp
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
