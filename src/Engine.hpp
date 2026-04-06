#pragma once

#include "Args.hpp"
#include "Core/Class.hpp"
#include "World/World.hpp"

enum class EngineScene
{
    MainMenu,
    World,
};

class Engine : public Object
{
    CLASS(Engine, Object);

public:
    Engine(const Args& args);

    bool is_running() const { return m_window->is_running(); }

    void tick(float delta);

    void draw();

private:
    EngineScene m_scene = EngineScene::MainMenu;
    Ref<Window> m_window;

    World m_world;
    Ref<Entity> m_player;

    // main menu stuff
    int m_main_menu_world_type = 0;
    char m_world_seed_buf[32] = "0";

    Result<void> draw_main_menu();
    Result<void> draw_world_scene();

    void create_world_and_start();
};
