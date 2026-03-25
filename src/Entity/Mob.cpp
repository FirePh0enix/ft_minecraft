#include "Entity/Mob.hpp"
#include "World/World.hpp"

Mob::Mob()
{
}

// static AABB get_broadphase_box(const AABB& object, const glm::vec3& v)
// {
//     glm::vec3 min(object.min_x(), object.min_y(), object.min_z());
//     glm::vec3 max(object.max() + v);

//     glm::vec3 center = min + (max - min) / 2.0f;
//     glm::vec3 half_extent = (max - min) / 2.0f;

//     return AABB(center, half_extent);
// }

static float sweep_aabb(const AABB& object, const AABB& other, const glm::vec3& v, glm::vec3& dir)
{
    // AABB broadphase_box = get_broadphase_box(object, v);
    // if (!broadphase_box.intersect(other))
    // {
    //     return 1.0f;
    // }

    float dx_entry, dx_exit, tx_entry, tx_exit;
    float dy_entry, dy_exit, ty_entry, ty_exit;
    float dz_entry, dz_exit, tz_entry, tz_exit;

    if (v.x > 0.0f)
    {
        dx_entry = other.min_x() - object.max_x();
        dx_exit = other.max_x() - object.min_x();
    }
    else
    {
        dx_entry = other.max_x() - object.min_x();
        dx_exit = other.min_x() - object.max_x();
    }
    if (v.y > 0.0f)
    {
        dy_entry = other.min_y() - object.max_y();
        dy_exit = other.max_y() - object.min_y();
    }
    else
    {
        dy_entry = other.max_y() - object.min_y();
        dy_exit = other.min_y() - object.max_y();
    }
    if (v.y > 0.0f)
    {
        dz_entry = other.min_z() - object.max_z();
        dz_exit = other.max_z() - object.min_z();
    }
    else
    {
        dz_entry = other.max_z() - object.min_z();
        dz_exit = other.min_z() - object.max_z();
    }

    // Calculate time from distance/velocity
    if (v.x == 0.0)
    {
        tx_entry = -std::numeric_limits<float>::infinity();
        tx_exit = std::numeric_limits<float>::infinity();
    }
    else
    {
        tx_entry = dx_entry / v.x;
        tx_exit = dx_exit / v.x;
    }
    if (v.y == 0.0)
    {
        ty_entry = -std::numeric_limits<float>::infinity();
        ty_exit = std::numeric_limits<float>::infinity();
    }
    else
    {
        ty_entry = dy_entry / v.y;
        ty_exit = dy_exit / v.y;
    }
    if (v.z == 0.0)
    {
        tz_entry = -std::numeric_limits<float>::infinity();
        tz_exit = std::numeric_limits<float>::infinity();
    }
    else
    {
        tz_entry = dz_entry / v.z;
        tz_exit = dz_exit / v.z;
    }

    const float entry_time = std::max(tx_entry, std::max(ty_entry, tz_entry));
    const float exit_time = std::min(tx_exit, std::min(ty_exit, tz_exit));

    if (entry_time > exit_time || (tx_entry < 0.0f && ty_entry < 0.0f && tz_entry < 0.0f) || tx_entry > 1.0f || ty_entry > 1.0f || tz_entry > 1.0f)
    {
        return 1.0f;
    }

    // if (tx_entry > ty_entry && tx_entry > tz_entry)
    // {
    //     if (dx_entry > 0.0)
    //         dir = glm::vec3(1, 0, 0);
    //     else
    //         dir = glm::vec3(-1, 0, 0);
    // }
    // else if (ty_entry > tx_entry && ty_entry > tz_entry)
    // {
    //     if (dy_entry > 0.0)
    //         dir = glm::vec3(0, 1, 0);
    //     else
    //         dir = glm::vec3(0, -1, 0);
    // }
    // else
    // {
    //     if (dz_entry > 0.0)
    //         dir = glm::vec3(0, 0, 1);
    //     else
    //         dir = glm::vec3(0, 0, -1);
    // }

    return entry_time;
}

void Mob::move_and_collide(bool enable_collision)
{
    if (enable_collision)
    {
        m_transform.position() += m_velocity;
        return;
    }

    const AABB pbox = m_aabb.translate(m_transform.position());
    const Dimension& dimension = m_world->get_dimension(m_dimension);
    const std::vector<AABB> colliders = dimension.get_boxes_that_may_collide(pbox); // TODO: broad phase.

    float lowest_collision_time = 1.0f;

    for (const auto& collider : colliders)
    {
        glm::vec3 dir;
        float collision_time = sweep_aabb(pbox, collider, m_velocity, dir);

        if (collision_time < lowest_collision_time)
        {
            lowest_collision_time = collision_time;
        }
    }

    const glm::vec3& pos = m_transform.position();
    m_on_ground = dimension.has_solid_block((int64_t)std::round(pos.x), (int64_t)std::round(pos.y - 0.9), (int64_t)std::round(pos.z));

    m_transform.position() += m_velocity * lowest_collision_time;
}
