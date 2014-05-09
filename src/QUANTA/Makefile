TOP_DIR = ../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/QUANTA
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = $(SAGE_CFLAGS) -O3 -MMD
LDFLAGS = -lpthread -lm -ldl $(SUN_LDFLAGS) $(SAGE_LDFLAGS)

SOURCES = \
QUANTAnet_tcp_c.cxx \
QUANTAnet_socketbase_c.cxx \
QUANTAnet_iovec_c.cxx \
QUANTAglobals.cxx \
QUANTAnet_perfMonitor_c.cxx \
QUANTAnet_perfDaemon_c.cxx \
QUANTAnet_perfClient_c.cxx \
QUANTAnet_datapack_c.cxx

OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cxx=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cxx=.d})

TARGETS = $(LIB_DIR)/$(QUANTA_LIB)

all: $(TARGETS)

$(LIB_DIR)/$(QUANTA_LIB): $(OBJECTS) 
	$(CC) $(SHARED_LDFLAGS) $(OBJECTS) $(LDFLAGS) -o $(LIB_DIR)/$(QUANTA_LIB)

$(OBJ_DIR)/%.o : %.cxx 
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ_DIR)/*.d $(OBJ_DIR)/*.o
	rm -f $(TARGETS)

distclean: clean

-include $(DEPENDS)
