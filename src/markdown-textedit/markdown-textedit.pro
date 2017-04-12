
TARGET = markdown-textedit
TEMPLATE = lib
VERSION = 1.0.0

include(../global.pri)

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += BUILDING_MARKDOWN_TEXTEDIT_DLL

# 头文件
HEADERS += $$files(*.h*, true)

# 源文件
SOURCES += $$files(*.c*, true)

# ui 文件
FORMS += $$files(*.ui, true)

# 资源文件
RESOURCES += $$files(*.qrc, true)

# pmh-adapter
INCLUDEPATH += $$PWD/..
LIBS += -L$$OUT_PWD/../pmh-adapter$${OUT_TAIL} -lpmh-adapter

# peg-markdown-highlight
INCLUDEPATH += $$PWD/../../3rdparty/peg-markdown-highlight
LIBS += -L$$OUT_PWD/../peg-markdown-highlight$${OUT_TAIL} -lpmh

# nut
INCLUDEPATH += $$PWD/../../3rdparty/nut.git/src
