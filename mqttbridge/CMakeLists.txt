cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)
# add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0)
find_package(
    snodec 0.90.0
    COMPONENTS net-in-stream-legacy
               net-in6-stream-legacy
               net-un-stream-legacy
               net-l2-stream-legacy
               net-rc-stream-legacy
               net-in-stream-tls
               net-in6-stream-tls
               net-un-stream-tls
               net-l2-stream-tls
               net-rc-stream-tls
               mqtt
)

set(MQTTBRIDGE_CPP mqttbridge.cpp
                   SocketContextFactory.cpp
)
set(MQTTBRIDGE_H SocketContextFactory.h)

add_executable(mqttbridge ${MQTTBRIDGE_CPP} ${MQTTBRIDGE_H})

target_include_directories(
    mqttbridge PUBLIC ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    mqttbridge
    PUBLIC snodec::net-in-stream-legacy
           snodec::net-un-stream-legacy
           snodec::net-in6-stream-legacy
           snodec::net-rc-stream-legacy
           snodec::net-l2-stream-legacy
           snodec::net-in-stream-tls
           snodec::net-un-stream-tls
           snodec::net-in6-stream-tls
           snodec::net-rc-stream-tls
           snodec::net-l2-stream-tls
           snodec::mqtt
           mqtt-bridge
)

install(TARGETS mqttbridge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
