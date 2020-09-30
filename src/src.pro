include(../config.pri)

TARGET = nemocontacts
PLUGIN_IMPORT_PATH = org/nemomobile/contacts

TEMPLATE = lib
CONFIG += qt plugin hide_symbols

QT = \
    core  \
    qml
PKGCONFIG += mlocale5

packagesExist(mlite5) {
    PKGCONFIG += mlite5
    DEFINES += HAS_MLITE
} else {
    warning("mlite not available. Some functionality may not work as expected.")
}

INCLUDEPATH += ../lib
LIBS += -L../lib -lcontactcache-qt5

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += qmldir plugins.qmltypes
qmldir.path +=  $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += qmldir

qmltypes.commands = qmlplugindump -nonrelocatable org.nemomobile.contacts 1.0 > $$PWD/plugins.qmltypes
QMAKE_EXTRA_TARGETS += qmltypes

SOURCES += $$PWD/plugin.cpp \
           $$PWD/seasideaddressbook.cpp \
           $$PWD/seasideaddressbookmodel.cpp \
           $$PWD/seasideperson.cpp \
           $$PWD/seasidefilteredmodel.cpp \
           $$PWD/seasidedisplaylabelgroupmodel.cpp \
           $$PWD/seasidestringlistcompressor.cpp \
           $$PWD/seasidevcardmodel.cpp \
           $$PWD/seasidesimplecontactmodel.cpp \
           $$PWD/seasideconstituentmodel.cpp \
           $$PWD/seasidemergecandidatemodel.cpp \
           $$PWD/knowncontacts.cpp

HEADERS += $$PWD/seasideperson.h \
           $$PWD/seasideaddressbook.h \
           $$PWD/seasideaddressbookmodel.h \
           $$PWD/seasidefilteredmodel.h \
           $$PWD/seasidedisplaylabelgroupmodel.h \
           $$PWD/seasidestringlistcompressor.h \
           $$PWD/seasidevcardmodel.h \
           $$PWD/seasidesimplecontactmodel.h \
           $$PWD/seasideconstituentmodel.h \
           $$PWD/seasidemergecandidatemodel.h \
           $$PWD/knowncontacts.h
