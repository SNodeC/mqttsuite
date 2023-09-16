cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(nlohmann_json 3.7.0)
find_package(
    snodec 1.0.0 COMPONENTS net-in-stream-legacy net-in-stream-tls http-client
                            websocket-client mqtt-client
)

set(MQTTINTEGRATOR_CPP mqttintegrator.cpp SocketContextFactory.cpp)
set(MQTTINTEGRATOR_H SocketContextFactory.h)

add_executable(mqttintegrator ${MQTTINTEGRATOR_CPP} ${MQTTINTEGRATOR_H})

target_include_directories(
    mqttintegrator PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    mqttintegrator PUBLIC snodec::net-in-stream-tls mqtt-mapping snodec::mqtt
                          mqtt-integrator
)

install(TARGETS mqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(WSMQTTINTEGRATOR_CPP wsmqttintegrator.cpp)
set(WSMQTTINTEGRATOR_H)

add_executable(wsmqttintegrator ${WSMQTTINTEGRATOR_CPP} ${WSMQTTINTEGRATOR_H})

target_link_libraries(
    wsmqttintegrator PUBLIC snodec::net-in-stream-legacy
                            snodec::net-in-stream-tls snodec::http-client
)

install(TARGETS wsmqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(
    wsmqttintegrator-dynamic ${WSMQTTINTEGRATOR_CPP} ${WSMQTTINTEGRATOR_H}
)

target_compile_definitions(
    wsmqttintegrator-dynamic PRIVATE LINK_SUBPROTOCOL_STATIC
)

target_include_directories(
    wsmqttintegrator-dynamic PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    wsmqttintegrator-dynamic
    PUBLIC websocket-mqtt-client snodec::net-in-stream-legacy
           snodec::net-in-stream-tls snodec::http-client
           snodec::websocket-client snodec::mqtt-client
)

set_target_properties(
    wsmqttintegrator-dynamic
    PROPERTIES INSTALL_RPATH
               "${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}"
)

install(TARGETS wsmqttintegrator-dynamic
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
