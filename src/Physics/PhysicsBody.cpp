#include "Physics/PhysicsBody.hpp"

PhysicsBody::PhysicsBody(const glm::vec3& position, Collider *collider)
    : m_position(position), m_collider(collider)
{
}

void PhysicsBody::step(float delta)
{
    m_position += m_velocity * delta;
}
