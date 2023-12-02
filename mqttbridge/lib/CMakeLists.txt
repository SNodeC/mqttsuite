cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt-client)

# Create bridge-schema.json.h in case bridge-schema.json has changed on disk.
add_custom_command(
    PRE_BUILD
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bridge-schema.json.h
    COMMAND
        ${CMAKE_COMMAND} -DTARGET_PATH=${CMAKE_CURRENT_BINARY_DIR} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/recreate-bridge-schema-json-h.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bridge-schema.json
    COMMENT "Creating ${CMAKE_CURRENT_BINARY_DIR}/bridge-schema.json.h"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(
    mqtt-bridge SHARED
    Broker.cpp
    Broker.h
    Mqtt.cpp
    Mqtt.h
    Bridge.cpp
    Bridge.h
    BridgeStore.cpp
    BridgeStore.h
    bridge-schema.json.h
)

target_include_directories(mqtt-bridge PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqtt-bridge PUBLIC snodec::mqtt-client nlohmann_json_schema_validator
)

set_target_properties(mqtt-bridge PROPERTIES SOVERSION "${SNODEC_SOVERSION}")

install(TARGETS mqtt-bridge RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
