TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/atlantis-mpi-head
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = -O -fomit-frame-pointer -ffast-math -I$(SRC_DIR)/sage -I/opt/mpich/include -MMD
LDFLAGS = -lglut -lGLU -lGL -L/usr/X11R6/lib64 -lXmu -lXi -lX11 -lm -ldl -L$(LIB_DIR) -lsail -L/opt/mpich/ch-p4/lib64 -lmpich

SOURCES = atlantis.c dolphin.c shark.c swim.c whale.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

TARGETS = $(BIN_DIR)/atlantis-mpi-head

all: $(TARGETS)

$(BIN_DIR)/atlantis-mpi-head: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(SAGE_LDFLAGS) $(CFLAGS) $(LDFLAGS)

# Use $(CC) instead of $(cc)
$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

-include $(DEPENDS)
