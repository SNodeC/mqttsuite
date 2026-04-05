add_library(mqtt-orchestrator Engine.cpp)

target_link_libraries(
    mqtt-orchestrator
    PUBLIC nlohmann_json::nlohmann_json
)

target_include_directories(
    mqtt-orchestrator
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
