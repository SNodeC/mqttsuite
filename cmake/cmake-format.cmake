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

# Additional targets to perform cmake-format

find_program(CMAKE_FORMAT "cmake-format")

if(CMAKE_FORMAT)
    file(GLOB_RECURSE CMAKELISTS_TXT_FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt
         ${CMAKE_SOURCE_DIR}/cmake/*.cmake ${CMAKE_SOURCE_DIR}/*.cmake.in
    )

    # Remove strings matching given regular expression from a list.
    # @param(in,out) aItems Reference of a list variable to filter. @param
    # aRegEx Value of regular expression to match.
    function(filter_items aItems aRegEx)
        # For each item in our list
        foreach(item ${${aItems}})
            # Check if our items matches our regular expression
            if("${item}" MATCHES ${aRegEx})
                # Remove current item from our list
                list(REMOVE_ITEM ${aItems} ${item})
            endif("${item}" MATCHES ${aRegEx})
        endforeach(item)
        # Provide output parameter
        set(${aItems}
            ${${aItems}}
            PARENT_SCOPE
        )
    endfunction(filter_items)

    filter_items(CMAKELISTS_TXT_FILES "/json-schema-validator/")

    add_custom_command(
        OUTPUT format-cmds
        APPEND
        COMMAND ${CMAKE_FORMAT} -i ${CMAKELISTS_TXT_FILES}
        COMMENT "Auto formatting of all CMakeLists.txt files"
    )
else(CMAKE_FORMAT)
    message(
        WARNING
            " cmake-format not found:\n"
            "    cmake-format is used to format all CMakeLists.txt files consitently.\n"
            "    It is highly recommented to install it, if you intend to modify the code of SNode.C.\n"
            "    In case you have it installed run \"cmake --target format\" to format all CMakeLists.txt files.\n"
            "    If you do not want to contribute to SNode.C, you can ignore this warning.\n"
    )
endif(CMAKE_FORMAT)
