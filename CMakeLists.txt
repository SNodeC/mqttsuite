cmake_minimum_required(VERSION 3.5)

project(MQTTBroker LANGUAGES CXX)

include(GNUInstallDirs)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(format)
include(doxygen)
include(uninstall)
include(iwyu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi # needed for RaspberryPi
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -pedantic-errors
    -fexec-charset=UTF-8
    -Wno-shadow
    $<$<CXX_COMPILER_ID:Clang>:-Weverything>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-padded>
)

add_link_options(LINKER:--no-undefined)

add_subdirectory(lib)
add_subdirectory(mqttbroker)
add_subdirectory(mqttintegrator)
