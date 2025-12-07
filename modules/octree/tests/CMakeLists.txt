cmake_minimum_required(VERSION 3.20)

set(OCTREE_TESTS octree_tests)

add_executable(${OCTREE_TESTS} test_octree.cpp)

target_link_libraries(${OCTREE_TESTS} PUBLIC PUBLIC stdc++fs Octree ParticleConfig Catch2::Catch2WithMain)

add_test(NAME ${OCTREE_TESTS} COMMAND ${OCTREE_TESTS})
