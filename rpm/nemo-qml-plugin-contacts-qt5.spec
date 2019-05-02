Name:       nemo-qml-plugin-contacts-qt5

Summary:    Nemo QML contacts plugin
Version:    0.2.7
Release:    1
Group:      System/Libraries
License:    BSD
URL:        https://git.merproject.org/mer-core/nemo-qml-plugin-contacts
Source0:    %{name}-%{version}.tar.bz2
Requires:   qtcontacts-sqlite-qt5 >= 0.1.37
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Contacts)
BuildRequires:  pkgconfig(Qt5Versit)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(qtcontacts-sqlite-qt5-extensions) >= 0.2.31
BuildRequires:  pkgconfig(contactcache-qt5) >= 0.2.3
BuildRequires:  pkgconfig(mlocale5)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  qt5-qttools-linguist
BuildRequires:  qt5-qttools

%description
%{summary}.

%package ts-devel
Summary:   Translation source for nemo-qml-plugin-contacts-qt5
Group:     System/Libraries

%description ts-devel
Translation source for nemo-qml-plugin-contacts-qt5

%package tools
Summary:    Development tools for qmlcontacts
License:    BSD
Group:      Applications/System
Provides:   qmlcontacts-tools > 0.4.9
Obsoletes:  qmlcontacts-tools <= 0.4.9

%description tools
%{summary}.

%package tests
Summary:    QML contacts plugin tests
Group:      System/Libraries
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

%files
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/nemomobile/contacts/libnemocontacts.so
%{_libdir}/qt5/qml/org/nemomobile/contacts/plugins.qmltypes
%{_libdir}/qt5/qml/org/nemomobile/contacts/qmldir
%{_datadir}/translations/nemo-qml-plugin-contacts_eng_en.qm

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
