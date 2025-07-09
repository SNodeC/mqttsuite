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
find_package(fmt REQUIRED)
find_package(
    snodec 1.0.0 REQUIRED
    COMPONENTS net-in-stream-tls
               net-in6-stream-legacy
               net-in6-stream-tls
               net-un-stream-legacy
               net-un-stream-tls
               http-server-express-legacy-in
               http-server-express-legacy-in6
               http-server-express-legacy-un
               http-server-express-tls-in
               http-server-express-tls-in6
               http-server-express-tls-un
               mqtt-server
)

set(MQTTBROKER_CPP mqttbroker.cpp SocketContextFactory.cpp)
set(MQTTBROKER_H SocketContextFactory.h)

set_property(
    SOURCE mqttbroker.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS
             CMAKE_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"
)

add_executable(mqttbroker ${MQTTBROKER_CPP} ${MQTTBROKER_H})

file(GLOB_RECURSE WEB_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/html/*.html"
     "${CMAKE_CURRENT_SOURCE_DIR}/html/*.css"
     "${CMAKE_CURRENT_SOURCE_DIR}/html/*.js"
)

target_sources(mqttbroker PRIVATE ${WEB_ASSETS})

target_link_libraries(
    mqttbroker
    PUBLIC snodec::net-in-stream-tls
           snodec::net-in6-stream-legacy
           snodec::net-in6-stream-tls
           snodec::net-un-stream-legacy
           snodec::net-un-stream-tls
           snodec::http-server-express-legacy-in
           snodec::http-server-express-legacy-in6
           snodec::http-server-express-legacy-un
           snodec::http-server-express-tls-in
           snodec::http-server-express-tls-in6
           snodec::http-server-express-tls-un
           snodec::mqtt-server
           mqtt-mapping
           mqtt-broker
           fmt
)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/var/www/mqttsuite/mqttbroker
    FILES_MATCHING
    PATTERN "*.html"
    PATTERN "*.css"
    PATTERN "*.js"
)
