#include "Engine.hpp"
#include "Args.hpp"
#include "Core/Types.hpp"
#include "Entity/Player.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "Render/Graph.hpp"
#include "Render/WebGPU/DriverWebGPU.hpp"
#include "imgui.h"

Engine::Engine(const Args& args)
{
    m_window = newobj(Window, "ft_minecraft", 1280, 720);

    Input::init(*m_window);
    Input::load_config();

#ifdef __has_webgpu
    RenderingDriver::create_singleton<RenderingDriverWebGPU>();
#else
#error "WebGPU is the only API supported"
#endif

    (void)RenderingDriver::get()->initialize(*m_window, args.has("enable-gpu-validation"));
    (void)RenderingDriver::get()->initialize_imgui(*m_window);

    // create_world_and_start();
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
                Result<> result = RenderingDriver::get()->configure_surface(event->window.data1, event->window.data2, VSync::On);
                ERR_EXPECT_B(result, "Failed to configure the surface");

                if (m_scene == EngineScene::World)
                    m_world->get_active_camera()->update_projection((float)event->window.data1 / (float)event->window.data2);

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
        break;
    }
}

void Engine::draw_main_menu()
{
    const Extent2D window_size = m_window->size();

#ifdef __has_debug_menu
    {
        RenderPassEncoder encoder = RenderGraph::get().render_pass_begin({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{}});

        ImGui::SetNextWindowPos(ImVec2());
        ImGui::SetNextWindowSize(ImVec2(window_size.width, window_size.height));
        if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration))
        {
            const char *types[] = {"flat", "normal"};
            if (ImGui::Combo("World Type", &m_main_menu_world_type, types, 2))
            {
            }

            if (ImGui::Button("Play"))
            {
                create_world_and_start();
            }
        }
        ImGui::End();

        encoder.imgui();
    }
#endif

    RenderingDriver::get()->draw_graph(RenderGraph::get());
    RenderGraph::get().reset();
}

void Engine::draw_world_scene()
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

#if defined(__has_debug_menu)
        encoder.imgui();
#endif
    }

    RenderingDriver::get()->draw_graph(RenderGraph::get());
    RenderGraph::get().reset();
}

void Engine::create_world_and_start()
{
    uint64_t seed = 1;
    m_world = newobj(World, seed);

    Ref<Camera> camera = newobj(Camera);
    camera->get_transform().position() = glm::vec3(0, 0.85, 0);

    m_player = newobj(Player);
    m_player->get_transform().position() = glm::vec3(0, 15.0, 0);
    m_player->add_child(camera);

    m_world->add_entity(World::overworld, m_player);
    m_world->set_active_camera(camera);

    m_scene = EngineScene::World;
}
