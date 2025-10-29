cmake_minimum_required(VERSION 3.20)

if (CMAKE_CROSSCOMPILING)
    # set(_exe "${CMAKE_SOURCE_DIR}/build/datapacktool${CMAKE_HOST_EXECUTABLE_SUFFIX}")
    # add_custom_command(
    #     OUTPUT "${_exe}"
    #     COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING="Release" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH="${CMAKE_BINARY_DIR}" -B "${CMAKE_SOURCE_DIR}/build" -H "${CMAKE_SOURCE_DIR}/tools/datapack"
    #     COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release
    # )
    add_custom_target(datapacktool)
    set(DATAPACKTOOL_COMMAND "${CMAKE_SOURCE_DIR}/build-emscripten/datapacktool/datapacktool" PARENT_SCOPE)
else()
    add_executable(datapacktool main.cpp ../../src/DataPack.cpp)
    target_include_directories(datapacktool PRIVATE ../../src/)
    set_property(TARGET datapacktool PROPERTY CXX_STANDARD 20)
    set(DATAPACKTOOL_COMMAND "${CMAKE_BINARY_DIR}/datapacktool" PARENT_SCOPE)
endif()
