cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME ft_minecraft)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
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
option(DETERMINISTIC "Try to guaranty deterministic maths" OFF)

list(APPEND SOURCES
    src/Args.cpp
    src/Console.cpp
    src/DataPack.cpp
    src/Engine.cpp
    src/Font.cpp
    src/Input.cpp
    src/main.cpp
    src/Window.cpp
    src/Core/Noise/Simplex.cpp
    src/Core/Class.cpp
    src/Core/Error.cpp
    src/Core/Filesystem.cpp
    src/Core/Ref.cpp
    src/Core/Registry.cpp
    src/Core/String.cpp
    src/Core/StringView.cpp
    src/MP/Manager.cpp
    src/MP/Peer.cpp
    src/MP/Synchronizer.cpp
    src/Platform/Platform.cpp
    src/Physics/Collider.cpp
    src/Physics/PhysicsBody.cpp
    src/Physics/PhysicsSpace.cpp
    src/Render/WebGPU/DriverWebGPU.cpp
    src/Render/Driver.cpp
    src/Render/Graph.cpp
    src/Render/Shader.cpp
    src/Scene/Components/Camera.cpp
    src/Scene/Components/RigidBody.cpp
    src/Scene/Components/Transformed3D.cpp
    src/Scene/Entity.cpp
    src/Scene/Player.cpp
    src/Scene/Scene.cpp
    src/World/Pass/Surface.cpp
    src/World/Chunk.cpp
    src/World/Generator.cpp
    src/World/Registry.cpp
    src/World/Save.cpp
    src/World/World.cpp
)

if (EMSCRIPTEN)
    set(TARGET_IS_WEB YES)
elseif (UNIX AND NOT APPLE)
    set(TARGET_IS_LINUX YES)
elseif(APPLE)
    set(TARGET_IS_MACOS YES)
elseif(WIN32)
    set(TARGET_IS_WINDOWS YES)
endif()

cmake_host_system_information(RESULT DISTRIB_INFO QUERY DISTRIB_NAME)
if (TARGET_IS_LINUX AND DISTRIB_INFO MATCHES "Alpine")
    set(TARGET_IS_ALPINE_LINUX YES)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(TARGET_IS_WEB)
    list(APPEND SOURCES
        src/Platform/Emscripten.cpp
    )
endif()

# if (TARGET_IS_MACOS)
#     # Some Objective-C is called to initialize WebGPU.
#     set_source_files_properties(src/Render/WebGPU/DriverWebGPU.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
# endif()

# Generate the `compile_commands.json`
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Disable exceptions
# target_compile_options(${TARGET_NAME} PUBLIC -fno-exceptions -fno-rtti)

if(DETERMINISTIC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${TARGET_NAME} PUBLIC -ffp-model=precise) # -ffp-contract=off
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TARGET_NAME} PUBLIC -fexcess-precision=standard)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${TARGET_NAME} PUBLIC /fp:precise)
    endif()
endif()

# Enable warnings
target_compile_options(${TARGET_NAME} PUBLIC -Wall -Wextra)

# Enable support for C++20
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# Should prevent FetchContent for trying to connect online after downloading dependencies.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Temporary to analyze the binary
# target_link_options(${TARGET_NAME} PUBLIC -Wl,-Map,ft_minecraft.map)

if (TARGET_IS_LINUX)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_linux)
elseif(TARGET_IS_MACOS)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_macos)
elseif(TARGET_IS_WINDOWS)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_windows)
elseif(TARGET_IS_WEB)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_web)
endif()

if(TARGET_IS_ALPINE_LINUX)
    target_link_libraries(${TARGET_NAME} PRIVATE -static)
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
if(MOLD_FOUND AND NOT TARGET_IS_WEB)
    target_link_options(${TARGET_NAME} PUBLIC -fuse-ld=mold)
endif()

