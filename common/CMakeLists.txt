#
# Libraries
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(rrframe STATIC rrframe.cpp)
target_link_libraries(rrframe rrutil ${TJPEG_LIBRARY})


#
# Unit tests
#

add_executable(rrframeut rrframeut.cpp)
target_link_libraries(rrframeut rrframe ${FBXLIB} rrglframe)
