TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/atlantis-fbo
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = -I/usr/local/include -I/usr/X11R6/include -I$(SRC_DIR)/sage -O3 -Wno-deprecated -fomit-frame-pointer -ffast-math -MMD
LDFLAGS = -L/usr/X11R6/lib -L$(LIB_DIR) -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm -lpthread -lsail

# Machine specific settings
MACHINE=$(shell uname -p)
   #powerpc  MacOSX
   #x86_64   Opteron Linux
   #ia64     Itanium Linux (SGI)
   #else     32bit Linux
ifeq ($(MACHINE), powerpc)
   CFLAGS += -FGLUT -FOpenGL
   LDFLAGS = -framework GLUT -framework OpenGL -lobjc -lm -lpthread -lsail
else
ifeq ($(MACHINE), x86_64)
   LDFLAGS = -L/usr/X11R6/lib64 -L$(LIB_DIR) -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm -lpthread -lsail
else
ifeq ($(MACHINE), ia64)
   LDFLAGS = -L/usr/X11R6/lib64 -L$(LIB_DIR) -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm -lpthread -lsail
else
   # anything else is 32bit 
   LDFLAGS += $(LIB_DIR) -lquanta_32
endif
endif
endif

SOURCES = atlantis.c dolphin.c shark.c swim.c whale.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

TARGETS = $(BIN_DIR)/atlantis-fbo

all: $(TARGETS)

$(BIN_DIR)/atlantis-fbo: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(SAGE_LDFLAGS) $(CFLAGS) $(LDFLAGS)

# Use $(CC) instead of $(cc)
$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
