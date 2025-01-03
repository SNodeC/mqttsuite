cmake_minimum_required(VERSION 3.14)

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
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -Wfloat-equal
    -pedantic-errors
    -fexec-charset=UTF-8
    -Wno-shadow # yes we do
    -Wno-psabi # needed for RaspberryPi
    $<$<CXX_COMPILER_ID:Clang>:-Weverything>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat-pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-exit-time-destructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-global-constructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-shadow-field>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-padded>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation-unknown-command>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,16.0>>:-Wno-unsafe-buffer-usage>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,18.0>>:-Wno-switch-default>
)

add_link_options(LINKER:--no-undefined)

add_subdirectory(lib)
add_subdirectory(mqttbroker)
add_subdirectory(mqttintegrator)
add_subdirectory(mqttbridge)
