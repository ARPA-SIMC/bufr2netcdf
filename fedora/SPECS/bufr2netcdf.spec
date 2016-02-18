Summary: Tools to convert bufr weather reports in Netcdf file format 
Name: bufr2netcdf
Version: 1.3
Release: 1
License: GPL2
Group: Applications/Meteo
Source0: https://github.com/arpa-simc/%{name}/archive/v%{version}-%{release}.tar.gz#/%{name}-%{version}-%{release}.tar.gz
URL: https://github.com/ARPA-SIMC/bufr2netcdf
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: doxygen, libtool
%description
Tools to convert bufr weather reports in Netcdf file format in DWD standard

%prep
%setup -q -n %{name}-%{version}-%{release}
sh autogen.sh

%build
%configure
make
make check

%install
[ "%{buildroot}" != / ] && rm -rf %{buildroot}
%makeinstall

%clean
[ "%{buildroot}" != / ] && rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%{_bindir}/bufr2netcdf
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*


%changelog
* Thu Feb 18 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 1.3-1%{dist}
- c++11 porting
* Tue Sep 28 2010 root <ppatruno@arpa.emr.it> - 0.9
- Initial build.
