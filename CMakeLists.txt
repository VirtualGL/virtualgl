###############################################################################
# SETUP
###############################################################################

cmake_minimum_required(VERSION 2.8.12)
# Shut up CMake 3.1 and later
if(POLICY CMP0053)
	cmake_policy(SET CMP0053 NEW)
endif()

if(WIN32)
	project(VirtualGL-Utils)
else()
	project(VirtualGL)
endif()
string(TOLOWER ${CMAKE_PROJECT_NAME} CMAKE_PROJECT_NAME_LC)
set(VERSION 3.1)
set(DOCVERSION 3.1)

macro(boolean_number var)
	if(${var})
		set(${var} 1 ${ARGN})
	else()
		set(${var} 0 ${ARGN})
	endif()
endmacro()

macro(report_option var desc)
	if(${var})
		message(STATUS "${desc} enabled (${var} = ${${var}})")
	else()
		message(STATUS "${desc} disabled (${var} = ${${var}})")
	endif()
endmacro()

if(UNIX AND NOT APPLE AND NOT CYGWIN)
	option(VGL_BUILDSERVER "Build ${CMAKE_PROJECT_NAME} server components" TRUE)
	boolean_number(VGL_BUILDSERVER)
	report_option(VGL_BUILDSERVER "VirtualGL server components")
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

string(TIMESTAMP DEFAULT_BUILD "%Y%m%d")
set(BUILD ${DEFAULT_BUILD} CACHE STRING "Build string (default: ${DEFAULT_BUILD})")

message(STATUS "VERSION = ${VERSION}, BUILD = ${BUILD}")
add_definitions(-D__VERSION="${VERSION}" -D__BUILD="${BUILD}"
	-D__APPNAME="${CMAKE_PROJECT_NAME}")

# Detect CPU type and word size
math(EXPR BITS "${CMAKE_SIZEOF_VOID_P} * 8")
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CMAKE_SYSTEM_PROCESSOR_LC)
if(CMAKE_SYSTEM_PROCESSOR_LC MATCHES "x86_64" OR
	CMAKE_SYSTEM_PROCESSOR_LC MATCHES "amd64" OR
	CMAKE_SYSTEM_PROCESSOR_LC MATCHES "i[0-9]86" OR
	CMAKE_SYSTEM_PROCESSOR_LC MATCHES "x86" OR
	CMAKE_SYSTEM_PROCESSOR_LC MATCHES "ia32")
	if(BITS EQUAL 64)
		set(CPU_TYPE x86_64)
	else()
		set(CPU_TYPE i386)
	endif()
	if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL ${CPU_TYPE})
		set(CMAKE_SYSTEM_PROCESSOR ${CPU_TYPE})
	endif()
elseif(CMAKE_SYSTEM_PROCESSOR_LC STREQUAL "aarch64" OR
	CMAKE_SYSTEM_PROCESSOR_LC MATCHES "^arm")
	if(BITS EQUAL 64)
		set(CPU_TYPE arm64)
	else()
		set(CPU_TYPE arm)
	endif()
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64" AND BITS EQUAL 32)
	set(CPU_TYPE ppc)
	set(CMAKE_SYSTEM_PROCESSOR ${CPU_TYPE})
else()
	set(CPU_TYPE ${CMAKE_SYSTEM_PROCESSOR_LC})
endif()
message(STATUS "${BITS}-bit build (${CPU_TYPE})")
add_definitions(-D__BITS=${BITS})

if(UNIX)
	set(CMAKE_INSTALL_DEFAULT_PREFIX /opt/${CMAKE_PROJECT_NAME})
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_DEFAULT_PREFIX}" CACHE PATH
			"Directory into which to install ${CMAKE_PROJECT_NAME} (default: ${CMAKE_INSTALL_DEFAULT_PREFIX})"
			FORCE)
	endif()
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(GNUCC 1)
else()
	set(GNUCC 0)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(GNUCXX 1)
else()
	set(GNUCXX 0)
endif()

# When the prefix is /opt/${CMAKE_PROJECT_NAME}, we assume that an "official"
# build is being created, and thus we install things into specific locations.

