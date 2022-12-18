cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(snodec COMPONENTS mqtt-server)

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
        -Wno-weak-vtables
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

add_library(mqtt-broker SHARED Mqtt.cpp Mqtt.h MqttModel.cpp MqttModel.h)

target_include_directories(
    mqtt-broker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                       ${PROJECT_SOURCE_DIR}
)

target_link_libraries(mqtt-broker PUBLIC snodec::mqtt-server mqtt-mapping)

install(TARGETS mqtt-broker RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})