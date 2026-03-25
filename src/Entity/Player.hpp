#pragma once

#include "Core/Ref.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Mob.hpp"
#include "Render/Graph.hpp"
#include "World/Block.hpp"
#include "World/World.hpp"

class Player : public Mob
{
    CLASS(Player, Mob);

public:
    Player();

    virtual ~Player() {}

    virtual void tick(float delta) override;
    virtual void draw(RenderPassEncoder& encoder) override;

    void on_ready() override;

    void set_gravity_enabled(bool v)
    {
        m_gravity_enabled = v;
    }

    bool is_gravity_enabled() const
    {
        return m_gravity_enabled;
    }

    void set_gravity_value(float v)
    {
        m_gravity_value = v;
    }

    float get_gravity_value() const
    {
        return m_gravity_value;
    }

    float get_speed() const { return m_speed; }
    void set_speed(float speed) { m_speed = speed; }

    void on_hit_by(Entity *entity) override;

protected:
    void die() override;

private:
    Ref<Camera> m_camera;

    float m_speed = 50.0;
    float m_gravity_value = 9.81;
    bool m_gravity_enabled = false;

    bool m_was_attack_pressed = false;

    bool m_has_jumped = false;

    std::optional<glm::vec3> m_aimed_block = std::nullopt;
    Ref<Mesh> m_highlight_mesh;
    Ref<Shader> m_highlight_shader;
    Ref<Material> m_highlight_material;
    Model m_highlight_model;
    Ref<Buffer> m_highlight_model_buffer;
};
