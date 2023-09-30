cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt-server-websocket)

set(MQTTSERVERSUBPROTOCOL_CPP SubProtocolFactory.cpp)
set(MQTTSERVERSUBPROTOCOL_H SubProtocolFactory.h)

add_library(
    websocket-mqtt-server SHARED ${MQTTSERVERSUBPROTOCOL_CPP}
                                 ${MQTTSERVERSUBPROTOCOL_H}
)

target_link_libraries(
    websocket-mqtt-server PUBLIC snodec::mqtt-server-websocket mqtt-broker
                                 nlohmann_json::nlohmann_json
)

set_target_properties(
    websocket-mqtt-server PROPERTIES OUTPUT_NAME "snodec-websocket-mqtt-server"
                                     SOVERSION "${SNODEC_SO_VERSION}"
)

install(TARGETS websocket-mqtt-server
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}
)
