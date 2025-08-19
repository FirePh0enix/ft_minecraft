cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME ft_minecraft)
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(TARGET_NAME ft_minecraft)

project(${TARGET_NAME})

include(FetchContent)

option(BUILD_DOC "Build documentation" OFF)
option(RUN_TESTS "Run tests" OFF)
option(ENABLE_SHADER_HOTRELOAD "Shader hot reloading" OFF)
option(ENABLE_DEBUG_MENU "Enable an advanced debug menu" OFF)
option(SANITIZE_ADDRESS "Enable AddressSanitizer" OFF)
option(SANITIZE_THREAD "Enable ThreadSanitizer" OFF)

list(APPEND SOURCES
    src/Args.cpp
    src/Config.cpp
    src/DataPack.cpp
    src/Font.cpp
    src/Input.cpp
    src/main.cpp
    src/Window.cpp
    src/Core/Class.cpp
    src/Core/Error.cpp
    src/Core/Filesystem.cpp
    src/Core/Ref.cpp
    src/Platform/Platform.cpp
    src/Render/Driver.cpp
    src/Render/Graph.cpp
    src/Render/Shader.cpp
    src/Scene/Components/Camera.cpp
    src/Scene/Components/RigidBody.cpp
    src/Scene/Entity.cpp
    src/Scene/Player.cpp
    src/Scene/Scene.cpp
    src/World/Chunk.cpp
    src/World/Registry.cpp
    src/World/Save.cpp
    src/World/World.cpp
    src/World/Generation/SimplexNoise.cpp
    src/World/Generation/Terrain.cpp
)

if (EMSCRIPTEN)
    set(TARGET_IS_WEB YES)
elseif (UNIX AND NOT APPLE)
    set(TARGET_IS_LINUX YES)
elseif(APPLE)
    set(TARGET_IS_APPLE YES)
elseif(WIN32)
    set(TARGET_IS_MSVC YES)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(TARGET_IS_WEB)
    list(APPEND SOURCES
        src/Render/WebGPU/DriverWebGPU.cpp
        src/Platform/Emscripten.cpp
    )
else()
    list(APPEND SOURCES
        src/Render/Vulkan/DriverVulkan.cpp
    )
endif()

# Generate the `compile_commands.json`
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Speed-up compilation by precompiling large headers
target_precompile_headers(${TARGET_NAME} PUBLIC src/precompiled.hpp)

# Disable exceptions
target_compile_options(${TARGET_NAME} PUBLIC -fno-exceptions -fno-rtti)

# Enable warnings
target_compile_options(${TARGET_NAME} PUBLIC -Wall -Wextra)

# Enable support for C++20
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# Temporary to analyze the binary
# target_link_options(${TARGET_NAME} PUBLIC -Wl,-Map,ft_minecraft.map)

if (TARGET_IS_LINUX)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_linux)
elseif(TARGET_IS_APPLE)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_macos)
elseif(TARGET_IS_MSVC)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_msvc)
elseif(TARGET_IS_WEB)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_web)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${TARGET_NAME} PRIVATE __DEBUG__)
endif()

if (ENABLE_SHADER_HOTRELOAD)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_shader_hot_reload)
endif()

if (ENABLE_DEBUG_MENU)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_debug_menu)
endif()

if (SANITIZE_ADDRESS)
    target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_address_sanitizer)
endif()

if (SANITIZE_THREAD)
    target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=thread)
    target_link_options(${TARGET_NAME} PRIVATE -fsanitize=thread)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_thread_sanitizer)
endif()

# Use mold when available to speed-up linking
find_program(MOLD mold)
if(MOLD AND NOT TARGET_IS_WEB)
    target_link_options(${TARGET_NAME} PUBLIC -fuse-ld=mold)
endif()

if(TARGET_IS_WEB)
    configure_file(${CMAKE_SOURCE_DIR}/web/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)

    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-sUSE_WEBGPU=1 -sASYNCIFY=1 -sALLOW_MEMORY_GROWTH=1 --preload-file assets --bind")

    # Make clangd recognize emscripten paths
    execute_process(COMMAND ${CMAKE_C_COMPILER} --cflags OUTPUT_VARIABLE CLANGD_FLAGS_TO_ADD)
    separate_arguments(CLANGD_FLAGS_TO_ADD UNIX_COMMAND "${CLANGD_FLAGS_TO_ADD}")
    list(JOIN CLANGD_FLAGS_TO_ADD ", " CLANGD_FLAGS_TO_ADD)
    set(CLANGD_TEMPLATE ${CMAKE_SOURCE_DIR}/web/clangd.in)

    configure_file(${CMAKE_SOURCE_DIR}/web/clangd.in ${CMAKE_SOURCE_DIR}/.clangd @ONLY)
