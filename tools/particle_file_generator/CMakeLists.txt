cmake_minimum_required(VERSION 3.20)

set(EXEC_NAME particle_file_generator)

add_executable(${EXEC_NAME} main.cpp)

target_link_libraries(${EXEC_NAME} PUBLIC ParticleConfig)

install(TARGETS ${EXEC_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/tools)