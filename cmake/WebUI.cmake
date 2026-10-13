# Static assets are shipped with the source tree so C++ and cross builds do not
# require Node. These explicit targets regenerate them from the imported sources.
function(mqttsuite_web_ui target component)
    set(html_dir "var/www/mqttsuite/${target}")
    target_compile_definitions(
        ${target}
        PRIVATE MQTTSUITE_HTML_DIR="${CMAKE_INSTALL_PREFIX}/${html_dir}"
    )
    file(GLOB_RECURSE assets "${CMAKE_CURRENT_SOURCE_DIR}/html/*")
    target_sources(${target} PRIVATE ${assets})
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/html/"
        DESTINATION "${html_dir}"
        COMPONENT ${component}
    )

    find_program(NPM_EXECUTABLE npm)
    if(NPM_EXECUTABLE)
        add_custom_target(
            ${target}-ui
            COMMAND ${NPM_EXECUTABLE} ci --no-audit --no-fund
            COMMAND ${NPM_EXECUTABLE} run build
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ui"
            COMMENT "Regenerating ${target} HTML assets"
            VERBATIM
        )
    endif()
endfunction()