endif()

if(TARGET_IS_LINUX OR TARGET_IS_APPLE OR TARGET_IS_MSVC)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_vulkan)
elseif(TARGET_IS_WEB)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_webgpu)
endif()

if (TARGET_IS_LINUX OR TARGET_IS_APPLE)
    target_compile_options(${TARGET_NAME} PRIVATE -fdiagnostics-color)
endif()


#
# Tools
#

# shadercomp
add_executable(shadercomp tools/shadercomp/main.cpp)
target_include_directories(shadercomp PRIVATE src/)
set_property(TARGET shadercomp PROPERTY CXX_STANDARD 20)

file(GLOB_RECURSE Shaders_FILES
    ${PROJECT_SOURCE_DIR}/assets/shaders/*
)

set(SHADER_BASE_PATHS "")
set(SHADER_OUTPUTS "")

foreach(SHADER_FILE IN LISTS Shaders_FILES)
    cmake_path(REMOVE_EXTENSION SHADER_FILE)
    message(STATUS "${SHADER_FILE}")
    list(APPEND SHADER_BASE_PATHS ${SHADER_FILE})
endforeach()
list(REMOVE_DUPLICATES SHADER_BASE_PATHS)

foreach(SHADER_BASE_PATH ${SHADER_BASE_PATHS})
    file(GLOB_RECURSE SHADER_DEPENDENCIES
        ${SHADER_BASE_PATH}.*
    )

    set(SHADER_OUTPUT_FILE ${SHADER_BASE_PATH}.map.json)
    cmake_path(RELATIVE_PATH SHADER_BASE_PATH BASE_DIRECTORY ${CMAKE_SOURCE_DIR})

    # TODO: .spv on desktop, .wgsl on the web
    list(TRANSFORM SHADER_DEPENDENCIES APPEND ".spv" OUTPUT_VARIABLE SHADER_OUTPUT_FILES)
    list(TRANSFORM SHADER_OUTPUT_FILES PREPEND "${CMAKE_BINARY_DIR}/")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_FILE} ${SHADER_OUTPUT_FILES}
        COMMAND ${CMAKE_BINARY_DIR}/shadercomp ${CMAKE_BINARY_DIR} ${SHADER_BASE_PATH}
        DEPENDS ${SHADER_DEPENDENCIES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT_FILE} ${SHADER_OUTPUT_FILES})
endforeach()
add_custom_target(shaders-generate ALL DEPENDS shadercomp ${SHADER_OUTPUTS})
add_dependencies(${TARGET_NAME} shaders-generate)


# datapacktools
add_executable(datapacktool tools/datapack/main.cpp src/DataPack.cpp)
target_include_directories(datapacktool PRIVATE src/)

file(GLOB_RECURSE Assets_FILES
    ${PROJECT_SOURCE_DIR}/assets/blocks/*.json
    ${PROJECT_SOURCE_DIR}/assets/fonts/*
    ${PROJECT_SOURCE_DIR}/assets/textures/*.png
    ${CMAKE_BINARY_DIR}/assets/shaders/*.spv
    ${CMAKE_BINARY_DIR}/assets/shaders/*.map.json
    ${CMAKE_BINARY_DIR}/assets/shaders/**/*.spv
    ${CMAKE_BINARY_DIR}/assets/shaders/**/*.map.json
)
string(REPLACE ";" " " "${Assets_FILES}" Assets_FILES)
set(Assets_OUTPUT ${CMAKE_BINARY_DIR}/${TARGET_NAME}.data)

add_custom_command(
    OUTPUT ${Assets_OUTPUT}
    COMMAND ${CMAKE_BINARY_DIR}/datapacktool ${Assets_OUTPUT} ${PROJECT_SOURCE_DIR}/ ${Assets_FILES}
    DEPENDS ${Assets_FILES}
)
add_custom_target(datapacktool-generate ALL DEPENDS ${Assets_OUTPUT})
add_dependencies(datapacktool-generate shaders-generate)
add_dependencies(${TARGET_NAME} datapacktool-generate)


