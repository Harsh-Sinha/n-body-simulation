cmake_minimum_required(VERSION 3.20)

project(ParallelNBodySimulation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "building for ${CMAKE_BUILD_TYPE}")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -fomit-frame-pointer -march=native -funroll-loops -ffast-math -fopenmp-simd -fno-math-errno")
endif()

if (${ENABLE_TESTING})
    add_subdirectory(external/Catch2)
    enable_testing()
endif()

add_subdirectory(modules)
add_subdirectory(tools)
add_subdirectory(playground)
add_subdirectory(benchmark)