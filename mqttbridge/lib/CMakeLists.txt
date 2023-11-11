cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)
find_package(snodec COMPONENTS mqtt-client)

add_library(mqtt-bridge SHARED Mqtt.cpp Mqtt.h Bridge.cpp Bridge.h)

target_include_directories(mqtt-bridge PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
    mqtt-bridge PUBLIC snodec::mqtt-client nlohmann_json::nlohmann_json
)

set_target_properties(
    mqtt-bridge PROPERTIES SOVERSION "${SNODEC_SOVERSION}"
)

install(TARGETS mqtt-bridge RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
