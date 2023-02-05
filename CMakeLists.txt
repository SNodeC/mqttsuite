cmake_minimum_required(VERSION 3.5)

project(MQTTBroker LANGUAGES CXX)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

get_filename_component(
    SNODEC_DOC_ROOTDIR "${CMAKE_SOURCE_DIR}/../mqttbroker-doc/" ABSOLUTE
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(format)
include(doxygen)
include(uninstall)
include(iwyu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

option(CHOWN_BIN_DIR "Check binary dir is writable" OFF)
if(CHOWN_BIN_DIR)
    add_custom_target(
        chown ALL
        COMMAND sudo chown -R $ENV{user}:$ENV{user} ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT
            "Make sure binary directory is writeable: ${CMAKE_CURRENT_BINARY_DIR}"
    )
endif(CHOWN_BIN_DIR)

add_subdirectory(lib)
add_subdirectory(mqttbroker)
add_subdirectory(mqttintegrator)
