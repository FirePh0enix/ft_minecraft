#pragma once

#include "Core/Ref.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Transformed3D.hpp"
#include "World/World.hpp"

class Player : public Component
{
    CLASS(Player, Component);

public:
    Player(const Ref<World>& world, const Ref<Mesh>& cube_mesh)
        : m_world(world), m_cube_mesh(cube_mesh)
    {
    }

    virtual void start() override;
    virtual void tick(double delta) override;

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

private:
    Ref<Transformed3D> m_transform;
    Ref<Camera> m_camera;
    Ref<RigidBody> m_body;
    Ref<World> m_world;

    Ref<Mesh> m_cube_mesh;
    Ref<Entity> m_cube_highlight;

    float m_speed = 10.0;
    float m_gravity_value = 9.81;
    bool m_gravity_enabled = true;

    bool m_has_jumped = false;

    void on_block_aimed(BlockState state, int64_t x, int64_t y, int64_t z, glm::vec3 dir);
};
