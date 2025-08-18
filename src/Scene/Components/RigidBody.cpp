#include "Scene/Components/RigidBody.hpp"
#include "Core/Math.hpp"
#include "Scene/Entity.hpp"

void RigidBody::start()
{
    m_transform = m_entity->get_transform();
}

void RigidBody::tick(double delta)
{
    (void)delta;
}

void RigidBody::move_and_collide(const Ref<World>& world, double delta)
{
    constexpr float multiplier = 0.95;

    Transform3D transform = m_transform->get_transform();

    if (!m_disabled)
    {
        while (true)
        {
            glm::vec3 position = transform.position() + glm::vec3(m_velocity.x, 0, 0) * (float)delta;
            if (!intersect_world(position, world))
                break;
            m_velocity.x *= multiplier;

            if (math::is_zero_e(m_velocity.x * (float)delta))
            {
                m_velocity.x = 0;
                break;
            }
        }

        while (true)
        {
            glm::vec3 position = transform.position() + glm::vec3(0, m_velocity.y, 0) * (float)delta;
            if (!intersect_world(position, world))
                break;
            m_velocity.y *= multiplier;

            if (math::is_zero_e(m_velocity.y * (float)delta))
            {
                m_velocity.y = 0;
                break;
            }
        }

        while (true)
        {
            glm::vec3 position = transform.position() + glm::vec3(0, 0, m_velocity.z) * (float)delta;
            if (!intersect_world(position, world))
                break;
            m_velocity.z *= multiplier;

            if (math::is_zero_e(m_velocity.z * (float)delta))
            {
                m_velocity.z = 0;
                break;
            }
        }
    }

    transform.position() += m_velocity * (float)delta;
    m_transform->set_transform(transform);
}

bool RigidBody::is_on_ground(const Ref<World>& world)
{
    return intersect_world(m_transform->get_transform().position() + glm::vec3(0, -0.1, 0), world);
}

bool RigidBody::intersect_world(glm::vec3 position, const Ref<World>& world)
{
    int64_t px = static_cast<int64_t>(position.x);
    int64_t py = static_cast<int64_t>(position.y);
    int64_t pz = static_cast<int64_t>(position.z);

    constexpr int64_t min_x_bound = -2;
    constexpr int64_t max_x_bound = 2;
    constexpr int64_t min_y_bound = -2;
    constexpr int64_t max_y_bound = 3;
    constexpr int64_t min_z_bound = -2;
    constexpr int64_t max_z_bound = 2;

    AABB player_aabb = m_aabb;
    player_aabb.center = position;

    for (int64_t x = min_x_bound; x <= max_x_bound; x++)
    {
        for (int64_t y = min_y_bound; y <= max_y_bound; y++)
        {
            for (int64_t z = min_z_bound; z <= max_z_bound; z++)
            {
                if (world->get_block_state(x + px, y + py, z + pz).is_air())
                {
                    continue;
                }

                float bx = (float)(x + px) - 0.5f;
                float by = (float)(y + py) - 0.5f;
                float bz = (float)(z + pz) - 0.5f;

                AABB aabb(glm::vec3(bx, by, bz), glm::vec3(0.5));
                if (aabb.intersect(player_aabb))
                {
                    return true;
                }
            }
        }
    }

    return false;
}
