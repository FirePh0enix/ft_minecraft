#include "Engine.hpp"
#include "Core/Error.hpp"
#include "Core/Filesystem.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "Profiler.hpp"

#ifdef __has_webgpu
#include "Render/WebGPU/DriverWebGPU.hpp"
#endif

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

static void register_engine_classes();

Engine::Engine(Args args, const std::string& app_name)
    : m_args(args), m_app_name(app_name)
{
#if !defined(__has_address_sanitizer) || !defined(__platform_web)
    // NOTE: Address sanitizer seems the mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    register_engine_classes();

    TracySetThreadName("Main");

    m_window = newobj(Window, "ft_minecraft", 1280, 720);
    Input::init(*m_window);
    Input::load_config();

#ifdef __has_webgpu
    RenderingDriver::create_singleton<RenderingDriverWebGPU>();
#else
#error "Only WebGPU is supported"
#endif

    EXPECT(RenderingDriver::get()->initialize(*m_window, m_args.has("enable-gpu-validation")));
    EXPECT(RenderingDriver::get()->initialize_imgui(*m_window));
}

void Engine::run()
{
#ifdef __platform_web
    emscripten_set_main_loop_arg([](void *engine)
                                 { ((Engine *)engine)->update_callback(); }, this, 0, true);
#else
    while (m_window->is_running())
        iterate();

    if (m_shutdown_callback.callback)
        m_shutdown_callback.callback(this, m_shutdown_callback.user);
#endif
}

void Engine::iterate()
{
    const float elapsed_time = (float)(clock() - m_last_tick_time) / CLOCKS_PER_SEC;

    if (elapsed_time >= m_fixed_update_time)
    {
        m_last_tick_time = clock();
        update_callback();
    }
}

void Engine::update_callback()
{
    FrameMark;
    ZoneScoped;

    std::optional<SDL_Event> event;

    {
        ZoneScopedN("handle events");

        while ((event = m_window->poll_event()))
        {
            switch (event->type)
            {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                m_window->close();
            }
            break;
            case SDL_EVENT_WINDOW_RESIZED:
            {
                Result<> result = RenderingDriver::get()->configure_surface(*m_window, VSync::Off);
                ERR_EXPECT_B(result, "Failed to configure the surface");

                // TODO
                // config.get_category("window").set("width", (int64_t)window->size().width);
                // config.get_category("window").set("height", (int64_t)window->size().height);
                // config.save_to("config.ini");
            }
            break;
            default:
                break;
            }

#ifdef __has_debug_menu
            ImGui_ImplSDL3_ProcessEvent(&*event);

            ImGuiIO& imgui_io = ImGui::GetIO();

            if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
            {
                continue;
            }
#endif

            Input::process_event(event.value());
        }
    }

    RenderingDriver::get()->poll();

#ifdef __has_debug_menu
    {
        // ZoneScopedN("tick.debug_menu");

        // ImGui::NewFrame();

        // if (ImGui::Begin("Commands"))
        // {
        //     ImGui::InputText("##", console.get_buffer(), console.get_buffer_size(), ImGuiInputTextFlags_None);
        //     ImGui::SameLine();
        //     if (ImGui::Button(">"))
        //     {
        //         console.exec();
        //     }
        // }
        // ImGui::End();

        // m_world->imgui_debug_window();
    }
#endif

    // TODO: Differentiate between rendering ticks and physics ticks.
    m_world->tick(float(m_fixed_update_time));

    draw();

    Input::post_events();
}

void Engine::draw()
{
    // The first pass: Depth only
    {
        RenderPassEncoder encoder = RenderGraph::get().render_pass_begin({.name = "depth pass", .depth_attachment = RenderPassDepthAttachment{.save = true}});
        m_world->draw(encoder);
    }

    // The main color pass.
    {
        RenderPassEncoder encoder = RenderGraph::get().render_pass_begin({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{.load = true}});
        m_world->draw(encoder);
    }

    RenderingDriver::get()->draw_graph(RenderGraph::get());
    RenderGraph::get().reset();
}

Args default_args()
{
    Args args;
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});

    return args;
}

static void register_engine_classes()
{
    REGISTER_CLASSES(
        Font,
        Entity, Camera,
        RenderingDriver, Buffer, Texture, Mesh, MaterialBase, Material, ComputeMaterial, Shader);

#ifdef __has_webgpu
    REGISTER_CLASSES(RenderingDriverWebGPU, BufferWebGPU, TextureWebGPU);
#endif

    REGISTER_STRUCTS(
        uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::uvec3, glm::uvec4);
}
