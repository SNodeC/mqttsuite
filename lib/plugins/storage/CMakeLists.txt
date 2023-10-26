cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mqtt-mapping-plugin-storage SHARED Storage.cpp Storage.h)

target_include_directories(mqtt-mapping-plugin-storage PUBLIC ${PROJECT_SOURCE_DIR})

install(TARGETS mqtt-mapping-plugin-storage RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
