#include "Physics/PhysicsBody.hpp"

PhysicsBody::PhysicsBody(PhysicsBodyKind kind, Collider *collider)
    : m_kind(kind), m_collider(collider)
{
}

void PhysicsBody::step(float delta)
{
    m_position += m_velocity * delta;
}
