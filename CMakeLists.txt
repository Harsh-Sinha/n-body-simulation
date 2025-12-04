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

# build and install imath first before adding alembic and installing rest of the project
add_custom_target(configure_imath
    COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_SOURCE_DIR}/external/imath
        -B ${CMAKE_BINARY_DIR}/imath_temp
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_TESTING=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/third_party
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "configuring imath"
)
add_custom_target(install_imath
    COMMAND ${CMAKE_COMMAND} --build imath_temp --target install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS configure_imath
    COMMENT "installing imath"
)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/imath_installed.stamp
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/imath_installed.stamp
    DEPENDS install_imath
)
add_custom_target(imath_stamp ALL
    DEPENDS ${CMAKE_BINARY_DIR}/imath_installed.stamp
)
add_custom_target(reconfigure_after_imath ALL
    DEPENDS imath_stamp
    COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_SOURCE_DIR}
        -B ${CMAKE_BINARY_DIR}
)
if(NOT DEFINED IMATH_ALREADY_BUILT)
    set(IMATH_ALREADY_BUILT TRUE CACHE INTERNAL "")

    message(STATUS "Imath not built yet, generating build targets…")

else()
    message(STATUS "Imath already built — using installed version")

    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/third_party")
    find_package(Imath REQUIRED CONFIG)

    set(CMAKE_BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/third_party/lib")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/third_party/lib")
    
    add_subdirectory(external/alembic)

    add_subdirectory(modules)
    add_subdirectory(tools)
    add_subdirectory(playground)
    add_subdirectory(benchmark)
endif()
