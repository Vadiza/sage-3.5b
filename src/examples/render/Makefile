TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/render
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = $(SAGE_CFLAGS) -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA -DQUANTA_USE_PTHREADS -DQUANTA_THREAD_SAFE $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) $(GLUT_CFLAGS) -MMD

ifeq ($(MACHINE), Darwin)
	CFLAGS += -FGLUT -FOpenGL
	LDFLAGS = -lpthread -L$(LIB_DIR) -lsail -framework GLUT -framework OpenGL -lobjc -lm 
else
	LDFLAGS = -lpthread $(SUN_LDFLAGS) -lm -ldl $(GLUT_LDFLAGS) -lGL -lGLU -L$(LIB_DIR) -lquanta $(GLEW_LDFLAGS) -L$(LIB_DIR) -lsail
endif
LDFLAGS += $(PORTAUDIO_LDFLAGS)

SOURCES = render.cpp render-mpi.cpp
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

TARGETS = $(BIN_DIR)/render
# $(BIN_DIR)/render-mpi

all: $(TARGETS)

$(BIN_DIR)/render: $(OBJ_DIR)/render.o
	$(CC) $(SAGE_LDFLAGS) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(BIN_DIR)/render-mpi: $(OBJ_DIR)/render-mpi.o
	$(CC) $(SAGE_LDFLAGS) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.o : %.cpp
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
