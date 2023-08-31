include(lib.pri)

TEMPLATE = lib

# 'contacts' is too generic for the target name - use 'contactcache'
TARGET = contactcache-qt$${QT_MAJOR_VERSION}
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

DEFINES += CONTACTCACHE_BUILD

CONFIG += create_pc create_prl no_install_prl

QT -= gui
QT += core

develheaders.path = /usr/include/$$TARGET
develheaders.files = $$PUBLIC_HEADERS

QMAKE_PKGCONFIG_NAME = $$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Sailfish OS contact cache library
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$develheaders.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_REQUIRES = Qt$${QT_MAJOR_VERSION}Core Qt$${QT_MAJOR_VERSION}Contacts Qt$${QT_MAJOR_VERSION}Versit qtcontacts-sqlite-qt$${QT_MAJOR_VERSION}-extensions
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += develheaders
