*******************************************************************************
**     Building VirtualGL-Utils (Windows)
*******************************************************************************

Although this version of VirtualGL no longer provides a native Windows client,
native versions of several of the benchmarking utilities can still be built
using this source tree.  These utilities are distributed in a separate package
called "VirtualGL-Utils".


==================
Build Requirements
==================

-- CMake (http://www.cmake.org) v2.8 or later

-- If building SSL support:
   * OpenSSL (http://www.OpenSSL.org) -- see "Building SSL Support" below

-- Microsoft Visual C++ 2005 or later

   If you don't already have Visual C++, then the easiest way to get it is by
   installing the Windows SDK:

   http://msdn.microsoft.com/en-us/windows/bb980924.aspx

   The Windows SDK includes both 32-bit and 64-bit Visual C++ compilers and
   everything necessary to build VirtualGL-Utils.

   * For 32-bit builds, you can also use Microsoft Visual C++ Express
     Edition.  Visual C++ Express Edition is a free download.
   * If you intend to build VirtualGL-Utils from the command line, then add
     the appropriate compiler and SDK directories to the INCLUDE, LIB, and PATH
     environment variables.  This is generally accomplished by executing
     vcvars32.bat or vcvars64.bat and SetEnv.cmd.  vcvars32.bat and
     vcvars64.bat are part of Visual C++ and are located in the same directory
     as the compiler.  SetEnv.cmd is part of the Windows SDK.  You can pass
     optional arguments to SetEnv.cmd to specify a 32-bit or 64-bit build
     environment.

... OR ...

-- MinGW

   MinGW-builds (http://sourceforge.net/projects/mingwbuilds/) or
   tdm-gcc (http://tdm-gcc.tdragon.net/) recommended if building on a Windows
   machine.  Both distributions install a Start Menu link that can be used to
   launch a command prompt with the appropriate compiler paths automatically
   set.


==================
Out-of-Tree Builds
==================

Binary objects, libraries, and executables are generated in the same directory
from which CMake was executed (the "binary directory"), and this directory need
not necessarily be the same as the VirtualGL source directory.  You can create
multiple independent binary directories, in which different versions of
VirtualGL-Utils can be built from the same source tree using different
compilers or settings.  In the sections below, {build_directory} refers to the
binary directory, whereas {source_directory} refers to the VirtualGL source
directory.  For in-tree builds, these directories are the same.


===============
Build Procedure
===============


Visual C++ (Command Line)
-------------------------

  cd {build_directory}
  cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release [additional CMake flags] {source_directory}
  nmake

This will build either a 32-bit or a 64-bit version of VirtualGL-Utils,
depending on which version of cl.exe is in the PATH.


Visual C++ (IDE)
----------------

Choose the appropriate CMake generator option for your version of Visual Studio
(run "cmake" with no arguments for a list of available generators.)  For
instance:

  cd {build_directory}
  cmake -G "Visual Studio 9 2008" [additional CMake flags] {source_directory}

You can then open ALL_BUILD.vcproj in Visual Studio and build one of the
configurations in that project ("Debug", "Release", etc.) to generate a full
build of VirtualGL-Utils.


MinGW
-----

NOTE: This assumes that you are building on a Windows machine.  Cross-compiling
on a Linux/Unix machine is left as an exercise for the reader.

  cd {build_directory}
  cmake -G "MinGW Makefiles" [additional CMake flags] {source_directory}
  mingw32-make


===========
Debug Build
===========

Add "-DCMAKE_BUILD_TYPE=Debug" to the CMake command line.  Or, if building with
NMake, remove "-DCMAKE_BUILD_TYPE=Release" (Debug builds are the default with
NMake.)


===========================================
Building Secure Sockets Layer (SSL) Support
===========================================

If built with SSL support, NetTest can be used to test the performance of
a particular OpenSSL implementation.  To enable SSL support, set the VGL_USESSL
CMake variable to 1.

The easiest way to get OpenSSL is to install the Win32 OpenSSL or Win64 OpenSSL
package from here:

http://www.slproweb.com/products/Win32OpenSSL.html

You can then add one of the following to the CMake command line to statically
link NetTest with OpenSSL:

  Win64 Release (Visual Studio):

    -DVGL_USESSL=1 -DOPENSSL_INCLUDE_DIR=c:\OpenSSL-Win64\include \
      -DLIB_EAY_RELEASE=c:\OpenSSL-Win64\lib\VC\libeay32MT.lib \
      -DSSL_EAY_RELEASE=c:\OpenSSL-Win64\lib\VC\ssleay32MT.lib

  Win64 Debug (Visual Studio):

    -DVGL_USESSL=1 -DOPENSSL_INCLUDE_DIR=c:\OpenSSL-Win64\include \
      -DLIB_EAY_DEBUG=c:\OpenSSL-Win64\lib\VC\libeay32MTd.lib \
      -DSSL_EAY_DEBUG=c:\OpenSSL-Win64\lib\VC\ssleay32MTd.lib

  Win32 Release (Visual Studio):

    -DVGL_USESSL=1 -DOPENSSL_INCLUDE_DIR=c:\OpenSSL-Win32\include \
      -DLIB_EAY_RELEASE=c:\OpenSSL-Win32\lib\VC\libeay32MT.lib \
      -DSSL_EAY_RELEASE=c:\OpenSSL-Win32\lib\VC\ssleay32MT.lib

  Win32 Debug (Visual Studio):

    -DVGL_USESSL=1 -DOPENSSL_INCLUDE_DIR=c:\OpenSSL-Win32\include \
      -DLIB_EAY_DEBUG=c:\OpenSSL-Win32\lib\VC\libeay32MTd.lib \
      -DSSL_EAY_DEBUG=c:\OpenSSL-Win32\lib\VC\ssleay32MTd.lib

  Win32 (MinGW):

    -DVGL_USESSL=1 -DOPENSSL_INCLUDE_DIR=c:/OpenSSL-Win32/include \
      -DLIB_EAY=c:/OpenSSL-Win32/lib/MinGW/libeay32.a \
      -DSSL_EAY=c:/OpenSSL-Win32/lib/MinGW/ssleay32.a


*******************************************************************************
**     Advanced CMake Options
*******************************************************************************

To list and configure other CMake options not specifically mentioned in this
guide, run

  cmake-gui {source_directory}

after initially configuring the build.  This will display all variables that
are relevant to the VirtualGL-Utils build, their current values, and a help
string describing what they do.


*******************************************************************************
**     Installing VirtualGL-Utils
*******************************************************************************

You can use the build system to install VirtualGL-Utils into a directory of
your choosing.  To do this, add:

  -DCMAKE_INSTALL_PREFIX={install_directory}

to the CMake command line.  Then, you can run 'make install' or 'nmake install'
(or build the "install" target in the Visual Studio IDE) to build and install
VirtualGL-Utils.  Running 'make uninstall' or 'nmake uninstall' (or building
the "uninstall" target in the Visual Studio IDE) will uninstall
VirtualGL-Utils.

If you don't specify CMAKE_INSTALL_PREFIX, then the default is
c:\Program Files\VirtualGL-Utils.


*******************************************************************************
**     Creating Release Packages
*******************************************************************************

If using NMake:

  cd {build_directory}
  nmake installer

If using the Visual Studio IDE, build the "installer" project.

The installer package (VirtualGL[64]-Utils-{version}.exe) will be located
under {build_directory}.  If building using the Visual Studio IDE, then the
installer package will be located in a subdirectory with the same name as the
configuration you built (such as {build_directory}\Debug\ or
{build_directory}\Release\).

Building a Windows installer requires the Nullsoft Install System
(http://nsis.sourceforge.net/).  makensis.exe should be in your PATH.