if((UNIX AND CMAKE_INSTALL_PREFIX STREQUAL "${CMAKE_INSTALL_DEFAULT_PREFIX}")
	OR WIN32)
	set(CMAKE_INSTALL_DEFAULT_DATAROOTDIR "")
	set(CMAKE_INSTALL_DEFAULT_DOCDIR "<CMAKE_INSTALL_DATAROOTDIR>/doc")
	if(UNIX)
		set(CMAKE_INSTALL_DEFAULT_LIBDIR "lib${BITS}")
	endif()
endif()
if(WIN32)
	set(CMAKE_INSTALL_DEFAULT_BINDIR ".")
	set(CMAKE_INSTALL_DEFAULT_DOCDIR ".")
endif()

include(cmakescripts/GNUInstallDirs.cmake)

macro(report_directory var)
	if(CMAKE_INSTALL_${var} STREQUAL CMAKE_INSTALL_FULL_${var})
		message(STATUS "CMAKE_INSTALL_${var} = ${CMAKE_INSTALL_${var}}")
	else()
		message(STATUS "CMAKE_INSTALL_${var} = ${CMAKE_INSTALL_${var}} (${CMAKE_INSTALL_FULL_${var}})")
	endif()
	mark_as_advanced(CLEAR CMAKE_INSTALL_${var})
endmacro()

set(DIRLIST "BINDIR;DATAROOTDIR;DOCDIR")
if(VGL_BUILDSERVER)
	list(APPEND DIRLIST "INCLUDEDIR;LIBDIR")
endif()
foreach(dir ${DIRLIST})
	report_directory(${dir})
endforeach()

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

# This ensures that we don't depend on libstdc++ or libgcc
if(GNUCXX AND NOT APPLE AND NOT CYGWIN)
	set(DEFAULT_VGL_BUILDSTATIC 1)
	option(VGL_BUILDSTATIC
		"Link statically against libgcc and libstdc++, if possible"
		${DEFAULT_VGL_BUILDSTATIC})
	if(VGL_BUILDSTATIC)
		string(REGEX REPLACE " " ";" CMAKE_CXX_FLAGS_SEMICOLON
			"${CMAKE_CXX_FLAGS}")
		execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS_SEMICOLON}
			--print-file-name=libstdc++.a OUTPUT_VARIABLE LIBSTDCPLUSPLUS
			RESULT_VARIABLE RESULT)
		string(REGEX REPLACE "\n" "" LIBSTDCPLUSPLUS ${LIBSTDCPLUSPLUS})
		if(RESULT MATCHES 0 AND LIBSTDCPLUSPLUS AND
			NOT LIBSTDCPLUSPLUS STREQUAL "libstdc++.a")
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
		if(CMAKE_COMPILER_IS_GNUCXX)
			add_definitions(-static-libgcc)
		endif()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
		set(CMAKE_SHARED_LINKER_FLAGS
			"${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
	endif()
endif()

# Don't build RPATH into libraries generated in the build directory
set(CMAKE_SKIP_BUILD_RPATH 1)

if(GNUCC)
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
	set(MINUSZ "-Wl,-z,")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc")
	set(MAPFLAG "-Wl,-M,")
	if("${CMAKE_C_COMPILER_ID} ${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro)
		add_definitions(-mt -norunpath)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mt")
		set(CMAKE_SHARED_LINKER_FLAGS
			"-z direct -lCrun -z nodirect -mt ${CMAKE_SHARED_LINKER_FLAGS}")
		set(MAPFLAG "-M")
		set(MINUSZ "-z")

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
endif()

include(CheckCSourceCompiles)
if(MSVC)
	set(INLINE_OPTIONS "__inline;inline")
else()
	set(INLINE_OPTIONS "__inline__;inline")
endif()
option(VGL_FORCEINLINE "Force function inlining" TRUE)
boolean_number(VGL_FORCEINLINE)
if(VGL_FORCEINLINE)
	if(MSVC)
		list(INSERT INLINE_OPTIONS 0 "__forceinline")
	else()
		list(INSERT INLINE_OPTIONS 0 "inline __attribute__((always_inline))")
		list(INSERT INLINE_OPTIONS 0 "__inline__ __attribute__((always_inline))")
	endif()
endif()
foreach(inline ${INLINE_OPTIONS})
	check_c_source_compiles("${inline} static int foo(void) { return 0; } int main(void) { return foo(); }"
		INLINE_WORKS)
	if(INLINE_WORKS)
		set(INLINE ${inline})
		break()
	endif()
endforeach()
if(NOT INLINE_WORKS)
	message(FATAL_ERROR "Could not determine how to inline functions.")
endif()
message(STATUS "INLINE = ${INLINE} (VGL_FORCEINLINE = ${VGL_FORCEINLINE})")
configure_file(include/vglinline.h.in include/vglinline.h)
include_directories(${CMAKE_BINARY_DIR}/include)

option(VGL_USEHELGRIND
	"Include code annotations to suppress helgrind warning messages about innocuous VirtualGL behaviors."
	OFF)
if(VGL_USEHELGRIND)
	add_definitions(-DUSEHELGRIND)
	boolean_number(VGL_USEHELGRIND)
	report_option(VGL_USEHELGRIND "Helgrind annotations")
endif()

if(WIN32)

set(VGL_USEXV 0)
add_subdirectory(util)
add_subdirectory(wgldemos)
add_subdirectory(diags)

else()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(CMAKE_LIBRARY_PATH /usr/lib/${CPU_TYPE}-linux-gnu;/usr/lib${BITS};/usr/lib)
endif()
include(FindX11)
# Clarify that we need to link with the legacy OpenGL library (libGL) rather
# than the GLVND OpenGL library.  This shuts up CMake 3.11 and later.
set(OpenGL_GL_PREFERENCE LEGACY)
include(FindOpenGL)

# Ensure that we build and link against the X11 version of OpenGL on OS X/macOS
if(APPLE)
	set(X11_X11_INCLUDE_PATH /opt/X11/include)
	set(OPENGL_gl_LIBRARY /opt/X11/lib/libGL.dylib)
	set(OPENGL_glu_LIBRARY /opt/X11/lib/libGLU.dylib)
endif()

include_directories(${X11_X11_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})

option(VGL_USEXV "Enable X Video support" TRUE)
boolean_number(VGL_USEXV)

if(NOT X11_Xv_INCLUDE_PATH OR NOT X11_Xv_LIB)
	set(VGL_USEXV 0)
endif()
report_option(VGL_USEXV "X Video support")

if(VGL_USEXV)
	add_definitions(-DUSEXV)
	include_directories(${X11_Xv_INCLUDE_PATH})
endif()

if(NOT WIN32)
	include(cmakescripts/FindTurboJPEG.cmake)
endif()

if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(LIBDL dl)
endif()

endif() # WIN32

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UC)

set(EFFECTIVE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UC}}")
message(STATUS "Compiler flags = ${EFFECTIVE_C_FLAGS}")

