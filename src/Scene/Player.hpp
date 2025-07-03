#pragma once

#include "Core/Ref.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Transform3D.hpp"

class Player : public Component
{
    CLASS(Player, Component);

public:
    Player(Ref<World> world)
        : m_world(world)
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
    Ref<TransformComponent3D> m_transform;
    Ref<Camera> m_camera;
    Ref<RigidBody> m_body;
    Ref<World> m_world;

    float m_speed = 4.0;
    float m_gravity_value = 9.81;
    bool m_gravity_enabled = true;
};
