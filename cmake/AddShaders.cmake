function(add_shaders CURRENT_TARGET_NAME)
    set(SHADER_SOURCE_FILES ${ARGN})

    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "Cannot create a shaders target without any source files")
    endif()

    set(SHADER_PRODUCTS)

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)
        cmake_path(GET SHADER_SOURCE PARENT_PATH SHADER_DIRECTORY)

        set(SHADER_PRODUCT "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders/${SHADER_NAME}.spv")

        list(APPEND SHADER_PRODUCTS ${SHADER_PRODUCT})

        add_custom_command(
            OUTPUT ${SHADER_PRODUCT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders/${SHADER_DIRECTORY}"
            COMMAND "glslc" "--target-env=vulkan1.2" "${SHADER_SOURCE}" "-o" "${SHADER_PRODUCT}"
            DEPENDS ${SHADER_SOURCE}
        )
    endforeach()

    add_custom_target(${CURRENT_TARGET_NAME} DEPENDS ${SHADER_PRODUCTS})
endfunction()
