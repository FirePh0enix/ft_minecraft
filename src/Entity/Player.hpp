#pragma once

#include "Core/Ref.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Mob.hpp"
#include "Model.hpp"
#include "Render/Graph.hpp"

enum class GameMode
{
    Survival,
    Creative,
};

class Player : public Mob
{
    CLASS(Player, Mob);

public:
    Player();

    virtual ~Player() {}

    virtual void tick(float delta) override;
    virtual void draw(const RenderPassNode& node) override;

    void on_ready() override;

    float get_speed() const { return m_speed; }
    void set_speed(float speed) { m_speed = speed; }

    void set_remote() { m_local_player = false; }

    void set_gamemode(GameMode gamemode) { m_gamemode = gamemode; }

    virtual void on_hit_by(Entity& entity) override;

    bool has_gravity() const { return m_gamemode == GameMode::Survival; }

protected:
    void die() override;

private:
    Ref<Camera> m_camera;
    GameMode m_gamemode = GameMode::Survival;

    float m_speed = 8.0;
    float m_gravity_value = 9.81 / 10.0f;
    float m_jump_force = 0.24f;

    std::optional<glm::vec3> m_aimed_block = std::nullopt;
    Ref<Material> m_aim_material;
    Ref<Buffer> m_aim_buffer;

    static constexpr glm::vec3 aim_color = glm::vec3(0.94, 0.63, 0.1);

    Ref<Model> m_model;
    Animator m_animator;

    /**
     * Player class is a little special since its behavor is different if this is the local or remote.
     */
    bool m_local_player = true;
};
