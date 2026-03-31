#pragma once

#include "Args.hpp"
#include "Render/Renderer.hpp"
#include "World/World.hpp"

enum class EngineScene
{
    MainMenu,
    World,
};

class Engine
{
public:
    Engine();

    bool is_running() const { return m_window->is_running(); }

    void init(const Args& args);
    void tick(float delta);
    void draw();

    static Engine& get()
    {
        static Engine engine;
        return engine;
    }

    RVRenderer& get_renderer() { return m_renderer; };

private:
    EngineScene m_scene = EngineScene::MainMenu;
    Ref<Window> m_window;
    RVRenderer m_renderer;

    Ref<World> m_world;
    Ref<Entity> m_player;

    // main menu stuff
    int m_main_menu_world_type = 0;
    char m_world_seed_buf[32] = "0";

    void draw_main_menu();
    void draw_world_scene();

    void draw_world_cpu();

    void create_world_and_start();
};
