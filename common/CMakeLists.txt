#
# Unit tests
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	add_executable(rrframeut rrframeut.cpp rrframe.cpp ../client/rrglframe.cpp
		../client/xdk-sym.cpp)
else()
	add_executable(rrframeut rrframeut.cpp)
endif()
target_link_libraries(rrframeut rrutil ${FBXLIB} ${TJPEG_LIBRARY}
	${OPENGL_gl_LIBRARY})
