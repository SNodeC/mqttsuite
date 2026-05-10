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
    COMPONENTS mqtt-client db-mariadb
    REQUIRED
)

add_library(
    mqtt-store SHARED
    ConfigSections.cpp
    ConfigSections.h
    MariaDbStore.cpp
    MariaDbStore.h
    Mqtt.cpp
    Mqtt.h
    MqttMessage.h
)

target_include_directories(mqtt-store PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
    mqtt-store
    PUBLIC snodec::mqtt-client snodec::db-mariadb nlohmann_json::nlohmann_json
)

set_target_properties(
    mqtt-store PROPERTIES VERSION ${MQTTSuite_VERSION} SOVERSION
                                                   ${MQTTSUITE_SOVERSION}
)

install(TARGETS mqtt-store RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