if(TARGET_IS_WEB)
    configure_file(${CMAKE_SOURCE_DIR}/web/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)

    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "--use-port=emdawnwebgpu -sALLOW_MEMORY_GROWTH=1") # --bind -sERROR_ON_WASM_CHANGES_AFTER_LINK -sWASM_BIGINT -O0 -sASYNCIFY=1

    target_link_options(${TARGET_NAME} PRIVATE
            "-Wl,--threads=20"
            "-Wl,--lto-partitions=20"
            "-Wl,--lto-O0"
            "-Wl,--lto-CGO0")

    # Make clangd recognize emscripten paths
    execute_process(COMMAND ${CMAKE_C_COMPILER} --cflags OUTPUT_VARIABLE CLANGD_FLAGS_TO_ADD)
    separate_arguments(CLANGD_FLAGS_TO_ADD UNIX_COMMAND "${CLANGD_FLAGS_TO_ADD}")
    list(JOIN CLANGD_FLAGS_TO_ADD ", " CLANGD_FLAGS_TO_ADD)
    set(CLANGD_TEMPLATE ${CMAKE_SOURCE_DIR}/web/clangd.in)

    configure_file(${CMAKE_SOURCE_DIR}/web/clangd.in ${CMAKE_SOURCE_DIR}/.clangd @ONLY)
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE __has_webgpu)

if (TARGET_IS_LINUX OR TARGET_IS_MACOS)
    target_compile_options(${TARGET_NAME} PRIVATE -fdiagnostics-color)
endif()


#
# Tools
#

# datapacktool
# add_subdirectory(tools/datapack)

add_executable(datapacktool tools/datapack/main.cpp src/DataPack.cpp src/Core/String.cpp src/Core/StringView.cpp)
target_include_directories(datapacktool PRIVATE ./src/)
set_property(TARGET datapacktool PROPERTY CXX_STANDARD 20)
set(DATAPACKTOOL_COMMAND "${CMAKE_BINARY_DIR}/datapacktool")

file(GLOB_RECURSE Assets_FILES
    ${PROJECT_SOURCE_DIR}/assets/blocks/*.json
    ${PROJECT_SOURCE_DIR}/assets/fonts/*
    ${PROJECT_SOURCE_DIR}/assets/textures/*.png
    ${PROJECT_SOURCE_DIR}/assets/shaders/*.slang
    ${PROJECT_SOURCE_DIR}/assets/shaders/**/*.slang

    ${PROJECT_SOURCE_DIR}/assets/shaders/*.comp
    ${PROJECT_SOURCE_DIR}/assets/shaders/**/*.comp
)
string(REPLACE ";" " " "${Assets_FILES}" Assets_FILES)
set(Assets_OUTPUT ${CMAKE_BINARY_DIR}/${TARGET_NAME}.data)

add_custom_command(
    OUTPUT ${Assets_OUTPUT}
    COMMAND ${DATAPACKTOOL_COMMAND} ${Assets_OUTPUT} ${PROJECT_SOURCE_DIR}/assets/ ${Assets_FILES}
    DEPENDS ${Assets_FILES}
)
add_custom_target(datapacktool-generate ALL DEPENDS datapacktool ${Assets_OUTPUT})
add_dependencies(${TARGET_NAME} datapacktool-generate)


# Dont build shared libraries
set(BUILD_SHARED_LIBS 0)

#
# Fetch dependencies
#

# doctest for unit tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5 # v2.4.12
)
FetchContent_MakeAvailable(doctest)
target_link_libraries(${TARGET_NAME} PRIVATE doctest)

