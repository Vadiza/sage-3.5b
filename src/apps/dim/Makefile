TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/dim
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

SOURCES = $(shell find . -type f -name '*.py')
TARGETS = \
$(addprefix $(BIN_DIR)/dim/,${SOURCES:.py=.pyc}) \
$(addprefix $(BIN_DIR)/dim/,$(shell find . -type f -name '*.wav'))

all: $(TARGETS)

$(BIN_DIR)/dim/%.pyc: %.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

$(BIN_DIR)/dim/%.wav: %.wav
	mkdir -p $(dir $@)
	cp $< $@

clean:
	rm -rf *~ $(TARGETS) $(BIN_DIR)/dim

distclean: clean
