foreach(program glthreads glthreadsint)
	add_executable(${program} ${program}.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} pthread
		m)
	set_property(TARGET ${program} PROPERTY COMPILE_FLAGS "-DPTHREADS")
endforeach()

foreach(program offset testgl5)
	add_executable(${program} ${program}.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
		${X11_Xext_LIB} m)
endforeach()

add_executable(glxinfo glxinfo.c glinfo_common.c)
target_link_libraries(glxinfo ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
	${OPENGL_glu_LIBRARY} m)
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
	target_link_libraries(glxinfo Xmu)
endif()
install(TARGETS glxinfo DESTINATION ${CMAKE_INSTALL_BINDIR})

if(VGL_EGLBACKEND)
	add_executable(eglinfo eglinfo.c glinfo_common.c)
	target_link_libraries(eglinfo ${OPENGL_gl_LIBRARY} ${OPENGL_egl_LIBRARY})
	install(TARGETS eglinfo DESTINATION ${CMAKE_INSTALL_BINDIR})

	add_executable(eglxinfo eglinfo.c glinfo_common.c)
	target_compile_definitions(eglxinfo PUBLIC -DEGLX)
	target_link_libraries(eglxinfo ${OPENGL_gl_LIBRARY} ${OPENGL_egl_LIBRARY}
		${X11_X11_LIB})
	install(TARGETS eglxinfo DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

set(GLXSPHERES glxspheres)
if(BITS EQUAL 64)
	set(GLXSPHERES glxspheres64)
endif()
add_executable(${GLXSPHERES} glxspheres.c)
target_link_libraries(${GLXSPHERES} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
	${OPENGL_glu_LIBRARY} m)
install(TARGETS ${GLXSPHERES} DESTINATION ${CMAKE_INSTALL_BINDIR})

if(VGL_EGLBACKEND)
	set(EGLXSPHERES eglxspheres)
	if(BITS EQUAL 64)
		set(EGLXSPHERES eglxspheres64)
	endif()
	add_executable(${EGLXSPHERES} eglxspheres.c xfonts.c)
	target_link_libraries(${EGLXSPHERES} ${OPENGL_gl_LIBRARY} ${OPENGL_egl_LIBRARY}
		${X11_X11_LIB} ${OPENGL_glu_LIBRARY} m)
	install(TARGETS ${EGLXSPHERES} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

foreach(program pbdemo)
	add_executable(${program} ${program}.c pbutil.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} m)
endforeach()
