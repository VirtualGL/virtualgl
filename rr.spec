%ifarch x86_64
%define usrlib /usr/lib64
%define package %{_appname}64
%else
%define usrlib /usr/lib
%define package %{_appname}
%endif
%define _XINIT /etc/X11/xinit/xinitrc.d/rrxclient
%define _XINITSSL /etc/X11/xinit/xinitrc.d/rrxclient_ssl
%define _DAEMON /usr/bin/rrxclient_daemon
%define _DAEMONSSL /usr/bin/rrxclient_ssldaemon
%define _POSTSESSION /etc/X11/gdm/PostSession/Default

Summary: A non-intrusive remote rendering package
Name: %{package}
Version: %{_version}
Vendor: Landmark Graphics
Group: Applications/Graphics
Release: %{_build}
License: wxWindows Library License, v3
BuildRoot: %{_curdir}/%{name}-buildroot
Prereq: /sbin/ldconfig, /usr/bin/perl
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
mkdir -p $RPM_BUILD_ROOT/opt/%{package}-diags/bin

%ifarch x86_64
install -m 755 %{_curdir}/%{_bindir}/rrlaunch64 $RPM_BUILD_ROOT/usr/bin/rrlaunch64
%else
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d

install -m 755 %{_curdir}/%{_bindir}/rrxclient $RPM_BUILD_ROOT/usr/bin/rrxclient
install -m 755 %{_curdir}/%{_bindir}/rrlaunch $RPM_BUILD_ROOT/usr/bin/rrlaunch
install -m 755 %{_curdir}/rr/newrrcert $RPM_BUILD_ROOT/usr/bin/newrrcert
install -m 644 %{_curdir}/rr/rrcert.cnf $RPM_BUILD_ROOT/etc/rrcert.cnf
install -m 755 %{_curdir}/rr/rrxclient.sh $RPM_BUILD_ROOT%{_DAEMON}
install -m 755 %{_curdir}/rr/rrxclient_ssl.sh $RPM_BUILD_ROOT%{_DAEMONSSL}
install -m 755 %{_curdir}/rr/rrxclient_config $RPM_BUILD_ROOT/usr/bin/rrxclient_config

install -m 755 %{_curdir}/%{_bindir}/tcbench $RPM_BUILD_ROOT/opt/%{package}-diags/bin/tcbench

%endif

install -m 755 %{_curdir}/%{_bindir}/nettest $RPM_BUILD_ROOT/opt/%{package}-diags/bin/nettest
install -m 755 %{_curdir}/%{_bindir}/cpustat $RPM_BUILD_ROOT/opt/%{package}-diags/bin/cpustat

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
%{_DAEMON} stop
%{_DAEMONSSL} stop
if [ -x %{_XINIT} ]; then rm %{_XINIT}; fi
if [ -x %{_XINITSSL} ]; then rm %{_XINITSSL}; fi
if [ -x %{_POSTSESSION} ]; then
	/usr/bin/perl -i -p -e "s:%{_DAEMON} stop\n::g;" %{_POSTSESSION}
	/usr/bin/perl -i -p -e "s:%{_DAEMONSSL} stop\n::g;" %{_POSTSESSION}
fi
%endif

%files -n %{package}
%defattr(-,root,root)

%ifarch x86_64
/usr/bin/rrlaunch64

%else
/usr/bin/rrxclient
/usr/bin/newrrcert
/etc/rrcert.cnf
%{_DAEMON}
%{_DAEMONSSL}
/usr/bin/rrxclient_config
/usr/bin/rrlaunch

/opt/%{package}-diags/bin/tcbench

%endif

/opt/%{package}-diags/bin/nettest
/opt/%{package}-diags/bin/cpustat

%{usrlib}/librrfaker.so
%{usrlib}/libhpjpeg.so

%changelog