# Dont build shared libraries
set(BUILD_SHARED_LIBS 0)

#
# Fetch dependencies
#

if(TARGET_IS_LINUX OR TARGET_IS_APPLE OR TARGET_IS_MSVC)
    # VulkanHeaders
    FetchContent_Declare(
        VulkanHeaders
        URL https://github.com/KhronosGroup/Vulkan-Headers/archive/v1.4.313.tar.gz
    )
    FetchContent_MakeAvailable(VulkanHeaders)
    target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::Headers)
endif()

# tint (Dawn's shader compiler)
set(TINT_BUILD_TESTS OFF)

set(TINT_BUILD_SPV_READER OFF)
set(TINT_BUILD_WGSL_READER ON)

set(TINT_BUILD_GLSL_WRITER OFF)
set(TINT_BUILD_HLSL_WRITER OFF)
set(TINT_BUILD_MSL_WRITER OFF)
set(TINT_BUILD_SPV_WRITER ON)

set(TINT_BUILD_GLSL_VALIDATOR OFF)
set(TINT_BUILD_CMD_TOOLS OFF)

# This disable automatic validation and allow us to perform our own and set capabilities.
set(TINT_ENABLE_IR_VALIDATION OFF)

# abseil
# FetchContent_Declare(
#     abseil
#     GIT_REPOSITORY https://github.com/abseil/abseil-cpp
#     GIT_TAG e430bb9b0833137c4365fee39dfa930657b408fc
# )
# FetchContent_MakeAvailable(abseil)

# set(DAWN_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# set(DAWN_EMDAWNWEBGPU_DIR ${CMAKE_BINARY_DIR}/_deps/dawn-src/third_party/emdawnwebgpu)
# set(DAWN_ABSEIL_DIR ${abseil_SOURCE_DIR})

# SPIRV-Headers
FetchContent_Declare(
    spirvheaders
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
    GIT_TAG origin/vulkan-sdk-1.4.313
)
FetchContent_MakeAvailable(spirvheaders)

# SPIRV-Tools
FetchContent_Declare(
    spirvtools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
    GIT_TAG origin/vulkan-sdk-1.4.313
)
FetchContent_MakeAvailable(spirvtools)

# glslang
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG b888ebcee8ffaa59536b5f33277936536249d9be
)
FetchContent_MakeAvailable(glslang)

target_link_libraries(shadercomp PUBLIC glslang)

# TODO: Replace this with the dawn repository with patches
# set(DAWN_SPIRV_TOOLS_DIR ${spirvtools_SOURCE_DIR})
# set(DAWN_SPIRV_HEADERS_DIR ${spirvheaders_SOURCE_DIR})
# set(DAWN_GLSLANG_DIR ${glslang_SOURCE_DIR})

# FetchContent_Declare(
#     dawn
#     SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/dawn
# )
# FetchContent_MakeAvailable(dawn)

# target_link_libraries(shadercomp PUBLIC tint_api tint_lang_spirv_writer_helpers)

# SDL
set(SDL_STATIC ON)
set(SDL_DEPS_SHARED OFF)

set(SDL_DUMMYAUDIO OFF)
set(SDL_DUMMYVIDEO OFF)
set(SDL_OPENGL OFF)
set(SDL_OPENGLES OFF)
set(SDL_METAL OFF)
set(SDL_OPENVR OFF)
set(SDL_KMSDRM OFF)
set(SDL_OFFSCREEN OFF)
set(SDL_RENDER OFF)
set(SDL_CAMERA OFF)
set(SDL_WAYLAND OFF) # Disable wayland for now

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG 96292a5b464258a2b926e0a3d72f8b98c2a81aa6 # 3.2.20
)
FetchContent_MakeAvailable(SDL)
target_link_libraries(${TARGET_NAME} PUBLIC SDL3::SDL3)

# SDL_image
FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image
    GIT_TAG 1c23b06a13161314f6f33e7ab35a2e5c601f1bc3
)
FetchContent_MakeAvailable(SDL_image)
target_link_libraries(${TARGET_NAME} PUBLIC SDL3_image::SDL3_image)

# ImGui
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG b5a73033ab54009186bb8e4c711e03e6b939cb91
)
FetchContent_MakeAvailable(ImGui)
target_sources(${TARGET_NAME} PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)
target_include_directories(${TARGET_NAME} PRIVATE ${imgui_SOURCE_DIR})
target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES VK_NO_PROTOTYPES)

