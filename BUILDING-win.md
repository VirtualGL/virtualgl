Building VirtualGL-Utils (Windows)
==================================

Although this version of VirtualGL no longer provides a native Windows client,
native versions of several of the benchmarking utilities can still be built
using this source tree.  These utilities are distributed in a separate package
called "VirtualGL-Utils".


Build Requirements
------------------

- [CMake](http://www.cmake.org) v2.8.12 or later

- Microsoft Visual C++ 2005 or later

  If you don't already have Visual C++, then the easiest way to get it is by
  installing
  [Visual Studio Community Edition](https://visualstudio.microsoft.com),
  which includes everything necessary to build VirtualGL-Utils.

  * You can also download and install the standalone Windows SDK (for Windows 7
    or later), which includes command-line versions of the 32-bit and 64-bit
    Visual C++ compilers.
  * If you intend to build VirtualGL-Utils from the command line, then add
    the appropriate compiler and SDK directories to the `INCLUDE`, `LIB`, and
    `PATH` environment variables.  This is generally accomplished by executing
    `vcvars32.bat` or `vcvars64.bat`, which are located in the same directory
    as the compiler.

   ... OR ...

- MinGW

  [MSYS2](http://msys2.github.io/) or [tdm-gcc](http://tdm-gcc.tdragon.net/)
  recommended if building on a Windows machine.  Both distributions install a
  Start Menu link that can be used to launch a command prompt with the
  appropriate compiler paths automatically set.


Out-of-Tree Builds
------------------

Binary objects, libraries, and executables are generated in the directory from
which CMake is executed (the "binary directory"), and this directory need not
necessarily be the same as the VirtualGL source directory.  You can create
multiple independent binary directories, in which different versions of
VirtualGL-Utils can be built from the same source tree using different
compilers or settings.  In the sections below, *{build_directory}* refers to
the binary directory, whereas *{source_directory}* refers to the VirtualGL
source directory.  For in-tree builds, these directories are the same.


Build Procedure
---------------


### Visual C++ (Command Line)

    cd {build_directory}
    cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release [additional CMake flags] {source_directory}
    nmake

This will build either a 32-bit or a 64-bit version of VirtualGL-Utils,
depending on which version of **cl.exe** is in the `PATH`.


### Visual C++ (IDE)

Choose the appropriate CMake generator option for your version of Visual Studio
(run `cmake` with no arguments for a list of available generators.)  For
instance:

    cd {build_directory}
    cmake -G"Visual Studio 10" [additional CMake flags] {source_directory}

You can then open **ALL_BUILD.vcproj** in Visual Studio and build one of the
configurations in that project ("Debug", "Release", etc.) to generate a full
build of VirtualGL-Utils.


### MinGW

NOTE: This assumes that you are building on a Windows machine using the MSYS
environment.  Cross-compiling on a Linux/Unix machine is left as an exercise
for the reader.

    cd {build_directory}
    cmake -G"MSYS Makefiles" [additional CMake flags] {source_directory}
    make


Debug Build
-----------

Add `-DCMAKE_BUILD_TYPE=Debug` to the CMake command line.  Or, if building with
NMake, remove `-DCMAKE_BUILD_TYPE=Release` (Debug builds are the default with
NMake.)


Advanced CMake Options
----------------------

To list and configure other CMake options not specifically mentioned in this
guide, run

    cmake-gui {source_directory}

from the build directory after initially configuring the build.  This will
display all variables that are relevant to the VirtualGL-Utils build, their
current values, and a help string describing what they do.


Installing VirtualGL-Utils
==========================

You can use the build system to install VirtualGL-Utils into a directory of
your choosing.  To do this, add:

    -DCMAKE_INSTALL_PREFIX={install_directory}

to the CMake command line.  Then, you can run `make install` or `nmake install`
(or build the "install" target in the Visual Studio IDE) to build and install
VirtualGL-Utils.  Running `make uninstall` or `nmake uninstall` (or building
the "uninstall" target in the Visual Studio IDE) will uninstall
VirtualGL-Utils.

If you don't specify `CMAKE_INSTALL_PREFIX`, then the default is
**c:\Program Files\VirtualGL-Utils**.


Creating a Distribution Package
===============================

If using NMake:

    cd {build_directory}
    nmake installer

If using MinGW:

    cd {build_directory}
    make installer

If using the Visual Studio IDE, build the "installer" target.

The installer package (VirtualGL[64]-Utils-{version}.exe) will be located
under *{build_directory}*.  If building using the Visual Studio IDE, then the
installer package will be located in a subdirectory with the same name as the
configuration you built (such as *{build_directory}*\Debug\ or
*{build_directory}*\Release\).

Building a Windows installer requires the
[Nullsoft Install System](http://nsis.sourceforge.net/).  makensis.exe should
be in your `PATH`.
