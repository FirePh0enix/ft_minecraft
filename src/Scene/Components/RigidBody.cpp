#include "Scene/Components/RigidBody.hpp"
#include "Scene/Entity.hpp"
#include <limits>

void RigidBody::start()
{
    m_transform = m_entity->get_component<TransformComponent3D>();
}

void RigidBody::tick(double delta)
{
}

void RigidBody::move_and_collide(Ref<World>& world, double delta)
{
    Transform3D transform = m_transform->get_transform();
    glm::vec3 position = transform.position();
    constexpr float precision = 0.001f;
    constexpr std::size_t max_iterations = 300;

    bool vx_positive = m_velocity.x > 0;
    position.x += m_velocity.x * (float)delta;
    bool collide_x = intersect_world(position, world);

    std::size_t x_iteration = 0;
    while (((vx_positive && m_velocity.x > 0) || (!vx_positive && m_velocity.x < 0)) && collide_x && x_iteration < max_iterations)
    {
        if (vx_positive)
        {
            position.x -= precision;
            m_velocity.x -= precision;
        }
        else
        {
            position.x += precision;
            m_velocity.x += precision;
        }
        collide_x = intersect_world(position, world);
        x_iteration++;
    }

    if ((vx_positive && m_velocity.x < 0) || (!vx_positive && m_velocity.x > 0))
    {
        m_velocity.x = 0;
    }
    if (x_iteration >= max_iterations)
    {
        std::println("Failed to resolve X Axis collision");
    }

    bool vy_positive = m_velocity.y > 0;
    position.y += m_velocity.y * (float)delta;
    bool collide_y = intersect_world(position, world);

    std::size_t y_iteration = 0;
    while (((vy_positive && m_velocity.y > 0) || (!vy_positive && m_velocity.y < 0)) && collide_y && y_iteration < max_iterations)
    {
        if (vy_positive)
        {
            position.y -= precision;
            m_velocity.y -= precision;
        }
        else
        {
            position.y += precision;
            m_velocity.y += precision;
        }
        collide_y = intersect_world(position, world);
        y_iteration++;
    }

    if ((vy_positive && m_velocity.y < 0) || (!vy_positive && m_velocity.y > 0))
    {
        m_velocity.y = 0;
    }

    if (y_iteration >= max_iterations)
    {
        std::println("Failed to resolve Y Axis collision");
    }

    bool vz_positive = m_velocity.z > 0;
    position.z += m_velocity.z * (float)delta;
    bool collide_z = intersect_world(position, world);

    std::size_t z_iteration = 0;
    while (((vz_positive && m_velocity.z > 0) || (!vz_positive && m_velocity.z < 0)) && collide_z && z_iteration < max_iterations)
    {
        if (vz_positive)
        {
            position.z -= precision;
            m_velocity.z -= precision;
        }
        else
        {
            position.z += precision;
            m_velocity.z += precision;
        }
        collide_z = intersect_world(position, world);
        z_iteration++;
    }

    if (z_iteration >= max_iterations)
    {
        std::println("Failed to resolve Z Axis collision");
    }

    if ((vz_positive && m_velocity.z < 0) || (!vz_positive && m_velocity.z > 0))
    {
        m_velocity.z = 0;
    }

    transform.position() = position;
    m_transform->set_transform(transform);
}

bool RigidBody::intersect_world(glm::vec3 position, Ref<World> world)
{

    int64_t px = static_cast<int64_t>(position.x + 0.5f);
    int64_t py = static_cast<int64_t>(position.y + 0.5f);
    int64_t pz = static_cast<int64_t>(position.z + 0.5f);

    AABB player_aabb = m_aabb;
    player_aabb.center = m_transform->get_global_transform().position();

    for (int64_t x = -1; x <= 2; x++)
    {
        for (int64_t y = -1; y <= 3; y++)
        {
            for (int64_t z = -1; z <= 2; z++)
            {
                if (world->get_block_state(x + px, y + py, z + pz).is_air())
                {
                    continue;
                }

                AABB aabb(glm::vec3((float)(x + px) + 0.5, (float)(y + py) + 0.5, (float)(z + pz) + 0.5), glm::vec3(0.5));

                if (aabb.intersect(player_aabb))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool RigidBody::is_on_ground(glm::vec3 position, Ref<World> world)
{
    int64_t px = static_cast<int64_t>(position.x + 0.5f);
    int64_t py = static_cast<int64_t>(position.y + 0.5f);
    int64_t pz = static_cast<int64_t>(position.z + 0.5f);

    AABB player_aabb = m_aabb;
    player_aabb.center = m_transform->get_transform().position();

    AABB aabb(glm::vec3((float)(px) + 0.5, (float)(1 + py) + 0.5, (float)(pz) + 0.5), glm::vec3(0.5));

    if (aabb.intersect(player_aabb) && !world->get_block_state(px, 1 + py, pz).is_air())
    {
        return true;
    }
    return false;
}
