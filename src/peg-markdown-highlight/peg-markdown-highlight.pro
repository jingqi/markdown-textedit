
TARGET = pmh
TEMPLATE = lib
CONFIG += staticlib

include(../global.pri)

CONFIG -= qt

SRC_ROOT = $$PWD/../../3rdparty/peg-markdown-highlight.git

INCLUDEPATH += \
    $${SRC_ROOT}

SOURCES += \
    pmh_parser.c \
    pmh_styleparser.c

HEADERS += \
    $${SRC_ROOT}/pmh_styleparser.h \
    $${SRC_ROOT}/pmh_parser.h \
    $${SRC_ROOT}/pmh_definitions.h
