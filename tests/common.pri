include(../config.pri)
include(basename.pri)

TEMPLATE = app
CONFIG -= app_bundle

QT += testlib qml dbus

SRCDIR = $$PWD/../src/
INCLUDEPATH += $$SRCDIR $$PWD/../lib/
DEPENDPATH += $$INCLUDEPATH
LIBS += -L$$PWD/../lib -lcontactcache-qt5

target.path = /opt/tests/$${BASENAME}/contacts
INSTALLS += target
