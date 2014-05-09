# Mac - Linux : to expand with: qmake -spec macx-g++
# Win32 :
#  set QMAKESPEC=win32-msvc2008
#  qmake -tp vc -config debug

TOP_DIR = ../../..
SRC_DIR = $$TOP_DIR/src
OBJ_DIR = $$TOP_DIR/obj/apps/qshare
LIB_DIR = $$TOP_DIR/lib
BIN_DIR = $$TOP_DIR/bin

UI_DIR = "$$OBJ_DIR"
MOC_DIR = "$$OBJ_DIR"
OBJECTS_DIR = "$$OBJ_DIR"

DESTDIR = "$$BIN_DIR"

QT += widgets

TARGET = qshare
TEMPLATE = app

CONFIG += x86_64
FORMS = capturewindow.ui

SOURCES = \
main.cpp \
capturewindow.cpp \
FastDXT/libdxt.cpp \
FastDXT/dxt.cpp \
FastDXT/util.cpp \
FastDXT/intrinsic.cpp

HEADERS += capturewindow.h

INCLUDEPATH += FastDXT pixfc-sse

QMAKE_CXXFLAGS += -DDXT_INTR

RC_FILE = qsharerc.rc

macx { 
    INCLUDEPATH += ../../include
    LIBS += -Lpixfc-sse/macosx64 -lpixfc-sse -L/Users/luc/Dev/SVN/sage-new/lib -lsail -framework OpenGL -framework Cocoa -framework IOKit -lobjc -lm
}
unix:!macx { 
    INCLUDEPATH += $$SRC_DIR/sage
    LIBS += -L$$LIB_DIR -lsail -lquanta -lXext -lX11
}
win32 {
    QMAKE_CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS
    CONFIG += embed_manifest_exe
    INCLUDEPATH += ../../include
    INCLUDEPATH += ../../win32/include
    LIBS += -L../../lib -L../../win32/lib -lsail -lpthread -lWs2_32 -lquanta
}

RESOURCES += qshare.qrc

#target.path=../../bin
#INSTALLS += target

OTHER_FILES +=

message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(Qt resources can be found in the following locations:)
message(Header files: $$[QT_INSTALL_HEADERS])
message(Libraries: $$[QT_INSTALL_LIBS])
