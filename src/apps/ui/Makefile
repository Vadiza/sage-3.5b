TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/ui
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

SOURCES = $(shell find . -type f -name '*.py')
TARGETS = \
$(addprefix $(BIN_DIR)/ui/,${SOURCES:.py=.pyc}) \
$(addprefix $(BIN_DIR)/ui/,$(shell find . -type f -name '*.jpg')) \
$(addprefix $(BIN_DIR)/ui/,$(shell find . -type f -name '*.icns'))

all: $(TARGETS)

$(BIN_DIR)/ui/%.pyc: %.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

$(BIN_DIR)/ui/%.jpg: %.jpg
	mkdir -p $(dir $@)
	cp $< $@

$(BIN_DIR)/ui/%.icns: %.icns
	mkdir -p $(dir $@)
	cp $< $@

clean:
	rm -rf *~ $(TARGETS) $(BIN_DIR)/ui

distclean: clean
