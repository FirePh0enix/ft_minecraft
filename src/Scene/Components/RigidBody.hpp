#pragma once

#include "AABB.hpp"
#include "Scene/Components/Component.hpp"
#include "World/World.hpp"

class RigidBody : public Component
{
    CLASS(RigidBody, Component);

public:
    virtual void start() override;
    virtual void tick(double delta) override;

private:
    float m_velocity;
    AABB m_aabb;

    bool intersect_world(Ref<World> world);
};
