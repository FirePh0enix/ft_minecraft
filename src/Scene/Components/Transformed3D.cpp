#include "Scene/Components/Transformed3D.hpp"
#include "Scene/Components/RigidBody.hpp"

void Transformed3D::set_position(Variant position)
{
    if (Ref<RigidBody> rb = m_entity->get_component<RigidBody>())
    {
        rb->get_body()->set_position(position.operator glm::vec3());
    }
    else
    {
        m_transform.position() = position.operator glm::vec3();
    }
}
