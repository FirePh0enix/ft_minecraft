#include "Engine.hpp"

#include "Audio/AudioMixer.hpp"
#include "Audio/MusicPlayer.hpp"
#include "Core/Error.hpp"
#include "Entity/Cow.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Player.hpp"
#include "Entity/Zombie.hpp"
#include "Input.hpp"
#include "Network/LocalServer.hpp"
#include "Network/RemoteServer.hpp"
#include "Profiler.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <cstdlib>
#include <imgui.h>

#include <cstddef>
#include <ctime>
#include <memory>

#define WINDOW_INIT_WIDTH 1920
#define WINDOW_INIT_HEIGHT 1080

Engine::Engine(bool disable_save)
    : m_disable_save(disable_save), m_thread_pool(std::max<ssize_t>(std::thread::hardware_concurrency() - 3, 1)), m_mesh_thread_pool(std::min<ssize_t>(2, std::thread::hardware_concurrency()))
{
    singleton = this;
    m_window = std::make_shared<Window>("ft_minecraft", WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT);
    m_audio_mixer = std::make_unique<AudioMixer>();
    m_music_player = std::make_unique<MusicPlayer>(*m_audio_mixer);

    Input::init(*m_window);
    Input::load_config();

    InitFlags flags = InitFlagBits::Validation;

    register_entities(); // TODO: put this in GameRegistry
    register_recipes();

    EXPECT(m_renderer.init(*m_window, flags));

    EXPECT(Font::init_library());
    m_font = EXPECT(Font::create("data/fonts/Anonymous.ttf", 64));

    go_to_main_menu();
}

Engine::~Engine()
{
    Font::deinit_library();
}

void Engine::register_entities()
{
    Entity::bind_methods();
    Player::bind_methods();

    m_entity_registry.register_entity<Player>();
    m_entity_registry.register_entity<Cow>();
    m_entity_registry.register_entity<Zombie>();
}

// TODO: Create a helper for creating recipe maybe ?
// TODO: YAML/JSON should do the trick.
void Engine::register_recipes()
{
    Recipe crafting_table;

    crafting_table.width = 2;
    crafting_table.height = 2;

    for (size_t i = 0; i < 9; i++)
        crafting_table.pattern[i] = Id<Item>();

    crafting_table.pattern[0 + 0 * 3] = Items::stone;
    crafting_table.pattern[1 + 0 * 3] = Items::stone;
    crafting_table.pattern[0 + 1 * 3] = Items::stone;
    crafting_table.pattern[1 + 1 * 3] = Items::stone;

    crafting_table.result = ItemStack(Items::crafting_table_block, 1);

    m_registry.add_recipe(crafting_table);
}

void Engine::tick(float delta)
{
    ZoneScoped;

    if (m_switch_to_main_menu)
    {
        m_server = nullptr;
        m_menu = std::bind(&Engine::main_menu_gui, this);
        m_switch_to_main_menu = false;
    }

    std::optional<SDL_Event> event_opt;

    {
        ZoneScopedN("handle events");

        while ((event_opt = m_window->poll_event()))
        {
            SDL_Event event = event_opt.value();

            switch (event.type)
            {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                m_window->close();
            }
            break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                const uint32_t w = event.window.data1;
                const uint32_t h = event.window.data2;
                Renderer::get().configure_surface(w, h);

                if (m_server != nullptr)
                {
                    std::shared_ptr<Player> player = m_server->get_player();
                    if (player != nullptr)
                        player->get_camera()->update_projection((float)w / (float)h);
                }
            }
            break;
            case SDL_EVENT_KEY_DOWN:
            {
                Event event2(event);
                if (m_server != nullptr)
                {
                    std::shared_ptr<Player> player = m_server->get_player();
                    if (player != nullptr)
                        player->process_event(event2);
                }
            }
            break;
            case SDL_EVENT_TEXT_INPUT:
            {
                Event event2(event);
                if (m_server != nullptr)
                {
                    std::shared_ptr<Player> player = m_server->get_player();
                    if (player != nullptr)
                        player->process_event(event2);
                }
            }
            break;
            default:
                break;
            }

            if (m_menu != nullptr)
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
                ImGuiIO& imgui_io = ImGui::GetIO();

                if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
                    continue;
            }

            Input::process_event(event);
        }
    }

    m_current_tps++;
    if (m_last_second_timer_time >= 1.0)
    {
        m_current_memory_usage = 0; // TODO
        m_tps = m_current_tps;
        m_current_tps = 0;
        m_last_second_timer_time -= 1.0;
    }
    m_last_second_timer_time += delta;

    if (m_server != nullptr && m_menu == nullptr)
    {
        m_server->tick();
    }

    Input::post_events();

    if (m_time_pass)
    {
        m_ticks_since_start_of_day += 1 * m_tick_scale;
        if (m_ticks_since_start_of_day > ticks_per_day)
            m_ticks_since_start_of_day = 0;
    }

    m_music_player->update(delta);
}

