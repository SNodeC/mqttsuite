# MQTTSuite focused implementation-correction tests

cmake_minimum_required(VERSION 3.18)

add_executable(mqtt-mapper-tests MqttMapperTests.cpp)
target_link_libraries(mqtt-mapper-tests PRIVATE mqtt-mapping)
add_test(NAME mqtt-mapper-tests COMMAND mqtt-mapper-tests)

add_executable(mqtt-store-storage-plan-tests StoragePlanTests.cpp)
target_link_libraries(mqtt-store-storage-plan-tests PRIVATE mqtt-store)
add_test(NAME mqtt-store-storage-plan-tests COMMAND mqtt-store-storage-plan-tests)

# SNode.C installs its component libraries in nested directories. The test
# executables link MQTTSuite libraries directly, so their RUNPATH does not cover
# every transitive SNode.C dependency while running from the build tree.
if(UNIX AND NOT APPLE)
    set(_mqttsuite_test_dependency_dirs)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        file(GLOB_RECURSE _mqttsuite_test_dependencies LIST_DIRECTORIES FALSE
            "${_prefix}/${CMAKE_INSTALL_LIBDIR}/snode.c/*.so*")
        foreach(_dependency IN LISTS _mqttsuite_test_dependencies)
            get_filename_component(_dependency_dir "${_dependency}" DIRECTORY)
            list(APPEND _mqttsuite_test_dependency_dirs "${_dependency_dir}")
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _mqttsuite_test_dependency_dirs)
    list(JOIN _mqttsuite_test_dependency_dirs ":" _mqttsuite_test_library_path)

    set_tests_properties(
        mqtt-mapper-tests
        mqtt-store-storage-plan-tests
        PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${_mqttsuite_test_library_path}:$ENV{LD_LIBRARY_PATH}"
    )
endif()
