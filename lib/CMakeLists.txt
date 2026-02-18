# MQTTSuite - A lightweight MQTT Integration System
# Copyright (C) Volker Christian <me@vchrist.at>
#               2022, 2023, 2024, 2025, 2026
#               Tobias Pfeil
#               2025, 2026
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

set(TMP_CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${CMAKE_CXX_INCLUDE_WHAT_YOU_USE})
# Do not use iwyu for json-schema-validator
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE)
add_subdirectory(json-schema-validator)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${TMP_CMAKE_CXX_INCLUDE_WHAT_YOU_USE})

target_compile_options(
    nlohmann_json_schema_validator
    PUBLIC -Wno-float-equal -Wno-reserved-macro-identifier -Wno-switch-enum
           -Wno-weak-vtables -Wno-missing-noreturn
)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec
    COMPONENTS mqtt
    REQUIRED
)

set_target_properties(
    nlohmann_json_schema_validator PROPERTIES POSITION_INDEPENDENT_CODE ON
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        # Yes, there are some unsafe-buffer-usage
        set(DISABLED_WARNINGS "-Wno-unsafe-buffer-usage")
    endif()
    set(DISABLED_WARNINGS
        "${DISABLED_WARNINGS} -Wno-unused-const-variable -Wno-unused-member-function -Wno-reserved-identifier -Wno-old-style-cast -Wno-documentation -Wno-extra-semi -Wno-unused-const-variable -Wno-missing-variable-declarations -Wno-deprecated-copy-with-dtor -Wno-unreachable-code-return"
    )
endif()

set_target_properties(
    nlohmann_json_schema_validator PROPERTIES COMPILE_FLAGS
                                              "${DISABLED_WARNINGS}"
)

# Create mapping-schema.json.h in case mapping-schema.json has changed on disk.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mapping-schema.json.h
    COMMAND
        ${CMAKE_COMMAND} -DTARGET_PATH=${CMAKE_CURRENT_BINARY_DIR} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/recreate-mapping-schema-json-h.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mapping-schema.json
    COMMENT "Creating ${CMAKE_CURRENT_BINARY_DIR}/mapping-schema.json.h"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(
    mqtt-mapping STATIC
    JsonMappingReader.cpp
    MqttMapper.cpp
    JsonMappingReader.h
    MqttMapper.h
    mapping-schema.json.h
    inja.hpp
    MappingAdminRouter.cpp
    MappingAdminRouter.h
    ConfigApplication.cpp
    ConfigApplication.h
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        set_source_files_properties(
            MqttMapper.cpp PROPERTIES COMPILE_FLAGS -Wno-unsafe-buffer-usage
                                      -Wno-CopyConstructor -Wno-OeratorEq
        )
    endif()
endif()

target_include_directories(mqtt-mapping PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqtt-mapping PUBLIC snodec::mqtt nlohmann_json_schema_validator
                        nlohmann_json::nlohmann_json
)

set_target_properties(mqtt-mapping PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(mqtt-mapping PRIVATE -Wno-shadow-field)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

target_compile_options(mqtt-mapping PUBLIC -Wno-float-equal)

add_subdirectory(plugins)
