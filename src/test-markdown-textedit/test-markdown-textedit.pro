
TARGET = test-qmarkdown-textedit
TEMPLATE = app

include(../global.pri)

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 头文件
HEADERS += $$files(*.h*, true)

# 源文件
SOURCES += $$files(*.c*, true)

# ui 文件
FORMS += $$files(*.ui, true)

# qmarkdown-textedit
INCLUDEPATH += $$PWD/../qmarkdown-textedit
LIBS += -L$$OUT_PWD/../qmarkdown-textedit$${OUT_TAIL}
win32: LIBS += -lqmarkdown-textedit1
else: LIBS += -lqmarkdown-textedit

# 资源打包
mac {
    libs.path = Contents/Frameworks
    libs.files = \
        $$OUT_PWD/../qmarkdown-textedit/libqmarkdown-textedit.1.dylib
    QMAKE_BUNDLE_DATA += libs
} else: win32 {
    # 拷贝资源文件
    SRC = $$OUT_PWD/../qmarkdown-textedit/$${DEBUG_MODE}/qmarkdown-textedit1.dll
    POST_TARGETDEPS += $${SRC}
    SRC ~= s,/,\\,
    QMAKE_POST_LINK += $$quote(cmd /c xcopy /y /i $${SRC} $${DST}$$escape_expand(\n\t))
} else: unix {
    SRC = $$OUT_PWD/../qmarkdown-textedit/libqmarkdown-textedit.1.so
    POST_TARGETDEPS += $${SRC}
    QMAKE_POST_LINK += cp -Lf $${SRC} $$OUT_PWD/ ;
}
