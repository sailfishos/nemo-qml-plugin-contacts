include(../config.pri)
include(basename.pri)

TEMPLATE = app
CONFIG -= app_bundle

QT += testlib qml dbus

SRCDIR = $$PWD/../src/
INCLUDEPATH += $$SRCDIR $$PWD/../lib/
DEPENDPATH += $$INCLUDEPATH
LIBS += -L$$PWD/../lib -L../../lib -lcontactcache-qt$${QT_MAJOR_VERSION}

target.path = /opt/tests/$${BASENAME}/contacts
INSTALLS += target
