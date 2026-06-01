#include "Entity/Mob.hpp"

constexpr int attempts = 16;

glm::vec3 Mob::safe_normalize(const glm::vec3& v)
{
    float len2 = glm::length2(v);
    if (len2 < 1e-8f)
        return glm::vec3(0.0f);
    return v / sqrt(len2);
}

void Mob::follow_path(float delta_time)
{
    if (!m_following_path || !m_path || m_path->look_points.empty())
        return;

    if (m_path_index >= m_path->look_points.size())
    {
        m_following_path = false;
        m_velocity = glm::vec3(0.0f);
        return;
    }

    glm::vec3 pos = m_transform.position();
    glm::vec3 target = m_path->look_points.get_unchecked(m_path_index);
    glm::vec3 to_target = target - pos;
    to_target.y = 0.0f;

    // Slow down when reaching last waypoint.
    float speed_percent = 1.0f;
    if (m_path_index >= m_path->slow_down_index && m_stopping_dst > 0.0f)
    {
        const size_t size = m_path->look_points.size();

        glm::vec2 end_2d = glm::vec2(m_path->look_points.get_unchecked(size - 1).x, m_path->look_points.get_unchecked(size - 1).z);
        float dist_to_end = glm::distance(glm::vec2(pos.x, pos.z), end_2d);
        speed_percent = glm::clamp(dist_to_end / m_stopping_dst, 0.0f, 1.0f);

        if (speed_percent < 0.01f)
        {
            m_following_path = false;
            m_velocity = glm::vec3(0.0f);
            return;
        }
    }

    // Movement + Rotation.
    glm::vec3 target_dir_norm = safe_normalize(to_target);

    // Ensure that mob will slide if he is stuck against a block.
    // For example, sometimes, Cow want to move forward but he is slightly at aligned at left,
    // since he's facing perpendicularly toward block, there is no chance of sliding.
    // X X X X X X
    // X X O O O X
    // X O ↑ X X X
    // So i'm applying a slight diagonal velocity to allow him to slide.
    if (m_blocked_x || m_blocked_z)
    {
        glm::vec3 right = glm::normalize(glm::cross(target_dir_norm, glm::vec3(0, 1, 0)));

        float steer = 0.0f;

        if (m_blocked_x)
            steer += (m_velocity.x > 0.0f) ? -1.0f : 1.0f;

        if (m_blocked_z)
            steer += (m_velocity.z > 0.0f) ? 1.0f : -1.0f;

        target_dir_norm = glm::normalize(target_dir_norm + right * steer * 0.35f);
    }

    if (m_path_index + 1 < m_path->look_points.size())
    {
        glm::vec3 b = m_path->look_points.get_unchecked(m_path_index + 1);
        glm::vec3 seg = b - target;
        seg.y = 0.0f;

        if (glm::length2(seg) > 1e-6f)
        {
            glm::vec3 dir_b_norm = safe_normalize(seg);
            float look_strength = -0.25f;
            target_dir_norm = safe_normalize(target_dir_norm + dir_b_norm * look_strength);
        }
    }

    glm::quat target_rot = glm::quatLookAt(target_dir_norm, glm::vec3(0, 1, 0));
    m_transform.rotation() = glm::slerp(m_transform.rotation(), target_rot, delta_time * m_turn_speed);
    glm::vec3 move = target_dir_norm * m_speed * speed_percent * delta_time;

    m_velocity.x = move.x;
    m_velocity.z = move.z;

    // Jump.
    float dy = m_path->look_points.get_unchecked(m_path_index).y - pos.y;

    if (m_on_ground && m_velocity.y <= 0.0f && dy >= 0.4f)
        m_velocity.y = m_jump_force;

    // Switch to next waypoint.
    float reach_radius = 0.2f;
    glm::vec2 pos_2d = glm::vec2(pos.x, pos.z);
    glm::vec2 target_2d = glm::vec2(target.x, target.z);
    float next_waypoint_len2 = glm::length2(target_2d - pos_2d);

    if (next_waypoint_len2 < reach_radius * reach_radius)
        m_path_index++;
}

void Mob::flee_to(const glm::ivec3& to)
{

    glm::ivec3 cow_grid = glm::ivec3(glm::round(m_transform.position()));

    auto value = m_pathfinding->find_path(cow_grid, to).has_value();
    if (m_pathfinding->m_path.empty() || !value)
    {
        m_following_path = false;
        return;
    }

    Result<Vector<glm::vec3>> waypoints = m_pathfinding->simplify_path(m_pathfinding->m_path);
    m_path.emplace(waypoints.value(), m_stopping_dst);
    m_following_path = true;
    m_path_index = 1;
}

bool Mob::verify_if_path_still_valid()
{
    const LocalVector<uint32_t>& full_path = m_pathfinding->m_path;
    const size_t path_size = full_path.size();

    const LocalVector<PathNode>& nodes = m_pathfinding->m_nodes;

    glm::ivec3 grid_pos = glm::ivec3(glm::round(m_path->look_points.get_unchecked(m_path_index)));
    size_t start_index = path_size;

    // Find which node is actually targeting, since it's unnecessary to check nodes that has been already reached.
    for (size_t i = 0; i < path_size; ++i)
    {
        if (nodes.get_unchecked(full_path.get_unchecked(i)).m_gridPos == grid_pos)
        {
            start_index = i;
            break;
        }
    }

    for (; start_index < path_size; ++start_index)
    {
        if (!m_pathfinding->is_walkable(nodes.get_unchecked(full_path.get_unchecked(start_index)).m_gridPos, 1))
            return false;
    }

    return true;
}

glm::ivec3 Mob::find_random_walkable_position(int radius, const glm::vec3& preferred_dir)
{
    glm::ivec3 start = glm::ivec3(glm::round(m_transform.position()));
    glm::ivec3 result = start;

    for (size_t i = 0; i < attempts; i++)
    {
        int dx = rand() % (radius * 2 + 1) - radius;
        int dz = rand() % (radius * 2 + 1) - radius;
        int dy = rand() % 5 - 2;

        glm::vec3 dir = safe_normalize(glm::vec3(dx, 0.0f, dz));

        if (glm::length2(preferred_dir) > 0.0f)
            dir = safe_normalize(dir + preferred_dir);

        int dist = rand() % radius + 1;

        glm::ivec3 horizontal = glm::ivec3(glm::round(dir * (float)dist));
        glm::ivec3 pos = start + glm::ivec3(horizontal.x, dy, horizontal.z);
        glm::ivec3 below(pos.x, pos.y - 1, pos.z);

        if (m_pathfinding->is_walkable(pos, 1) && !m_pathfinding->is_walkable(below, 1))
        {
            result = pos;
            break;
        }
    }

    return result;
}