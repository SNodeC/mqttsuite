cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(websocket)

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
               http-client
               mqtt
)

set(MQTTBRIDGE_CPP mqttbridge.cpp SocketContextFactory.cpp)
set(MQTTBRIDGE_H SocketContextFactory.h)

add_executable(mqttbridge ${MQTTBRIDGE_CPP} ${MQTTBRIDGE_H})

macro(add_instance INSTANCE SOURCE)
    set(${INSTANCE} "ON" CACHE BOOL "${INSTANCE}")

    string(TOUPPER ${${INSTANCE}} INSTANCE_UPPER)
    if (${INSTANCE_UPPER} STREQUAL "ON" OR ${INSTANCE_UPPER} STREQUAL "TRUE")
        message("Append ${INSTANCE} to ${SOURCE}")
        set_property(
            SOURCE ${SOURCE}
            APPEND PROPERTY COMPILE_DEFINITIONS ${INSTANCE}
        )
    endif(${INSTANCE_UPPER} STREQUAL "ON" OR ${INSTANCE_UPPER} STREQUAL "TRUE")
endmacro(add_instance INSTANCE)

add_instance(MQTTBRIDGE_IN_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_L2_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_RC_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_UN_STREAM_LEGACY mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN_STREAM_TLS mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_STREAM_TLS mqttbridge.cpp)
add_instance(MQTTBRIDGE_L2_STREAM_TLS mqttbridge.cpp)
add_instance(MQTTBRIDGE_RC_STREAM_TLS mqttbridge.cpp)
add_instance(MQTTBRIDGE_UN_STREAM_TLS mqttbridge.cpp)

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

set(WSMQTTBRIDGE_CPP wsmqttbridge.cpp)
set(WSMQTTBRIDGE_H)

add_executable(wsmqttbridge ${WSMQTTBRIDGE_CPP} ${WSMQTTBRIDGE_H})

target_link_libraries(
    wsmqttbridge PUBLIC snodec::net-in-stream-legacy snodec::net-in-stream-tls
                        snodec::http-client mqtt-bridge
)

install(TARGETS wsmqttbridge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
