include(../config.pri)

TEMPLATE = app
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/../src/ $$PWD/../lib/
DEPENDPATH += $$INCLUDEPATH
LIBS += -L$$PWD/../lib -lcontactcache-qt5
