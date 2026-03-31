#include "Engine.hpp"
#include "Args.hpp"
#include "Core/Types.hpp"
#include "Entity/Player.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "Render/Renderer.hpp"

#include <string>

Engine::Engine()
{
}

void Engine::init(const Args& args)
{
    m_window = newobj(Window, "ft_minecraft", 1280, 720);

    Input::init(*m_window);
    Input::load_config();

    // BlockRegistry::load_blocks();
    // BlockRegistry::create_gpu_resources();

    // #ifdef __has_webgpu
    //     RenderingDriver::create_singleton<RenderingDriverWebGPU>();
    // #else
    // #error "WebGPU is the only API supported"
    // #endif

    // InitFlags flags;
    // if (args.has("enable-gpu-validation"))
    // {
    //     flags |= InitFlagBits::Validation;
    // }

    // (void)RenderingDriver::get()->initialize(*m_window, flags);

    m_renderer.init(RVInitFlagBits::Validation, *m_window);
    create_world_and_start();
}

void Engine::tick(float delta)
{
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
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                Result<> result = m_renderer.configure(*m_window, RVVSync::On); // RenderingDriver::get()->configure_surface(event->window.data1, event->window.data2, VSync::On);
                ERR_EXPECT_B(result, "Failed to configure the surface");

                // if (m_scene == EngineScene::World)
                //     m_world->get_active_camera()->update_projection((float)event->window.data1 / (float)event->window.data2);

                // TODO
                // config.get_category("window").set("width", (int64_t)window->size().width);
                // config.get_category("window").set("height", (int64_t)window->size().height);
                // config.save_to("config.ini");
            }
            break;
            default:
                break;
            }

            ImGui_ImplSDL3_ProcessEvent(&*event);

            ImGuiIO& imgui_io = ImGui::GetIO();

            if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
            {
                continue;
            }

            Input::process_event(event.value());
        }
    }

    // RenderingDriver::get()->poll();

    switch (m_scene)
    {
    case EngineScene::MainMenu:
        break;
    case EngineScene::World:
        m_world->tick(delta);
        break;
    }
}

void Engine::draw()
{
    switch (m_scene)
    {
    case EngineScene::MainMenu:
        draw_main_menu();
        break;
    case EngineScene::World:
        draw_world_scene();
        // draw_world_cpu();
        break;
    }
}

void Engine::draw_main_menu()
{
    const Extent2D window_size = m_window->size();

    m_renderer.prepare_render();

    const float size_x = (float)window_size.width * 0.4f;
    const float size_y = (float)window_size.height * 0.6f;

    ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, (float)window_size.height / 2 - size_y / 2));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration))
    {
        imguitk_centered_next_widget("Main Menu");
        ImGui::Text("Main Menu");
        ImGui::NewLine();

        const char *types[] = {"flat", "normal"};
        if (ImGui::Combo("World Type", &m_main_menu_world_type, types, 2))
        {
        }

        ImGui::InputText("Seed", m_world_seed_buf, sizeof(m_world_seed_buf));

        imguitk_centered_next_widget("Play");
        if (ImGui::Button("Play"))
        {
            create_world_and_start();
        }
    }
    ImGui::End();
}

void Engine::draw_world_scene()
{
    m_renderer.render_world(m_world);
}

void Engine::create_world_and_start()
{
    uint64_t seed = std::stoull(m_world_seed_buf);
    m_world = newobj(World, seed);

    m_player = newobj(Player);
    m_player->get_transform().position() = glm::vec3(0, 0.0, 1);
    m_world->add_entity(World::overworld, m_player);

    m_scene = EngineScene::World;
}
