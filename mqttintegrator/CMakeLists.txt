# MQTTSuite - A lightweight MQTT Integration System Copyright (C) Volker
# Christian <me@vchrist.at> 2022, 2023, 2024, 2025
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
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.14)

# dep_option(NAME HELP_TEXT DEFAULT DEP1 [DEP2 ...]) - If all DEPs are TRUE →
# create a normal CACHE BOOL option (visible in GUI). - Else → remove the cache
# entry (so it disappears from GUI) and force the var OFF.
function(dep_option NAME HELP DEFAULT)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(
        DO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    # Remaining args after DEFAULT are the dependency vars
    set(_deps_ok TRUE)
    foreach(d ${DO_UNPARSED_ARGUMENTS})
        if(NOT ${d})
            set(_deps_ok FALSE)
        endif()
    endforeach()

    if(_deps_ok)
        # Create/keep a visible option; preserve -D overrides
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
        # Hide by removing cache entry; also force normal var OFF for if() logic
        if(DEFINED ${NAME})
            unset(${NAME} CACHE)
        endif()
        set(${NAME}
            OFF
            PARENT_SCOPE
        )
        set(${NAME} OFF) # local too
    endif()
endfunction()

option(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4 "MQTTSuite integrator TCP IPv4" ON)
option(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6 "MQTTSuite integrator TCP IPv6" ON)
option(CONFIG_MQTTSUITE_INTEGRATOR_UNIX "MQTTSuite integrator Unix" ON)
option(CONFIG_MQTTSUITE_INTEGRATOR_WS "MQTTSuite integrator WebSocket" ON)

# Dependent TLS/WSS toggles (only visible when the integrator is ON)
dep_option(
    CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4 "SSL/TLS (IPv4)" ON
    CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4
)

dep_option(
    CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6 "SSL/TLS (IPv6)" ON
    CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6
)

dep_option(
    CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS "SSL/TLS (Unix domain sockets)" ON
    CONFIG_MQTTSUITE_INTEGRATOR_UNIX
)

dep_option(
    CONFIG_MQTTSUITE_INTEGRATOR_WSS "WebSocket (WSS)" ON
    CONFIG_MQTTSUITE_INTEGRATOR_WS
)

set(MQTTINTEGRATOR_COMPONENTS "")
set(MQTTINTEGRATOR_LINK_LIBS "")
# Common nets
if(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV4)
    list(APPEND MQTTINTEGRATOR_COMPONENTS net-in-stream-legacy)
    if(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV4)
        list(APPEND MQTTINTEGRATOR_COMPONENTS net-in-stream-tls)
    endif()
endif()
if(CONFIG_MQTTSUITE_INTEGRATOR_TCP_IPV6)
    list(APPEND MQTTINTEGRATOR_COMPONENTS net-in6-stream-legacy)
    if(CONFIG_MQTTSUITE_INTEGRATOR_TLS_IPV6)
        list(APPEND MQTTINTEGRATOR_COMPONENTS net-in6-stream-tls)
    endif()
endif()
if(CONFIG_MQTTSUITE_INTEGRATOR_UNIX)
    list(APPEND MQTTINTEGRATOR_COMPONENTS net-un-stream-legacy)
    if(CONFIG_MQTTSUITE_INTEGRATOR_UNIX_TLS)
        list(APPEND MQTTINTEGRATOR_COMPONENTS net-un-stream-tls)
    endif()
endif()
if(CONFIG_MQTTSUITE_INTEGRATOR_WS OR CONFIG_MQTTSUITE_INTEGRATOR_WSS)
    list(APPEND MQTTINTEGRATOR_COMPONENTS http-client)
endif()
list(REMOVE_DUPLICATES MQTTINTEGRATOR_COMPONENTS)

foreach(c IN LISTS MQTTINTEGRATOR_COMPONENTS)
    list(APPEND MQTTINTEGRATOR_LINK_LIBS snodec::${c})
endforeach()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec 1.0.0 REQUIRED COMPONENTS ${MQTTINTEGRATOR_COMPONENTS} mqtt-client
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/config.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

set(MQTTINTEGRATOR_CPP mqttintegrator.cpp SocketContextFactory.cpp)
set(MQTTINTEGRATOR_H SocketContextFactory.h)

add_executable(mqttintegrator ${MQTTINTEGRATOR_CPP} ${MQTTINTEGRATOR_H})

target_include_directories(mqttintegrator PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqttintegrator PUBLIC ${MQTTINTEGRATOR_LINK_LIBS} mqtt-mapping
                          mqtt-integrator
)

install(TARGETS mqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
