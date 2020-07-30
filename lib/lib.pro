include(../config.pri)

TEMPLATE = lib
CONFIG += qt hide_symbols
CONFIG += link_pkgconfig create_pc create_prl no_install_prl

# 'contacts' is too generic for the target name - use 'contactcache'
TARGET = contactcache-qt5
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# version for generated pkgconfig files is defined in the spec file
QMAKE_PKGCONFIG_INCDIR = /usr/include/$${TARGET}
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

QMAKE_PKGCONFIG_NAME = $$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Sailfish OS contact cache library
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$develheaders.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_REQUIRES = Qt5Core Qt5Contacts Qt5Versit qtcontacts-sqlite-qt5-extensions
QMAKE_PKGCONFIG_VERSION = $$VERSION

CONFIG += link_pkgconfig
packagesExist(mlite5) {
    PKGCONFIG += mlite5
    DEFINES += HAS_MLITE
} else {
    warning("mlite not available. Some functionality may not work as expected.")
}
PKGCONFIG += mlocale5 mce qtcontacts-sqlite-qt5-extensions
LIBS += -lphonenumber

DEFINES += CONTACTCACHE_BUILD

# We need access to QtContacts private headers
QT += contacts-private
QT -= gui

# We need the moc output for ContactManagerEngine from sqlite-extensions
extensionsIncludePath = $$system(pkg-config --cflags-only-I qtcontacts-sqlite-qt5-extensions)
VPATH += $$replace(extensionsIncludePath, -I, )
HEADERS += contactmanagerengine.h

SOURCES += \
    $$PWD/cacheconfiguration.cpp \
    $$PWD/seasidecache.cpp \
    $$PWD/seasideexport.cpp \
    $$PWD/seasideimport.cpp \
    $$PWD/seasidecontactbuilder.cpp \
    $$PWD/seasidepropertyhandler.cpp

HEADERS += \
    $$PWD/cacheconfiguration.h \
    $$PWD/contactcacheexport.h \
    $$PWD/seasidecache.h \
    $$PWD/seasideexport.h \
    $$PWD/seasideimport.h \
    $$PWD/seasidecontactbuilder.h \
    $$PWD/synchronizelists.h \
    $$PWD/seasidepropertyhandler.h

headers.files = \
    $$PWD/cacheconfiguration.h \
    $$PWD/contactcacheexport.h \
    $$PWD/seasidecache.h \
    $$PWD/seasideexport.h \
    $$PWD/seasideimport.h \
    $$PWD/seasidecontactbuilder.h \
    $$PWD/synchronizelists.h \
    $$PWD/seasidepropertyhandler.h
headers.path = /usr/include/$$TARGET
INSTALLS += headers
