#
# Setup
#

cmake_minimum_required(VERSION 2.6)

include(CheckCSourceCompiles)

if(UNIX)
	set(CMAKE_INSTALL_PREFIX /opt/VirtualGL CACHE PATH
		"Install path prefix, prepended onto install directories.")
endif()

project(VirtualGL)
set(VERSION 2.2.80)

if(UNIX)
	execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
	string(REGEX REPLACE "\n" "" BUILD ${BUILD})
elseif(WIN32 AND NOT MINGW)
	execute_process(COMMAND "${CMAKE_SOURCE_DIR}/cmakescripts/getdate.bat"
		OUTPUT_VARIABLE BUILD)
	string(REGEX REPLACE "\n" "" BUILD ${BUILD})
else()
	message(FATAL_ERROR "Platform not supported.")
endif()

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

message(STATUS "Install prefix = ${CMAKE_INSTALL_PREFIX}")

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
	add_definitions(-wd4996 -D_CRT_SECURE_NO_DEPRECATE -DXDK)
	include_directories(${CMAKE_SOURCE_DIR}/windows/include)
	include_directories(${CMAKE_SOURCE_DIR}/windows/include/xdk)

	# Don't auto-generate manifests
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif()

# Detect whether compiler is 64-bit
if(CMAKE_SIZEOF_VOID_P MATCHES 8)
	set(64BIT 1)
	message(STATUS "64-bit build")
else()
	message(STATUS "32-bit build")
endif()

# This ensures that we don't depend on libstdc++ or libgcc
if(CMAKE_COMPILER_IS_GNUCXX)
	option(VGL_BUILDSTATIC
		"Link statically against libgcc and libstdc++, if possible" ON)
	if(VGL_BUILDSTATIC)
		# For some reason, simply passing ${CMAKE_CXX_FLAGS} to the compiler in
		# execute_process() doesn't work.  Grrr...
		if(64BIT)
			execute_process(COMMAND ${CMAKE_CXX_COMPILER} -m64
				--print-file-name=libstdc++.a OUTPUT_VARIABLE LIBSTDCPLUSPLUS
				RESULT_VARIABLE RESULT)
		else()
			execute_process(COMMAND ${CMAKE_CXX_COMPILER} -m32
				--print-file-name=libstdc++.a OUTPUT_VARIABLE LIBSTDCPLUSPLUS
				RESULT_VARIABLE RESULT)
		endif()
		string(REGEX REPLACE "\n" "" LIBSTDCPLUSPLUS ${LIBSTDCPLUSPLUS})
		if(RESULT MATCHES 0 AND LIBSTDCPLUSPLUS)
			message(STATUS "Linking with static libstdc++:\n   ${LIBSTDCPLUSPLUS}")
			file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/staticlib)
			execute_process(COMMAND ${CMAKE_COMMAND} -E remove
				${CMAKE_BINARY_DIR}/staticlib/libstdc++.a)
			execute_process(COMMAND ${CMAKE_COMMAND} -E	create_symlink
				${LIBSTDCPLUSPLUS} ${CMAKE_BINARY_DIR}/staticlib/libstdc++.a)
			set(CMAKE_EXE_LINKER_FLAGS
				"${CMAKE_EXE_LINKER_FLAGS} -L${CMAKE_BINARY_DIR}/staticlib")
			set(CMAKE_SHARED_LINKER_FLAGS
				"${CMAKE_SHARED_LINKER_FLAGS} -L${CMAKE_BINARY_DIR}/staticlib")
		else()
			message(WARNING Cannot find static libstdc++.  VirtualGL will depend on dynamic libstdc++.)
		endif()
		add_definitions(-static-libgcc)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
		set(CMAKE_SHARED_LINKER_FLAGS
			"${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
	endif()
endif()

if(CMAKE_COMPILER_IS_GNUCC)
	add_definitions(-D_GNU_SOURCE)
	if(NOT APPLE)
		set(MAPFLAG "-Wl,--version-script,")
	endif()
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
endif()

if(UNIX)
	include(FindX11)
	include(FindOpenGL)
elseif(WIN32)
	set(X11_X11_LIB xlib.lib)
	set(X11_Xext_LIB hclshm.lib)
	set(OPENGL_gl_LIBRARY hclglx.lib)
	set(OPENGL_glu_LIBRARY hclglu.lib)
endif()

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

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
	set(DEFAULT_VGL_USEMEDIALIB 0)
	if(CMAKE_SYSTEM_NAME MATCHES "sparc")
		set(DEFAULT_VGL_USEMEDIALIB 1)
	endif()
	option(VGL_USEMEDIALIB
		"Use mediaLib to perform JPEG compression/decompression and YUV encoding/decoding rather than libjpeg-turbo.  If this option is enabled, then VirtualGL will also use mediaLib to perform gamma correction."
		${DEFAULT_VGL_USEMEDIALIB})

	if(VGL_USEMEDIALIB)
		message(STATUS "Using mediaLib for image encoding/decoding and gamma correction")
		add_definitions(-DUSEMEDIALIB)
		set(TJPEG_LIBRARY tjpeg)
	endif()
endif()
if(NOT VGL_USEMEDIALIB)
	include(cmakescripts/FindTurboJPEG.cmake)
endif()


#
# Targets
#

if(WIN32)
	set(BINDIR .)
else()
	set(BINDIR bin)
endif()

set(DEFAULT_VGL_BUILDSERVER 0)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux"
	OR ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
	set(DEFAULT_VGL_BUILDSERVER 1)
endif()
option(VGL_BUILDSERVER "Build VirtualGL server components"
	${DEFAULT_VGL_BUILDSERVER})

if(VGL_BUILDSERVER)
	message(STATUS "Building VirtualGL server components")
	if("${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)
		add_definitions(-KPIC)
	elseif(CMAKE_COMPILER_IS_GNUCC)
		add_definitions(-fPIC)
	endif()
endif()

add_subdirectory(util)
add_subdirectory(jpeg)
if(VGL_BUILDSERVER)
add_subdirectory(fltk)
endif()
add_subdirectory(rr)
if(WIN32)
add_subdirectory(putty)
add_subdirectory(x11windows)
else()
add_subdirectory(mesademos)
endif()
add_subdirectory(diags)
add_subdirectory(doc)


#
# Installation and packaging
#

include(cmakescripts/BuildPackages.cmake)

configure_file("${CMAKE_SOURCE_DIR}/cmakescripts/cmake_uninstall.cmake.in"
  "cmake_uninstall.cmake" IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P cmake_uninstall.cmake)
