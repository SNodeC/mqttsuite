# Additional targets to perform cmake-format

find_program(CMAKE_FORMAT "cmake-format")

if(CMAKE_FORMAT)
    file(GLOB_RECURSE CMAKELISTS_TXT_FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt
         ${CMAKE_SOURCE_DIR}/cmake/*.cmake ${CMAKE_SOURCE_DIR}/*.cmake.in
    )

    # Remove strings matching given regular expression from a list.
    # @param(in,out) aItems Reference of a list variable to filter. @param
    # aRegEx Value of regular expression to match.
    function(filter_items aItems aRegEx)
        # For each item in our list
        foreach(item ${${aItems}})
            # Check if our items matches our regular expression
            if("${item}" MATCHES ${aRegEx})
                # Remove current item from our list
                list(REMOVE_ITEM ${aItems} ${item})
            endif("${item}" MATCHES ${aRegEx})
        endforeach(item)
        # Provide output parameter
        set(${aItems}
            ${${aItems}}
            PARENT_SCOPE
        )
    endfunction(filter_items)

    filter_items(CMAKELISTS_TXT_FILES "/json-schema-validator/")

    add_custom_command(
        OUTPUT format-cmds
        APPEND
        COMMAND ${CMAKE_FORMAT} -i ${CMAKELISTS_TXT_FILES}
        COMMENT "Auto formatting of all CMakeLists.txt files"
    )
else(CMAKE_FORMAT)
    message(
        WARNING
            " cmake-format not found:\n"
            "    cmake-format is used to format all CMakeLists.txt files consitently.\n"
            "    It is highly recommented to install it, if you intend to modify the code of SNode.C.\n"
            "    In case you have it installed run \"cmake --target format\" to format all CMakeLists.txt files.\n"
            "    If you do not want to contribute to SNode.C, you can ignore this warning.\n"
    )
endif(CMAKE_FORMAT)
