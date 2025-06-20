#pragma once

#include "AABB.hpp"
#include "Scene/Components/Component.hpp"
#include "World/World.hpp"

class RigidBody : public Component
{
    CLASS(RigidBody, Component);

public:
    RigidBody(AABB aabb)
        : m_aabb(aabb)
    {
    }

    virtual void start() override;
    virtual void tick(double delta) override;

    inline void set_velocity(glm::vec3 velocity)
    {
        m_velocity = velocity;
    }

    void move_and_collide(const Ref<World>& world, double delta);
    bool intersect_world(glm::vec3 position, const Ref<World>& world);
    bool is_on_ground(glm::vec3 position, const Ref<World>& world);

private:
    Ref<TransformComponent3D> m_transform;
    glm::vec3 m_velocity;
    AABB m_aabb;
};
