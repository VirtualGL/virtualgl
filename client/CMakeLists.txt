if(WIN32)
add_subdirectory(putty)
add_subdirectory(x11windows)
endif()


#
# Client
#

include_directories(../common)

if(WIN32)
	add_library(rrglframe STATIC rrglframe.cpp xdk-sym.cpp)
	target_link_libraries(rrglframe psapi)
else()
	add_library(rrglframe STATIC rrglframe.cpp)
	target_link_libraries(rrglframe ${OPENGL_gl_LIBRARY})
endif()

add_executable(vglclient vglclient.cpp clientwin.cpp vgltransreceiver.cpp)
target_link_libraries(vglclient rrframe ${FBXLIB} rrglframe rrsocket)
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
