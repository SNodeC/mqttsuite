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

macro(add_instance INSTANCE SOURCE)
    set(${INSTANCE}
        "ON"
        CACHE BOOL "${INSTANCE}"
    )

    string(TOUPPER ${${INSTANCE}} ${INSTANCE}_UPPER)
    if(${${INSTANCE}_UPPER} STREQUAL "ON" OR ${${INSTANCE}_UPPER} STREQUAL
                                             "TRUE"
    )
        message("Append ${INSTANCE} to ${SOURCE}")
        set_property(
            SOURCE ${SOURCE}
            APPEND
            PROPERTY COMPILE_DEFINITIONS ${INSTANCE}
        )
    endif(${${INSTANCE}_UPPER} STREQUAL "ON" OR ${${INSTANCE}_UPPER} STREQUAL
                                                "TRUE"
    )
endmacro(add_instance INSTANCE)

set(MQTTBRIDGE_CPP mqttbridge.cpp SocketContextFactory.cpp)
set(MQTTBRIDGE_H SocketContextFactory.h)

add_executable(mqttbridge ${MQTTBRIDGE_CPP} ${MQTTBRIDGE_H})

add_instance(MQTTBRIDGE_IN_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN6_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_L2_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_L2_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_RC_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_RC_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_UN_STREAM_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_UN_STREAM_TLS mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN_WEBSOCKET_LEGACY mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_WEBSOCKET_LEGACY mqttbridge.cpp)

add_instance(MQTTBRIDGE_IN_WEBSOCKET_TLS mqttbridge.cpp)
add_instance(MQTTBRIDGE_IN6_WEBSOCKET_TLS mqttbridge.cpp)

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
           snodec::http-client
           snodec::mqtt
           mqtt-bridge
)

install(TARGETS mqttbridge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
