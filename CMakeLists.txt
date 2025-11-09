cmake_minimum_required(VERSION 3.20)

project(ParallelNBodySimulation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (${ENABLE_TESTING})
    add_subdirectory(external/Catch2)
    enable_testing()
endif()

add_subdirectory(modules)
add_subdirectory(tools)
add_subdirectory(playground)