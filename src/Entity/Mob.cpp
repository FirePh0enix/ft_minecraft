#include "Entity/Mob.hpp"
#include "Engine.hpp"
#include "Network/Network.hpp"
#include <cstddef>

constexpr int attempts = 16;

void Mob::draw(const RenderPass& pass, bool shadowmap)
{
    if (!m_model)
        return;

    Transform3D render_transform = get_global_transform();
    // Cow and zombie models face +Z; movement rotations point local -Z forward.
    render_transform.rotation() *= glm::angleAxis(glm::radians(180.0), glm::dvec3(0.0, 1.0, 0.0));
    if (m_dead)
    {
        const float t = glm::smoothstep(
            0.0f, 1.0f, std::min(m_death_animation_time / death_duration, 1.0f));
        render_transform.rotation() *= glm::angleAxis(
            glm::radians(90.0 * (double)t), glm::dvec3(0.0, 0.0, 1.0));
        render_transform.position().y -= 0.45 * (double)t;
    }
    m_model->encode(pass, render_transform, shadowmap);
}

void Mob::die()
{
    if (m_dead)
        return;

    m_dead = true;
    m_death_animation_time = 0.0f;
    call_rpc("on_death");
}

void Mob::on_death()
{
    m_dead = true;
    m_death_animation_time = 0.0f;
    m_following_path = false;
    m_velocity = glm::dvec3(0.0);
    m_animator.play_once("death", true);
}

bool Mob::tick_death(float delta_time)
{
    if (!m_dead)
        return false;

    m_velocity = glm::dvec3(0.0);
    m_following_path = false;
    m_death_animation_time += delta_time;
    m_animator.tick(delta_time);

    if (Engine::get().is_server() && m_death_animation_time >= death_duration)
    {
        const RemoveEntityPacket packet(id());
        Engine::get().server()->route_packet(NetworkConnection::create_packet(packet));
        m_world->remove_entity(m_dimension, id());
    }

    return true;
}

void Mob::animate_movement(float delta_time, MovementSound movement_state)
{
    if (movement_state != MovementSound::None)
        m_animator.play(movement_state == MovementSound::Swimming ? "swim" : "walk");
    else
        m_animator.play("idle");

    m_animator.tick(delta_time);
}

void Mob::follow_path(float delta_time)
{
    if (!m_following_path || !m_path || m_path.value().look_points.empty())
        return;

    if (m_path_index >= m_path.value().look_points.size())
    {
        m_following_path = false;
        m_velocity = glm::vec3(0.0f);
        return;
    }

    glm::vec3 pos = m_transform.position();
    glm::vec3 target = m_path.value().look_points[m_path_index];
    glm::vec3 to_target = target - pos;

    if (!is_in_water())
        to_target.y = 0.0f;

    // Reach the cell center before turning.
    const float horizontal_distance = glm::length(glm::vec2(to_target.x, to_target.z));
    const bool height_reached = std::abs(target.y - pos.y) < 0.5f;
    if (horizontal_distance < 0.01f && height_reached)
    {
        ++m_path_index;
        m_velocity.x = 0.0;
        m_velocity.z = 0.0;
        if (is_in_water())
            m_velocity.y = 0.0;
        return;
    }

    // Movement + Rotation.
    glm::vec3 target_dir_norm = safe_normalize(to_target);

    if (glm::length2(target_dir_norm) > 1e-6f && is_in_water())
    {
        glm::vec3 flat = target_dir_norm;
        // Ignoring pitch rotation because sometimes he was rotating so much that body was straight like ----- instead of   |
        //                                                                                                                  |
        //                                                                                                                  |
        // So he cannot move forward anymore xD.
        flat.y = 0.0f;

        if (glm::length2(flat) > 1e-6f)
        {
            flat = glm::normalize(flat);

            glm::dquat target_rot = glm::quatLookAt(flat, glm::vec3(0, 1, 0));
            m_transform.rotation() = glm::slerp(m_transform.rotation(), target_rot, (double)delta_time * m_turn_speed);
        }
    }
    else if (glm::length2(target_dir_norm) > 1e-6f)
    {
        glm::dquat target_rot = glm::quatLookAt(target_dir_norm, glm::vec3(0, 1, 0));
        m_transform.rotation() = glm::slerp(m_transform.rotation(), target_rot, (double)delta_time * m_turn_speed);
    }

    // Clamp the step so a fast frame cannot overshoot and orbit a waypoint.
    const float step = std::min(m_speed * delta_time, glm::length(to_target));
    glm::vec3 move = target_dir_norm * step;

    m_velocity.x = move.x;
    m_velocity.z = move.z;

    if (is_in_water())
        m_velocity.y = move.y;

    // Jump.
    float dy = m_path.value().look_points[m_path_index].y - pos.y;

    if (m_on_ground && m_velocity.y <= 0.0f && dy >= 0.4f)
        m_velocity.y = m_jump_force;
}

void Mob::flee_to(const glm::ivec3& to)
{

    glm::ivec3 grid_pos = glm::ivec3(glm::round(m_transform.position()));

    m_pathfinding->find_path(grid_pos, to, m_dimension);
    if (m_pathfinding->m_path.empty())
    {
        m_following_path = false;
        return;
    }

    std::vector<glm::vec3> waypoints = m_pathfinding->simplify_path(m_pathfinding->m_path);
    m_path = Path(waypoints);
    m_following_path = true;
    m_path_index = 1;
}

bool Mob::verify_if_path_still_valid()
{
    if (!m_path.has_value() || m_path->look_points.empty() || m_path_index >= m_path->look_points.size())
        return false;

    const auto full_path = m_pathfinding->m_path;
    const size_t path_size = full_path.size();

    if (path_size == 0)
        return false;

    glm::ivec3 grid_pos = glm::ivec3(glm::round(m_path.value().look_points[m_path_index]));

    size_t start_index = path_size;

    // Find which node is actually targeting, since it's unnecessary to check nodes that has been already reached.
    for (size_t i = 0; i < path_size; ++i)
    {
        size_t node_index = full_path[i];
        const auto& node = m_pathfinding->m_node_pool[node_index];

        if (node.m_gridPos == grid_pos)
        {
            start_index = i;
            break;
        }
    }

    if (start_index == path_size)
        return false;

    for (size_t i = start_index; i < path_size; ++i)
    {
        size_t node_index = full_path[i];
        const auto& node = m_pathfinding->m_node_pool[node_index];

        if (!m_pathfinding->is_walkable(node.m_gridPos, 1, m_dimension))
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

        if (m_pathfinding->is_walkable(pos, 1, m_dimension) && !m_pathfinding->is_walkable(below, 1, m_dimension))
        {
            result = pos;
            break;
        }
    }

    return result;
}
