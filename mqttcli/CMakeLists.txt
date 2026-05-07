# MQTTSuite - A lightweight MQTT Integration System
# Copyright (C) Volker Christian <me@vchrist.at>
#               2022, 2023, 2024, 2025, 2026
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# MIT License

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

option(CONFIG_MQTTSUITE_CLI_TCP_IPV4 "MQTTSuite mqtt-cli TCP IPv4" ON)
option(CONFIG_MQTTSUITE_CLI_TCP_IPV6 "MQTTSuite mqtt-cli TCP IPv6" ON)
option(CONFIG_MQTTSUITE_CLI_UNIX "MQTTSuite mqtt-cli Unix" ON)
option(CONFIG_MQTTSUITE_CLI_WS "MQTTSuite mqtt-cli WebSocket" ON)

dep_option(
    CONFIG_MQTTSUITE_CLI_TLS_IPV4 "SSL/TLS (IPv4)" ON
    CONFIG_MQTTSUITE_CLI_TCP_IPV4
)

dep_option(
    CONFIG_MQTTSUITE_CLI_TLS_IPV6 "SSL/TLS (IPv6)" ON
    CONFIG_MQTTSUITE_CLI_TCP_IPV6
)

dep_option(
    CONFIG_MQTTSUITE_CLI_UNIX_TLS "SSL/TLS (Unix domain sockets)" ON
    CONFIG_MQTTSUITE_CLI_UNIX
)

dep_option(
    CONFIG_MQTTSUITE_CLI_WSS "WebSocket (WSS)" ON CONFIG_MQTTSUITE_CLI_WS
)

set(MQTTCLI_COMPONENTS "")
set(MQTTCLI_LINK_LIBS "")

if(CONFIG_MQTTSUITE_CLI_TCP_IPV4)
    list(APPEND MQTTCLI_COMPONENTS net-in-stream-legacy http-server-express-legacy-in)
    if(CONFIG_MQTTSUITE_CLI_TLS_IPV4)
        list(APPEND MQTTCLI_COMPONENTS net-in-stream-tls http-server-express-tls-in)
    endif()
endif()
if(CONFIG_MQTTSUITE_CLI_TCP_IPV6)
    list(APPEND MQTTCLI_COMPONENTS net-in6-stream-legacy http-server-express-legacy-in6)
    if(CONFIG_MQTTSUITE_CLI_TLS_IPV6)
        list(APPEND MQTTCLI_COMPONENTS net-in6-stream-tls http-server-express-tls-in6)
    endif()
endif()
if(CONFIG_MQTTSUITE_CLI_UNIX)
    list(APPEND MQTTCLI_COMPONENTS net-un-stream-legacy http-server-express-legacy-un)
    if(CONFIG_MQTTSUITE_CLI_UNIX_TLS)
        list(APPEND MQTTCLI_COMPONENTS net-un-stream-tls http-server-express-tls-un)
    endif()
endif()
if(CONFIG_MQTTSUITE_CLI_WS OR CONFIG_MQTTSUITE_CLI_WSS)
    list(APPEND MQTTCLI_COMPONENTS http-client)
endif()
list(REMOVE_DUPLICATES MQTTCLI_COMPONENTS)

foreach(c IN LISTS MQTTCLI_COMPONENTS)
    list(APPEND MQTTCLI_LINK_LIBS snodec::${c})
endforeach()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(snodec 1.0.0 REQUIRED COMPONENTS ${MQTTCLI_COMPONENTS} mqtt-client)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/config.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

set(MQTT_CPP mqttcli.cpp SocketContextFactory.cpp)
set(MQTT_H SocketContextFactory.h)

set_property(
    SOURCE mqttcli.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS
             CMAKE_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"
)

add_executable(mqttcli ${MQTT_CPP} ${MQTT_H})

target_include_directories(mqttcli PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE WEB_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/html/*.html"
     "${CMAKE_CURRENT_SOURCE_DIR}/html/*.css"
     "${CMAKE_CURRENT_SOURCE_DIR}/html/*.js"
)

target_sources(mqttcli PRIVATE ${WEB_ASSETS})

target_link_libraries(
    mqttcli PUBLIC ${MQTTCLI_LINK_LIBS} snodec::mqtt-client mqtt-cli
)

install(TARGETS mqttcli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/var/www/mqttsuite/mqttcli
    FILES_MATCHING
    PATTERN "*.html"
    PATTERN "*.css"
    PATTERN "*.js"
)
