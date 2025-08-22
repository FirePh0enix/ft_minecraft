
function(compile_shaders LOCAL_TARGET_NAME BASE_PATH)
    set(ALL_ARGS ${ARGN})

    # Convert the argument list to the correct format for `shadercomp`.
    set(VARIANT_ARGS "") # command line arguments for `shadercomp`
    set(SOURCE_FILES "") # source files used for dependencies

    set(INDEX 0)
    math(EXPR ARG_COUNT "${ARGC} - 2")
    while(${INDEX} LESS ${ARG_COUNT})
        math(EXPR INDEX "${INDEX} + 1") # skip `VARIANT`, it is only here for readability
        list(GET ARGN ${INDEX} VARIANT_MARKER)

        list(APPEND VARIANT_ARGS --variant)
        set(FIRST_ARG TRUE)

        while (${INDEX} LESS ${ARG_COUNT})
            list(GET ARGN ${INDEX} ARG)

            if (${ARG} STREQUAL "VARIANT")
                # math(EXPR INDEX "${INDEX} - 1")
                break()
            else()
                if (NOT ${FIRST_ARG})
                endif()

                list(APPEND VARIANT_ARGS ${ARG})
                math(EXPR INDEX "${INDEX} + 1")
                set(FIRST_ARG FALSE)
            endif()
        endwhile()

        # math(EXPR INDEX "${INDEX} + 1")
    endwhile()

    set(MAP_FILE ${BASE_PATH}.map.json)
    set(OUTPUT_FILES "")

    foreach(SOURCE_FILE IN LISTS SOURCE_FILES)
        list(TRANSFORM SOURCE_FILE APPEND ".spv")
        list(TRANSFORM SOURCE_FILE PREPEND "${CMAKE_BINARY_DIR}/")
        list(APPEND OUTPUT_FILES ${SOURCE_FILE})
    endforeach()

    add_custom_command(
        OUTPUT ${MAP_FILE} ${OUTPUT_FILES}
        COMMAND ${CMAKE_BINARY_DIR}/shadercomp ${CMAKE_BINARY_DIR} ${BASE_PATH} ${VARIANT_ARGS}
        DEPENDS ${SOURCE_FILES}
    )
    add_custom_target(${LOCAL_TARGET_NAME} ALL DEPENDS shadercomp ${MAP_FILE} ${OUTPUT_FILES})
endfunction()