if(TARGET_IS_LINUX OR TARGET_IS_APPLE OR TARGET_IS_MSVC)
    target_sources(${TARGET_NAME} PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
elseif(TARGET_IS_WEB)
    target_sources(${TARGET_NAME} PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp)
endif()

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f
)
FetchContent_MakeAvailable(glm)
target_link_libraries(${TARGET_NAME} PRIVATE glm::glm-header-only)

# FreeType
FetchContent_Declare(
    FreeType
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG 8a152c824ae08fc3459df5c87e10770fc47f80b1
)
FetchContent_MakeAvailable(FreeType)
target_link_libraries(${TARGET_NAME} PRIVATE freetype)

# simdjson
# FetchContent_Declare(
#     simdjson
#     GIT_REPOSITORY https://github.com/simdjson/simdjson
#     GIT_TAG 0c0ce1bd48baa0677dc7c0945ea7cd1e8b52b297 # v3.13.0
# )
# FetchContent_MakeAvailable(simdjson)
# target_link_libraries(${TARGET_NAME} PRIVATE simdjson) # TODO: Replace by nlohmann/json
# target_link_libraries(shadercomp PRIVATE simdjson)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG 55f93686c01528224f448c19128836e7df245f72 # 3.12.0
)
FetchContent_MakeAvailable(json)
target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(shadercomp PRIVATE nlohmann_json::nlohmann_json)

# tracy
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy
    GIT_TAG 5d542dc09f3d9378d005092a4ad446bd405f819a # v0.11.1
)
FetchContent_MakeAvailable(tracy)
target_link_libraries(${TARGET_NAME} PRIVATE TracyClient)

if (ENABLE_TRACING)
    target_compile_definitions(${TARGET_NAME} PRIVATE TRACY_ENABLE)
endif()

# libbacktrace
FetchContent_Declare(
    libbacktrace
    GIT_REPOSITORY https://github.com/ianlancetaylor/libbacktrace
    GIT_TAG 793921876c981ce49759114d7bb89bb89b2d3a2d
)
FetchContent_MakeAvailable(libbacktrace)

if(TARGET_IS_LINUX) # TARGET_IS_APPLE FIXME: libbacktrace does not compile on macOS
    set(ENV{CC} ${CMAKE_C_COMPILER})
    set(ENV{CXX} ${CMAKE_CXX_COMPILER})

    # Configure
    execute_process(
        OUTPUT_QUIET
        COMMAND ${libbacktrace_SOURCE_DIR}/configure
        WORKING_DIRECTORY ${libbacktrace_BINARY_DIR}
    )

    # Build
    execute_process(
        OUTPUT_QUIET
        COMMAND make
        WORKING_DIRECTORY ${libbacktrace_BINARY_DIR}
    )

    target_link_directories(${TARGET_NAME} PRIVATE ${libbacktrace_BINARY_DIR}/.libs)
    target_link_libraries(${TARGET_NAME} PRIVATE backtrace)
    target_include_directories(${TARGET_NAME} PRIVATE ${libbacktrace_SOURCE_DIR})

    target_compile_definitions(${TARGET_NAME} PRIVATE __has_libbacktrace)
endif()

# MacOS dependencies
if (TARGET_IS_APPLE)
    # MoltenVK
    FetchContent_Declare(
        MoltenVK
        URL https://github.com/KhronosGroup/MoltenVK/releases/download/v1.2.11-artifacts/MoltenVK-macos.tar
    )
    FetchContent_MakeAvailable(MoltenVK)

    configure_file(${moltenvk_SOURCE_DIR}/MoltenVK/dynamic/dylib/macOS/MoltenVK_icd.json ${CMAKE_BINARY_DIR}/vulkan/icd.d/MoltenVK_icd.json COPYONLY)
    configure_file(${moltenvk_SOURCE_DIR}/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib ${CMAKE_BINARY_DIR}/vulkan/icd.d/libMoltenVK.dylib COPYONLY)
endif()

# doctest for unit tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5 # v2.4.12
)
FetchContent_MakeAvailable(doctest)
target_link_libraries(${TARGET_NAME} PRIVATE doctest)

if(NOT RUN_TESTS)
    target_compile_definitions(${TARGET_NAME} PRIVATE DOCTEST_CONFIG_DISABLE=1)
endif()

# Build documention if doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND AND BUILD_DOC)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM )
endif()
