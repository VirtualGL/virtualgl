#
# Setup
#

cmake_minimum_required(VERSION 2.8)
set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required

if(UNIX)
	set(CMAKE_INSTALL_PREFIX /opt/VirtualGL CACHE PATH
		"Install path prefix, prepended onto install directories.")
endif()

if(WIN32)
	project(VirtualGL-Utils)
else()
	project(VirtualGL)
endif()
set(VERSION 2.4)

if(UNIX)
	execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
	string(REGEX REPLACE "\n" "" BUILD ${BUILD})
elseif(WIN32)
	execute_process(COMMAND "wmic.exe" "os" "get" "LocalDateTime" OUTPUT_VARIABLE
		BUILD)
	string(REGEX REPLACE "[^0-9]" "" BUILD "${BUILD}")
	if (BUILD STREQUAL "")
		execute_process(COMMAND "cmd.exe" "/C" "DATE" "/T" OUTPUT_VARIABLE BUILD)
		string(REGEX REPLACE ".*[ ]([0-9]*)[/.]([0-9]*)[/.]([0-9]*).*" "\\3\\2\\1" BUILD "${BUILD}")
	else()
		string(SUBSTRING "${BUILD}" 0 8 BUILD)
	endif()
else()
	message(FATAL_ERROR "Platform not supported.")
endif()
string(REGEX REPLACE "\n" "" BUILD ${BUILD})

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# This only works if building from the command line.  There is currently no way
# to set a variable's value based on the build type when using the MSVC IDE.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(BUILD "${BUILD}d")
endif()

message(STATUS "VERSION = ${VERSION}, BUILD = ${BUILD}")
add_definitions(-D__VERSION="${VERSION}" -D__BUILD="${BUILD}"
	-D__APPNAME="${CMAKE_PROJECT_NAME}")

message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

set(DEFAULT_VGL_DOCDIR ${CMAKE_INSTALL_PREFIX}/doc)
if(WIN32)
	set(DEFAULT_VGL_BINDIR ${CMAKE_INSTALL_PREFIX})
