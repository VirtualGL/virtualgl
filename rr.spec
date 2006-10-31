%define _DAEMON /usr/bin/vglclient_daemon

# This is a hack to prevent the RPM from depending on libGLcore.so.1 and
# libnvidia-tls.so.1 if it was built on a system that has the NVidia
# drivers installed.  The custom find-requires script is created during
# install and removed during clean

%define __find_requires %{_tmppath}/%{name}-%{version}-%{release}-find-requires

Summary: A toolkit for displaying OpenGL applications to thin clients
Name: VirtualGL
Version: %{_version}
Vendor: The VirtualGL Project
URL: http://www.virtualgl.org
Group: Applications/Graphics
#-->Source0: http://prdownloads.sourceforge.net/virtualgl/VirtualGL-%{version}.tar.gz
Release: %{_build}
License: wxWindows Library License, v3
BuildRoot: %{_blddir}/%{name}-buildroot-%{version}-%{release}
BuildPrereq: openssl-devel, turbojpeg
Prereq: /sbin/ldconfig, /usr/bin/perl, turbojpeg >= 1.0
Provides: %{name} = %{version}-%{release}

%description
VirtualGL is a library which allows most Linux OpenGL applications to be
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

#-->%prep
#-->%setup -q -n vgl

#-->%build
#-->make DISTRO=
#-->%ifarch x86_64
#-->make M32=yes DISTRO=
#-->%endif

%install

rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
%ifarch x86_64
mkdir -p $RPM_BUILD_ROOT/usr/lib64
%endif
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/bin
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/include
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/lib
%ifarch x86_64
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/lib64
%endif
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/samples
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d

install -m 755 %{_bindir32}/vglclient $RPM_BUILD_ROOT/usr/bin/vglclient
install -m 755 %{_bindir}/vglrun $RPM_BUILD_ROOT/usr/bin/vglrun
install -m 755 %{_bindir}/vglrun $RPM_BUILD_ROOT/usr/bin/rrlaunch
install -m 755 rr/rrxclient.sh $RPM_BUILD_ROOT%{_DAEMON}
install -m 755 rr/rrxclient_config $RPM_BUILD_ROOT/usr/bin/vglclient_config
install -m 755 rr/vglgenkey $RPM_BUILD_ROOT/usr/bin/vglgenkey
install -m 755 %{_bindir32}/tcbench $RPM_BUILD_ROOT/opt/%{name}/bin/tcbench
install -m 755 %{_bindir32}/nettest $RPM_BUILD_ROOT/opt/%{name}/bin/nettest
install -m 755 %{_bindir32}/cpustat $RPM_BUILD_ROOT/opt/%{name}/bin/cpustat
install -m 755 %{_libdir32}/librrfaker.so $RPM_BUILD_ROOT/usr/lib/librrfaker.so
install -m 755 %{_libdir32}/libdlfaker.so $RPM_BUILD_ROOT/usr/lib/libdlfaker.so
install -m 755 %{_libdir32}/librr.so $RPM_BUILD_ROOT/usr/lib/librr.so
install -m 644 rr/rr.h  $RPM_BUILD_ROOT/opt/%{name}/include
install -m 644 samples/rrglxgears.c  $RPM_BUILD_ROOT/opt/%{name}/samples
install -m 644 samples/Makefile.linux $RPM_BUILD_ROOT/opt/%{name}/samples/
%ifarch x86_64
install -m 755 %{_libdir}/librrfaker.so $RPM_BUILD_ROOT/usr/lib64/librrfaker.so
install -m 755 %{_libdir}/libdlfaker.so $RPM_BUILD_ROOT/usr/lib64/libdlfaker.so
install -m 755 %{_libdir}/librr.so $RPM_BUILD_ROOT/usr/lib64/librr.so
install -m 644 samples/Makefile.linux64 $RPM_BUILD_ROOT/opt/%{name}/samples/
%endif
ln -fs /usr/lib/librrfaker.so $RPM_BUILD_ROOT/opt/%{name}/lib/libGL.so
%ifarch x86_64
ln -fs /usr/lib64/librrfaker.so $RPM_BUILD_ROOT/opt/%{name}/lib64/libGL.so
%endif

chmod 644 LGPL.txt LICENSE.txt LICENSE-OpenSSL.txt doc/index.html doc/*.png doc/*.gif doc/*.css

echo '/usr/lib/rpm/find-requires|grep -v libGLcore|grep -v libnvidia-tls' >%{_tmppath}/%{name}-%{version}-%{release}-find-requires
chmod 755 %{_tmppath}/%{name}-%{version}-%{release}-find-requires

%clean
rm -rf $RPM_BUILD_ROOT
rm %{_tmppath}/%{name}-%{version}-%{release}-find-requires

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%preun
%{_DAEMON} stop
/usr/bin/vglclient_config -remove

%files -n %{name}
%defattr(-,root,root)
%doc LGPL.txt LICENSE.txt LICENSE-OpenSSL.txt doc/index.html doc/*.png doc/*.gif doc/*.css

%dir /opt/%{name}
%dir /opt/%{name}/bin
%dir /opt/%{name}/include
%dir /opt/%{name}/lib
%ifarch x86_64
%dir /opt/%{name}/lib64
%endif
%dir /opt/%{name}/samples

/usr/bin/vglclient
%{_DAEMON}
/usr/bin/vglclient_config
/usr/bin/vglrun
/usr/bin/rrlaunch
/usr/bin/vglgenkey

/opt/%{name}/bin/tcbench
/opt/%{name}/bin/nettest
/opt/%{name}/bin/cpustat

/opt/%{name}/include/rr.h
/opt/%{name}/samples/rrglxgears.c
/opt/%{name}/samples/Makefile.linux
%ifarch x86_64
/opt/%{name}/samples/Makefile.linux64
%endif

/opt/%{name}/lib/libGL.so
%ifarch x86_64
/opt/%{name}/lib64/libGL.so
%endif

/usr/lib/librrfaker.so
/usr/lib/libdlfaker.so
/usr/lib/librr.so
%ifarch x86_64
/usr/lib64/librrfaker.so
/usr/lib64/libdlfaker.so
/usr/lib64/librr.so
%endif

%changelog
