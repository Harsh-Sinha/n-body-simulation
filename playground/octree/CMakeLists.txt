cmake_minimum_required(VERSION 3.20)

set(EXEC_NAME octree)

add_executable(${EXEC_NAME} main.cpp)

install(TARGETS ${EXEC_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/playground)