TOP_DIR = ../../../..
SRC_DIR = $$TOP_DIR/src
OBJ_DIR = $$TOP_DIR/obj/sam/client
LIB_DIR = $$TOP_DIR/lib
BIN_DIR = $$TOP_DIR/bin

UI_DIR = "$$OBJ_DIR"
MOC_DIR = "$$OBJ_DIR"
OBJECTS_DIR = "$$OBJ_DIR"

CONFIG(debug, debug|release) {
    DESTDIR = "$$LIB_DIR/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$LIB_DIR"
}

QT += network
QT -= gui

TARGET = sac
TEMPLATE = lib

DEFINES += LIBSAC_LIBRARY
CONFIG += staticlib

# For debugging, comment out
CONFIG -= warn_on
DEFINES += QT_NO_DEBUG_OUTPUT

# uncomment to build for 32-bit arch (mac only)
#CONFIG+=x86

# For version without JACK, uncomment the next line
#DEFINES += SAC_NO_JACK
contains(DEFINES, SAC_NO_JACK) {
    message(libsac: SAC_NO_JACK defined: building without JACK...)
}
else {
    message(libsac: building with JACK...)
    LIBS += -ljack
}

SOURCES += \
sam_client.cpp \
sac_audio_interface.cpp \
rtpsender.cpp \
../rtp.cpp \
../rtcp.cpp \
../osc.cpp

HEADERS +=\
libsac_global.h \
sam_client.h \
sac_audio_interface.h \
rtpsender.h \
../rtp.h \
../rtcp.h \
../osc.h

INCLUDEPATH += /usr/local/include ../

#target.path = $$LIB_DIR
#INSTALLS += target

message(libsac.pro complete)

