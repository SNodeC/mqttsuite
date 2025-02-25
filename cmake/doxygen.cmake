unset(DOXYGEN_EXECUTABLE CACHE)

find_package(Doxygen)

if(DOXYGEN_FOUND)
    option(
        BUILD_DOCUMENTATION
        "Create and install the HTML based API documentation (requires Doxygen)"
        ${DOXYGEN_FOUND}
    )

    if(BUILD_DOCUMENTATION)
        get_filename_component(
            SNODEC_DOC_ROOTDIR "${CMAKE_SOURCE_DIR}/../mqttbroker-doc/"
            ABSOLUTE
        )

        set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/docs/Doxygen.in)
        set(DOXYFILE ${CMAKE_SOURCE_DIR}/docs/Doxyfile)

        configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

        add_custom_target(
            doc
            COMMAND rm -rf ${SNODEC_DOC_ROOTDIR}/html
            COMMAND rm -f ${SNODEC_DOC_ROOTDIR}/inline_umlgraph_cache_all.pu
            COMMAND mkdir -p ${SNODEC_DOC_ROOTDIR}/html/docs/
            COMMAND cp -a ${CMAKE_SOURCE_DIR}/docs/images
                    ${SNODEC_DOC_ROOTDIR}/html/docs/
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        add_custom_target(
            doc-fast
            COMMAND rm -f ${SNODEC_DOC_ROOTDIR}/inline_umlgraph_cache_all.pu
            COMMAND mkdir -p ${SNODEC_DOC_ROOTDIR}/html/docs/
            COMMAND cp -a ${CMAKE_SOURCE_DIR}/docs/images
                    ${SNODEC_DOC_ROOTDIR}/html/docs/
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif(BUILD_DOCUMENTATION)
else(DOXYGEN_FOUND)
    message(
        WARNING
            " Docygen not found:\n"
            "    Doxygen is used to build the documentation of snode.c in html format.\n"
            "    If you do not intend to build the documentation you can ignore this warning\n"
            "    Otherwise,  you can install doxygen on an debian-style system by executing\n"
            "       sudo apt install doxygen"
    )
endif(DOXYGEN_FOUND)
