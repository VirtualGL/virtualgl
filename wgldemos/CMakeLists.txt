include(FindOpenGL)

add_executable(wglspheres wglspheres.c)
target_link_libraries(wglspheres ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

install(TARGETS wglspheres DESTINATION ${VGL_BINDIR})
