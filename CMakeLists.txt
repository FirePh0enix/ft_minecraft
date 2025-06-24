cmake_minimum_required(VERSION 3.20)

# Build for the web:
# - Run cmake with `emcmake cmake -B build`

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(TARGET_NAME ft_minecraft)

project(${TARGET_NAME})

include(FetchContent)

option(BUILD_DOC "Build documentation" OFF)
option(RUN_TESTS "Run tests" OFF)
option(ENABLE_SHADER_HOTRELOAD "Shader hot reloading" OFF)
option(SANITIZE_ADDRESS "Enable AddressSanitizer" OFF)
option(SANITIZE_THREAD "Enable AddressSanitizer" OFF)

list(APPEND SOURCES
    src/Core/Class.cpp
    src/Core/Error.cpp
    src/Core/Ref.cpp
    src/Render/Driver.cpp
    src/Render/Graph.cpp
    src/Render/Shader.cpp
    src/Render/WGSLParser.cpp
    src/Scene/Components/Camera.cpp
    src/Scene/Components/RigidBody.cpp
    src/Scene/Entity.cpp
    src/Scene/Player.cpp
    src/Scene/Scene.cpp
    src/Input.cpp
    src/main.cpp
    src/Window.cpp
    src/World/Chunk.cpp
    src/World/Registry.cpp
    src/World/Save.cpp
    src/World/World.cpp
    src/Font.cpp
)

set(RESOURCES_COPY
    assets/fonts/Anonymous.ttf

    assets/textures/Dirt.png
    assets/textures/Grass_Side.png
    assets/textures/Grass_Top.png
    assets/textures/Sand.png
    assets/textures/Stone.png
    assets/textures/Water.png
)

set(RESOURCES_SHADERS
    assets/shaders/font.wgsl
    assets/shaders/voxel.wgsl
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
        src/Render/DriverWebGPU.cpp
        src/Emscripten.cpp
    )
else()
    list(APPEND SOURCES
        src/Render/DriverVulkan.cpp
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
if(NOT TARGET_IS_WEB)
    set(DAWN_BUILD_SAMPLES OFF)
    set(DAWN_BUILD_TESTS OFF)

    set(DAWN_USE_GLFW OFF)

    set(DAWN_ENABLE_D3D11 OFF)
    set(DAWN_ENABLE_D3D12 OFF)
    set(DAWN_ENABLE_METAL OFF)
    set(DAWN_ENABLE_NULL OFF)
    set(DAWN_ENABLE_DESKTOP_GL OFF)
    set(DAWN_ENABLE_OPENGLES OFF)
    set(DAWN_ENABLE_VULKAN OFF)

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
    FetchContent_Declare(
        abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp
        GIT_TAG e430bb9b0833137c4365fee39dfa930657b408fc
    )
    FetchContent_MakeAvailable(abseil)

    # set(DAWN_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    # set(DAWN_EMDAWNWEBGPU_DIR ${CMAKE_BINARY_DIR}/_deps/dawn-src/third_party/emdawnwebgpu)
    set(DAWN_ABSEIL_DIR ${abseil_SOURCE_DIR})

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

    set(DAWN_SPIRV_TOOLS_DIR ${spirvtools_SOURCE_DIR})
    set(DAWN_SPIRV_HEADERS_DIR ${spirvheaders_SOURCE_DIR})
    set(DAWN_GLSLANG_DIR ${glslang_SOURCE_DIR})

    FetchContent_Declare(
        dawn
        GIT_REPOSITORY https://dawn.googlesource.com/dawn
        GIT_TAG 4d33621217632e25d8e88e2f0dd2544596d129ac
        # This line will prevent cmake from fetching submodules, instead we fetch only the required dependencies
        # with FetchContent.
        GIT_SUBMODULES ""
    )
    FetchContent_MakeAvailable(dawn)

    target_link_libraries(${TARGET_NAME} PRIVATE tint_api tint_lang_spirv_writer_helpers)
endif()

# SDL
set(SDL_STATIC ON)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG 8aa5b97bb5be8e8f62fd76c3181439e20f3de5e9
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

if(TARGET_IS_LINUX OR TARGET_IS_APPLE)
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

# foreach(RESOURCE IN LISTS RESOURCES_COPY)
#     configure_file(${CMAKE_SOURCE_DIR}/${RESOURCE} ${CMAKE_BINARY_DIR}/${RESOURCE} COPYONLY)
# endforeach()
