%ifarch x86_64
%define usrlib /usr/lib64
%define package %{_appname}64
%else
%define usrlib /usr/lib
%define package %{_appname}
%endif

Summary: A non-intrusive remote rendering package
Name: %{package}
Version: %{_version}
Vendor: Landmark Graphics
Group: Applications/Graphics
Release: %{_build}
License: wxWindows Library License, v3
BuildRoot: %{_curdir}/%{name}-buildroot
Prereq: /sbin/ldconfig
Provides: %{name} = %{version}-%{release}

%description
%{_appname} is a non-intrusive remote rendering package that
allows any 3D OpenGL application to be remotely displayed over a network
in true thin client fashion without modifying the application.  %{_appname}
relies upon X11 to send the GUI and receive input events, and it sends the
OpenGL pixels separately using a motion-JPEG style protocol (and a high-speed
MMX/SSE-aware JPEG codec.)  The OpenGL pixels are composited into the
X window on the client.  %{_appname} intercepts GLX calls on the server
to force the application to do its rendering in a Pbuffer, and it then
determines when to read back this Pbuffer, compress it, and send it to the
client display.

%{_appname} is based upon ideas presented in various academic papers on
this topic, including "A Generic Solution for Hardware-Accelerated Remote
Visualization" (Stegmaier, Magallon, Ertl 2002) and "A Framework for
Interactive Hardware Accelerated Remote 3D-Visualization" (Engel, Sommer,
Ertl 2000.)

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT%{usrlib}

%ifarch x86_64
install -m 755 %{_curdir}/%{_bindir}/rrlaunch64 $RPM_BUILD_ROOT/usr/bin/rrlaunch64
%else
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d

install -m 755 %{_curdir}/%{_bindir}/rrxclient $RPM_BUILD_ROOT/usr/bin/rrxclient
install -m 755 %{_curdir}/%{_bindir}/rrlaunch $RPM_BUILD_ROOT/usr/bin/rrlaunch
install -m 755 %{_curdir}/rr/newrrcert $RPM_BUILD_ROOT/usr/bin/newcert
install -m 644 %{_curdir}/rr/rrcert.cnf $RPM_BUILD_ROOT/etc/rrcert.cnf
install -m 755 %{_curdir}/rr/rrxclient.sh $RPM_BUILD_ROOT/etc/rc.d/init.d/rrxclient
%endif

install -m 755 %{_curdir}/%{_libdir}/librrfaker.so $RPM_BUILD_ROOT%{usrlib}/librrfaker.so
install -m 755 %{_curdir}/%{_libdir}/libhpjpeg.so $RPM_BUILD_ROOT%{usrlib}/libhpjpeg.so

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%ifarch x86_64
%else
%preun
service rrxclient stop
/sbin/chkconfig --del rrxclient
%endif

%files -n %{package}
%defattr(-,root,root)

%ifarch x86_64
/usr/bin/rrlaunch64

%else
/usr/bin/rrxclient
/usr/bin/newcert
/etc/rrcert.cnf
/etc/rc.d/init.d/rrxclient
/usr/bin/rrlaunch

%endif

%{usrlib}/librrfaker.so
%{usrlib}/libhpjpeg.so

%changelog
