#include "Engine.hpp"

#include "Audio/AudioMixer.hpp"
#include "Audio/MusicPlayer.hpp"
#include "Core/Error.hpp"
#include "Core/Filesystem.hpp"
#include "Entity/Arrow.hpp"
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
    : m_disable_save(disable_save), m_thread_pool(4), m_mesh_thread_pool(2)
{
    singleton = this;
}

std::expected<void, Error> Engine::init()
{
    m_window = std::make_shared<Window>("ft_minecraft", WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT);
    TRY_ASSIGN(m_audio_mixer, AudioMixer::create());
    m_music_player = std::make_unique<MusicPlayer>(*m_audio_mixer);

    Input::init(*m_window);
    Input::load_config();

    InitFlags flags = InitFlagBits::Validation;

    register_entities(); // TODO: put this in GameRegistry
    register_recipes();

    TRY(m_renderer.init(*m_window, flags));

    TRY(Font::init_library());
    TRY_ASSIGN(m_font, Font::create("data/fonts/Anonymous.ttf", 64));

#ifdef __platform_windows
    m_time_at_start = std::chrono::steady_clock::now();
#else
    clock_gettime(CLOCK_MONOTONIC, &m_time_at_start);
#endif

    // Detect available saves
    if (std::filesystem::exists(Filesystem::get_data_directory() + "saves/"))
    {
        for (auto iter : std::filesystem::directory_iterator(Filesystem::get_data_directory() + "saves/"))
        {
            std::filesystem::path path = iter.path();
            path.append("info.dat");
            if (!iter.is_directory() || !std::filesystem::exists(path))
                continue;
            m_saves.push_back(iter.path().filename().string());
        }
    }

    go_to_main_menu();

    return std::expected<void, Error>();
}

Engine::~Engine()
{
    m_thread_pool.stop();
    m_mesh_thread_pool.stop();
    m_server = nullptr;
    Font::deinit_library();
}

void Engine::register_entities()
{
    Entity::bind_methods();
    Player::bind_methods();
    Cow::bind_methods();
    Zombie::bind_methods();

    m_entity_registry.register_entity<Player>();
    m_entity_registry.register_entity<Cow>();
    m_entity_registry.register_entity<ArrowEntity>();
    m_entity_registry.register_entity<Zombie>();
}

void Engine::register_recipes()
{
    std::array<std::array<Id<Item>, 1>, 1> oak_planks{{{Items::oak_log}}};
    m_registry.add_recipe(Recipe::create(ItemStack(Items::oak_planks, 4), oak_planks));

    std::array<std::array<Id<Item>, 1>, 1> spruce_planks{{{Items::spruce_log}}};
    m_registry.add_recipe(Recipe::create(ItemStack(Items::spruce_planks, 4), spruce_planks));

    std::array<std::array<Id<Item>, 2>, 2> crafting_table{{{Items::oak_planks, Items::oak_planks},
                                                           {Items::oak_planks, Items::oak_planks}}};
    m_registry.add_recipe(Recipe::create(ItemStack(Items::crafting_table_block), crafting_table));
    std::array<std::array<Id<Item>, 2>, 2> crafting_table_spruce{{{Items::spruce_planks, Items::spruce_planks},
                                                                  {Items::spruce_planks, Items::spruce_planks}}};
    m_registry.add_recipe(Recipe::create(ItemStack(Items::crafting_table_block), crafting_table_spruce));
}

void Engine::finish_update_stage(std::string_view name, double& started)
{
    const double now = time();
    const double elapsed = (now - started) * 1000.0;
    if (elapsed > m_update_stage_ms)
    {
        m_update_stage = name;
        m_update_stage_ms = elapsed;
    }
    started = now;
}

