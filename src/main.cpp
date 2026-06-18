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

MAIN(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) && !defined(__platform_web)
    // NOTE: Address sanitizer mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    Option<String> data_dir;
    for (int i = 1; i < argc; i++)
    {
        if (StringView(argv[i]) == "--data-dir" && i + 1 < argc)
            data_dir = Option<String>(argv[++i]);
    }

    TracySetThreadName("Main");

    Ref<Engine> engine = newref<Engine>();

    if (data_dir.has_value())
    {
        String s = data_dir.value();
        s.append("/");
        Filesystem::data_dir = s;
    }

    info("using data directory `{}`", Filesystem::get_data_directory());

    while (engine->is_running())
    {
        const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
        if (elapsed_time >= fixed_update_time)
        {
            FrameMark;

            last_update_time = clock();

            engine->tick(float(fixed_update_time)); // TODO: change to elapsed time or something
            Input::post_events();
        }

        engine->draw(float(fixed_update_time));
    }

    engine = nullptr;

    return 0;
}
