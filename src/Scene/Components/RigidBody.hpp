#pragma once

#include "AABB.hpp"
#include "Scene/Components/Component.hpp"
#include "World/World.hpp"

class RigidBody : public Component
{
    CLASS(RigidBody, Component);

public:
    RigidBody(AABB aabb)
        : m_velocity(0.0), m_aabb(aabb)
    {
    }

    virtual void start() override;
    virtual void tick(double delta) override;

    const glm::vec3& velocity() const
    {
        return m_velocity;
    }

    glm::vec3& velocity()
    {
        return m_velocity;
    }

    const bool& disabled() const
    {
        return m_disabled;
    }

    bool& disabled()
    {
        return m_disabled;
    }

    void move_and_collide(const Ref<World>& world, double delta);
    bool is_on_ground(const Ref<World>& world);

private:
    Ref<Transformed3D> m_transform;
    glm::vec3 m_velocity = glm::vec3();
    AABB m_aabb;
    bool m_disabled = false;

    bool intersect_world(glm::vec3 position, const Ref<World>& world);
};
