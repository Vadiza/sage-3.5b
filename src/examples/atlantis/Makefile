TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/atlantis
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = $(SAGE_CFLAGS) -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(PORTAUDIO_CFLAGS) -MMD
ifeq ($(MACHINE), Darwin)
   CFLAGS += -FGLUT -FOpenGL
   LDFLAGS = -lpthread -L$(LIB_DIR) -lsail -framework GLUT -framework OpenGL -lobjc -lm 
else
ifeq ($(MACHINE), SunOS)
   CFLAGS = $(SAGE_CFLAGS) -O3 -I$(SRC_DIR)/sage -I$(SRC_DIR)/QUANTA $(GLUT_CFLAGS) -MMD
   LDFLAGS = -lpthread -lm -lGL -lGLU -L$(LIB_DIR) -lquanta -lsail $(GLUT_LDFLAGS) 
else
   LDFLAGS = -lpthread -lm -lGL -lGLU $(GLUT_LDFLAGS) -L$(LIB_DIR) -lquanta -lsail $(GLEW_LDFLAGS)
endif
endif
LDFLAGS += $(PORTAUDIO_LDFLAGS)

SOURCES = atlantis.c dolphin.c shark.c swim.c whale.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

TARGETS = $(BIN_DIR)/atlantis

all: $(TARGETS)

$(BIN_DIR)/atlantis: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(SAGE_LDFLAGS) $(CFLAGS) $(LDFLAGS)

# Use $(CC) instead of $(cc)
$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
