#pragma once

#include "Physics/Collider.hpp"

class PhysicsBody
{
public:
    PhysicsBody(const glm::vec3& position, Collider *collider);

    void step(float delta);

    glm::vec3 get_position() const { return m_position; }
    void set_position(glm::vec3 position) { m_position = position; };

    glm::vec3 get_velocity() const { return m_velocity; }
    void set_velocity(glm::vec3 velocity) { m_velocity = velocity; }

    Collider *get_collider() const { return m_collider; }
    void set_collider(Collider *collider) { m_collider = collider; }

private:
    glm::vec3 m_position;
    glm::vec3 m_velocity = glm::vec3(0.0);
    float m_mass = 1.0;

    Collider *m_collider = nullptr;
};
