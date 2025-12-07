#include "Scene/Components/RigidBody.hpp"
#include "Physics/PhysicsBody.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

RigidBody::RigidBody(Collider *collider)
    : m_body(new PhysicsBody(PhysicsBodyKind::Kinematic, collider))
{
}

RigidBody::~RigidBody()
{
    m_entity->get_scene()->get_physics_space().remove_body(m_body);
    if (m_body)
        delete m_body;
}

void RigidBody::start()
{
    m_transform = m_entity->get_transform();

    m_body->set_position(m_transform->get_global_transform().position());

    m_entity->get_scene()->get_physics_space().add_body(m_body);
}
