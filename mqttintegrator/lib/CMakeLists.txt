cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt-client)

add_library(mqtt-integrator SHARED Mqtt.cpp Mqtt.h)

target_include_directories(mqtt-integrator PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
    mqtt-integrator PUBLIC snodec::mqtt-client nlohmann_json::nlohmann_json
                           mqtt-mapping
)

set_target_properties(
    mqtt-integrator PROPERTIES SOVERSION "${SNODEC_SOVERSION}"
)

install(TARGETS mqtt-integrator RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
