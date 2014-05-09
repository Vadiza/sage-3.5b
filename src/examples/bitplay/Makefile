TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/bitplay
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = $(SAGE_CFLAGS) -O3 -I$(SRC_DIR)/sage -Ilibimage -Ilibimg -I$(SRC_DIR)/QUANTA $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) -MMD
LDFLAGS = -lpthread -lm -ldl -L$(LIB_DIR) -lsail -lquanta $(PORTAUDIO_LDFLAGS) $(GLEW_LDFLAGS) -limg -limage -ltiff -ljpeg -lm

SOURCES = bpio.c bpiotest.c bplay.c img2bmv.c onethread.c txspeed.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

LIBRARIES = $(LIB_DIR)/libimg.a $(LIB_DIR)/libimage.a
TARGETS = $(BIN_DIR)/bplay-noglut $(BIN_DIR)/img2bmv $(BIN_DIR)/img2bmv $(BIN_DIR)/bpio $(BIN_DIR)/onethread

all: $(TARGETS)

$(LIB_DIR)/libimg.a:
	$(MAKE) -C libimg

$(LIB_DIR)/libimage.a:
	$(MAKE) -C libimage

$(BIN_DIR)/bplay-noglut: $(LIBRARIES) $(OBJ_DIR)/bplay.o $(OBJ_DIR)/bpio.o 
	$(CC) -o $@ $(SAGE_LDFLAGS) $(OBJ_DIR)/bplay.o $(OBJ_DIR)/bpio.o $(LDFLAGS)

$(BIN_DIR)/img2bmv: $(LIBRARIES) $(OBJ_DIR)/img2bmv.o
	$(cc) -o $@ $(OBJ_DIR)/img2bmv.o $(CFLAGS) $(LDFLAGS)

$(BIN_DIR)/txspeed: $(LIBRARIES) $(OBJ_DIR)/txspeed.o
	$(CC) -o $@ $(OBJ_DIR)/txspeed.o -g -O $(CFLAGS) $(LDFLAGS)

$(BIN_DIR)/bpio: $(LIBRARIES) $(OBJ_DIR)/bpiotest.o $(OBJ_DIR)/bpio.o
	$(CC) -o $@ $(OBJ_DIR)/bpiotest.o $(OBJ_DIR)/bpio.o $(CFLAGS) $(LDFLAGS)

$(BIN_DIR)/onethread: $(LIBRARIES) $(OBJ_DIR)/onethread.o
	$(CC) -o $@ $(OBJ_DIR)/onethread.o $(CFLAGS) $(LDFLAGS)

# Use $(CC) instead of $(cc)
$(OBJ_DIR)/bplay.o : bplay.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Use $(CC) instead of $(cc)
$(OBJ_DIR)/bpio.o : bpio.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Use $(CC) instead of $(cc)
$(OBJ_DIR)/bpiotest.o : bpiotest.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(cc) $(CFLAGS) -c -o $@ $<

clean:
	$(MAKE) -C libimage clean
	$(MAKE) -C libimg clean
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
