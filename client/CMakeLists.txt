#
# Client
#

include_directories(../common)

add_library(glframe STATIC GLFrame.cpp)
target_link_libraries(glframe ${OPENGL_gl_LIBRARY})

add_executable(vglclient vglclient.cpp ClientWin.cpp VGLTransReceiver.cpp)
target_link_libraries(vglclient vglcommon ${FBXLIB} glframe vglsocket)
install(TARGETS vglclient DESTINATION ${VGL_BINDIR})

add_custom_target(vglconnect ALL
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_SOURCE_DIR}/vglconnect
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglconnect
	COMMAND chmod u+rx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglconnect
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vglconnect)
install(PROGRAMS vglconnect DESTINATION ${VGL_BINDIR})
