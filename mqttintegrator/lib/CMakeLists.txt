cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt-client)


set(MQTTINTEGRATOR_CPP Mqtt.cpp SocketContextFactory.cpp)
set(MQTTINTEGRATOR_H SocketContextFactory.h)

add_library(mqtt-integrator SHARED ${MQTTINTEGRATOR_CPP} ${MQTTINTEGRATOR_H})

target_include_directories(mqtt-integrator PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
    mqtt-integrator PUBLIC snodec::mqtt-client nlohmann_json::nlohmann_json
                           mqtt-mapping
)

set_target_properties(
    mqtt-integrator PROPERTIES SOVERSION "${SNODEC_SO_VERSION}"
)

install(TARGETS mqtt-integrator RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
