if(WIN32)
	set(FBXLIB fbx-x11)
else()
	set(FBXLIB fbx)
endif()
if(VGL_USEXV)
	set(FBXLIB ${FBXLIB};fbxv)
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
install(TARGETS vglclient DESTINATION ${BINDIR})

if(WIN32)
	add_custom_target(vglconnect.bat ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/vglconnect.bat
			${CMAKE_CURRENT_BINARY_DIR}/vglconnect.bat
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vglconnect.bat)
	install(PROGRAMS vglconnect.bat DESTINATION ${BINDIR})
else()
	add_custom_target(vglconnect ALL
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/vglconnect
			${CMAKE_CURRENT_BINARY_DIR}/vglconnect
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vglconnect)
	install(PROGRAMS vglconnect DESTINATION ${BINDIR})
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
