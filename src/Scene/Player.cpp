#include "Scene/Player.hpp"
#include "Input.hpp"
#include "Ray.hpp"
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

    Ray ray(m_transform->position(), forward);

    if (Input::get().is_action_pressed(Action::Attack))
    {
        float t = 0;

        while (t < 5.0)
        {
            glm::vec3 pos = ray.at(t);

            int64_t x = (int64_t)(pos.x + 0.5);
            int64_t y = (int64_t)(pos.y + 0.5);
            int64_t z = (int64_t)(pos.z + 0.5);

            BlockState state = m_world->get_block_state(x, y, z);

            if (!state.is_air())
            {
                m_world->set_block_state(x, y, z, BlockState());
                m_world->update_buffers();
                break;
            }

            t += 0.1;
        }
    }
}
