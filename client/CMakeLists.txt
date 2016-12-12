###############################################################################
# CLIENT
###############################################################################

include_directories(../common)

add_library(glframe STATIC GLFrame.cpp)
target_link_libraries(glframe ${OPENGL_gl_LIBRARY})

add_executable(vglclient vglclient.cpp ClientWin.cpp VGLTransReceiver.cpp)
target_link_libraries(vglclient vglcommon ${FBXLIB} glframe vglsocket)
install(TARGETS vglclient DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(vglconnect.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglconnect
	@ONLY)
execute_process(COMMAND chmod +x vglconnect
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vglconnect
	DESTINATION ${CMAKE_INSTALL_BINDIR})
