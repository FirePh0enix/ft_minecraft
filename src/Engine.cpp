#include "Engine.hpp"
#include "Core/Error.hpp"
#include "Core/Filesystem.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "MP/Peer.hpp"
#include "MP/Synchronizer.hpp"
#include "Scene/Components/MeshInstance.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Visual.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

#ifdef __has_webgpu
#include "Render/WebGPU/DriverWebGPU.hpp"
#endif

#include <tracy/Tracy.hpp>

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

    // After this point reading from the datapack become possible.
    Filesystem::open_data();

    tracy::SetThreadName("Main");

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
                                 { ((Engine *)engine)->loop_callback(); }, this, 0, true);
#else
    while (m_window->is_running())
        loop_callback();

    if (m_shutdown_callback.callback)
        m_shutdown_callback.callback(this, m_shutdown_callback.user);
#endif
}

void Engine::loop_callback()
{
    if ((float)(clock() - m_last_tick_time) / CLOCKS_PER_SEC >= m_fixed_update_time)
    {
        m_last_tick_time = clock();
        update_callback();
    }
}

void Engine::update_callback()
{
    FrameMark;

    Ref<Scene>& scene = Scene::get_active_scene();
    std::optional<SDL_Event> event;

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

    RenderingDriver::get()->poll();

#ifdef __has_debug_menu
    {
        ZoneScopedN("tick.debug_menu");

        ImGui::NewFrame();

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

        scene->imgui_debug_window();
    }
#endif

    // TODO: Differentiate between rendering ticks and physics ticks.
    scene->tick(float(m_fixed_update_time));

    scene->run_systems(EarlyUpdate);
    scene->run_systems(Update);
    scene->run_systems(LateUpdate);

    scene->run_systems(EarlyFixedUpdate);
    scene->run_systems(FixedUpdate);
    scene->run_systems(LateFixedUpdate);

    Input::post_events();
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
        Entity,
        Component, VisualComponent, MeshInstance, RigidBody, Camera, Transformed3D, MPPeer, MPSynchronizer,
        RenderingDriver, Buffer, Texture, Mesh, MaterialBase, Material, ComputeMaterial, Shader);

#ifdef __has_webgpu
    REGISTER_CLASSES(RenderingDriverWebGPU, BufferWebGPU, TextureWebGPU);
#endif

    REGISTER_STRUCTS(
        uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::uvec3, glm::uvec4);
}
