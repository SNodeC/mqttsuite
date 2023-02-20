cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(ADDITIONAL_OPTIONS
        -Weverything
        -Wno-c++98-compat
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-shadow
        -Wno-shadow-field
        -Wno-used-but-marked-unused
        -Wno-weak-vtables
        -Wno-reserved-macro-identifier
        -Wno-c++98-compat-pedantic
        -Wno-covered-switch-default
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

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
    -Wno-float-equal
    -Wno-switch-enum
    -Wno-implicit-int-float-conversion
    -pedantic-errors
    -fexec-charset=UTF-8
    ${ADDITIONAL_OPTIONS}
)

add_subdirectory(json-schema-validator)

# Create mapping-schema.json.h in case mapping-schema.json has changed on disk.
add_custom_command(
    PRE_BUILD
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mapping-schema.json.h
    COMMAND
        ${CMAKE_COMMAND} -DTARGET_PATH=${CMAKE_CURRENT_BINARY_DIR} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/recreate-mapping-schema-json-h.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mapping-schema.json
    COMMENT "Creating ${CMAKE_CURRENT_BINARY_DIR}/mapping-schema.json.h"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(
    mqtt-mapping STATIC JsonMappingReader.cpp MqttMapper.cpp
                        JsonMappingReader.h MqttMapper.h mapping-schema.json.h
)

set_property(TARGET mqtt-mapping PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(mqtt-mapping PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqtt-mapping PRIVATE snodec::mqtt nlohmann_json_schema_validator
                         nlohmann_json::nlohmann_json
)