if((TARGET_IS_LINUX OR TARGET_IS_MACOS OR TARGET_IS_WINDOWS) AND NOT TARGET_IS_ALPINE_LINUX)
    if (TARGET_IS_LINUX)
        set(WGPU_PLATFORM "linux")
        set(WGPU_ARCH "x86_64")
    elseif (TARGET_IS_MACOS)
        set(WGPU_PLATFORM "macos")
        set(WGPU_ARCH "aarch64")
    elseif(TARGET_IS_WINDOWS)
        set(WGPU_PLATFORM "windows")
        set(WGPU_ARCH "x86_64")
    endif()

    # wgpu-native
    FetchContent_Declare(
        wgpu-native
        URL https://github.com/gfx-rs/wgpu-native/releases/download/v27.0.2.0/wgpu-${WGPU_PLATFORM}-${WGPU_ARCH}-debug.zip
    )
    FetchContent_MakeAvailable(wgpu-native)

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/include)
    target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/lib)
    target_link_libraries(${TARGET_NAME} PRIVATE wgpu_native)
elseif(TARGET_IS_ALPINE_LINUX)
    FetchContent_Declare(
        wgpu-native
        GIT_REPOSITORY https://github.com/gfx-rs/wgpu-native
        GIT_TAG 74f8c24c903b6352d09f1928c56962ce06f77a4d # v27.0.2.0
    )
    FetchContent_MakeAvailable(wgpu-native)

    add_custom_command(
        OUTPUT ${wgpu-native_SOURCE_DIR}/dist/wgpu--debug.zip
        COMMAND make package
        WORKING_DIRECTORY ${wgpu-native_SOURCE_DIR}
    )

    add_custom_command(
        OUTPUT ${wgpu-native_SOURCE_DIR}/dist/lib/libwgpu_native.so
        COMMAND unzip -o wgpu--debug.zip
        WORKING_DIRECTORY ${wgpu-native_SOURCE_DIR}/dist
        DEPENDS ${wgpu-native_SOURCE_DIR}/dist/wgpu--debug.zip
    )

    add_custom_target(wgpu_native_compilations DEPENDS ${wgpu-native_SOURCE_DIR}/dist/lib/libwgpu_native.so)
    add_dependencies(${TARGET_NAME} wgpu_native_compilations)

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/dist/include)
    target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/dist/lib)
    target_link_libraries(${TARGET_NAME} PRIVATE wgpu_native)
endif()

# TODO: Must tell slang to use our Vulkan-Headers or doctest or this will not build
FetchContent_Declare(
    slang
    GIT_REPOSITORY https://github.com/shader-slang/slang
    GIT_TAG b7df3c7aa27301f88e31ed0a7bbf230688adab6a # v2025.14.3
    PATCH_COMMAND git apply ${CMAKE_SOURCE_DIR}/patches/slang-fix-fgetpos64.diff ${CMAKE_SOURCE_DIR}/patches/slang-fix-platform.diff
    UPDATE_DISCONNECTED 1
)

set(SLANG_LIB_TYPE STATIC)
set(SLANG_ENABLE_TESTS OFF)
set(SLANG_ENABLE_EXAMPLES OFF)
set(SLANG_ENABLE_CUDA OFF)
set(SLANG_ENABLE_GFX OFF)
set(SLANG_ENABLE_XLIB OFF)
set(SLANG_ENABLE_SLANGD OFF)
set(SLANG_ENABLE_SLANGC ON)
set(SLANG_ENABLE_SLANGI OFF)
set(SLANG_ENABLE_SLANGRT OFF)
set(SLANG_ENABLE_REPLAYER OFF)
set(SLANG_ENABLE_OPTIX OFF)
set(SLANG_ENABLE_SLANG_RHI OFF)
# set(SLANG_ENABLE_SPLIT_DEBUG_INFO OFF)
set(SLANG_LLVM_FLAVOUR DISABLE)
set(SLANG_HAS_EXCEPTIONS OFF)
set(SLANG_SERIALIZE_FOSSIL_ENABLE_VALIDATION_CHECKS OFF)

# set(SLANG_GENERATORS_PATH ${CMAKE_SOURCE_DIR}/build-emscripten/slang/build/generators/Release/bin)

