Instructions for building the VirtualGL Server for Windows:

1) Download and install the Microsoft Detours package from:
   http://research.microsoft.com/sn/detours/

2) Build the Detours libraries:
   cd /d "C:\Program Files\Microsoft Research\Detours Express 2.1\"
   nmake CLIB=/MT

3) Add "C:\Program Files\Microsoft Research\Detours Express 2.1\src\lib" to
   your system's LIB environment variable
   Add "C:\Program Files\Microsoft Research\Detours Express 2.1\src\include" to
   your system INCLUDE environment variable
   Add "C:\Program Files\Microsoft Research\Detours Express 2.1\src\bin" to
   your system PATH environment variable

4) Build VirtualGL

   cd vgl
   make

   (refer to vgl\BUILDING.txt for further instructions.)

5) Build the VirtualGL Server for Windows

   cd vgl\winfaker
   make

6) (optionally) build an installer

   cd vgl\winfaker
   make dist

   * This requires the Nullsoft Install System (http://nsis.sourceforge.net/.)
     makensis.exe should be in your PATH.


Instructions for running the VirtualGL Server for Windows.

1) Launch the application using a command line such as

   {path to}\vglrun.bat {path to}\your_opengl_app.exe

   * Execute vglrun.bat with no arguments to display usage information.
