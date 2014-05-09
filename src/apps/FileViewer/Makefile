TOP_DIR = ../../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/apps/FileViewer
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

TARGETS = $(BIN_DIR)/Stereo3D $(BIN_DIR)/officeviewer

all: $(TARGETS)
	$(MAKE) -C ImageViewer
	$(MAKE) -C PDFViewer
	$(MAKE) -C FileServer

$(BIN_DIR)/Stereo3D: Stereo3D
	cp $< $@

$(BIN_DIR)/officeviewer: officeviewer
	cp $< $@

clean:
	$(MAKE) -C ImageViewer clean
	$(MAKE) -C PDFViewer clean
	$(MAKE) -C FileServer clean

distclean: clean
	-rm -rf $(TARGETS)
