if(VGL_USEMEDIALIB)
	add_library(tjpeg STATIC turbojpeg-mlib.c)
	target_link_libraries(tjpeg mlib)
endif()

add_executable(jpgtest jpgtest.c)
target_link_libraries(jpgtest ${TJPEG_LIBRARY} rrutil)
if(UNIX)
	target_link_libraries(jpgtest ${TJPEG_LIBRARY} m)
endif()
	
add_executable(jpegut jpegut.c)
target_link_libraries(jpegut ${TJPEG_LIBRARY})
