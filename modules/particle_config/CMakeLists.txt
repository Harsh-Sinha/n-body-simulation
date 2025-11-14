cmake_minimum_required(VERSION 3.20)

set(LIB_NAME ParticleConfig)

add_library(${LIB_NAME} INTERFACE)

target_include_directories(${LIB_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES particle_config.hpp DESTINATION ${CMAKE_INSTALL_PREFIX}/inc)

if (${ENABLE_TESTING})
    add_subdirectory(tests)
endif()