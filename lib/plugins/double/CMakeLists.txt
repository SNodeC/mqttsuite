cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mqtt-mapping-plugin-double SHARED Double.cpp)

target_include_directories(
    mqtt-mapping-plugin-double PUBLIC ${PROJECT_SOURCE_DIR}
)

install(TARGETS mqtt-mapping-plugin-double
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
