#include "Scene/Player.hpp"
#include "Input.hpp"
#include "Scene/Entity.hpp"

void Player::start()
{
    m_transform = m_entity->get_component<Transform3D>();
    m_camera = m_entity->get_component<Camera>();
    m_body = m_entity->get_component<RigidBody>();
}

void Player::tick(double delta)
{
    const glm::vec3 forward = m_camera->get_forward();
    const glm::vec3 right = m_camera->get_right();
    const glm::vec3 up(0.0, 1.0, 0.0);

    const glm::vec3 dir = Input::get().get_movement_vector();
    m_transform->position() += forward * dir.z * m_speed;
    m_transform->position() += up * dir.y * m_speed;
    m_transform->position() += right * dir.x * m_speed;
}
