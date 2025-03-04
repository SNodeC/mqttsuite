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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec 1.0.0 REQUIRED
    COMPONENTS net-in-stream-legacy
               net-in6-stream-legacy
               net-un-stream-legacy
               net-in-stream-tls
               net-in6-stream-tls
               net-un-stream-tls
               http-client
               mqtt
    OPTIONAL_COMPONENTS net-l2-stream-legacy net-rc-stream-legacy
                        net-l2-stream-tls net-rc-stream-tls
)

macro(add_instance INSTANCE SOURCE)
    set(${INSTANCE}
        "ON"
        CACHE BOOL "${INSTANCE}"
    )

    string(TOUPPER ${${INSTANCE}} ${INSTANCE}_UPPER)
    if(${${INSTANCE}_UPPER} STREQUAL "ON" OR ${${INSTANCE}_UPPER} STREQUAL
                                             "TRUE"
    )
        message("Append ${INSTANCE} to ${SOURCE}")
        set_property(
            SOURCE ${SOURCE}
            APPEND
            PROPERTY COMPILE_DEFINITIONS ${INSTANCE}
        )
    endif(${${INSTANCE}_UPPER} STREQUAL "ON" OR ${${INSTANCE}_UPPER} STREQUAL
                                                "TRUE"
    )
endmacro(add_instance INSTANCE)

set(MQTTBRIDGE_CPP mqttbridge.cpp SocketContextFactory.cpp)
set(MQTTBRIDGE_H SocketContextFactory.h)

add_executable(mqttbridge ${MQTTBRIDGE_CPP} ${MQTTBRIDGE_H})

add_instance(MQTTBRIDGE_IN_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN6_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_UN_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_UN_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN_WEBSOCKET_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_WEBSOCKET_LEGACY mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN_WEBSOCKET_TLS mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_WEBSOCKET_TLS mqttbridge.cpp)

if(TARGET snodec::net-l2-stream-legacy)
    add_instance(MQTTBRIDGE_L2_STREAM_LEGACY mqttbridge.cpp)
    list(APPEND BLUETOOTH_LIBS "snodec::net-l2-stream-legacy")
endif(TARGET snodec::net-l2-stream-legacy)
if(TARGET snodec::net-l2-stream-tls)
    add_instance(MQTTBRIDGE_L2_STREAM_TLS mqttbridge.cpp)
    list(APPEND BLUETOOTH_LIBS "snodec::net-l2-stream-tls")
endif(TARGET snodec::net-l2-stream-tls)

if(TARGET snodec::net-rc-stream-legacy)
    add_instance(MQTTBRIDGE_RC_STREAM_LEGACY mqttbridge.cpp)
    list(APPEND BLUETOOTH_LIBS "snodec::net-rc-stream-legacy")
endif(TARGET snodec::net-rc-stream-legacy)
if(TARGET snodec::net-rc-stream-tls)
    add_instance(MQTTBRIDGE_RC_STREAM_TLS mqttbridge.cpp)
    list(APPEND BLUETOOTH_LIBS "snodec::net-rc-stream-tls")
endif(TARGET snodec::net-rc-stream-tls)

target_link_libraries(
    mqttbridge
    PUBLIC snodec::net-in-stream-legacy
           snodec::net-un-stream-legacy
           snodec::net-in6-stream-legacy
           snodec::net-in-stream-tls
           snodec::net-un-stream-tls
           snodec::net-in6-stream-tls
           ${BLUETOOTH_LIBS}
           snodec::http-client
           snodec::mqtt
           mqtt-bridge
)

install(TARGETS mqttbridge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
