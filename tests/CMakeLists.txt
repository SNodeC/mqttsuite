find_package(Python3 REQUIRED COMPONENTS Interpreter)

foreach(program mqttbroker mqttbridge mqttintegrator mqttstore mqttcli)
    if(TARGET ${program})
        add_test(
            NAME smoke_${program}_help
            COMMAND
                ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/help_smoke.py
                $<TARGET_FILE:${program}>
        )
        set_tests_properties(smoke_${program}_help PROPERTIES TIMEOUT 10)
    endif()
endforeach()

if(TARGET mqttbroker AND CONFIG_MQTTSUITE_BROKER_TCP_IPV4)
    add_test(
        NAME smoke_broker_pubsub
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/broker_smoke.py
            $<TARGET_FILE:mqttbroker>
    )
    set_tests_properties(smoke_broker_pubsub PROPERTIES TIMEOUT 30)
endif()
