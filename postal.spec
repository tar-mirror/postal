Summary: A program for benchmarking mail servers
Name: postal
Version: 0.71
Release: 1
License: GPL
Group: Utilities/Benchmarking
Source: http://www.coker.com.au/postal/postal_%{version}.tar.gz 
BuildRoot: /tmp/%{name}-buildroot

%description
The Postal suite consists of an SMTP delivery benchmark (postal), a POP
retrieval benchmark (rabid) and other programs soon to be added.

%prep
%setup -q

%build
./configure --prefix=/usr
make 

%install
rm -rf ${RPM_BUILD_ROOT}
DESTDIR=${RPM_BUILD_ROOT} make install
install -d ${RPM_BUILD_ROOT}/usr/share/man/man1
install -d ${RPM_BUILD_ROOT}/usr/share/man/man8
install -m 644 *.1 ${RPM_BUILD_ROOT}/usr/share/man/man1
install -m 644 *.8 ${RPM_BUILD_ROOT}/usr/share/man/man8

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc changes.txt

/usr/bin/postal-list
/usr/sbin/bhm
/usr/sbin/postal
/usr/sbin/rabid
/usr/share/man/man1/postal-list.1*
/usr/share/man/man8/bhm.8*
/usr/share/man/man8/postal.8*
/usr/share/man/man8/rabid.8*

%changelog
* Fri May 20 2011 Michael Brown <michael@netdirect.ca>
- changed configure line to not know about RPM_BUILD_ROOT - handled by install
- added bhm binary and manpage
- corrected postal-list path to /usr/bin
- added wildcard to manpage paths to handle compressed pages
- updated docfile list

* Mon Feb 19 2001 Russell Coker <russell@coker.com.au>
- first packaging