# if (NOT TARGET_IS_WINDOWS)
#     set(SLANG_ENABLE_DXIL OFF)
# endif()

# if (TARGET_IS_WEB)
#     set(SLANG_ENABLE_SLANG_GLSLANG OFF)
# endif()

# set(SLANG_OVERRIDE_DOCTEST ${doctest_SOURCE_DIR})

FetchContent_MakeAvailable(slang)
target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/slang-src/lib)
target_link_libraries(${TARGET_NAME} PRIVATE slang)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/slang-src/include)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/slang-src/source/core)

# SDL
set(SDL_STATIC ON)
set(SDL_DEPS_SHARED ON)

set(SDL_DUMMYAUDIO OFF)
set(SDL_DUMMYVIDEO OFF)
set(SDL_OPENGL OFF)
set(SDL_OPENGLES OFF)
set(SDL_OPENVR OFF)
set(SDL_KMSDRM OFF)
set(SDL_OFFSCREEN OFF)
set(SDL_RENDER OFF)
set(SDL_CAMERA OFF)
set(SDL_WAYLAND OFF) # Disable wayland for now

if (TARGET_IS_MACOS)
    set(SDL_METAL ON)
endif()


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
    GIT_TAG 9a5d5c45f54b1301ea471622eddede70384243af # v1.92.4
)
FetchContent_MakeAvailable(ImGui)
target_sources(${TARGET_NAME} PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
)
target_include_directories(${TARGET_NAME} PRIVATE ${imgui_SOURCE_DIR})
# target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES VK_NO_PROTOTYPES)

if(TARGET_IS_WEB)
    target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN=1)
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU=1)
endif()

# enet
set(BUILD_SHARED_LIBS 0)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet
    GIT_TAG 8be2368a8001f28db44e81d5939de5e613025023
)
FetchContent_MakeAvailable(enet)
target_link_libraries(${TARGET_NAME} PRIVATE enet)
target_include_directories(${TARGET_NAME} PRIVATE ${enet_SOURCE_DIR}/include)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f
)
FetchContent_MakeAvailable(glm)
target_link_libraries(${TARGET_NAME} PRIVATE glm::glm-header-only)
target_link_libraries(datapacktool PRIVATE glm::glm-header-only)

# FreeType
FetchContent_Declare(
    FreeType
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG 8a152c824ae08fc3459df5c87e10770fc47f80b1
)
FetchContent_MakeAvailable(FreeType)
target_link_libraries(${TARGET_NAME} PRIVATE freetype)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG 55f93686c01528224f448c19128836e7df245f72 # 3.12.0
)
FetchContent_MakeAvailable(json)
target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)

# toml++
set(TOML_EXCEPTIONS OFF)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus
    GIT_TAG e7aaccca3fa3dbde9818ab8313250f3da4976e37
)
FetchContent_MakeAvailable(tomlplusplus)
target_link_libraries(${TARGET_NAME} PRIVATE tomlplusplus::tomlplusplus)

# tracy
if (ENABLE_TRACING)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy
        GIT_TAG 5d542dc09f3d9378d005092a4ad446bd405f819a # v0.11.1
    )
    FetchContent_MakeAvailable(tracy)

    target_link_libraries(${TARGET_NAME} PRIVATE TracyClient)
    target_compile_definitions(${TARGET_NAME} PRIVATE TRACY_ENABLE)
endif()

# libbacktrace
FetchContent_Declare(
    libbacktrace
    GIT_REPOSITORY https://github.com/ianlancetaylor/libbacktrace
    GIT_TAG 793921876c981ce49759114d7bb89bb89b2d3a2d
)
FetchContent_MakeAvailable(libbacktrace)

if(TARGET_IS_LINUX OR TARGET_IS_MACOS)
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

if(RUN_TESTS)
    target_compile_definitions(${TARGET_NAME} PRIVATE DOCTEST_CONFIG_ENABLE=1)
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
