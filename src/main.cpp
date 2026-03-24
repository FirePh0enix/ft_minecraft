#include "Args.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Engine.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Player.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "Render/Driver.hpp"
#include "World/Generator.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"
#include <SDL3_image/SDL_image.h>

#ifdef __has_webgpu
#include "Render/WebGPU/DriverWebGPU.hpp"
#endif

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

#ifdef __platform_web
#define MAIN(...) __attribute__((used, visibility("default"))) extern "C" int emscripten_main(__VA_ARGS__)
#else
#define MAIN(...) int main(__VA_ARGS__)
#endif

#ifndef DOCTEST_CONFIG_ENABLE

static constexpr double fixed_update_time = 1.0 / 60.0;
static clock_t last_update_time;

static void register_engine_classes();
static void register_all_classes();

static void shutdown_callback();
static void loop_update();
static void update_callback();

Ref<Engine> engine;

MAIN(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) || !defined(__platform_web)
    // NOTE: Address sanitizer mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    // String s = "hello world";
    // String s_copy(s);
    // println("{}", s_copy);

    Args args;
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});
    args.add_arg("disable-save", {.type = ArgType::Bool});
    args.add_arg("connect-to", {.type = ArgType::String});
    args.parse(argv, argc);

    register_engine_classes();
    register_all_classes();

    engine = newobj(Engine, args);

    TracySetThreadName("Main");

    BlockRegistry::load_blocks();
    BlockRegistry::create_gpu_resources();

    // TODO: Add this back
    // Font::init_library();

    // auto font_result = Font::create("assets/fonts/Anonymous.ttf", 20);
    // EXPECT(font_result);
    // Ref<Font> font = font_result.value();

    if (args.has("disable-save"))
    {
        // TODO
        info("World won't be saved, `--disable-save` is present.");
    }

#ifdef __platform_web
    emscripten_set_main_loop_arg([](void *engine)
                                 { ((Engine *)engine)->update_callback(); }, nullptr, 0, true);
#else
    while (engine->is_running())
        loop_update();

    shutdown_callback();
#endif

    return 0;
}

static void loop_update()
{
    const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
    if (elapsed_time >= fixed_update_time)
    {
        last_update_time = clock();
        update_callback();
    }
}

static void update_callback()
{
    FrameMark;
    ZoneScoped;

    // TODO: Differentiate between rendering ticks and physics ticks.
    engine->tick(float(fixed_update_time));

    ImGui::NewFrame();
    engine->draw();

    Input::post_events();
}

static void shutdown_callback()
{
    println("Shutting down");

    engine = nullptr;

    BlockRegistry::destroy();
    // Font::deinit_library();

    RenderingDriver::destroy_singleton();
}

static void register_all_classes()
{
    REGISTER_CLASSES(World, Chunk, Block, Player, Generator, GenerationPass, BiomeGenerationPass, SurfaceGenerationPass, FeaturesGenerationPass);
}

static void register_engine_classes()
{
    REGISTER_CLASSES(
        Font,
        Entity, Camera,
        RenderingDriver, Buffer, Texture, Mesh, Material, Shader);

#ifdef __has_webgpu
    REGISTER_CLASSES(RenderingDriverWebGPU, BufferWebGPU, TextureWebGPU);
#endif
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
