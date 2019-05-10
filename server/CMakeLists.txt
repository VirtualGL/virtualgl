###############################################################################
# SERVER
###############################################################################

set(VGL_DEFAULTSYSTEMFLTK 0)
option(VGL_SYSTEMFLTK
	"Link against a system-supplied version of FLTK instead of the in-tree version."
	${VGL_DEFAULTSYSTEMFLTK})
if(VGL_SYSTEMFLTK)
	find_package(FLTK)
else()
	message(STATUS "Using in-tree version of FLTK")
	add_subdirectory(fltk)
	set(FLTK_INCLUDE_DIR fltk)
	set(FLTK_BASE_LIBRARY fltk_static)
endif()

configure_file(fakerconfig.h.in fakerconfig.h)
configure_file(vendor.h.in vendor.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
	../common)

set(FBXFAKERLIB fbx-faker)
if(VGL_USEXV)
	set(FBXFAKERLIB ${FBXFAKERLIB};fbxv)
endif()

option(VGL_FAKEXCB "Interpose enough of the XCB API to make Qt 5 work" ON)
if(VGL_FAKEXCB)
	add_definitions(-DFAKEXCB)
	set(FAKER_XCB_SOURCES faker-xcb.cpp XCBConnHash.cpp)
	set(VGL_DEFAULTSYSTEMXCB 0)
	option(VGL_SYSTEMXCB
		"Compile against system-supplied versions of the XCB headers instead of the in-tree versions."
			${VGL_DEFAULTSYSTEMXCB})
	if(VGL_SYSTEMXCB)
		add_definitions(-DSYSXCBHEADERS)
	endif()
endif()

get_directory_property(DEFS_PROP COMPILE_DEFINITIONS)
foreach(def ${DEFS_PROP})
	set(DEFINES ${DEFINES};-D${def})
endforeach()
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/faker-mapfile
	COMMAND ${CMAKE_C_COMPILER} ${DEFINES} -E
		${CMAKE_CURRENT_SOURCE_DIR}/faker-mapfile.c
		> ${CMAKE_CURRENT_BINARY_DIR}/faker-mapfile
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/faker-mapfile.c)

set(FAKER_SOURCES
	ConfigHash.cpp
	ContextHash.cpp
	DisplayHash.cpp
	faker.cpp
	faker-gl.cpp
	faker-glx.cpp
	faker-sym.cpp
	faker-x11.cpp
	${FAKER_XCB_SOURCES}
	fakerconfig.cpp
	GlobalCriticalSection.cpp
	GLXDrawableHash.cpp
	glxvisual.cpp
	PixmapHash.cpp
	ReverseConfigHash.cpp
	TransPlugin.cpp
	VirtualDrawable.cpp
	VirtualPixmap.cpp
	VirtualWin.cpp
	VisualHash.cpp
	WindowHash.cpp
	X11Trans.cpp
	vglconfigLauncher.cpp
	VGLTrans.cpp)
if(VGL_USEXV)
	set(FAKER_SOURCES ${FAKER_SOURCES};XVTrans.cpp)
endif()

