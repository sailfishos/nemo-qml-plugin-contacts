include(../common.pri)

TARGET = tst_resolve
QT += contacts-private dbus

PKGCONFIG += mlocale5
LIBS += -lphonenumber

# We need the moc output for ContactManagerEngine from sqlite-extensions
extensionsIncludePath = $$system(pkg-config --cflags-only-I qtcontacts-sqlite-qt5-extensions)
VPATH += $$replace(extensionsIncludePath, -I, )
HEADERS += contactmanagerengine.h

SOURCES += tst_resolve.cpp
