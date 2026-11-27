# Evaluated by CPack after release/version overrides have been applied.
list(APPEND CPACK_SOURCE_IGNORE_FILES "/node_modules/" "/[.]angular/")
set(CPACK_IGNORE_FILES "${CPACK_SOURCE_IGNORE_FILES}")
set(names "${CPACK_PACKAGE_NAME}\n")
set(full_requires "")
foreach(component IN LISTS CPACK_COMPONENTS_ALL)
    if(NOT component STREQUAL "full")
        string(TOLOWER "${CPACK_PACKAGE_NAME}-${component}" name)
        string(TOUPPER "${component}" upper)
        string(APPEND names "${name}\n")
        set(CPACK_RPM_${upper}_PACKAGE_NAME "${name}")
        list(APPEND full_requires
             "${name} = ${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}"
        )
    endif()
endforeach()
list(JOIN full_requires ", " CPACK_RPM_FULL_PACKAGE_REQUIRES)
set(CPACK_DEBIAN_PACKAGE_BREAKS
    "${CPACK_PACKAGE_NAME} (<< ${CPACK_PACKAGE_VERSION})"
)
set(CPACK_DEBIAN_PACKAGE_REPLACES
    "${CPACK_PACKAGE_NAME} (<< ${CPACK_PACKAGE_VERSION})"
)
# Publication inventory comes from the same component model as the packages.
# Exact native dependencies record the wave's frozen build inputs.
if(DEFINED ENV{DEPENDENCIES_JSON})
    set(dependencies "$ENV{DEPENDENCIES_JSON}")
    string(JSON count LENGTH "${dependencies}")
    if(NOT count EQUAL 1)
        message(FATAL_ERROR "Expected the resolved snodec dependency")
    endif()
    string(JSON package GET "${dependencies}" 0 package)
    string(JSON version GET "${dependencies}" 0 version)
    string(JSON revision GET "${dependencies}" 0 revision)
    if(NOT package STREQUAL "snodec")
        message(FATAL_ERROR "Unexpected resolved dependency: ${package}")
    endif()
    if(CPACK_GENERATOR STREQUAL "DEB")
        string(JSON codename GET "$ENV{TARGET_JSON}" codename)
        set(bound "snodec (= ${version}-${revision}~${codename})")
        set(variables CPACK_DEBIAN_PACKAGE_DEPENDS)
    else()
        string(JSON disttag GET "$ENV{TARGET_JSON}" disttag)
        set(bound "snodec = ${version}-${revision}.${disttag}")
        set(variables CPACK_RPM_PACKAGE_REQUIRES
                      CPACK_RPM_FULL_PACKAGE_REQUIRES
        )
    endif()
    foreach(variable IN LISTS variables)
        if(NOT "${${variable}}" STREQUAL "")
            string(APPEND ${variable} ", ")
        endif()
        string(APPEND ${variable} "${bound}")
    endforeach()
endif()
file(MAKE_DIRECTORY "${CPACK_OUTPUT_FILE_PREFIX}")
file(WRITE "${CPACK_OUTPUT_FILE_PREFIX}/${CPACK_PACKAGE_NAME}.packages"
     "${names}"
)
