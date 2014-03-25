if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	add_executable(cpustat cpustat.c)
	install(TARGETS cpustat DESTINATION ${VGL_BINDIR})
endif()

add_executable(imgdiff imgdiff.c)
target_link_libraries(imgdiff vglutil)
target_link_libraries(imgdiff m)

add_executable(tcbench tcbench.cpp dsimple.c)
target_link_libraries(tcbench fbx)
install(TARGETS tcbench DESTINATION ${VGL_BINDIR})
