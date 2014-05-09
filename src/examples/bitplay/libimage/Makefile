TOP_DIR = ../../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/bitplay/libimage
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

MACHINE=$(shell uname -s)
ifeq ($(MACHINE), SunOS)
CFLAGS = -O2 -xchar=u -MMD
else
CFLAGS = -O2 -funsigned-char -MMD
endif

SOURCES = close.c filbuf.c flsbuf.c name.c open.c pix.c rdwr.c rle.c row.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

TARGETS = $(LIB_DIR)/libimage.a

all: $(TARGETS)

$(LIB_DIR)/libimage.a: $(OBJECTS)
	rm -f $@
	ar r $@ $(OBJECTS)
	ranlib $@

$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(cc) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

distclean: clean

-include $(DEPENDS)
