# MQTTSuite focused implementation-correction tests

cmake_minimum_required(VERSION 3.18)

add_executable(mqtt-mapper-tests MqttMapperTests.cpp)
target_link_libraries(mqtt-mapper-tests PRIVATE mqtt-mapping)
add_test(NAME mqtt-mapper-tests COMMAND mqtt-mapper-tests)

add_executable(mqtt-store-storage-plan-tests StoragePlanTests.cpp)
target_link_libraries(mqtt-store-storage-plan-tests PRIVATE mqtt-store)
add_test(NAME mqtt-store-storage-plan-tests COMMAND mqtt-store-storage-plan-tests)
