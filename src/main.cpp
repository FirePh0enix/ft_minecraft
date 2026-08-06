#include "Core/Filesystem.hpp"
#include "Core/Logger.hpp"
#include "Engine.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "UI/Widget.hpp"

#include <imgui.h>

static constexpr double fixed_update_time = 1.0 / 60.0;
static clock_t last_update_time;

int main(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) && !defined(__platform_web)
    // NOTE: Address sanitizer mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    bool disable_save = false;
    for (int i = 0; i < argc; i++)
    {
        if (std::string_view(argv[i]) == "--disable-save")
            disable_save = true;
    }

    TracySetThreadName("Main");

    Engine engine(disable_save);

    Widget::bind_static();
    ColorRectWidget::bind_static();
    TextureRectWidget::bind_static();
    LabelWidget::bind_static();

    info("using data directory `{}`", Filesystem::get_data_directory());
    if (disable_save)
    {
        info("save are disabled, modification will not be saved");
    }

    while (engine.is_running())
    {
        const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
        if (elapsed_time >= fixed_update_time)
        {
            FrameMark;
            last_update_time = clock();

            engine.tick(float(fixed_update_time)); // TODO: change to elapsed time or something
            // engine.draw(float(fixed_update_time));
            Input::post_events();
        }

        engine.draw(float(fixed_update_time));
    }

    return 0;
}
