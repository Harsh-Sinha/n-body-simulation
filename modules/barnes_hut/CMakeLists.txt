cmake_minimum_required(VERSION 3.20)

set(EXEC_NAME b_hut)

add_executable(${EXEC_NAME} main.cpp barnes_hut.cpp data_store.cpp)

target_link_libraries(${EXEC_NAME} PUBLIC Octree ParticleConfig OpenMP::OpenMP_CXX)

install(TARGETS ${EXEC_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

