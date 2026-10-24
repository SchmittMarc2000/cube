cmake_minimum_required(VERSION 3.24)

project(OpenCube
    VERSION 0.1.0
    DESCRIPTION "A modern, open-source re-implementation of the Cube World alpha engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /MP /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Vendored dependencies (see tools/fetch_sdl3.ps1)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL3")
find_package(SDL3 REQUIRED)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

option(OPENCUBE_BUILD_TESTS "Build the test suite" ON)

enable_testing()

add_subdirectory(engine)
add_subdirectory(net)
add_subdirectory(apps/opencube)
add_subdirectory(apps/opencube-server)
if(OPENCUBE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
