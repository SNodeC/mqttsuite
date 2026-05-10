# MQTTSuite - A lightweight MQTT Integration System
# Copyright (C) Volker Christian <me@vchrist.at>
#               2022, 2023, 2024, 2025, 2026
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.

cmake_minimum_required(VERSION 3.14)

function(dep_option NAME HELP DEFAULT)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(
        DO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    set(_deps_ok TRUE)
    foreach(d ${DO_UNPARSED_ARGUMENTS})
        if(NOT ${d})
            set(_deps_ok FALSE)
        endif()
    endforeach()

    if(_deps_ok)
        if(DEFINED ${NAME})
            set(_cur "${${NAME}}")
            option(${NAME} "${HELP}" ${DEFAULT})
            set(${NAME}
                "${_cur}"
                CACHE BOOL "${HELP}" FORCE
            )
        else()
            option(${NAME} "${HELP}" ${DEFAULT})
        endif()
        mark_as_advanced(CLEAR ${NAME})
    else()
        if(DEFINED ${NAME})
            unset(${NAME} CACHE)
        endif()
        set(${NAME}
            OFF
            PARENT_SCOPE
        )
        set(${NAME} OFF)
    endif()
endfunction()

option(CONFIG_MQTTSUITE_STORE_TCP_IPV4 "MQTTSuite mqtt-store TCP IPv4" ON)
option(CONFIG_MQTTSUITE_STORE_TCP_IPV6 "MQTTSuite mqtt-store TCP IPv6" ON)
option(CONFIG_MQTTSUITE_STORE_UNIX "MQTTSuite mqtt-store Unix" ON)
option(CONFIG_MQTTSUITE_STORE_WS "MQTTSuite mqtt-store WebSocket" ON)

dep_option(
    CONFIG_MQTTSUITE_STORE_TLS_IPV4 "SSL/TLS (IPv4)" ON
    CONFIG_MQTTSUITE_STORE_TCP_IPV4
)

dep_option(
    CONFIG_MQTTSUITE_STORE_TLS_IPV6 "SSL/TLS (IPv6)" ON
    CONFIG_MQTTSUITE_STORE_TCP_IPV6
)

dep_option(
    CONFIG_MQTTSUITE_STORE_UNIX_TLS "SSL/TLS (Unix domain sockets)" ON
    CONFIG_MQTTSUITE_STORE_UNIX
)

dep_option(
    CONFIG_MQTTSUITE_STORE_WSS "WebSocket (WSS)" ON CONFIG_MQTTSUITE_STORE_WS
)

set(MQTTSTORE_COMPONENTS "")
set(MQTTSTORE_LINK_LIBS "")

if(CONFIG_MQTTSUITE_STORE_TCP_IPV4)
    list(APPEND MQTTSTORE_COMPONENTS net-in-stream-legacy)
    if(CONFIG_MQTTSUITE_STORE_TLS_IPV4)
        list(APPEND MQTTSTORE_COMPONENTS net-in-stream-tls)
    endif()
endif()
if(CONFIG_MQTTSUITE_STORE_TCP_IPV6)
    list(APPEND MQTTSTORE_COMPONENTS net-in6-stream-legacy)
    if(CONFIG_MQTTSUITE_STORE_TLS_IPV6)
        list(APPEND MQTTSTORE_COMPONENTS net-in6-stream-tls)
    endif()
endif()
if(CONFIG_MQTTSUITE_STORE_UNIX)
    list(APPEND MQTTSTORE_COMPONENTS net-un-stream-legacy)
    if(CONFIG_MQTTSUITE_STORE_UNIX_TLS)
        list(APPEND MQTTSTORE_COMPONENTS net-un-stream-tls)
    endif()
endif()
if(CONFIG_MQTTSUITE_STORE_WS OR CONFIG_MQTTSUITE_STORE_WSS)
    list(APPEND MQTTSTORE_COMPONENTS http-client)
endif()
list(REMOVE_DUPLICATES MQTTSTORE_COMPONENTS)

foreach(c IN LISTS MQTTSTORE_COMPONENTS)
    list(APPEND MQTTSTORE_LINK_LIBS snodec::${c})
endforeach()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec 1.0.0 REQUIRED COMPONENTS ${MQTTSTORE_COMPONENTS} mqtt-client
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/config.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

set(MQTTSTORE_CPP mqttstore.cpp SocketContextFactory.cpp)
set(MQTTSTORE_H SocketContextFactory.h)

add_executable(mqttstore ${MQTTSTORE_CPP} ${MQTTSTORE_H})

target_include_directories(mqttstore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqttstore PUBLIC ${MQTTSTORE_LINK_LIBS} snodec::mqtt-client mqtt-store
)

install(TARGETS mqttstore RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
