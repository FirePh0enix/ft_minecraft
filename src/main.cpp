#include "Args.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Logger.hpp"
#include "Engine.hpp"
#include "Input.hpp"
#include "Profiler.hpp"

#include <imgui.h>

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

#ifdef __platform_web
#define MAIN(...) __attribute__((used, visibility("default"))) extern "C" int emscripten_main(__VA_ARGS__)
#else
#define MAIN(...) int main(__VA_ARGS__)
#endif

static constexpr double fixed_update_time = 1.0 / 60.0;
static clock_t last_update_time;

// static void shutdown_callback();
// static void loop_update();
// static void update_callback();

MAIN(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) && !defined(__platform_web)
    // NOTE: Address sanitizer mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    Args args;
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});
    args.add_arg("data-dir", {.type = ArgType::String});
    args.parse(argv, argc);

    TracySetThreadName("Main");

    Ref<Engine> engine = newref<Engine>(args);

    if (args.has("data-dir"))
    {
        String s(args.get_arg("data-dir").string);
        s.append("/");
        Filesystem::data_dir = s;
    }

    info("using data directory `{}`", Filesystem::get_data_directory());

    // #ifdef __platform_web
    //     emscripten_set_main_loop_arg([](void *engine)
    //                                  { ((Engine *)engine)->update_callback(); }, nullptr, 0, true);
    // #else
    //     while (engine->is_running())
    //     {
    //         loop_update();
    //     }

    //     shutdown_callback();
    // #endif

    while (engine->is_running())
    {
        const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
        if (elapsed_time >= fixed_update_time)
        {
            FrameMark;

            last_update_time = clock();

            engine->tick(float(fixed_update_time)); // TODO: change to elapsed time or something
            engine->draw(float(fixed_update_time));
            Input::post_events();
        }
    }

    engine = nullptr;

    return 0;
}

// static void loop_update()
// {
//     const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
//     if (elapsed_time > fixed_update_time)
//     {
//         last_update_time = clock();
//         update_callback();
//     }
// }

// static void update_callback()
// {
//     FrameMark;
//     ZoneScoped;

//     // TODO: Differentiate between rendering ticks and physics ticks.
//     engine->tick(float(fixed_update_time));
//     engine->draw();

//     Input::post_events();
// }

// static void shutdown_callback()
// {
//     info("Shutting down");
//     engine = nullptr;
// }
