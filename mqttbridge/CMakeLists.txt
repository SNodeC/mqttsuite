cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)
#add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0)
find_package(
    snodec 0.90.0
    COMPONENTS net-in-stream-legacy net-un-stream-legacy mqtt
)

set(MQTTBRIDGE_CPP mqttbridge.cpp SocketContextFactory.cpp)
set(MQTTBRIDGE_H SocketContextFactory.h)

add_executable(mqttbridge ${MQTTBRIDGE_CPP} ${MQTTBRIDGE_H})

target_include_directories(
    mqttbridge PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    mqttbridge
    PUBLIC snodec::net-in-stream-legacy snodec::net-un-stream-legacy snodec::mqtt
           mqtt-bridge nlohmann_json_schema_validator
)

install(TARGETS mqttbridge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
