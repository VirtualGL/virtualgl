add_library(vglutil STATIC GenericQ.cpp Log.cpp Mutex.cpp Thread.cpp bmp.c)
target_link_libraries(vglutil pthread)
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
	target_link_libraries(vglutil rt)
endif()

add_library(vglsocket STATIC Socket.cpp)
target_link_libraries(vglsocket vglutil)
if(VGL_USESSL)
	target_link_libraries(vglsocket ${OPENSSL_LIBRARIES})
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
	target_link_libraries(vglsocket socket xnet)
endif()

add_executable(nettest nettest.cpp)
target_link_libraries(nettest vglsocket)
if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
	target_link_libraries(nettest kstat)
endif()
install(TARGETS nettest DESTINATION ${VGL_BINDIR})

add_library(fbx STATIC fbx.c)

if(VGL_BUILDSERVER)
	add_library(fbx-faker STATIC fbx.c)
	set_property(TARGET fbx-faker APPEND PROPERTY COMPILE_DEFINITIONS INFAKER)
	target_link_libraries(fbx-faker ${X11_X11_LIB} ${X11_Xext_LIB})
endif()

add_executable(fbxtest fbxtest.cpp)
target_link_libraries(fbxtest fbx vglutil)

target_link_libraries(fbx ${X11_X11_LIB} ${X11_Xext_LIB})

if(VGL_USEXV)
	add_library(fbxv STATIC fbxv.c)
	target_link_libraries(fbxv ${X11_Xv_LIB} ${X11_X11_LIB} ${X11_Xext_LIB})

	add_executable(fbxvtest fbxvtest.c)
	target_link_libraries(fbxvtest fbxv)
endif()

add_executable(threadtest threadtest.cpp)
target_link_libraries(threadtest vglutil)

add_executable(glreadtest glreadtest.cpp)
target_link_libraries(glreadtest ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY}
	${X11_X11_LIB})
if(VGL_BUILDSERVER)
	install(TARGETS glreadtest DESTINATION ${VGL_BINDIR})
endif()
