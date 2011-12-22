add_subdirectory(fltk)


#
# Server
#

configure_file(fakerconfig.h.in fakerconfig.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
	../common)

set(FBXFAKERLIB fbx-faker)
if(VGL_USEXV)
	set(FBXFAKERLIB ${FBXFAKERLIB};fbxv)
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

set(RRFAKER_SOURCES faker.cpp faker-sym.cpp pbdrawable.cpp pbwin.cpp
	pbpm.cpp glxvisual.cpp x11trans.cpp vgltransconn.cpp transplugin.cpp
	fakerconfig.cpp)
if(VGL_USEXV)
	set(RRFAKER_SOURCES ${RRFAKER_SOURCES};xvtrans.cpp)
endif()
add_library(rrfaker SHARED ${RRFAKER_SOURCES})
if(MAPFLAG)
	set_source_files_properties(faker-sym.cpp PROPERTIES
		OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/faker-mapfile)
	set_target_properties(rrfaker PROPERTIES
		LINK_FLAGS "-z defs ${MAPFLAG}${CMAKE_CURRENT_BINARY_DIR}/faker-mapfile")
else()
	set_target_properties(rrfaker PROPERTIES LINK_FLAGS "-z defs")
endif()
target_link_libraries(rrfaker rrutil ${FBXFAKERLIB} ${TJPEG_LIBRARY} rrsocket
	m ${OPENGL_gl_LIBRARY} dl)
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_COMPILER_IS_GNUCXX)
	# For some reason, on SunOS/GCC CMake tries to use the C compiler to link
	# C++ shared libs.  Grrr...
	target_link_libraries(rrfaker stdc++)
endif()
install(TARGETS rrfaker DESTINATION ${VGL_LIBDIR})
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
	\$ENV{DESTDIR}${VGL_FAKELIBDIR})")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
	${VGL_LIBDIR}/librrfaker.so
	\$ENV{DESTDIR}${VGL_FAKELIBDIR}/libGL.so)")

add_library(dlfaker SHARED dlfaker.c)
target_link_libraries(dlfaker dl)
install(TARGETS dlfaker DESTINATION ${VGL_LIBDIR})

add_library(gefaker SHARED gefaker.c)
set_target_properties(gefaker PROPERTIES LINK_FLAGS "-z defs")
target_link_libraries(gefaker dl)
install(TARGETS gefaker DESTINATION ${VGL_LIBDIR})

set(HEADERS rr.h rrtransport.h)
install(FILES ${HEADERS} DESTINATION ${VGL_INCDIR})

add_executable(vglconfig vglconfig.cpp fakerconfig.cpp)
target_link_libraries(vglconfig ${X11_X11_LIB} fltk_static pthread rrutil)
if(VGL_USEXV)
	target_link_libraries(vglconfig ${X11_Xv_LIB})
endif()
install(TARGETS vglconfig DESTINATION ${VGL_BINDIR})

install(PROGRAMS vglgenkey vgllogin vglserver_config DESTINATION
	${VGL_BINDIR})

add_custom_target(vglrun ALL
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_SOURCE_DIR}/vglrun
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglrun
	COMMAND chmod u+rx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglrun
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vglrun)
install(PROGRAMS vglrun DESTINATION ${VGL_BINDIR})


#
# Unit tests
#

if(WIN32)
	add_executable(x11transut x11transut.cpp fakerconfig.cpp x11trans.cpp
		frame.cpp)
else()
	add_executable(x11transut x11transut.cpp fakerconfig.cpp x11trans.cpp)
endif()
target_link_libraries(x11transut rrutil ${FBXLIB} ${TJPEG_LIBRARY})

add_executable(vgltransconnut vgltransconnut.cpp vgltransconn.cpp
	fakerconfig.cpp)
target_link_libraries(vgltransconnut rrutil ${FBXLIB} rrsocket
	${TJPEG_LIBRARY})

add_executable(vgltransreceiverut vgltransreceiverut.cpp vgltransconn.cpp
	fakerconfig.cpp)
target_link_libraries(vgltransreceiverut rrutil ${FBXLIB} rrsocket
	${TJPEG_LIBRARY})

add_executable(dlfakerut dlfakerut.c)
target_link_libraries(dlfakerut ${X11_X11_LIB} dl)

add_library(GLdlfakerut SHARED libGLdlfakerut.c)

add_executable(fakerut fakerut.cpp)
target_link_libraries(fakerut "-z now ${OPENGL_gl_LIBRARY}"
	${OPENGL_glu_LIBRARY} "-z now ${X11_X11_LIB}" dl pthread)

add_library(vgltrans_test SHARED testplugin.cpp vgltransconn.cpp)
if(VGL_USESSL AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Work around this issue:
	# http://rt.openssl.org/Ticket/Display.html?user=guest&pass=guest&id=1521
	set_target_properties(vgltrans_test PROPERTIES LINK_FLAGS "-z defs -Wl,-Bsymbolic")
else()
	set_target_properties(vgltrans_test PROPERTIES LINK_FLAGS "-z defs")
endif()
target_link_libraries(vgltrans_test rrutil ${TJPEG_LIBRARY} rrsocket)
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(vgltrans_test stdc++)
endif()

add_library(vgltrans_test2 SHARED testplugin2.cpp x11trans.cpp)
target_link_libraries(vgltrans_test2 rrutil ${FBXFAKERLIB} ${TJPEG_LIBRARY})
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(vgltrans_test2 stdc++)
endif()
