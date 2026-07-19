#pragma once

#include "Console.hpp"
#include "Core/Class.hpp"
#include "Core/Definitions.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Player.hpp"
#include "Font.hpp"
#include "Network/Network.hpp"
#include "Render/Renderer.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

enum class GameScene
{
    MainMenu,
    World,
    WaitingForWorld,
};

/**
 * Number of ticks per in-game day.
 */
constexpr int64_t ticks_per_day = 60 * 60 * 24;
constexpr int64_t ticks_noon = ticks_per_day / 2;
constexpr int64_t ticks_sunrise = ticks_per_day / 4;
constexpr int64_t ticks_sunset = ticks_noon + ticks_per_day / 4;

class Engine : public Object
{
    CLASS(Engine, Object);

public:
    Engine();
    ~Engine();

    bool is_running() const { return m_window->is_running(); }

    void tick(float delta);
    void draw(float delta);

    bool is_server() const { return m_authority == RpcTarget::Server; }

    size_t get_memory_usage() const { return m_current_memory_usage; }

    Ref<World> get_world()
    {
        return m_world;
    }

    NetworkConnection& connection()
    {
        return m_connection;
    }

    ThreadPool& get_thread_pool()
    {
	return m_thread_pool;
    }

    bool is_online() const { return m_connection.state() != ConnectionState::Idle; }

    GameRegistry& registry() { return m_registry; }
    EntityRegistry& entities() { return m_entity_registry; }

    Ref<Player> get_player() const { return m_player; }

    Ref<Font> get_font() const { return m_font; }

    float time();

    ALWAYS_INLINE int64_t get_fps() const { return m_fps; }
    ALWAYS_INLINE int64_t get_tps() const { return m_tps; }

    /**
     * Time of day in ticks since the start of the day.
     */
    int64_t time_of_day() const { return m_ticks_since_start_of_day; }

    static Engine& get() { return *singleton; }

    static inline Engine *singleton;

private:
    GameScene m_scene = GameScene::MainMenu;
    Ref<Window> m_window;

    GameRegistry m_registry;
    EntityRegistry m_entity_registry;

    RpcTarget m_authority = RpcTarget::Server;
    NetworkConnection m_connection;
    Map<ENetPeer *, Ref<Player>> m_players;

    Renderer m_renderer;

    ThreadPool m_thread_pool;

    Ref<World> m_world;
    Ref<Entity> m_player;
    Ref<Font> m_font;

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

    // main menu stuff
    int m_main_menu_world_type = 1;
    char m_world_seed_buf[32] = "0";
    char m_connect_ip[32] = "127.0.0.1";
    int m_connect_port = NetworkConnection::default_port;
    char m_username[32] = "steve";

    // Debug Menu
    Console m_console;

    bool has_player_with_name(const StringView& name) const {
	if (name == StringView(m_player.cast_to<Player>()->get_username())) {
	    return true;
	}
	for (const auto& [peer, player] : m_players) {
	    if (StringView(player->get_username()) == name) {
		return true;
	    }
	}
	return false;
    }

    void register_entities();
    void register_recipes();

    void draw_main_menu();
    void draw_world_scene();

    void create_world_and_start();
    void connect_to_remote_world();

    static void receive_client(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client);
    static void connect_client(void *, NetworkConnection& conn, const Client& client);
    static void disconnect_client(void *, NetworkConnection& conn, const Client& client);

    static void receive_server(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client);
    static void connect_server(void *, NetworkConnection& conn, const Client& client);
    static void disconnect_server(void *, NetworkConnection& conn, const Client& client);
};