else()
	set(DEFAULT_VGL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()

set(VGL_BINDIR ${DEFAULT_VGL_BINDIR} CACHE PATH
	"Directory into which executables and scripts should be installed (default: ${DEFAULT_VGL_BINDIR})")
set(VGL_DOCDIR ${DEFAULT_VGL_DOCDIR} CACHE PATH
	"Directory into which docs should be installed (default: ${DEFAULT_VGL_DOCDIR})")

message(STATUS "VGL_BINDIR = ${VGL_BINDIR}")
message(STATUS "VGL_DOCDIR = ${VGL_DOCDIR}")

include_directories(${CMAKE_SOURCE_DIR}/include)

if(MSVC)
	# Use the static C library for all build types
	foreach(var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if(${var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
		endif()
	endforeach()

	add_definitions(-wd4996 -D_CRT_SECURE_NO_DEPRECATE)

	# Don't auto-generate manifests
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

  # Avoid linker warning when doing Debug build if dependent libraries are
  # linked with the Release version of the static C library.
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:LIBCMT")
endif()

# Detect CPU type and word size
math(EXPR BITS "${CMAKE_SIZEOF_VOID_P} * 8")
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64"
	OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "amd64"
	OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64"
	OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[0-9]86")
	if(BITS EQUAL 64)
		set(CPU_TYPE x86_64)
	else()
		set(CPU_TYPE i386)
	endif()
else()
	string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CPU_TYPE)
endif()
message(STATUS "${BITS}-bit build (${CPU_TYPE})")

# This ensures that we don't depend on libstdc++ or libgcc
if(CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE AND NOT CYGWIN)
	set(DEFAULT_VGL_BUILDSTATIC 1)
	if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		set(DEFAULT_VGL_BUILDSTATIC 0)
	endif()
	option(VGL_BUILDSTATIC
		"Link statically against libgcc and libstdc++, if possible"
		${DEFAULT_VGL_BUILDSTATIC})
	if(VGL_BUILDSTATIC)
		# For some reason, simply passing ${CMAKE_CXX_FLAGS} to the compiler in
		# execute_process() doesn't work.  Grrr...
		execute_process(COMMAND ${CMAKE_CXX_COMPILER} -m${BITS}
			--print-file-name=libstdc++.a OUTPUT_VARIABLE LIBSTDCPLUSPLUS
			RESULT_VARIABLE RESULT)
		string(REGEX REPLACE "\n" "" LIBSTDCPLUSPLUS ${LIBSTDCPLUSPLUS})
		if(RESULT MATCHES 0 AND LIBSTDCPLUSPLUS)
			message(STATUS "Linking with static libstdc++:\n   ${LIBSTDCPLUSPLUS}")
			file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/staticlib)
			execute_process(COMMAND ${CMAKE_COMMAND} -E remove
				${CMAKE_BINARY_DIR}/staticlib/libstdc++.a)
			execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
				${LIBSTDCPLUSPLUS} ${CMAKE_BINARY_DIR}/staticlib/libstdc++.a)
			set(CMAKE_EXE_LINKER_FLAGS
				"${CMAKE_EXE_LINKER_FLAGS} -L${CMAKE_BINARY_DIR}/staticlib")
			set(CMAKE_SHARED_LINKER_FLAGS
				"${CMAKE_SHARED_LINKER_FLAGS} -L${CMAKE_BINARY_DIR}/staticlib")
		else()
			message(WARNING "Cannot find static libstdc++.  VirtualGL will depend on dynamic libstdc++.")
		endif()
		add_definitions(-static-libgcc)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
		set(CMAKE_SHARED_LINKER_FLAGS
			"${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
	endif()
endif()

# Don't build RPATH into libraries generated in the build directory
set(CMAKE_SKIP_BUILD_RPATH 1)

if(CMAKE_COMPILER_IS_GNUCC)
	add_definitions(-D_GNU_SOURCE)
	if(NOT APPLE)
		set(MAPFLAG "-Wl,--version-script,")
	endif()
	# Use the maximum optimization level for release builds
	foreach(var CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if(${var} MATCHES "-O2")
			string(REGEX REPLACE "-O2" "-O3" ${var} "${${var}}")
		endif()
	endforeach()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mt")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -mt -lc")
	set(MAPFLAG "-Wl,-M,")
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)
		set(CMAKE_SHARED_LINKER_FLAGS
			"-z direct -lCrun -z nodirect ${CMAKE_SHARED_LINKER_FLAGS}")
		set(MAPFLAG "-M")
	endif()
endif()

if("${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)
	add_definitions(-mt -norunpath)
	# Use the maximum optimization level for release builds
	foreach(var CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if(${var} MATCHES "-xO3")
			string(REGEX REPLACE "-xO3" "-xO5" ${var} "${${var}}")
		endif()
		if(${var} MATCHES "-xO2")
			string(REGEX REPLACE "-xO2" "-xO5" ${var} "${${var}}")
		endif()
	endforeach()
endif()

option(VGL_USESSL
	"Enable SSL (Secure Sockets Layer) encryption feature in the VGL Transport"
	OFF)
if(VGL_USESSL)
	include(FindOpenSSL)
	if(NOT OPENSSL_FOUND)
		message(STATUS "OpenSSL not found.  Disabling SSL support.")
	else()
		add_definitions(-DUSESSL)
		include_directories(${OPENSSL_INCLUDE_DIR})
	endif()
	set(VGL_USESSL 1)
else()
	set(VGL_USESSL 0)
endif()

if(WIN32)

set(VGL_USEXV 0)
add_subdirectory(util)
add_subdirectory(wgldemos)
add_subdirectory(diags)

else()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_LIBRARY_PATH /usr/lib/${CPU_TYPE}-linux-gnu;/usr/lib${BITS};/usr/lib)
endif()
include(FindX11)
include(FindOpenGL)

# Ensure that we build and link against the X11 version of OpenGL on OS X
if(APPLE)
	option(VGL_OSXOGLFIX "Adds Linker arguments needed to correctly select the X11 OpenGL Library" TRUE)
	if(VGL_OSXOGLFIX)
		set(CMAKE_EXE_LINKER_FLAGS
			"${CMAKE_EXE_LINKER_FLAGS} -Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib")
		set(CMAKE_SHARED_LINKER_FLAGS
			"${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib")
	endif()
	set(X11_X11_INCLUDE_PATH /usr/X11R6/include)
	set(OPENGL_gl_LIBRARY /usr/X11R6/lib/libGL.dylib)
	set(OPENGL_glu_LIBRARY /usr/X11R6/lib/libGLU.dylib)
endif()

# Not all OpenGL installations include all of the prototypes and constants in
# glx.h and glxext.h that are necessary to build VirtualGL.  If you know that
# yours does, then you can set VGL_SYSTEMGLX.
set(VGL_DEFAULTSYSTEMGLX 0)
option(VGL_SYSTEMGLX
	"Compile against system-supplied versions of glx.h and glxext.h instead of the in-tree versions."
	${VGL_DEFAULTSYSTEMGLX})
if(VGL_SYSTEMGLX)
	add_definitions(-DSYSGLXHEADERS)
endif()

include_directories(${X11_X11_INCLUDE_PATH})

option(VGL_USEXV "Enable X Video support" TRUE)

if(NOT X11_Xv_INCLUDE_PATH OR NOT X11_Xv_LIB)
	set(VGL_USEXV 0)
endif()

if(VGL_USEXV)
	message(STATUS "Enabling X Video support")
	add_definitions(-DUSEXV)
	include_directories(${X11_Xv_INCLUDE_PATH})
else()
	message(STATUS "X Video not available")
endif()

include(cmakescripts/FindTurboJPEG.cmake)

if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(LIBDL dl)
endif()


#
# Targets
#

set(DEFAULT_VGL_BUILDSERVER 0)
if(NOT APPLE AND NOT CYGWIN)
	set(DEFAULT_VGL_BUILDSERVER 1)
endif()
option(VGL_BUILDSERVER "Build VirtualGL server components"
	${DEFAULT_VGL_BUILDSERVER})

if(VGL_BUILDSERVER)
	message(STATUS "Building VirtualGL server components")

	if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/opt/VirtualGL")
		set(DEFAULT_VGL_FAKELIBDIR ${CMAKE_INSTALL_PREFIX}/fakelib${BITS})
		set(DEFAULT_VGL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib${BITS})
	else()
		set(DEFAULT_VGL_FAKELIBDIR ${CMAKE_INSTALL_PREFIX}/fakelib)
		set(DEFAULT_VGL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
	endif()
	set(DEFAULT_VGL_INCDIR ${CMAKE_INSTALL_PREFIX}/include)

	set(VGL_FAKELIBDIR ${DEFAULT_VGL_FAKELIBDIR} CACHE PATH
		"Directory into which the libGL symlinks for Chromium should be installed (default: ${DEFAULT_VGL_FAKELIBDIR})")
	set(VGL_INCDIR ${DEFAULT_VGL_INCDIR} CACHE PATH
		"Directory into which the VirtualGL transport API headers should be installed(default: ${DEFAULT_VGL_INCDIR})")
	set(VGL_LIBDIR ${DEFAULT_VGL_LIBDIR} CACHE PATH
		"Directory into which the VirtualGL faker libraries should be installed (default: ${DEFAULT_VGL_LIBDIR})")

	message(STATUS "VGL_FAKELIBDIR = ${VGL_FAKELIBDIR}")
	message(STATUS "VGL_INCDIR = ${VGL_INCDIR}")
	message(STATUS "VGL_LIBDIR = ${VGL_LIBDIR}")

	if("${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)
		add_definitions(-KPIC)
	elseif(CMAKE_COMPILER_IS_GNUCC)
		add_definitions(-fPIC)
	endif()
endif()

set(FBXLIB fbx)
if(VGL_USEXV)
	set(FBXLIB ${FBXLIB};fbxv)
endif()

add_subdirectory(util)
add_subdirectory(common)
if(VGL_BUILDSERVER)
add_subdirectory(server)
endif()
add_subdirectory(client)
add_subdirectory(glxdemos)
add_subdirectory(diags)
add_subdirectory(doc)

endif() # WIN32

#
# Installation and packaging
#

include(cmakescripts/BuildPackages.cmake)

configure_file("${CMAKE_SOURCE_DIR}/cmakescripts/cmake_uninstall.cmake.in"
  "cmake_uninstall.cmake" IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P cmake_uninstall.cmake)
