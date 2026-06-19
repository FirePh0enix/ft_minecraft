cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME ft_minecraft)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

set(TARGET_NAME ft_minecraft)

project(${TARGET_NAME})

include(CheckLibraryExists)
include(FetchContent)

option(BUILD_DOC "Build documentation" OFF)
option(ENABLE_TRACING "Enable tracing using Tracy" OFF)
option(SANITIZE_ADDRESS "Enable AddressSanitizer" OFF)
option(SANITIZE_THREAD "Enable ThreadSanitizer" OFF)
option(DETERMINISTIC "Try to guaranty deterministic maths" OFF)

list(APPEND SOURCES
    src/Console.cpp
    src/Engine.cpp
    src/Font.cpp
    src/Input.cpp
    src/Model.cpp
    src/stb_image.cpp
    src/Type.cpp
    src/Variant.cpp
    src/Window.cpp
    src/Block/Block.cpp
    src/Block/CraftingTable.cpp
    src/Core/Noise/Simplex.cpp
    src/Core/Alloc.cpp
    src/Core/Class.cpp
    src/Core/Error.cpp
    src/Core/Filesystem.cpp
    src/Core/IO.cpp
    src/Core/Ref.cpp
    src/Core/String.cpp
    src/Core/StringView.cpp
    src/Core/ThreadPool.cpp
    src/Entity/Camera.cpp
    src/Entity/Entity.cpp
    src/Entity/Item.cpp
    src/Entity/LivingEntity.cpp
    src/Entity/Mob.cpp
    src/Entity/Cow.cpp
    src/Entity/Zombie.cpp
    src/Entity/Player.cpp
    src/Entity/Pathfinding/Pathfinding.cpp
    src/Inventory/CraftingTable.cpp
    src/Inventory/Inventory.cpp
    src/Inventory/PlayerInventory.cpp
    src/Item/Bucket.cpp
    src/Item/Item.cpp
    src/Item/ItemStack.cpp
    src/Network/Network.cpp
    src/Network/Packet.cpp
    src/Render/Shader.cpp
    src/Render/Renderer.cpp
    src/UI/Chat.cpp
    src/UI/ColorRect.cpp
    src/UI/ItemSlot.cpp
    src/UI/Label.cpp
    src/UI/TextureRect.cpp
    src/World/Pass/Flat.cpp
    src/World/Pass/Overworld.cpp
    src/World/Chunk.cpp
    src/World/Dimension.cpp
    src/World/Generator.cpp
    src/World/Registry.cpp
    src/World/World.cpp
)

list(APPEND SOURCES_TEST
    tests/class.cpp
    tests/HashMap.cpp
    tests/main.cpp
    tests/Option.cpp
    tests/Ref.cpp
    tests/Result.cpp
    tests/Variant.cpp
    tests/Vector.cpp
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

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(TARGET_IS_WEB)
    list(APPEND SOURCES
        src/Platform/Emscripten.cpp
    )
endif()

# Generate the `compile_commands.json`
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${TARGET_NAME} src/main.cpp ${SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(${TARGET_NAME}_test ${SOURCES} ${SOURCES_TEST})
target_include_directories(${TARGET_NAME}_test PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)

# Disable exceptions
target_compile_options(${TARGET_NAME} PUBLIC -fno-exceptions -fno-rtti)

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
set_property(TARGET ${TARGET_NAME}_test PROPERTY CXX_STANDARD 20)

# Should prevent FetchContent for trying to connect online after downloading dependencies.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Temporary to analyze the binary
# target_link_options(${TARGET_NAME} PUBLIC -Wl,-Map,ft_minecraft.map)

if (TARGET_IS_LINUX)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_linux)
    target_compile_definitions(${TARGET_NAME}_test PRIVATE __platform_linux)
elseif(TARGET_IS_MACOS)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_macos)
    target_compile_definitions(${TARGET_NAME}_test PRIVATE __platform_macos)
elseif(TARGET_IS_WINDOWS)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_windows)
    target_compile_definitions(${TARGET_NAME}_test PRIVATE __platform_windows)
elseif(TARGET_IS_WEB)
    target_compile_definitions(${TARGET_NAME} PRIVATE __platform_web)
    target_compile_definitions(${TARGET_NAME}_test PRIVATE __platform_web)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${TARGET_NAME} PRIVATE __DEBUG__)
    target_compile_definitions(${TARGET_NAME}_test PRIVATE __DEBUG__)
endif()

if (SANITIZE_ADDRESS)
    target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    target_compile_definitions(${TARGET_NAME} PRIVATE __has_address_sanitizer)
endif()

# Always sanitize for tests.
target_compile_options(${TARGET_NAME}_test PRIVATE -fsanitize=address)
target_link_options(${TARGET_NAME}_test PRIVATE -fsanitize=address)
target_compile_definitions(${TARGET_NAME}_test PRIVATE __has_address_sanitizer)

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
# Fetch dependencies
#

