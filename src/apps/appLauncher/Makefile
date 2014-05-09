TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/appLauncher
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

SOURCES = $(shell find . -type f -name '*.py')
TARGETS = \
$(addprefix $(BIN_DIR)/appLauncher/,${SOURCES:.py=.pyc}) \
$(BIN_DIR)/appLauncher/GO

all: $(TARGETS)

$(BIN_DIR)/appLauncher/%.pyc: %.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

$(BIN_DIR)/appLauncher/GO: GO
	mkdir -p $(BIN_DIR)/appLauncher
	cp $< $@

clean:
	rm -rf *~ $(TARGETS) $(BIN_DIR)/appLauncher

distclean: clean
