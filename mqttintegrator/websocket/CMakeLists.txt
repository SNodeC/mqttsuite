cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt-client-websocket)

set(MQTTCLIENTSUBPROTOCOL_CPP SubProtocolFactory.cpp)
set(MQTTCLIENTSUBPROTOCOL_H SubProtocolFactory.h)

add_library(
    websocket-mqtt-client SHARED ${MQTTCLIENTSUBPROTOCOL_CPP}
                                 ${MQTTCLIENTSUBPROTOCOL_H}
)

target_link_libraries(
    websocket-mqtt-client PUBLIC snodec::mqtt-client-websocket mqtt-integrator
                                 nlohmann_json::nlohmann_json
)

set_target_properties(
    websocket-mqtt-client PROPERTIES OUTPUT_NAME "snodec-websocket-mqtt-client"
                                     SOVERSION "${SNODEC_SOVERSION}"
)

install(
    TARGETS websocket-mqtt-client
    LIBRARY
        DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/wsmqttintegrator/
)

install(
    TARGETS websocket-mqtt-client
    LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/mqttintegrator/
)
