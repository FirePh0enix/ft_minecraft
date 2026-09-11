#pragma once

#include "Audio/AudioMixer.hpp"
#include "Audio/MusicPlayer.hpp"
#include "Core/ThreadPool.hpp"
#include "Font.hpp"
#include "Network/Server.hpp"
#include "Render/Renderer.hpp"
#include "World/Registry.hpp"

#include <memory>

/// Number of ticks per in-game day.
constexpr int64_t ticks_per_day = 60 * 60 * 24;
constexpr int64_t ticks_noon = ticks_per_day / 2;
constexpr int64_t ticks_sunrise = ticks_per_day / 4;
constexpr int64_t ticks_sunset = ticks_noon + ticks_per_day / 4;

class Widget;

class Engine
{
public:
    Engine(bool disable_save);
    ~Engine();

    bool is_running() const { return m_window->is_running(); }

    void tick(float delta);
    void draw(float delta);

    size_t get_memory_usage() const { return m_current_memory_usage; }

    std::shared_ptr<Window> window() const { return m_window; }
    std::shared_ptr<Server> server() const { return m_server; }

    ThreadPool& get_thread_pool()
    {
        return m_thread_pool;
    }

    ThreadPool& get_mesh_thread_pool()
    {
        return m_mesh_thread_pool;
    }

    // bool is_online() const { return m_connection.state() != ConnectionState::Idle; }

    GameRegistry& registry() { return m_registry; }
    EntityRegistry& entity_registry() { return m_entity_registry; }

    std::shared_ptr<Font> get_font() const { return m_font; }

    /// Returns the current in seconds with a nanosecond precision.
    double time();

    ALWAYS_INLINE int64_t get_fps() const { return m_fps; }
    ALWAYS_INLINE int64_t get_tps() const { return m_tps; }

    bool is_save_disabled() const { return m_disable_save; }

    /// Time of day in ticks since the start of the day.
    int64_t time_of_day() const { return m_ticks_since_start_of_day; }

    AudioMixer& audio_mixer() { return *m_audio_mixer; }
    MusicPlayer& music_player() { return *m_music_player; }

    bool is_server() const { return m_server != nullptr && m_current_target == RpcTarget::Server; }
    bool is_client() const { return m_server != nullptr && m_current_target == RpcTarget::Client; }

    void go_to_main_menu();

    static Engine& get() { return *singleton; }

private:
    static inline Engine *singleton;

    std::shared_ptr<Window> m_window;
    std::shared_ptr<Server> m_server;
    RpcTarget m_current_target = RpcTarget::Server;

    std::function<void()> m_menu;

    GameRegistry m_registry;
    EntityRegistry m_entity_registry;
    Renderer m_renderer;

    bool m_disable_save;

    bool m_switch_to_main_menu = false;

    std::shared_ptr<Font> m_font;

    ThreadPool m_thread_pool;
    ThreadPool m_mesh_thread_pool;

    bool m_time_pass = true;
    int64_t m_tick_scale = 15;
    int64_t m_ticks_since_start_of_day = 0;
    int64_t m_fps = 0;
    int64_t m_tps = 0;
    int64_t m_current_fps = 0;
    int64_t m_current_tps = 0;

    float m_last_second_timer_time = 0.0;
    float m_last_second_frame_time = 0.0;
    size_t m_current_memory_usage = 0;

    std::unique_ptr<AudioMixer> m_audio_mixer;
    std::unique_ptr<MusicPlayer> m_music_player;

    // Menu
    char m_username_buf[32] = "steve";
    int m_current_save = 0;         // load
    char m_name_buf[32] = "unamed"; // create
    char m_seed_buf[32] = "0";
    bool m_should_create_online = false;
    char m_ip_buf[32] = "127.0.0.1"; // join

    void register_entities();
    void register_recipes();

    void main_menu_gui();
};
