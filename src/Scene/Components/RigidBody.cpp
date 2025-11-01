#include "Scene/Components/RigidBody.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

RigidBody::RigidBody()
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
    m_body = new PhysicsBody(m_transform->get_global_transform().position(), new BoxCollider(glm::vec3(), glm::vec3()));
    m_entity->get_scene()->get_physics_space().add_body(m_body);
}
