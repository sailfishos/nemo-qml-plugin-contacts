include(../common.pri)
PKGCONFIG += qtcontacts-sqlite-qt5-extensions

TARGET = vcardconverter

SOURCES += main.cpp

target.path = $$INSTALL_ROOT/usr/bin/
INSTALLS += target
