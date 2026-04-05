# MQTTSuite - MQTT Orchestration Engine

cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(nlohmann_json 3.7.0 REQUIRED)
find_package(
    snodec 1.0.0 REQUIRED
    COMPONENTS http-server-express-legacy-in http-server-express-tls-in
)

add_subdirectory(lib)

set(MQTTORCHESTRATOR_CPP mqttorchestrator.cpp)
set(MQTTORCHESTRATOR_H)

add_executable(mqttorchestrator ${MQTTORCHESTRATOR_CPP} ${MQTTORCHESTRATOR_H})

file(
    GLOB_RECURSE
    WEB_ASSETS
    "${CMAKE_CURRENT_SOURCE_DIR}/html/*.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/html/*.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/html/*.js"
)

target_sources(mqttorchestrator PRIVATE ${WEB_ASSETS})

target_link_libraries(
    mqttorchestrator
    PUBLIC snodec::http-server-express-legacy-in
           snodec::http-server-express-tls-in mqtt-orchestrator
)

install(TARGETS mqttorchestrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/var/www/mqttsuite/mqttorchestrator
    FILES_MATCHING
    PATTERN "*.html"
    PATTERN "*.css"
    PATTERN "*.js"
)
