include(FindOpenGL)

include_directories(${OPENGL_INCLUDE_DIR})

add_executable(wglspheres wglspheres.c)
target_link_libraries(wglspheres ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

install(TARGETS wglspheres DESTINATION ${CMAKE_INSTALL_BINDIR})
