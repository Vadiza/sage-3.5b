TOP_DIR = ../../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/examples/bitplay/libimg
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = -O2 -MMD

SOURCES = softdisp.c tiffdisp.c rasdisp.c sgidisp.c jpegdisp.c pnmdisp.c gifdisp.c getimginfo.c getimgdata.c
OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.c=.d})

TARGETS = $(LIB_DIR)/libimg.a

all: $(TARGETS)

$(LIB_DIR)/libimg.a: $(OBJECTS)
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
