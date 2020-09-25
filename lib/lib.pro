include(lib.pri)

TEMPLATE = lib

# 'contacts' is too generic for the target name - use 'contactcache'
TARGET = contactcache-qt5
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

DEFINES += CONTACTCACHE_BUILD

CONFIG += create_pc create_prl no_install_prl

QT -= gui

develheaders.path = /usr/include/$$TARGET
develheaders.files = $$PUBLIC_HEADERS

QMAKE_PKGCONFIG_NAME = $$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Sailfish OS contact cache library
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$develheaders.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_REQUIRES = Qt5Core Qt5Contacts Qt5Versit qtcontacts-sqlite-qt5-extensions
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += develheaders
