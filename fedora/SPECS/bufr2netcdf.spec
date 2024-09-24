%global releaseno 1
# Note: define _srcarchivename in Travis build only.
%{!?srcarchivename: %global srcarchivename %{name}-%{version}-%{releaseno}}

Summary: Tools to convert bufr weather reports in Netcdf file format 
Name: bufr2netcdf
Version: 1.8
Release: %{releaseno}%{?dist}
License: GPL2
Group: Applications/Meteo
Source0: https://github.com/arpa-simc/%{name}/archive/v%{version}-%{releaseno}.tar.gz#/%{srcarchivename}.tar.gz
URL: https://github.com/ARPA-SIMC/bufr2netcdf
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: doxygen
BuildRequires: meson
BuildRequires: gcc-c++
BuildRequires: libwreport-devel
BuildRequires: netcdf-cxx-devel

%description
Tools to convert BUFR weather reports in NetCDF file format in DWD standard

%prep
%setup -q -n %{srcarchivename}

%build
%meson
%meson_build

%check
%meson_test

%install
%meson_install

%clean
[ "%{buildroot}" != / ] && rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%{_bindir}/bufr2netcdf
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*


%changelog
* Tue Sep 24 2024 Daniele Branchini <dbranchini@arpae.it> - 1.8-1%{?dist}
- Porting to meson and to c++17
* Mon Feb 26 2024 Daniele Branchini <dbranchini@arpae.it> - 1.7-1%{?dist}
- Discriminate messages by BUFR table version numbers, to avoid conflicts
  between messages with the same Data Descriptor Section but which expand to
  different data structures (#11, #13)
- Use doubles for latitude and longitude (#10)
* Mon Sep 14 2020 Daniele Branchini <dbranchini@arpae.it> - 1.6-1%{?dist}
- Convert BUFR messages that use C03 reference value override codes (#7)
* Wed Aug 22 2018 Davide Cesari <dcesari@arpae.it> - 1.5-1%{dist}
- add continuous integration
* Thu Sep 7 2017 Daniele Branchini <dbranchini@arpae.it> - 1.4-1%{dist}
- newer DWD tables v2 and add tables in new format v3 for development
* Thu Feb 18 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 1.3-1%{dist}
- c++11 porting
* Tue Sep 28 2010 root <ppatruno@arpa.emr.it> - 0.9
- Initial build.
