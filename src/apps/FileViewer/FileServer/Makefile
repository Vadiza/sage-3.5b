TOP_DIR = ../../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/FileViewer/FileServer
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

SOURCES = $(shell find . -type f -name '*.py')
TARGETS = \
$(addprefix $(BIN_DIR)/fileServer/,${SOURCES:.py=.pyc}) \
$(BIN_DIR)/fileServer/youtube-dl

all: $(TARGETS)

$(BIN_DIR)/fileServer/%.pyc: %.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

$(BIN_DIR)/fileServer/youtube-dl: youtube-dl
	mkdir -p $(BIN_DIR)/fileServer
	cp $< $@

clean:
	rm -rf *~ $(TARGETS) $(BIN_DIR)/fileServer

distclean: clean
