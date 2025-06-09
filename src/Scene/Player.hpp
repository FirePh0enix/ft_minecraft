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

private:
    Ref<Transform3D> m_transform;
    Ref<Camera> m_camera;
    Ref<RigidBody> m_body;
    Ref<World> m_world;

    float m_speed = 0.05;
};
