cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TMP_CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${CMAKE_CXX_INCLUDE_WHAT_YOU_USE})
# Do not use iwyu for json-schema-validator
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE)
add_subdirectory(json-schema-validator)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${TMP_CMAKE_CXX_INCLUDE_WHAT_YOU_USE})

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt)

target_compile_options(
    nlohmann_json_schema_validator
    PUBLIC -Wno-float-equal -Wno-reserved-macro-identifier -Wno-switch-enum
           -Wno-weak-vtables
)

set_target_properties(
    nlohmann_json_schema_validator PROPERTIES POSITION_INDEPENDENT_CODE ON
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        # Yes, there are some unsafe-buffer-usage
        set(DISABLED_WARNINGS "-Wno-unsafe-buffer-usage")
    endif()
    set(DISABLED_WARNINGS
        "${DISABLED_WARNINGS} -Wno-unused-const-variable -Wno-unused-member-function -Wno-reserved-identifier -Wno-old-style-cast -Wno-documentation -Wno-extra-semi -Wno-unused-const-variable -Wno-missing-variable-declarations -Wno-deprecated-copy-with-dtor -Wno-unreachable-code-return"
    )
endif()

set_target_properties(
    nlohmann_json_schema_validator PROPERTIES COMPILE_FLAGS
                                              "${DISABLED_WARNINGS}"
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

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        set_source_files_properties(
            MqttMapper.cpp PROPERTIES COMPILE_FLAGS -Wno-unsafe-buffer-usage -Wno-CopyConstructor -Wno-OeratorEq
        )
    endif()
endif()

target_include_directories(mqtt-mapping PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqtt-mapping PUBLIC snodec::mqtt nlohmann_json_schema_validator
                        nlohmann_json::nlohmann_json
)

set_target_properties(mqtt-mapping PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(mqtt-mapping PRIVATE -Wno-shadow-field)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

target_compile_options(mqtt-mapping PUBLIC -Wno-float-equal)

add_subdirectory(plugins)
