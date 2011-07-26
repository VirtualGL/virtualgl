foreach(program glthreads glthreadsint)
	add_executable(${program} ${program}.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} pthread
		m)
	set_property(TARGET ${program} PROPERTY COMPILE_FLAGS "-DPTHREADS")
endforeach()

foreach(program glxdemo glxgears glxheads glxpixmap manywin shape testgl5
	wincopy xfont)
	add_executable(${program} ${program}.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
		${X11_Xext_LIB} m)
endforeach()

add_executable(glxinfo glxinfo.c)
target_link_libraries(glxinfo ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
	${OPENGL_glu_LIBRARY} m)
if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
	target_link_libraries(glxinfo Xmu)
endif()
install(TARGETS glxinfo DESTINATION ${VGL_BINDIR})

foreach(program glxspheres offset)
	add_executable(${program} ${program}.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
		${OPENGL_glu_LIBRARY} m)
endforeach()
install(TARGETS glxspheres DESTINATION ${VGL_BINDIR})

foreach(program pbdemo pbinfo)
	add_executable(${program} ${program}.c pbutil.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} m)
endforeach()
