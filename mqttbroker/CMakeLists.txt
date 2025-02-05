cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0)
find_package(fmt)
find_package(
    snodec 0.90.0
    COMPONENTS net-in-stream-legacy
               net-in-stream-tls
               net-in6-stream-legacy
               net-in6-stream-tls
               net-un-stream-legacy
               net-un-stream-tls
               http-server-express
               mqtt-server
)

set(MQTTBROKER_CPP mqttbroker.cpp SharedSocketContextFactory.cpp)
set(MQTTBROKER_H SharedSocketContextFactory.h)

add_executable(mqttbroker ${MQTTBROKER_CPP} ${MQTTBROKER_H})

target_link_libraries(
    mqttbroker
    PUBLIC snodec::net-in-stream-legacy
           snodec::net-in-stream-tls
           snodec::net-in6-stream-legacy
           snodec::net-in6-stream-tls
           snodec::net-un-stream-legacy
           snodec::net-un-stream-tls
           snodec::http-server-express
           snodec::mqtt-server
           mqtt-mapping
           mqtt-broker
           fmt
)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