set(EFFECTIVE_LD_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UC}}")
message(STATUS "Linker flags = ${EFFECTIVE_LD_FLAGS}")


###############################################################################
# TARGETS
###############################################################################

if(NOT WIN32)

if(VGL_BUILDSERVER)
	set(DEFAULT_VGL_FAKER_NAME vglfaker)
	set(VGL_FAKER_NAME ${DEFAULT_VGL_FAKER_NAME} CACHE STRING
		"Base name for VirtualGL's GLX/OpenGL/X11/XCB interposer library (default: ${DEFAULT_VGL_FAKER_NAME})")

	set(DEFAULT_VGL_DLFAKER_NAME dlfaker)
	set(VGL_DLFAKER_NAME ${DEFAULT_VGL_DLFAKER_NAME} CACHE STRING
		"Base name for VirtualGL's dlopen() interposer library (default: ${DEFAULT_VGL_DLFAKER_NAME})")

	set(DEFAULT_VGL_GEFAKER_NAME gefaker)
	set(VGL_GEFAKER_NAME ${DEFAULT_VGL_GEFAKER_NAME} CACHE STRING
		"Base name for VirtualGL's getenv() interposer library (default: ${DEFAULT_VGL_GEFAKER_NAME})")

	set(CMAKE_POSITION_INDEPENDENT_CODE 1)
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
add_subdirectory(demos)
add_subdirectory(diags)
add_subdirectory(doc)

else()

install(FILES LICENSE.txt LGPL.txt DESTINATION ${CMAKE_INSTALL_DOCDIR})

endif() # WIN32


###############################################################################
# INSTALLATION AND PACKAGING
###############################################################################

include(cmakescripts/BuildPackages.cmake)

configure_file("${CMAKE_SOURCE_DIR}/cmakescripts/cmake_uninstall.cmake.in"
	"cmake_uninstall.cmake" IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P cmake_uninstall.cmake)
