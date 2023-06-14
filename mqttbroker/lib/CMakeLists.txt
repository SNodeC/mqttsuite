cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json)

find_package(snodec COMPONENTS mqtt-server)

add_library(mqtt-broker SHARED Mqtt.cpp Mqtt.h MqttModel.cpp MqttModel.h)

set_source_files_properties(
    MqttModel.cpp PROPERTIES COMPILE_FLAGS -Wno-exit-time-destructors
)

target_include_directories(
    mqtt-broker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                       ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    mqtt-broker PUBLIC snodec::mqtt-server mqtt-mapping
                       nlohmann_json::nlohmann_json
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(mqtt-broker PRIVATE -Wno-shadow-field)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

install(TARGETS mqtt-broker RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
