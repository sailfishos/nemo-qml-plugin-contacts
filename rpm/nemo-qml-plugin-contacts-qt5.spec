Name:       nemo-qml-plugin-contacts-qt5

Summary:    Nemo QML contacts library
Version:    0.3.24
Release:    1
License:    BSD
URL:        https://github.com/sailfishos/nemo-qml-plugin-contacts
Source0:    %{name}-%{version}.tar.bz2
Requires:   qtcontacts-sqlite-qt5 >= 0.1.37
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Contacts)
BuildRequires:  pkgconfig(Qt5Versit)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(qtcontacts-sqlite-qt5-extensions) >= 0.3.0
BuildRequires:  pkgconfig(mlocale5)
BuildRequires:  pkgconfig(mce)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(accounts-qt5)
BuildRequires:  libphonenumber-devel
BuildRequires:  qt5-qttools-linguist
BuildRequires:  qt5-qttools
BuildRequires:  qt5-qttools-qthelp-devel
BuildRequires:  sailfish-qdoc-template

%description
%{summary}.

%package devel
Summary:    Nemo QML contacts devel headers
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary}.

%package doc
Summary:    Nemo QML contacts documentation

%description doc
%{summary}.

%package ts-devel
Summary:   Translation source for nemo-qml-plugin-contacts-qt5

%description ts-devel
%{summary}.

%package tools
Summary:    Development tools for qmlcontacts
License:    BSD

%description tools
%{summary}.

%package tests
Summary:    QML contacts plugin tests
Requires:   %{name} = %{version}-%{release}
Requires:   blts-tools

%description tests
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 VERSION=%{version}
%make_build

%install
%qmake5_install

install -m 644 doc/html/*.html %{buildroot}/%{_docdir}/nemo-qml-plugin-contacts/
install -m 644 doc/html/nemo-qml-plugin-contacts.index %{buildroot}/%{_docdir}/nemo-qml-plugin-contacts/
install -m 644 doc/nemo-qml-plugin-contacts.qch %{buildroot}/%{_docdir}/nemo-qml-plugin-contacts/

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%license LICENSE.BSD
%{_libdir}/libcontactcache-qt5.so.*
%{_libdir}/qt5/qml/org/nemomobile/contacts/libnemocontacts.so
%{_libdir}/qt5/qml/org/nemomobile/contacts/plugins.qmltypes
%{_libdir}/qt5/qml/org/nemomobile/contacts/qmldir
%{_datadir}/translations/nemo-qml-plugin-contacts_eng_en.qm

%files devel
%{_libdir}/libcontactcache-qt5.so
%{_includedir}/contactcache-qt5/*
%{_libdir}/pkgconfig/contactcache-qt5.pc

%files doc
%{_docdir}/nemo-qml-plugin-contacts/*

%files ts-devel
%{_datadir}/translations/source/nemo-qml-plugin-contacts.ts

%files tools
%{_bindir}/vcardconverter
%{_bindir}/contacts-tool

%files tests
/opt/tests/nemo-qml-plugin-contacts-qt5/*
