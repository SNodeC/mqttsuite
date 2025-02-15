cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec 0.90.0
    COMPONENTS net-in-stream-legacy
               net-in-stream-tls
               net-in6-stream-legacy
               net-in6-stream-tls
               net-un-stream-legacy
               http-client
               websocket-client
               mqtt-client
    REQUIRED
)

set(MQTTINTEGRATOR_CPP mqttintegrator.cpp SocketContextFactory.cpp)
set(MQTTINTEGRATOR_H SocketContextFactory.h)

add_executable(mqttintegrator ${MQTTINTEGRATOR_CPP} ${MQTTINTEGRATOR_H})

target_include_directories(mqttintegrator PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqttintegrator
    PUBLIC snodec::net-in-stream-legacy
           snodec::net-in-stream-tls
           snodec::net-in6-stream-legacy
           snodec::net-in6-stream-tls
           snodec::net-un-stream-legacy
           snodec::http-client
           snodec::websocket-client
           snodec::mqtt-client
           mqtt-mapping
           mqtt-integrator
)

install(TARGETS mqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
