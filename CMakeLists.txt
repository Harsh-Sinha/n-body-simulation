cmake_minimum_required(VERSION 3.20)

project(ParallelNBodySimulation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/Catch2)

add_subdirectory(octree)
add_subdirectory(playground)