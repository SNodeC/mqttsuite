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

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec
    COMPONENTS mqtt-client
    REQUIRED
)

add_library(mqtt-cli SHARED Mqtt.cpp Mqtt.h)

target_include_directories(mqtt-cli PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
    mqtt-cli PUBLIC snodec::mqtt-client nlohmann_json::nlohmann_json
                    mqtt-mapping
)

set_target_properties(
    mqtt-cli
    PROPERTIES VERSION ${MQTTSuite_VERSION}
               VERSION ${MQTTSuite_VERSION}
               SOVERSION ${MQTTSUITE_SOVERSION}
)

install(TARGETS mqtt-cli RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
