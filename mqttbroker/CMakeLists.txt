cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(
    snodec COMPONENTS net-in-stream-legacy net-in-stream-tls
                      net-un-stream-legacy mqtt-server http-server-express
)

set(MQTTBROKER_CPP mqttbroker.cpp SharedSocketContextFactory.cpp)

set(MQTTBROKER_H SharedSocketContextFactory.h)

add_executable(mqttbroker ${MQTTBROKER_CPP} ${MQTTBROKER_H})

target_include_directories(
    mqttbroker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                      ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    mqttbroker
    PUBLIC snodec::net-in-stream-legacy snodec::net-in-stream-tls
           snodec::net-un-stream-legacy snodec::http-server-express mqtt-broker
)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
