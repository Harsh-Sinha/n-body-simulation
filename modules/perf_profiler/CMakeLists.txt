cmake_minimum_required(VERSION 3.20)

set(LIB_NAME PerfProfiler)

add_library(${LIB_NAME} STATIC perf_profiler.cpp)

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS ${LIB_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)