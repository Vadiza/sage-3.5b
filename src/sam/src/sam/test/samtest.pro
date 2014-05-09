TOP_DIR = ../../../../..
SRC_DIR = $$TOP_DIR/src
OBJ_DIR = $$TOP_DIR/obj/sam/samtest
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

QT += core network
QT -= gui

TARGET = samtest
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += samtest_main.cpp
HEADERS += samtester.h

INCLUDEPATH += $$SRC_DIR/sam/src $$SRC_DIR/sam/src/client
LIBS += -L$$LIB_DIR -lsac -ljack

message(samtest.pro complete)