void Engine::draw(float delta)
{
    ZoneScoped;

    if (m_menu != nullptr)
    {
        m_renderer.draw_ui([this](const RenderPass&)
                           { m_menu(); });
    }
    else if (m_server != nullptr && m_server->get_world() != nullptr)
    {
        std::shared_ptr<World> world = m_server->get_world();
        m_renderer.draw_forward(world);
    }

    m_current_fps++;
    if (m_last_second_frame_time >= 1.0)
    {
        m_fps = m_current_fps;
        m_current_fps = 0;
        m_last_second_frame_time -= 1.0;
    }
    m_last_second_frame_time += delta;
}

double Engine::time()
{
    struct timespec tp{};
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (double)(tp.tv_nsec + tp.tv_sec * 1000000000) / 1000000000.0;
}

void Engine::go_to_main_menu()
{
    m_switch_to_main_menu = true;
}

void Engine::main_menu_gui()
{
    const Extent2D window_size = m_window->size();
    const float size_x = (float)window_size.width * 0.4f;
    const float size_y = (float)window_size.height * 0.6f;

    ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, (float)window_size.height / 2 - size_y / 2));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    if (ImGui::Begin("Menu"))
    {
        imguitk_center_next_widget("Hello world");
        ImGui::Text("Hello world");

        ImGui::InputText("Username", m_username_buf, 32);

        const char *items[] = {"One", "Two", "Three"};

        if (ImGui::ListBox("Saves", &m_current_save, items, 3))
        {
        }

        ImGui::InputText("Name", m_name_buf, 32);
        ImGui::InputText("Seed", m_seed_buf, 32);

        imguitk_center_next_widget("Load");
        if (ImGui::Button("Load"))
        {
            m_server = std::make_shared<LocalServer>(m_username_buf, m_name_buf, std::atoll(m_seed_buf), m_should_create_online);
            m_server->start();
            m_current_target = RpcTarget::Server;

            m_menu = nullptr;
        }
        ImGui::SameLine();
        if (ImGui::Button("Create"))
        {
            m_server = std::make_shared<LocalServer>(m_username_buf, m_name_buf, std::atoll(m_seed_buf), m_should_create_online);
            m_server->start();
            m_current_target = RpcTarget::Server;

            m_menu = nullptr;
        }

        ImGui::SameLine();
        if (ImGui::Checkbox("Online", &m_should_create_online))
        {
        }

        ImGui::InputText("Ip", m_ip_buf, 32);

        imguitk_center_next_widget("Join");
        if (ImGui::Button("Join"))
        {
            m_server = std::make_shared<RemoteServer>(m_username_buf, m_ip_buf, NetworkConnection::default_port);
            m_server->start();
            m_current_target = RpcTarget::Client;

            m_menu = nullptr;
        }
    }
    ImGui::End();
}
