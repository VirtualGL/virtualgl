if(WIN32)
	set(FBXLIB fbx-x11)
else()
	set(FBXLIB fbx)
endif()
if(VGL_USEXV)
	set(FBXLIB ${FBXLIB};fbxv)
endif()

configure_file(fakerconfig.h.in fakerconfig.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})


#
# Server
#

if(VGL_BUILDSERVER)

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
		pbpm.cpp glxvisual.cpp rrblitter.cpp rrdisplayclient.cpp rrplugin.cpp
		fakerconfig.cpp)
	if(VGL_USEXV)
		set(RRFAKER_SOURCES ${RRFAKER_SOURCES};rrxvtrans.cpp)
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
	target_link_libraries(rrfaker rrutil ${FBXLIB} ${TJPEG_LIBRARY} rrsocket m
		${OPENGL_gl_LIBRARY} dl)
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

endif()


#
# Client
#

if(WIN32)
	add_executable(vglclient vglclient.cpp rrcwin.cpp rrdisplayserver.cpp
		rrframe.cpp rrglframe.cpp xdk-sym.cpp)
	target_link_libraries(vglclient psapi)
else()
	add_executable(vglclient vglclient.cpp rrcwin.cpp rrdisplayserver.cpp)
	target_link_libraries(vglclient ${OPENGL_gl_LIBRARY})
endif()
target_link_libraries(vglclient rrutil ${FBXLIB} rrsocket ${TJPEG_LIBRARY})
install(TARGETS vglclient DESTINATION ${VGL_BINDIR})

if(WIN32)
	add_custom_target(vglconnect.bat ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/vglconnect.bat
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/vglconnect.bat
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vglconnect.bat)
	install(PROGRAMS vglconnect.bat DESTINATION ${VGL_BINDIR})
else()
	add_custom_target(vglconnect ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/vglconnect
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglconnect
		COMMAND chmod u+rx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglconnect
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vglconnect)
	install(PROGRAMS vglconnect DESTINATION ${VGL_BINDIR})
endif()


#
# Unit tests
#

if(WIN32)
	add_executable(rrblitterut rrblitterut.cpp fakerconfig.cpp rrblitter.cpp
		rrframe.cpp)
else()
	add_executable(rrblitterut rrblitterut.cpp fakerconfig.cpp rrblitter.cpp)
endif()
target_link_libraries(rrblitterut rrutil ${FBXLIB} ${TJPEG_LIBRARY})

add_executable(rrdisplayclientut rrdisplayclientut.cpp rrdisplayclient.cpp
	fakerconfig.cpp)
target_link_libraries(rrdisplayclientut rrutil ${FBXLIB} rrsocket
	${TJPEG_LIBRARY})

add_executable(rrdisplayserverut rrdisplayserverut.cpp rrdisplayclient.cpp
	fakerconfig.cpp)
target_link_libraries(rrdisplayserverut rrutil ${FBXLIB} rrsocket
	${TJPEG_LIBRARY})

if(WIN32)
	add_executable(rrframeut rrframeut.cpp rrframe.cpp rrglframe.cpp xdk-sym.cpp)
else()
	add_executable(rrframeut rrframeut.cpp)
endif()
target_link_libraries(rrframeut rrutil ${FBXLIB} ${TJPEG_LIBRARY}
	${OPENGL_gl_LIBRARY})

if(VGL_BUILDSERVER)
	add_executable(dlfakerut dlfakerut.c)
	target_link_libraries(dlfakerut ${X11_X11_LIB} dl)

	add_library(GLdlfakerut SHARED libGLdlfakerut.c)

	add_executable(rrfakerut rrfakerut.cpp)
	target_link_libraries(rrfakerut "-z now ${OPENGL_gl_LIBRARY}"
		${OPENGL_glu_LIBRARY} "-z now ${X11_X11_LIB}" dl pthread)

	add_library(vgltrans_test SHARED testplugin.cpp rrdisplayclient.cpp)
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

	add_library(vgltrans_test2 SHARED testplugin2.cpp rrblitter.cpp)
	set_target_properties(vgltrans_test2 PROPERTIES LINK_FLAGS "-z defs")
	target_link_libraries(vgltrans_test2 rrutil ${FBXLIB} ${TJPEG_LIBRARY})
	if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_COMPILER_IS_GNUCXX)
		target_link_libraries(vgltrans_test2 stdc++)
	endif()

endif()
