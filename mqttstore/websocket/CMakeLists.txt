# MQTTSuite - A lightweight MQTT Integration System
# SPDX-License-Identifier: MIT OR GPL-3.0-or-later

cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec
    COMPONENTS net mqtt-client-websocket
    REQUIRED
)

add_library(
    websocket-mqtt-store-client SHARED SubProtocolFactory.cpp
                                       SubProtocolFactory.h
)

target_include_directories(
    websocket-mqtt-store-client PUBLIC ${PROJECT_SOURCE_DIR}
)

target_compile_options(
    websocket-mqtt-store-client
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fno-gnu-unique>
)

target_link_libraries(
    websocket-mqtt-store-client
    PUBLIC snodec::net mqtt-store
    PRIVATE snodec::mqtt-client-websocket
)

set_target_properties(
    websocket-mqtt-store-client
    PROPERTIES OUTPUT_NAME "snodec-websocket-mqtt-client"
               VERSION ${MQTTSuite_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
)

install(TARGETS websocket-mqtt-store-client
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/mqttstore/
)
