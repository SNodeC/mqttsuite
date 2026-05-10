# MQTTSuite - A lightweight MQTT Integration System
# Copyright (C) Volker Christian <me@vchrist.at>
#               2022, 2023, 2024, 2025, 2026
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.

cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec
    COMPONENTS net mqtt-client-websocket
    REQUIRED
)

set(MQTTSTORE_SUBPROTOCOL_CPP SubProtocolFactory.cpp)
set(MQTTSTORE_SUBPROTOCOL_H SubProtocolFactory.h)

add_library(
    websocket-mqtt-store-client SHARED ${MQTTSTORE_SUBPROTOCOL_CPP}
                                       ${MQTTSTORE_SUBPROTOCOL_H}
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
