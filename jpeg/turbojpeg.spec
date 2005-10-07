Summary: A JPEG codec that compresses full-screen images at interactive frame rates
Name: turbojpeg
Version: 1.0
Vendor: The VirtualGL Project
URL: http://virtualgl.sourceforge.net
Group: System Environment/Libraries
Release: %{_build}
License: wxWindows Library License, v3
BuildRoot: %{_blddir}/%{name}-buildroot
Prereq: /sbin/ldconfig
Provides: %{name} = %{version}-%{release}

%description
TurboJPEG provides a minimalistic interface for compressing true color
bitmaps into JPEG images in memory.  The library is designed for use by
video applications which need to compress large (> 1 Megapixel) images many
times per second, but it can be used for still image compression as well.

TurboJPEG abstracts a variety of vendor-specific codecs from Intel, Sun,
Apple, etc. into a common API that is used by VirtualGL and TurboVNC.  The
use of these vendor-specific codecs allows TurboJPEG to compress HDTV
resolutions at real-time or greater frame rates on Linux, Mac, and
Solaris systems.

TurboJPEG is used by VirtualGL and TurboVNC as the default method of image
compression.

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
%ifarch x86_64
mkdir -p $RPM_BUILD_ROOT/usr/lib64
%endif

%ifarch x86_64
install -m 755 %{_libdir}/libturbojpeg.so $RPM_BUILD_ROOT/usr/lib64/libturbojpeg.so
install -m 755 %{_libdir32}/libturbojpeg.so $RPM_BUILD_ROOT/usr/lib/libturbojpeg.so
%else
install -m 755 %{_libdir}/libturbojpeg.so $RPM_BUILD_ROOT/usr/lib/libturbojpeg.so
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files -n turbojpeg
%defattr(-,root,root)

/usr/lib/libturbojpeg.so
%ifarch x86_64
/usr/lib64/libturbojpeg.so
%endif

%changelog
