cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt)

add_subdirectory(json-schema-validator)

target_compile_options(
    nlohmann_json_schema_validator
    PUBLIC -Wno-c++98-compat-pedantic
           -Wno-covered-switch-default
           -Wno-exit-time-destructors
           -Wno-float-equal
           -Wno-global-constructors
           -Wno-reserved-macro-identifier
           -Wno-switch-enum
           -Wno-weak-vtables
)

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(
        mqtt-mapping PRIVATE -Wno-shadow-field-in-constructor -Wno-shadow-field
    )
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
