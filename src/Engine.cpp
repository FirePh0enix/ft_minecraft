#include "Engine.hpp"
#include "Args.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Entity/Player.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "Render/Graph.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "Render/Types.hpp"
#include "World/World.hpp"
#include "imgui.h"
#include "webgpu/webgpu.h"

#include <backends/imgui_impl_sdl3.h>

Engine::Engine(const Args& args)
{
    singleton = this;
    m_window = EXPECT(newref<Window>("ft_minecraft", 1280, 720));

    Input::init(*m_window);
    Input::load_config();

    InitFlags flags;
    if (args.has("enable-gpu-validation"))
    {
        flags |= InitFlagBits::Validation;
    }

    EXPECT(m_renderer.init(*m_window, flags));

    m_depth_texture = EXPECT(Texture::create(1280, 720, TextureFormat::Depth32, WGPUTextureUsage_RenderAttachment));
    m_color_texture = EXPECT(Texture::create(1280, 720, TextureFormat::RGBA8Srgb, WGPUTextureUsage_RenderAttachment));

    Ref<RenderPassNode> depth_pass = EXPECT(newref<RenderPassNode>());
    depth_pass->set_depth_output(m_depth_texture);

    Ref<RenderPassNode> color_pass = EXPECT(newref<RenderPassNode>());
    color_pass->set_depth_output(m_depth_texture);
    color_pass->set_color_output(m_color_texture);
    color_pass->set_load_depth(true);
    color_pass->set_output_to_surface(true);
    color_pass->set_next(nullptr);

    depth_pass->set_next(color_pass);
    m_graph.set_root(depth_pass);
}

void Engine::tick(float delta)
{
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
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                Result<void> result = Renderer::get().configure_surface(event->window.data1, event->window.data2, VSync::On);
                ERR_EXPECT_B(result, "Failed to configure the surface");

                if (m_scene == EngineScene::World)
                    m_world->get_active_camera()->update_projection((float)event->window.data1 / (float)event->window.data2);
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
    Result<void> res;

    switch (m_scene)
    {
    case EngineScene::MainMenu:
        res = draw_main_menu();
        break;
    case EngineScene::World:
        res = draw_world_scene();
        break;
    }

    if (res.has_error())
    {
        println(stderr, "Draw operation failed with error:");
        res.error().print();
    }
}

Result<void> Engine::draw_main_menu()
{
    const Extent2D window_size = m_window->size();

    m_renderer.draw(m_graph, [this, window_size](const RenderPassNode& node)
                    {
        if (node.is_final_pass())
        {
            const float size_x = (float)window_size.width * 0.4f;
            const float size_y = (float)window_size.height * 0.6f;

            ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, (float)window_size.height / 2 - size_y / 2));
            ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
            if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration))
            {
                imguitk_center_next_widget("Main Menu");
                ImGui::Text("Main Menu");
                ImGui::NewLine();

                const char *types[] = {"flat", "normal"};
                if (ImGui::Combo("World Type", &m_main_menu_world_type, types, 2))
                {
                }

                ImGui::InputText("Seed", m_world_seed_buf, sizeof(m_world_seed_buf));

                imguitk_center_next_widget("Play");
                if (ImGui::Button("Play"))
                {
                    create_world_and_start();
                }
            }
            ImGui::End();
        } });

    return Result<void>();
}

Result<void> Engine::draw_world_scene()
{
    Renderer::get().draw(m_graph, m_world);
    return Result<void>();
}

void Engine::create_world_and_start()
{
    uint64_t seed = StringView(m_world_seed_buf).parse_int<uint64_t>();
    m_world = EXPECT(newref<World>(seed, m_main_menu_world_type));

    m_player = EXPECT(newref<Player>());
    m_player->get_transform().position() = glm::vec3(0, 100.0, 3);
    m_world->add_entity(World::overworld, m_player);

    m_scene = EngineScene::World;
}
