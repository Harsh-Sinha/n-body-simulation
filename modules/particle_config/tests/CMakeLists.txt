cmake_minimum_required(VERSION 3.20)

set(PARSER_TESTS particle_config_tests)

add_executable(${PARSER_TESTS} test_particle_config.cpp)

target_link_libraries(${PARSER_TESTS} PUBLIC stdc++fs ParticleConfig Catch2::Catch2WithMain)

add_test(NAME ${PARSER_TESTS} COMMAND ${PARSER_TESTS})
