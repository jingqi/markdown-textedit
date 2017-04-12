
TARGET = pmh-adapter
TEMPLATE = lib
CONFIG += staticlib

QT += core gui widgets

CONFIG += c++11

SOURCES += \
    pmh_markdown_parser.cpp \
    style_parser.cpp

HEADERS  += \
    pmh_markdown_parser.h \
    style_parser.h \
    definitions.h

################################################################################
## DEPENDENCIES
################################################################################

#
# peg-markdown-highlight
#
INCLUDEPATH += $$PWD/../../3rdparty/peg-markdown-highlight.git
