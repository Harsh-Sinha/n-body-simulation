cmake_minimum_required(VERSION 3.20)

set(OCTREE_LIB Octree)

add_library(${OCTREE_LIB} STATIC octree.cpp)

target_include_directories(${OCTREE_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${OCTREE_LIB} PUBLIC ParticleConfig OpenMP::OpenMP_CXX)

install(TARGETS ${OCTREE_LIB} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

if (${ENABLE_TESTING})
    add_subdirectory(tests)
endif()