include(../common.pri)

PKGCONFIG += mlocale$${QT_MAJOR_VERSION}

CONFIG += c++11

HEADERS += \
        seasidecache.h \
        seasidefilteredmodel.h \
        $$SRCDIR/seasidefilteredmodel.h \
        $$SRCDIR/seasideaddressbook.h \
        $$SRCDIR/seasideperson.h

SOURCES += \
        seasidecache.cpp \
        tst_seasidefilteredmodel.cpp \
        $$SRCDIR/seasidefilteredmodel.cpp \
        $$SRCDIR/seasideaddressbook.cpp \
        $$SRCDIR/seasideperson.cpp
