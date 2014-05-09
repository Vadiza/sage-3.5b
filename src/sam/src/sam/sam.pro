TOP_DIR = ../../../..
SRC_DIR = $$TOP_DIR/src
OBJ_DIR = $$TOP_DIR/obj/sam/sam
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

SOURCES += \
sam_main.cpp \
sam.cpp \
sam_app.cpp \
jack_util.cpp \
rtpreceiver.cpp \
samui.cpp \
clientwidget.cpp \
masterwidget.cpp \
samparams.cpp \
../osc.cpp \
../rtp.cpp \
../rtcp.cpp

HEADERS += \
sam.h \
sam_app.h \
jack_util.h \
rtpreceiver.h \
samui.h \
clientwidget.h \
masterwidget.h \
samparams.h \
../osc.h \
../rtp.h \
../rtcp.h

FORMS += \
samui.ui

TARGET = sam

INCLUDEPATH += /usr/local/include $$SRC_DIR/sam/src $$SRC_DIR/sam/src/client

# For debugging, comment out
CONFIG -= warn_on
DEFINES += QT_NO_DEBUG_OUTPUT

LIBS += -ljack
QT += network

macx {
  message(Mac OS X)
  CONFIG -= app_bundle
}
linux-g++ {
  message(Linux)
}
linux-g++-64 {
  message(Linux 64bit)
}

#target.path = $$BIN_DIR
#INSTALLS += target

message(sam.pro complete)
