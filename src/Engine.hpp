#pragma once

#include "Args.hpp"
#include "Console.hpp"
#include "Core/Class.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Player.hpp"
#include "Network/Network.hpp"
#include "Render/Graph.hpp"
#include "Render/Renderer.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

enum class EngineScene
{
    MainMenu,
    World,
    WaitingForWorld,
};

class Engine : public Object
{
    CLASS(Engine, Object);

public:
    Engine(const Args& args);

    bool is_running() const { return m_window->is_running(); }

    void tick(float delta);
    void draw();

    void exit();

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

    bool is_online() const { return m_connection.state() == ConnectionState::Idle; }

    BlockRegistry& block_registry() { return m_block_registry; }
    EntityRegistry& entity_registry() { return m_entity_registry; }

    Ref<Player> get_player() const { return m_player; }

    void encode_debug_menu();

    static Engine& get() { return *singleton; }

    static inline Engine *singleton;

private:
    EngineScene m_scene = EngineScene::MainMenu;
    Ref<Window> m_window;

    BlockRegistry m_block_registry;
    EntityRegistry m_entity_registry;

    RpcTarget m_authority = RpcTarget::Server;
    NetworkConnection m_connection;
    std::map<ENetPeer *, Ref<Player>> m_players;

    Renderer m_renderer;
    RenderGraph m_graph;

    Ref<World> m_world;
    Ref<Entity> m_player;

    Ref<Texture> m_depth_texture;
    Ref<Texture> m_color_texture;

    float m_last_second_timer_time = 0.0;
    size_t m_current_memory_usage = 0;

    // main menu stuff
    int m_main_menu_world_type = 1;
    char m_world_seed_buf[32] = "0";
    char m_connect_ip[32] = "127.0.0.1";
    int m_connect_port = NetworkConnection::default_port;

    // Debug Menu
    bool m_debug_menu = false;
    Console m_console;

    void register_blocks();
    void register_entities();

    Result<void> draw_main_menu();
    Result<void> draw_world_scene();

    void create_world_and_start();
    void connect_to_remote_world();

    static void receive_client(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client);
    static void connect_client(void *, NetworkConnection& conn, const Client& client);
    static void disconnect_client(void *, NetworkConnection& conn, const Client& client);

    static void receive_server(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client);
    static void connect_server(void *, NetworkConnection& conn, const Client& client);
    static void disconnect_server(void *, NetworkConnection& conn, const Client& client);
};