void Engine::tick(float delta)
{
    ZoneScoped;
    const double update_begin = time();
    m_frame_begin = update_begin;
    m_update_stage = "unclassified";
    m_update_stage_ms = 0.0;
    (void)ThreadPool::take_submit_wait_ms();
    double stage_begin = update_begin;

    if (m_switch_to_main_menu)
    {
        // Jobs capture server, scheduler and dimension pointers. Finish them
        // before releasing the old world, and discard its queued work.
        m_thread_pool.reset();
        m_mesh_thread_pool.reset();
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
                    {
                        player->get_camera()->update_projection((float)w / (float)h);
                        player->get_inventory()->invalidate();
                        player->get_inventory()->get_toolbar()->invalidate();
                        if (player->get_open_inventory().has_value())
                            player->get_open_inventory().value()->invalidate();
                    }
                }
            }
            break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
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

            ImGui_ImplSDL3_ProcessEvent(&event);
            ImGuiIO& imgui_io = ImGui::GetIO();
            if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
                continue;

            Input::process_event(event);
        }
    }

    finish_update_stage("events/world transition", stage_begin);

    m_current_tps++;
    if (m_last_second_timer_time >= 1.0)
    {
        m_current_memory_usage = 0; // TODO
        m_tps = m_current_tps;
        m_current_tps = 0;
        m_last_second_timer_time -= 1.0;
    }
    m_last_second_timer_time += delta;

    if (m_server != nullptr)
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

    stage_begin = time();
    m_music_player->update(delta);
    finish_update_stage("music", stage_begin);
    m_submit_wait_ms = ThreadPool::take_submit_wait_ms();
    const double elapsed_ms = (time() - update_begin) * 1000.0;
    m_last_update_cpu_ms = elapsed_ms;
    m_update_cpu_ms += (m_update_cpu_ms == 0.0 ? 1.0 : 0.1) * (elapsed_ms - m_update_cpu_ms);
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

    if (m_menu == nullptr && m_server != nullptr)
    {
        const double now = time();
        const auto& render = m_renderer.cpu_frame_timings();
        const double total = (now - m_frame_begin) * 1000.0;
        if (total > m_pending_worst_frame.total_ms)
        {
            m_pending_worst_frame = {total, m_last_update_cpu_ms,
                render.last_acquire_ms, render.last_render_ms, render.last_present_ms,
                total - m_last_update_cpu_ms - render.last_acquire_ms
                      - render.last_render_ms - render.last_present_ms,
                m_update_stage, m_update_stage_ms, m_submit_wait_ms};
        }
        if (now - m_frame_report_time >= 5.0)
        {
            m_worst_frame = m_pending_worst_frame;
            if (m_worst_frame.total_ms > 40.0)
                info("Slowest frame work in 5s: {:.2f} ms (update {:.2f}, acquire {:.2f}, render+submit {:.2f}, present {:.2f}, other {:.2f}); update peak: {} {:.2f} ms; task mutex wait {:.2f} ms",
                    m_worst_frame.total_ms, m_worst_frame.update_ms, m_worst_frame.acquire_ms,
                    m_worst_frame.render_ms, m_worst_frame.present_ms, m_worst_frame.other_ms,
                    m_worst_frame.update_stage, m_worst_frame.update_stage_ms, m_worst_frame.submit_wait_ms);
            m_pending_worst_frame = {};
            m_frame_report_time = now;
        }
    }
    else
    {
        m_pending_worst_frame = {};
        m_worst_frame = {};
        m_frame_report_time = time();
    }

    m_current_fps++;
    m_last_second_frame_time += delta;
    if (m_last_second_frame_time >= 1.0)
    {
        m_fps = int64_t(double(m_current_fps) / m_last_second_frame_time);
        m_current_fps = 0;
        m_last_second_frame_time = 0.0;
    }
}

double Engine::time()
{
#ifdef __platform_windows
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_time_at_start).count();
#else
    struct timespec tp{};
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (double)(tp.tv_nsec + (tp.tv_sec - m_time_at_start.tv_sec) * 1000000000) / 1000000000.0;
#endif
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
        ImGui::InputText("Username", m_username_buf, 32);

        std::vector<const char *> items;
        for (const std::string& s : m_saves)
            items.push_back(s.c_str());
        if (ImGui::ListBox("Saves", &m_current_save, items.data(), (int)items.size()))
        {
        }

        ImGui::InputText("Name", m_name_buf, 32);
        ImGui::InputText("Seed", m_seed_buf, 32);

        imguitk_center_next_widget("Delete");
        if (ImGui::Button("Delete") && m_current_save < (int)m_saves.size())
        {
            std::filesystem::remove_all(Filesystem::get_data_directory() + "saves/" + m_saves[m_current_save]);
            m_saves.erase(std::find(m_saves.begin(), m_saves.end(), m_saves[m_current_save]));
            m_current_save = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Load") && m_current_save < (int)m_saves.size())
        {
            m_server = std::make_shared<LocalServer>(m_username_buf, m_saves[m_current_save], std::atoll(m_seed_buf), m_should_create_online);
            m_server->start();
            m_current_target = RpcTarget::Server;

            m_menu = std::bind(&Engine::world_load_gui, this);
        }
        ImGui::SameLine();
        if (ImGui::Button("Create") && std::find(m_saves.begin(), m_saves.end(), std::string(m_name_buf)) == m_saves.end())
        {
            m_server = std::make_shared<LocalServer>(m_username_buf, m_name_buf, std::atoll(m_seed_buf), m_should_create_online);
            m_server->start();
            m_current_target = RpcTarget::Server;

            m_menu = std::bind(&Engine::world_load_gui, this);
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

        if (ImGui::Checkbox("Fullscreen", &m_fullscreen))
        {
            m_window->set_fullscreen(m_fullscreen);
        }

        imguitk_center_next_widget("Quit");
        if (ImGui::Button("Quit"))
        {
            Engine::get().set_running(false);
        }
    }
    ImGui::End();
}

void Engine::world_load_gui()
{
    const Extent2D window_size = m_window->size();
    const float size_x = (float)window_size.width * 0.4f;
    const float size_y = (float)window_size.height * 0.6f;

    ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, (float)window_size.height / 2 - size_y / 2));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    if (ImGui::Begin("Loading"))
    {
        ImGui::ProgressBar(m_server == nullptr ? 0.0 : m_server->get_generation_progression());
    }
    ImGui::End();

    if (m_server->get_generation_progression() == 1.0 && m_server->has_generation_started())
    {
        m_server->get_world()->find_safe_spawn();
        m_server->spawn_player();
        m_menu = nullptr;
    }
}
