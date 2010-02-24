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
License: wxWindows Library License v3.1
BuildRoot: %{_blddir}/%{name}-buildroot-%{version}-%{release}
Prereq: /sbin/ldconfig, /usr/bin/perl
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
mkdir -p $RPM_BUILD_ROOT/usr/include
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/bin
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/fakelib
%ifarch x86_64
mkdir -p $RPM_BUILD_ROOT/opt/%{name}/fakelib/64
%endif

install -m 755 %{_bindir}/vglclient $RPM_BUILD_ROOT/usr/bin/vglclient
install -m 755 %{_bindir}/vglconfig $RPM_BUILD_ROOT/usr/bin/vglconfig
install -m 755 %{_bindir}/vglconnect $RPM_BUILD_ROOT/usr/bin/vglconnect
install -m 755 %{_bindir}/vgllogin $RPM_BUILD_ROOT/usr/bin/vgllogin
install -m 755 %{_bindir}/vglrun $RPM_BUILD_ROOT/usr/bin/vglrun
install -m 755 rr/vglgenkey $RPM_BUILD_ROOT/usr/bin/vglgenkey
install -m 755 rr/vglserver_config $RPM_BUILD_ROOT/usr/bin//vglserver_config
install -m 755 %{_bindir}/tcbench $RPM_BUILD_ROOT/opt/%{name}/bin/tcbench
install -m 755 %{_bindir}/nettest $RPM_BUILD_ROOT/opt/%{name}/bin/nettest
install -m 755 %{_bindir}/cpustat $RPM_BUILD_ROOT/opt/%{name}/bin/cpustat
install -m 755 %{_bindir}/glxinfo $RPM_BUILD_ROOT/opt/%{name}/bin/glxinfo
install -m 755 %{_bindir32}/glxspheres $RPM_BUILD_ROOT/opt/%{name}/bin/glxspheres
install -m 755 %{_libdir32}/librrfaker.so $RPM_BUILD_ROOT/usr/lib/librrfaker.so
install -m 755 %{_libdir32}/libdlfaker.so $RPM_BUILD_ROOT/usr/lib/libdlfaker.so
install -m 755 %{_libdir32}/libgefaker.so $RPM_BUILD_ROOT/usr/lib/libgefaker.so
%ifarch x86_64
install -m 755 %{_libdir}/librrfaker.so $RPM_BUILD_ROOT/usr/lib64/librrfaker.so
install -m 755 %{_libdir}/libdlfaker.so $RPM_BUILD_ROOT/usr/lib64/libdlfaker.so
install -m 755 %{_libdir}/libgefaker.so $RPM_BUILD_ROOT/usr/lib64/libgefaker.so
install -m 755 %{_bindir}/glxspheres $RPM_BUILD_ROOT/opt/%{name}/bin/glxspheres64
%endif
install -m 644 rr/rrtransport.h $RPM_BUILD_ROOT/usr/include/rrtransport.h
install -m 644 rr/rr.h $RPM_BUILD_ROOT/usr/include/rr.h
ln -fs /usr/lib/librrfaker.so $RPM_BUILD_ROOT/opt/%{name}/fakelib/libGL.so
%ifarch x86_64
ln -fs /usr/lib64/librrfaker.so $RPM_BUILD_ROOT/opt/%{name}/fakelib/64/libGL.so
%endif
ln -fs /usr/bin/vglclient $RPM_BUILD_ROOT/opt/%{name}/bin/vglclient
ln -fs /usr/bin/vglconfig $RPM_BUILD_ROOT/opt/%{name}/bin/vglconfig
ln -fs /usr/bin/vglconnect $RPM_BUILD_ROOT/opt/%{name}/bin/vglconnect
ln -fs /usr/bin/vglgenkey $RPM_BUILD_ROOT/opt/%{name}/bin/vglgenkey
ln -fs /usr/bin/vgllogin $RPM_BUILD_ROOT/opt/%{name}/bin/vgllogin
ln -fs /usr/bin/vglserver_config $RPM_BUILD_ROOT/opt/%{name}/bin/vglserver_config
ln -fs /usr/bin/vglrun $RPM_BUILD_ROOT/opt/%{name}/bin/vglrun
ln -fs /usr/share/doc/%{name}-%{version} $RPM_BUILD_ROOT/opt/%{name}/doc

cp fltk/COPYING LICENSE-FLTK.txt
cp putty/LICENCE LICENSE-PuTTY.txt
cp x11windows/xauth.license LICENSE-xauth.txt
chmod 644 LGPL.txt LICENSE.txt LICENSE-FLTK.txt LICENSE-PuTTY.txt LICENSE-xauth.txt ChangeLog.txt doc/index.html doc/*.png doc/*.gif doc/*.css

echo '/usr/lib/rpm/find-requires|grep -v libGLcore|grep -v libnvidia-tls' >%{_tmppath}/%{name}-%{version}-%{release}-find-requires
chmod 755 %{_tmppath}/%{name}-%{version}-%{release}-find-requires

%clean
rm -rf $RPM_BUILD_ROOT
rm %{_tmppath}/%{name}-%{version}-%{release}-find-requires

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files -n %{name}
%defattr(-,root,root)
%doc LGPL.txt LICENSE.txt LICENSE-FLTK.txt LICENSE-PuTTY.txt LICENSE-xauth.txt ChangeLog.txt doc/index.html doc/*.png doc/*.gif doc/*.css

%dir /opt/%{name}
%dir /opt/%{name}/bin
%dir /opt/%{name}/fakelib
%ifarch x86_64
%dir /opt/%{name}/fakelib/64
%endif

/usr/bin/vglclient
/usr/bin/vglconfig
/usr/bin/vglconnect
/usr/bin/vgllogin
/usr/bin/vglrun
/usr/bin/vglgenkey
/usr/bin/vglserver_config

/opt/%{name}/bin/tcbench
/opt/%{name}/bin/nettest
/opt/%{name}/bin/cpustat
/opt/%{name}/bin/glxinfo
/opt/%{name}/bin/vglclient
/opt/%{name}/bin/vglconfig
/opt/%{name}/bin/vglconnect
/opt/%{name}/bin/vglgenkey
/opt/%{name}/bin/vgllogin
/opt/%{name}/bin/vglserver_config
/opt/%{name}/bin/vglrun
/opt/%{name}/bin/glxspheres

/opt/%{name}/fakelib/libGL.so
%ifarch x86_64
/opt/%{name}/fakelib/64/libGL.so
%endif
/opt/%{name}/doc

/usr/lib/librrfaker.so
/usr/lib/libdlfaker.so
/usr/lib/libgefaker.so
%ifarch x86_64
/usr/lib64/librrfaker.so
/usr/lib64/libdlfaker.so
/usr/lib64/libgefaker.so
/opt/%{name}/bin/glxspheres64
%endif
/usr/include/rrtransport.h
/usr/include/rr.h

%changelog
