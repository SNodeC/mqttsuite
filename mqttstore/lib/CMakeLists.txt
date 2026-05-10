# MQTTSuite - A lightweight MQTT Integration System
# SPDX-License-Identifier: MIT OR GPL-3.0-or-later

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
    Mqtt.cpp
    Mqtt.h
    ConfigSections.cpp
    ConfigSections.h
    MariaDbRawStore.cpp
    MariaDbRawStore.h
    RawMessage.h
)

target_include_directories(
    mqtt-store PUBLIC ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(
    mqtt-store PUBLIC snodec::mqtt-client snodec::db-mariadb
                      nlohmann_json::nlohmann_json
)

set_target_properties(
    mqtt-store PROPERTIES VERSION ${MQTTSuite_VERSION} SOVERSION
                                                       ${MQTTSUITE_SOVERSION}
)

install(
    TARGETS mqtt-store
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
