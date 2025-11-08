cmake_minimum_required(VERSION 3.20)

set(OCTREE_LIB Octree)

add_library(${OCTREE_LIB} STATIC octree.cpp)

target_include_directories(${OCTREE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS ${OCTREE_LIB} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_subdirectory(tests)