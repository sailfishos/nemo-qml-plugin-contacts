include(../common.pri)

TARGET = contacts-tool
DEFINES *= VERSION_STRING=\"\\\"$$VERSION\\\"\"

SOURCES += main.cpp

target.path = $$INSTALL_ROOT/usr/bin/
INSTALLS += target
