include(../common.pri)
PKGCONFIG += qtcontacts-sqlite-qt$${QT_MAJOR_VERSION}-extensions

TARGET = vcardconverter

SOURCES += main.cpp

target.path = $$INSTALL_ROOT/usr/bin/
INSTALLS += target
