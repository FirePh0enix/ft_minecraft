#pragma once

#include "Core/Ref.hpp"
#include "World/World.hpp"

class Player : public Entity
{
    CLASS(Player, Entity);

public:
    Player()
    {
    }

    virtual ~Player() {}

    virtual void tick(float delta) override;

    void move_and_collide();

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

private:
    Ref<Camera> m_camera;

    Ref<Mesh> m_cube_mesh;
    Ref<Entity> m_cube_highlight;

    glm::vec3 m_velocity = glm::vec3(0.0);

    float m_speed = 50.0;
    float m_gravity_value = 9.81;
    bool m_gravity_enabled = true;

    bool m_has_jumped = false;
    bool m_on_ground = false;

    void on_block_aimed(BlockState state, int64_t x, int64_t y, int64_t z, glm::vec3 dir);
};
