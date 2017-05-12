Summary: A program for benchmarking hard drives and filesystems
Name: postal
Version: 0.60
Release: 1
Copyright: GPL
Group: Utilities/Benchmarking
Source: http://www.coker.com.au/postal/postal-%{version}.tgz 
BuildRoot: /tmp/%{name}-buildroot

%description
The Postal suite consists of an SMTP delivery benchmark (postal), a POP
retrieval benchmark (rabid) and other programs soon to be added.

%prep
%setup -q

%build
./configure --prefix=${RPM_BUILD_ROOT}
make 

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=${RPM_BUILD_ROOT} make install
install -d ${RPM_BUILD_ROOT}/usr/man/man8
install -d ${RPM_BUILD_ROOT}/usr/man/man1
install -m 644 *.8 $RPM_BUILD_ROOT/usr/man/man8
install -m 644 *.1 $RPM_BUILD_ROOT/usr/man/man1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc changelog.txt readme.html

/usr/sbin/postal
/usr/sbin/postal-list
/usr/sbin/rabid
/usr/man/man8/postal.8
/usr/man/man8/postal-list.8
/usr/man/man8/rabid.8

%changelog
* Mon Feb 19 2001 Russell Coker <russell@coker.com.au>
- first packaging
