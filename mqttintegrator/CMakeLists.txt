cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)

add_subdirectory(lib)
add_subdirectory(websocket)

find_package(
    snodec COMPONENTS net-in-stream-legacy net-in-stream-tls mqtt-client
                      http-client websocket-client
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
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

add_executable(wsmqttintegrator-dynamic ${WSMQTTINTEGRATOR_CPP} ${WSMQTTINTEGRATOR_H})

target_compile_definitions(
    wsmqttintegrator-dynamic PRIVATE LINK_SUBPROTOCOL_STATIC
)

target_include_directories(
    wsmqttintegrator-dynamic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    wsmqttintegrator-dynamic PUBLIC snodec::net-in-stream-legacy snodec::http-client
                            snodec::websocket-client mqttclientsubprotocol
)

target_link_directories(
    wsmqttintegrator-dynamic PRIVATE ${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}
)

install(TARGETS wsmqttintegrator-dynamic RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
