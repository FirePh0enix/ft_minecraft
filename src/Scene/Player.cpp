#include "Scene/Player.hpp"
#include "Input.hpp"
#include "Ray.hpp"
#include "Scene/Entity.hpp"

void Player::start()
{
    m_transform = m_entity->get_component<TransformComponent3D>();
    m_body = m_entity->get_component<RigidBody>();

    m_camera = m_entity->get_child(0)->get_component<Camera>();
}

void Player::tick(double delta)
{
    Transform3D transform = m_transform->get_transform();

    const glm::vec3 forward = transform.forward();
    const glm::vec3 right = transform.right();
    const glm::vec3 up(0.0, 1.0, 0.0);
    const glm::vec3 dir = Input::get().get_movement_vector();

    if (Input::get().is_mouse_grabbed())
    {
        const glm::vec2 relative = Input::get().get_mouse_relative();
        const glm::quat q_yaw = glm::angleAxis(relative.x * 0.01f, up);

        transform.rotation() *= q_yaw;
        m_transform->set_transform(transform);

        Ref<TransformComponent3D> camera_transform_comp = m_camera->get_entity()->get_component<TransformComponent3D>();
        Transform3D camera_transform = camera_transform_comp->get_transform();

        const glm::quat q_pitch = glm::angleAxis(relative.y * 0.01f, glm::vec3(1.0, 0.0, 0.0));
        camera_transform.rotation() *= q_pitch;

        camera_transform_comp->set_transform(camera_transform);
    }

    Ray ray(transform.position(), forward);

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
                break;
            }

            t += 0.1;
        }
    }

    m_body->velocity() += forward * dir.z * m_speed;
    m_body->velocity() += up * dir.y * m_speed;
    m_body->velocity() += right * dir.x * m_speed;

    if (m_gravity_enabled && !m_body->is_on_ground(m_world))
    {
        m_body->velocity().y -= m_gravity_value * (float)delta;
    }

    m_body->move_and_collide(m_world, delta);
    m_body->velocity().x = 0; // FIXME: Add friction, ...
    m_body->velocity().y = 0;
    m_body->velocity().z = 0;
}