foreach(fakerlib ${VGL_FAKER_NAME} ${VGL_FAKER_NAME}-nodl)
	add_library(${fakerlib} SHARED ${FAKER_SOURCES})
	if(${fakerlib} STREQUAL ${VGL_FAKER_NAME} AND
		CMAKE_SYSTEM_NAME MATCHES "Linux")
		# GCC 4.6 and later on Linux apparently passes --as-needed to the linker,
		# and since libvglfaker.so loads OpenGL/GLX functions indirectly (with
		# dlopen()/dlsym()), --as-needed prevents the linker from adding a
		# DT_NEEDED tag for libGL to libvglfaker.so.  Without the following fix,
		# when attempting to vglrun applications that themselves indirectly load
		# OpenGL functions, libGL would not be loaded into the process, and VGL's
		# calls to dlsym(RTLD_NEXT, ...) would fail.
		set(NOASNEEDED "-Wl,--no-as-needed")
	endif()
	if(MAPFLAG)
		set_source_files_properties(faker-sym.cpp PROPERTIES
			OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/faker-mapfile)
		set_target_properties(${fakerlib} PROPERTIES
			LINK_FLAGS "${MINUSZ}defs ${MAPFLAG}${CMAKE_CURRENT_BINARY_DIR}/faker-mapfile ${NOASNEEDED}")
	else()
		set_target_properties(${fakerlib} PROPERTIES LINK_FLAGS "${MINUSZ}defs ${NOASNEEDED}")
	endif()
	target_link_libraries(${fakerlib} vglcommon ${FBXFAKERLIB} vglsocket m
		${LIBDL})
	if(${fakerlib} STREQUAL ${VGL_FAKER_NAME})
		target_link_libraries(${fakerlib} ${OPENGL_gl_LIBRARY})
	endif()
	if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND GNUCXX)
		# For some reason, on SunOS/GCC CMake tries to use the C compiler to link
		# C++ shared libs.  Grrr...  (NOTE: This was fixed in CMake v3.4, so we
		# can remove this if we ever require a version later than that.)
		target_link_libraries(${fakerlib} stdc++)
	endif()
	install(TARGETS ${fakerlib} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endforeach()

add_library(${VGL_DLFAKER_NAME} SHARED dlfaker.c)
target_link_libraries(${VGL_DLFAKER_NAME} ${LIBDL})
install(TARGETS ${VGL_DLFAKER_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_library(${VGL_GEFAKER_NAME} SHARED gefaker.c)
set_target_properties(${VGL_GEFAKER_NAME} PROPERTIES LINK_FLAGS "${MINUSZ}defs")
target_link_libraries(${VGL_GEFAKER_NAME} ${LIBDL})
install(TARGETS ${VGL_GEFAKER_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(HEADERS ../common/rr.h rrtransport.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include_directories(SYSTEM ${FLTK_INCLUDE_DIR})
add_executable(vglconfig vglconfig.cpp fakerconfig.cpp)
target_link_libraries(vglconfig ${X11_X11_LIB} ${FLTK_BASE_LIBRARY} vglutil)
if(VGL_USEXV)
	target_link_libraries(vglconfig ${X11_Xv_LIB})
endif()
install(TARGETS vglconfig DESTINATION ${CMAKE_INSTALL_BINDIR})
set_property(SOURCE vglconfig.cpp APPEND_STRING PROPERTY COMPILE_FLAGS
	"-fno-strict-aliasing")

install(PROGRAMS vglgenkey vgllogin vglserver_config DESTINATION
	${CMAKE_INSTALL_BINDIR})

configure_file(vglrun.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglrun @ONLY)
execute_process(COMMAND chmod +x vglrun
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglrun
	DESTINATION ${CMAKE_INSTALL_BINDIR})
configure_file(vglrun.vars.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.vglrun.vars${BITS} @ONLY)
execute_process(COMMAND chmod +x .vglrun.vars${BITS}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.vglrun.vars${BITS}
	DESTINATION ${CMAKE_INSTALL_BINDIR})


###############################################################################
# UNIT TESTS
###############################################################################

add_executable(x11transut x11transut.cpp fakerconfig.cpp X11Trans.cpp)
target_link_libraries(x11transut vglcommon ${FBXLIB} ${TJPEG_LIBRARY})

add_executable(vgltransut vgltransut.cpp VGLTrans.cpp
	fakerconfig.cpp)
target_link_libraries(vgltransut vglcommon ${FBXLIB} vglsocket
	${TJPEG_LIBRARY})

add_executable(dlfakerut dlfakerut.c)
target_link_libraries(dlfakerut ${X11_X11_LIB} ${LIBDL})

add_library(GLdlfakerut SHARED libGLdlfakerut.c)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_library(deepbindtest SHARED libdeepbindtest.c)
	target_link_libraries(deepbindtest ${X11_X11_LIB} ${OPENGL_gl_LIBRARY}
		${LIBDL})
	set_target_properties(deepbindtest PROPERTIES LINK_FLAGS "${MINUSZ}defs")
endif()

add_executable(fakerut fakerut.cpp fakerut-supplement.c)
target_link_libraries(fakerut "${MINUSZ}now ${OPENGL_gl_LIBRARY}"
	${OPENGL_glu_LIBRARY} "${MINUSZ}now ${X11_X11_LIB}" ${LIBDL} vglutil)

add_library(vgltrans_test SHARED testplugin.cpp VGLTrans.cpp)
if(VGL_USESSL AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Work around this issue:
	# http://rt.openssl.org/Ticket/Display.html?user=guest&pass=guest&id=1521
	set_target_properties(vgltrans_test PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic")
endif()
target_link_libraries(vgltrans_test vglcommon ${FBXFAKERLIB} ${TJPEG_LIBRARY}
	vglsocket)
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND GNUCXX)
	target_link_libraries(vgltrans_test stdc++)
endif()

add_library(vgltrans_test2 SHARED testplugin2.cpp X11Trans.cpp)
target_link_libraries(vgltrans_test2 vglcommon ${FBXFAKERLIB} ${TJPEG_LIBRARY})
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND GNUCXX)
	target_link_libraries(vgltrans_test2 stdc++)
endif()

configure_file(servertest.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/servertest
	@ONLY)
execute_process(COMMAND chmod +x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/servertest)
