%ifarch x86_64
%define package %{_appname}64
%else
%define package %{_appname}
%endif

Summary: %{_appname} diagnostic tools
Name: %{package}-diags
Version: %{_version}
Vendor: Landmark Graphics
Group: Applications/Graphics
Release: %{_build}
License: wxWindows Library License, v3
BuildRoot: %{_curdir}/%{name}-buildroot
Requires: %{package}
Provides: %{name} = %{version}-%{release}

%description
This package provides tools for measuring the performance of the components
which make up %{_appname}.  It is useful for diagnosing network, CPU, and
other types of bottlenecks during deployment.

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/opt/%{package}/bin

install -m 755 %{_curdir}/%{_bindir}/nettest $RPM_BUILD_ROOT/opt/%{package}/bin/nettest
install -m 755 %{_curdir}/%{_bindir}/fbxtest $RPM_BUILD_ROOT/opt/%{package}/bin/fbxtest
install -m 755 %{_curdir}/%{_bindir}/jpgtest $RPM_BUILD_ROOT/opt/%{package}/bin/jpgtest
install -m 755 %{_curdir}/%{_bindir}/tcbench $RPM_BUILD_ROOT/opt/%{package}/bin/tcbench
install -m 755 %{_curdir}/%{_bindir}/cpustat $RPM_BUILD_ROOT/opt/%{package}/bin/cpustat
install -m 755 %{_curdir}/%{_bindir}/imgdiff $RPM_BUILD_ROOT/opt/%{package}/bin/imgdiff

%clean
rm -rf $RPM_BUILD_ROOT

%files -n %{package}-diags
%defattr(-,root,root)
/opt/%{package}/bin/nettest
/opt/%{package}/bin/fbxtest
/opt/%{package}/bin/jpgtest
/opt/%{package}/bin/tcbench
/opt/%{package}/bin/cpustat
/opt/%{package}/bin/imgdiff

%changelog
