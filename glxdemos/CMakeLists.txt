foreach(program glthreads glthreadsint sharedtex_mt)
	add_executable(${program} ${program}.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} pthread
		m)
	set_property(TARGET ${program} PROPERTY COMPILE_FLAGS "-DPTHREADS")
endforeach()

foreach(program glxcontexts glxdemo glxgears glxgears_pixmap glxheads	glxpbdemo
	glxpixmap manywin offset overlay shape sharedtex testgl5 wincopy xfont)
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

add_executable(glxspheres glxspheres.c)
target_link_libraries(glxspheres ${OPENGL_gl_LIBRARY} ${X11_X11_LIB}
	${OPENGL_glu_LIBRARY} m)
install(TARGETS glxspheres DESTINATION ${VGL_BINDIR})

foreach(program pbdemo pbinfo)
	add_executable(${program} ${program}.c pbutil.c)
	target_link_libraries(${program} ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} m)
endforeach()

add_executable(xrotfontdemo xrotfontdemo.c xuserotfont.c)
target_link_libraries(xrotfontdemo ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} m)