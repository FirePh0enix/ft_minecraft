#include "Args.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Engine.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Player.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "World/Generator.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>

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

static void shutdown_callback();

MAIN(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) || !defined(__platform_web)
    // NOTE: Address sanitizer mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    Args args;
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});
    args.add_arg("disable-save", {.type = ArgType::Bool});
    args.add_arg("connect-to", {.type = ArgType::String});
    args.parse(argv, argc);

    REGISTER_CLASSES(World, Chunk, Block, Player, Generator, GenerationPass, BiomeGenerationPass, SurfaceGenerationPass, FeaturesGenerationPass);
    REGISTER_CLASSES(Entity, Camera);

    TracySetThreadName("Main");

    Engine::get().init(args);

    if (args.has("disable-save"))
    {
        // TODO
        info("World won't be saved, `--disable-save` is present.");
    }

#ifdef __platform_web
    emscripten_set_main_loop_arg([](void *engine)
                                 { Engine::get().tick(float(fixed_update_time)); Engine::get().draw(); }, nullptr, 0, true);
#else
    while (Engine::get().is_running())
    {
        const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
        if (elapsed_time >= fixed_update_time)
        {
            last_update_time = clock();

            Engine::get().tick(float(fixed_update_time));
            Input::post_events();
        }

        Engine::get().draw();
    }

    shutdown_callback();
#endif

    return 0;
}

static void shutdown_callback()
{
    println("Shutting down");

    // BlockRegistry::destroy();
    // Font::deinit_library();

    // RenderingDriver::destroy_singleton();
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
