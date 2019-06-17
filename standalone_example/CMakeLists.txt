# CMake file to test the usage of a installed cmake package of OpenZen
# Require a lower cmake version to be compatible with Ubuntu 16.04
cmake_minimum_required(VERSION 3.5)
project(OpenZenStandaloneExample VERSION 0.1.0 LANGUAGES CXX;C)

set (CMAKE_CXX_STANDARD 14)

find_package(OpenZen REQUIRED)

add_executable(OpenZenStandaloneExample
    main.cpp
)

target_link_libraries(OpenZenStandaloneExample
    OpenZen::OpenZen)
