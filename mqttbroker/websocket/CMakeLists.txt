cmake_minimum_required(VERSION 3.5)

find_package(nlohmann_json)

find_package(snodec COMPONENTS websocket-server)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS
)

set(MQTTSERVERSUBPROTOCOL_CPP SubProtocolFactory.cpp)

set(MQTTSERVERSUBPROTOCOL_H SubProtocolFactory.h)

add_library(
    mqttserversubprotocol SHARED ${MQTTSERVERSUBPROTOCOL_CPP}
                                 ${MQTTSERVERSUBPROTOCOL_H}
)

target_link_libraries(
    mqttserversubprotocol PUBLIC mqtt-broker nlohmann_json::nlohmann_json
)

target_include_directories(
    mqttserversubprotocol PUBLIC ${WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS}
                                 ${PROJECT_SOURCE_DIR}
)

set_target_properties(
    mqttserversubprotocol PROPERTIES OUTPUT_NAME "snodec-websocket-mqtt"
                                     SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS mqttserversubprotocol
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}
)