# Dont build shared libraries
set(BUILD_SHARED_LIBS 0)

# doctest for unit tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG 1da23a3e8119ec5cce4f9388e91b065e20bf06f5 # v2.4.12
)
FetchContent_MakeAvailable(doctest)
target_link_libraries(${TARGET_NAME}_test PRIVATE doctest)

# set(WGVK_WGSL_SUPPORT "SIMPLE_WGSL")
# FetchContent_Declare(
#     wgvk
#     GIT_REPOSITORY https://github.com/manuel5975p/WGVK
#     GIT_TAG 76a6b7760cdf5acfeb2d1593982990045199565a # master
# )
# FetchContent_MakeAvailable(wgvk)
# target_link_libraries(${TARGET_NAME} PRIVATE wgvk)
# target_link_libraries(${TARGET_NAME}_test PRIVATE wgvk)

if(TARGET_IS_LINUX OR TARGET_IS_MACOS OR TARGET_IS_WINDOWS)
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

    set(WGPU_BUILD "release")

    # wgpu-native
    FetchContent_Declare(
        wgpu-native
        URL https://github.com/gfx-rs/wgpu-native/releases/download/v29.0.0.0/wgpu-${WGPU_PLATFORM}-${WGPU_ARCH}-${WGPU_BUILD}.zip
    )
    FetchContent_MakeAvailable(wgpu-native)

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/include)
    target_include_directories(${TARGET_NAME}_test PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/include)

    target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/lib)
    target_link_directories(${TARGET_NAME}_test PRIVATE ${CMAKE_BINARY_DIR}/_deps/wgpu-native-src/lib)

    target_link_libraries(${TARGET_NAME} PRIVATE wgpu_native)
    target_link_libraries(${TARGET_NAME}_test PRIVATE wgpu_native)
endif()

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
target_link_libraries(${TARGET_NAME}_test PUBLIC SDL3::SDL3)

# ImGui
# FetchContent_Declare(
#     ImGui
#     GIT_REPOSITORY https://github.com/ocornut/imgui
#     GIT_TAG dac07199cfd761113d966eb8ad739254e10df2fe # v1.92.7
# )
# FIXME: when ImGui includes this patch, we can use the official repository again
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/lucascompython/imgui
    GIT_TAG 2246cd09dbdba3153a75c2fa750dc9ea7f45830e
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
target_sources(${TARGET_NAME}_test PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
)
target_include_directories(${TARGET_NAME} PRIVATE ${imgui_SOURCE_DIR})
target_include_directories(${TARGET_NAME}_test PRIVATE ${imgui_SOURCE_DIR})

if (TARGET_IS_MACOS)
    set_source_files_properties(${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

if(TARGET_IS_WEB)
    target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN=1)
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGVK=1)
    target_compile_definitions(${TARGET_NAME}_test PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGVK=1)
endif()

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb)
target_include_directories(${TARGET_NAME} PRIVATE ${stb_SOURCE_DIR})
target_include_directories(${TARGET_NAME}_test PRIVATE ${stb_SOURCE_DIR})

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

target_link_libraries(${TARGET_NAME}_test PRIVATE enet)
target_include_directories(${TARGET_NAME}_test PRIVATE ${enet_SOURCE_DIR}/include)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f
)
FetchContent_MakeAvailable(glm)
target_link_libraries(${TARGET_NAME} PRIVATE glm::glm-header-only)
target_link_libraries(${TARGET_NAME}_test PRIVATE glm::glm-header-only)

# FreeType
FetchContent_Declare(
    FreeType
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG 8a152c824ae08fc3459df5c87e10770fc47f80b1
)
FetchContent_MakeAvailable(FreeType)
target_link_libraries(${TARGET_NAME} PRIVATE freetype)
target_link_libraries(${TARGET_NAME}_test PRIVATE freetype)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG 55f93686c01528224f448c19128836e7df245f72 # 3.12.0
)
FetchContent_MakeAvailable(json)
target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${TARGET_NAME}_test PRIVATE nlohmann_json::nlohmann_json)

# tracy
if (ENABLE_TRACING)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy
        GIT_TAG 00a069d6088ff8d93304eaac4d925cece0e9081c # master (v0.13.x)
    )
    FetchContent_MakeAvailable(tracy)

    target_link_libraries(${TARGET_NAME} PRIVATE TracyClient)
    target_compile_definitions(${TARGET_NAME} PRIVATE TRACY_ENABLE)
endif()

# libbacktrace
check_library_exists(backtrace backtrace_create_state "" HAVE_SYSTEM_LIBBACKTRACE)

if(TARGET_IS_LINUX)
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
endif()

target_compile_definitions(${TARGET_NAME}_test PRIVATE DOCTEST_CONFIG_ENABLE=1)
