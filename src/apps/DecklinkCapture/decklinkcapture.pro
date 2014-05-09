TOP_DIR = ../../..
SRC_DIR = $$TOP_DIR/src
OBJ_DIR = $$TOP_DIR/obj/apps/decklinkcapture
LIB_DIR = $$TOP_DIR/lib
BIN_DIR = $$TOP_DIR/bin

UI_DIR = "$$OBJ_DIR"
MOC_DIR = "$$OBJ_DIR"
OBJECTS_DIR = "$$OBJ_DIR"

DESTDIR = "$$BIN_DIR"

QT += core gui network

TARGET = decklinkcapture
TEMPLATE = app

CONFIG-=warn_on
CONFIG+=release

SOURCES += \
decklinkcapture.cpp \
include/DeckLinkAPIDispatch.cpp \
$$SRC_DIR/sage/fsClient.cpp \
$$SRC_DIR/sage/suil.cpp \
$$SRC_DIR/sage/sageMessage.cpp \
$$SRC_DIR/sage/misc.cpp

HEADERS += \
decklinkcapture.h \
include/DeckLinkAPI.h \
$$SRC_DIR/sam/src/client/sam_client.h

INCLUDEPATH += include
INCLUDEPATH += $$SRC_DIR/sage
INCLUDEPATH += $$SRC_DIR/sam/src
INCLUDEPATH += $$SRC_DIR/sam/src/client
INCLUDEPATH += $$SRC_DIR/QUANTA

unix:*-g++*: QMAKE_CXXFLAGS += -fpermissive

macx {
    LIBS += 
    CONFIG -= app_bundle
}
linux-g++ {
    LIBS +=
}
linux-g++-64 {
    LIBS +=
}
LIBS += -L$$LIB_DIR -lquanta -lsail -lpthread -lm -ldl
LIBS += -L$$LIB_DIR -ljack -lsac 

# Enable  deinterlacing (comment the config line if you don't have ffmpeg)
#CONFIG += deinterlacing
deinterlacing {
	message(deinterlacing enabled)
	# FFMPEG (deinterlacing)
	DEFINES += __STDC_CONSTANT_MACROS DEINTERLACING
	LIBS += -lavcodec -lavutil -lswscale
}

#target.path=../../bin
#INSTALLS += target

message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(Qt resources can be found in the following locations:)
message(Header files: $$[QT_INSTALL_HEADERS])
message(Libraries: $$[QT_INSTALL_LIBS])

