cmake_minimum_required(VERSION 3.20)

project(ParallelNBodySimulation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Octree/)

add_subdirectory(playground)