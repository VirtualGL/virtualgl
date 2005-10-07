%define _XINIT /etc/X11/xinit/xinitrc.d/rrxclient
%define _XINITSSL /etc/X11/xinit/xinitrc.d/rrxclient_ssl
%define _DAEMON /usr/bin/rrxclient_daemon
%define _DAEMONSSL /usr/bin/rrxclient_ssldaemon
%define _POSTSESSION /etc/X11/gdm/PostSession/Default

Summary: A framework for displaying OpenGL applications to thin clients
Name: VirtualGL
Version: %{_version}
Vendor: The VirtualGL Project
URL: http://virtualgl.sourceforge.net
Group: Applications/Graphics
Release: %{_build}
License: wxWindows Library License, v3
BuildRoot: %{_blddir}/%{name}-buildroot
Prereq: /sbin/ldconfig, /usr/bin/perl, turbojpeg >= 1.0
Provides: %{name} = %{version}-%{release}

%description
VirtualGL is a framework which allows most Linux OpenGL applications to be
remotely displayed to a thin client without the need to alter the
applications in any way.  VGL inserts itself into an application at run time
and intercepts a handful of GLX calls, which it reroutes to the server's
display (which presumably has a 3D accelerator attached.)  This causes all
3D rendering to occur on the server's display.  As each frame is rendered
by the server, VirtualGL reads back the pixels from the server's framebuffer
and sends them to the client for re-compositing into the appropriate X
Window.  VirtualGL can be used to give hardware-accelerated 3D capabilities to
VNC or other remote display environments that lack GLX support.  In a LAN
environment, it can also be used with its built-in motion-JPEG video delivery
system to remotely display full-screen 3D applications at 20+ frames/second.

VirtualGL is based upon ideas presented in various academic papers on
this topic, including "A Generic Solution for Hardware-Accelerated Remote
Visualization" (Stegmaier, Magallon, Ertl 2002) and "A Framework for
Interactive Hardware Accelerated Remote 3D-Visualization" (Engel, Sommer,
Ertl 2000.)

%install

rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
%ifarch x86_64
mkdir -p $RPM_BUILD_ROOT/usr/lib64
%endif
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/bin
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d

install -m 755 %{_bindir32}/rrxclient $RPM_BUILD_ROOT/usr/bin/rrxclient
install -m 755 %{_bindir}/rrlaunch $RPM_BUILD_ROOT/usr/bin/rrlaunch
install -m 755 rr/rrxclient.sh $RPM_BUILD_ROOT%{_DAEMON}
install -m 755 rr/rrxclient_ssl.sh $RPM_BUILD_ROOT%{_DAEMONSSL}
install -m 755 rr/rrxclient_config $RPM_BUILD_ROOT/usr/bin/rrxclient_config
install -m 755 %{_bindir32}/tcbench $RPM_BUILD_ROOT/opt/%{name}/bin/tcbench
install -m 755 %{_bindir32}/nettest $RPM_BUILD_ROOT/opt/%{name}/bin/nettest
install -m 755 %{_bindir32}/cpustat $RPM_BUILD_ROOT/opt/%{name}/bin/cpustat
install -m 755 %{_libdir32}/librrfaker.so $RPM_BUILD_ROOT/usr/lib/librrfaker.so
%ifarch x86_64
install -m 755 %{_libdir}/librrfaker.so $RPM_BUILD_ROOT/usr/lib64/librrfaker.so
%endif

chmod 644 LGPL.txt LICENSE.txt LICENSE-OpenSSL.txt doc/index.html doc/*.png doc/*.gif

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

%files -n %{name}
%defattr(-,root,root)
%doc LGPL.txt LICENSE.txt LICENSE-OpenSSL.txt doc/index.html doc/*.png doc/*.gif

/usr/bin/rrxclient
%{_DAEMON}
%{_DAEMONSSL}
/usr/bin/rrxclient_config
/usr/bin/rrlaunch

/opt/%{name}/bin/tcbench
/opt/%{name}/bin/nettest
/opt/%{name}/bin/cpustat

/usr/lib/librrfaker.so
%ifarch x86_64
/usr/lib64/librrfaker.so
%endif

%changelog
