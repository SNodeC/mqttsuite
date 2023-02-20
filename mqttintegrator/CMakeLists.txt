cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(
    snodec COMPONENTS net-in-stream-legacy net-in-stream-tls mqtt-client
                      http-client
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(ADDITIONAL_OPTIONS
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-bind-to-temporary-copy
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-shadow
        -Wno-used-but-marked-unused
        -Wno-weak-vtables
        -Wno-shadow-field
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

set(MQTTINTEGRATOR_CPP mqttintegrator.cpp SocketContextFactory.cpp)

set(MQTTINTEGRATOR_H SocketContextFactory.h)

add_executable(mqttintegrator ${MQTTINTEGRATOR_CPP} ${MQTTINTEGRATOR_H})

target_include_directories(
    mqttintegrator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    mqttintegrator PUBLIC snodec::net-in-stream-tls mqtt-integrator
)

install(TARGETS mqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(WSMQTTINTEGRATOR_CPP wsmqttintegrator.cpp)

set(WSMQTTINTEGRATOR_H)

add_executable(wsmqttintegrator ${WSMQTTINTEGRATOR_CPP} ${WSMQTTINTEGRATOR_H})

target_include_directories(
    wsmqttintegrator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    wsmqttintegrator PUBLIC snodec::net-in-stream-legacy snodec::http-client
)

install(TARGETS wsmqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
