Name:       nemo-qml-plugin-contacts-qt5

Summary:    Nemo QML contacts library
Version:    0.2.25
Release:    1
License:    BSD
URL:        https://git.merproject.org/mer-core/nemo-qml-plugin-contacts
Source0:    %{name}-%{version}.tar.bz2
Obsoletes:  libcontacts-qt5 <= 0.2.12
Obsoletes:  libcontacts-qt5-tests <= 0.2.11
Obsoletes:  libcontacts-qt5-devel <= 0.2.11
Requires:   qtcontacts-sqlite-qt5 >= 0.1.37
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Contacts)
BuildRequires:  pkgconfig(Qt5Versit)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(qtcontacts-sqlite-qt5-extensions) >= 0.2.31
BuildRequires:  pkgconfig(mlocale5)
BuildRequires:  pkgconfig(mce)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  libphonenumber-devel
BuildRequires:  qt5-qttools-linguist
BuildRequires:  qt5-qttools

%description
%{summary}.

%package devel
Summary:    Nemo QML contacts devel headers
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary}.

%package ts-devel
Summary:   Translation source for nemo-qml-plugin-contacts-qt5

%description ts-devel
%{summary}.

%package tools
Summary:    Development tools for qmlcontacts
License:    BSD
Provides:   qmlcontacts-tools > 0.4.9
Obsoletes:  qmlcontacts-tools <= 0.4.9

%description tools
%{summary}.

%package tests
Summary:    QML contacts plugin tests
Requires:   %{name} = %{version}-%{release}

%description tests
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 VERSION=%{version}

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libcontactcache-qt5.so*
%{_libdir}/qt5/qml/org/nemomobile/contacts/libnemocontacts.so
%{_libdir}/qt5/qml/org/nemomobile/contacts/plugins.qmltypes
%{_libdir}/qt5/qml/org/nemomobile/contacts/qmldir
%{_datadir}/translations/nemo-qml-plugin-contacts_eng_en.qm

%files devel
%defattr(-,root,root,-)
%{_includedir}/contactcache-qt5/*
%{_libdir}/pkgconfig/contactcache-qt5.pc

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/nemo-qml-plugin-contacts.ts

%files tools
%defattr(-,root,root,-)
%{_bindir}/vcardconverter
%{_bindir}/contacts-tool

%files tests
%defattr(-,root,root,-)
/opt/tests/nemo-qml-plugins-qt5/contacts/*
