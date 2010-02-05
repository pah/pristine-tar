Name: pristine-tar
Version: 1.01
Release: 2%{?dist}
Summary: regenerate pristine tarballs

Group: System Tools
License: GPLv2
Url: http://kitenet.net/~joey/code/pristine-tar/
Source0: http://ftp.debian.org/debian/pool/main/p/pristine-tar/%{name}_%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires: git, xdelta

%description
pristine-tar can regenerate a pristine upstream tarball using only a
small binary delta file and a copy of the source which can be a revision
control checkout.

The package also includes a pristine-gz command, which can regenerate
a pristine .gz file.

The delta file is designed to be checked into revision control along-side
the source code, thus allowing the original tarball to be extracted from
revision control.

pristine-tar is available in git at git://git.kitenet.net/pristine-tar/


%prep
%setup -q -n %{name}


%build
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc GPL TODO delta-format.txt
%{_bindir}/*
%{_mandir}/*


%changelog
* Tue Feb 24 2009 Jimmy Tang <jtang@tchpc.tcd.ie> - 0.21-1
- initial package

