#pragma once

#include "Args.hpp"
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

    Ref<World> get_world()
    {
        return m_world;
    }

    NetworkConnection& get_connection()
    {
        return m_connection;
    }

    static inline Engine *singleton;

private:
    EngineScene m_scene = EngineScene::MainMenu;
    Ref<Window> m_window;

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

    // main menu stuff
    int m_main_menu_world_type = 1;
    char m_world_seed_buf[32] = "0";
    char m_connect_ip[32] = "127.0.0.1";
    int m_connect_port = NetworkConnection::default_port;

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
