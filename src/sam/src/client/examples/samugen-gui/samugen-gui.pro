TOP_DIR = ../../../../../..
SRC_DIR = $$TOP_DIR/src
OBJ_DIR = $$TOP_DIR/obj/sam/samugen-gui
LIB_DIR = $$TOP_DIR/lib
BIN_DIR = $$TOP_DIR/bin

UI_DIR = "$$OBJ_DIR"
MOC_DIR = "$$OBJ_DIR"
OBJECTS_DIR = "$$OBJ_DIR"

CONFIG(debug, debug|release) {
    DESTDIR = "$$BIN_DIR/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$BIN_DIR"
}

QT       += core

TARGET = samugen-gui
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp samugengui.cpp
HEADERS += samugengui.h

INCLUDEPATH += /usr/local/include $$SRC_DIR/sam/src $$SRC_DIR/sam/src/client
LIBS += -L$$LIB_DIR -lsac
QT += network

# For version without JACK, uncomment the next line
#DEFINES += SAC_NO_JACK
contains(DEFINES, SAC_NO_JACK) {
    message(samugen-gui: SAC_NO_JACK defined: configuring without JACK...)
}
else {
    message(samugen-gui: configuring with JACK...)
    LIBS += -ljack
}

#target.path = /usr/local/bin
#INSTALLS += target

message(samugen-gui.pro complete)