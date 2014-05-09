TOP_DIR = ../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/tools
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

SOURCES = $(shell find . -type f -name '*.py')
TARGETS = \
$(addprefix $(BIN_DIR)/,${SOURCES:.py=.pyc}) \
$(BIN_DIR)/GO-sage \
$(BIN_DIR)/KILL_ALL \
$(BIN_DIR)/sage \
$(BIN_DIR)/sage.cmd \
$(BIN_DIR)/sageAudioManager.bat

all: $(TARGETS)

$(BIN_DIR)/%.pyc: %.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

$(BIN_DIR)/%: %
	mkdir -p $(BIN_DIR)
	cp $< $@

clean:
	rm -rf *~ $(TARGETS)

distclean: clean
