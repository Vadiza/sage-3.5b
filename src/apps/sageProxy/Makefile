TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/sageProxy
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

SOURCES = $(shell find . -type f -name '*.py')
TARGETS = \
$(addprefix $(BIN_DIR)/sageProxy/,${SOURCES:.py=.pyc})

all: $(TARGETS)

$(BIN_DIR)/sageProxy/%.pyc: %.py
	mkdir -p $(dir $@)
	python -mcompileall -d $(dir $@) $<
	cp ${<:.py=.pyc} $@

clean:
	rm -rf *~ $(TARGETS) $(BIN_DIR)/sageProxy

distclean: clean
