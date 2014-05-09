TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/webcam
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = $(SAGE_CFLAGS) -I$(SRC_DIR)/QUANTA -I$(SRC_DIR)/sage -IPixFC-SSE-0.3/include -MMD
LDFLAGS = -L$(LIB_DIR) -lquanta

ifeq ($(MACHINE), Darwin)
   CFLAGS += -FGLUT -FOpenGL
   LIBS = -lpthread -lsail -framework GLUT -framework OpenGL -lobjc -lm 
else
ifeq ($(MACHINE), SunOS)
   CFLAGS += -O3 $(GLUT_CFLAGS) 
   LIBS += -lpthread -lm -lGL -lGLU -lsail $(GLUT_LDFLAGS) 
else
   LIBS += -lpthread -lm -lGL -lGLU -LPixFC-SSE-0.3/lib -lpixfc-sse $(GLEW_LDFLAGS) -lsail -lv4l2
endif
endif

PROGRAM = $(BIN_DIR)/webcam
LIBRARY = $(LIB_DIR)/libpixfc-sse.a

SOURCES = capture.cpp
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS) $(LIBRARY)
	$(CC) -o $(PROGRAM) $(OBJECTS) $(LDFLAGS) $(CFLAGS) $(LIBS)

$(LIBRARY):
	$(MAKE) -C $(OBJ_DIR)/PixFC-SSE-0.3
	mv $(OBJ_DIR)/PixFC-SSE-0.3/src/libpixfc-sse.a $(LIB_DIR)

$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(cc) -c -o $@ $<

$(OBJ_DIR)/%.o : %.cpp
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(MAKE) -C $(OBJ_DIR)/PixFC-SSE-0.3 clean
	/bin/rm -f *~ *.o $(PROGRAM) $(LIBRARY) $(OBJECTS) $(DEPENDS)

distclean: clean

-include $(DEPENDS)
