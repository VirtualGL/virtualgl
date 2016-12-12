###############################################################################
# LIBRARIES
###############################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(vglcommon STATIC Frame.cpp Profiler.cpp)
target_link_libraries(vglcommon vglutil ${TJPEG_LIBRARY})


###############################################################################
# UNIT TESTS
###############################################################################

add_executable(frameut frameut.cpp)
target_link_libraries(frameut vglcommon ${FBXLIB} glframe)
