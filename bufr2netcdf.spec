Summary: Tools to convert bufr weather reports in Netcdf file format 
Name: bufr2netcdf
Version: 0.9
Release: 3335
License: GPL2
Group: Applications/Meteo
URL: http://www.arpa.emr.it/dettaglio_documento.asp?id=514&idlivello=64
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: doxygen, libtool, libwibble-devel
%description
Tools to convert bufr weather reports in Netcdf file format in DWD standard

%prep
%setup -q

%build
%configure
make
make check

%install
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"
make install DESTDIR="%{buildroot}"

%clean
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"


%files
%defattr(-,root,root,-)
%{_bindir}/bufr2netcdf
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*


%changelog
* Tue Sep 28 2010 root <ppatruno@arpa.emr.it> - 0.9
- Initial build.
