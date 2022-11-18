cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(config.h.in config.h @ONLY)

if(NOT SNODEC_INTREE_BUILD)
    find_package(
        snodec COMPONENTS net-in-stream-legacy net-in-stream-tls
                          net-un-stream-legacy mqtt-server
    )
endif(NOT SNODEC_INTREE_BUILD)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(ADDITIONAL_OPTIONS
        -Weverything
        -Wno-c++98-compat
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-shadow
        -Wno-shadow-field
        -Wno-used-but-marked-unused
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi # needed for RaspberryPi
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -Wno-float-equal
    -Wno-implicit-int-float-conversion
    -pedantic-errors
    -fexec-charset=UTF-8
    ${ADDITIONAL_OPTIONS}
)

set(MQTTBROKER_CPP mqttbroker.cpp SocketContext.cpp)

set(MQTTBROKER_H SharedSocketContextFactory.h SharedSocketContextFactory.hpp
                 SocketContext.h
)

add_executable(mqttbroker ${MQTTBROKER_CPP} ${MQTTBROKER_H})

target_include_directories(
    mqttbroker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                      ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    mqttbroker
    PUBLIC snodec::mqtt-server snodec::net-in-stream-legacy
           snodec::net-in-stream-tls snodec::net-un-stream-legacy
    PRIVATE mqtt-mapping
)

if(SNODEC_INTREE_BUILD)
    set_target_properties(
        mqttbroker PROPERTIES INSTALL_RPATH "${SNODEC_IOT_MQTT_RPATH}"
    )
endif(SNODEC_INTREE_BUILD)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})