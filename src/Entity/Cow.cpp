#include "Cow.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Print.hpp"
#include "Core/Result.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/PathNode.hpp"
#include "World/World.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>

constexpr size_t attempts = 16;

static glm::vec3 safe_normalize(const glm::vec3& v)
{
    float len2 = glm::length2(v);
    if (len2 < 1e-8f)
        return glm::vec3(0.0f);
    return v / std::sqrt(len2);
}

static glm::ivec3 find_random_walkable_position(Pathfinding& pathfinder, const glm::ivec3& start, int radius, const glm::vec3& preferred_dir = glm::vec3(0.0f))
{
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

        if (pathfinder.is_walkable(pos, 1) && !pathfinder.is_walkable(below, 1))
        {
            result = pos;
            break;
        }
    }

    return result;
}

void Cow::start() {};

void Cow::tick(float delta)
{

    if (!m_on_ground)
        m_velocity.y -= 9.81f * delta;

    follow_path(delta);

    glm::ivec3 cow_grid = glm::ivec3(glm::floor(m_transform.position()));

    if (m_following_path)
    {

        if (!verify_if_path_still_valid(m_path->look_points.get_unchecked(m_path_index)))
        {
            const glm::ivec3& to = m_path->look_points.get_unchecked(m_path->finish_line_index);
            bool is_final_pos_reachable = m_pathfinding->is_walkable(to, 0);

            if (is_final_pos_reachable)
                flee_to(m_path->look_points.get_unchecked(m_path->finish_line_index));
            else
                flee_from(*m_threat_entity, 10);
        }
    }
    else
    {
        if (m_on_ground)
        {
            const glm::ivec3 to = find_random_walkable_position(*m_pathfinding, cow_grid, 10);
            flee_to(to);
        }
    }

    move_and_collide();

    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (m_on_ground && m_velocity.y < 0.0f)
        m_velocity.y = 0.0f;
}

void Cow::draw(const RenderPassNode& node)
{
    m_model->encode(node, get_global_transform());
}

void Cow::on_ready()
{
    // register_rpc("on_hit", [this](Entity& attacker)
    //              { this->on_hit_by(attacker); }, RpcTarget::Server);

    m_model = EXPECT(Model::load("assets/models/player.json"));

    m_id = World::next_id();

    m_pathfinding = std::make_unique<Pathfinding>(m_world);
}

void Cow::die()
{
    m_active = false;
    println("Cow died !");
}

void Cow::on_hit_by(Entity& entity)
{
    Mob *mob_caller = reinterpret_cast<Mob *>(&entity);
    m_threat_entity = &entity;

    int damage = mob_caller->get_attack_damage();
    (void)damage;
    // take_damage(damage);

    if (m_health <= 0)
    {
        die();
        return;
    }

    flee_from(entity, 10);
}

void Cow::flee_from(const Entity& threat, int radius)
{
    glm::ivec3 cow_grid = glm::ivec3(glm::round(m_transform.position()));
    glm::ivec3 threat_grid = glm::ivec3(glm::round(threat.get_global_transform().position()));

    glm::vec3 flee_dir = safe_normalize(glm::vec3(cow_grid - threat_grid));
    glm::ivec3 flee_position = find_random_walkable_position(*m_pathfinding, cow_grid, radius, flee_dir);

    auto result = m_pathfinding->find_path(cow_grid, flee_position);

    if (m_pathfinding->m_path.empty() || !result)
    {
        m_following_path = false;
        return;
    }

    Result<Vector<glm::vec3>> waypoints = m_pathfinding->simplify_path(m_pathfinding->m_path);

    if (!waypoints.has_value())
        return;

    m_path.emplace(waypoints.value(), m_stopping_dst);
    m_following_path = true;
    // Cow is already at waypoint 0.
    m_path_index = 1;
}

void Cow::flee_to(const glm::ivec3& to)
{
    glm::ivec3 cow_grid = glm::ivec3(glm::round(m_transform.position()));

    auto value = m_pathfinding->find_path(cow_grid, to).has_value();
    if (m_pathfinding->m_path.empty() || !value)
    {
        m_following_path = false;
        return;
    }

    Result<Vector<glm::vec3>> waypoints = m_pathfinding->simplify_path(m_pathfinding->m_path);
    if (!waypoints.has_value())
        return;

    m_path.emplace(waypoints.value(), m_stopping_dst);
    m_following_path = true;
    // Cow is already at waypoint 0, so he should look toward actual target.
    m_path_index = 1;
}

void Cow::follow_path(float delta_time)
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
        size_t size = m_path->look_points.size();

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
    {
        m_velocity.y = m_jump_force;
        const auto to = m_path->look_points.get_unchecked(m_path_index);
        println("to: {} {} {}", to.x, to.y, to.z);
        println("pos: {} {} {}", pos.x, pos.y, pos.z);
        println("dy: {}", dy);
    }

    // Switch to next waypoint.
    float reach_radius = 0.2f;
    glm::vec2 pos_2d = glm::vec2(pos.x, pos.z);
    glm::vec2 target_2d = glm::vec2(target.x, target.z);
    float next_waypoint_len2 = glm::length2(target_2d - pos_2d);

    if (next_waypoint_len2 < reach_radius * reach_radius)
        m_path_index++;
}

bool Cow::verify_if_path_still_valid(const glm::vec3& waypoint)
{
    const LocalVector<uint32_t>& path = m_pathfinding->m_path;
    const size_t path_size = path.size();
    glm::ivec3 waypoint_grid_pos = glm::ivec3(waypoint);
    size_t start_index = path_size;

    const auto& nodes = m_pathfinding->m_nodes;

    for (size_t i = 0; i < path_size; ++i)
    {
        if (nodes.get_unchecked(path.get_unchecked(i)).m_gridPos == waypoint_grid_pos)
        {
            start_index = i;
            break;
        }
    }

    for (size_t i = start_index; i < path_size; i++)
    {
        if (!m_pathfinding->is_walkable(nodes.get_unchecked(path.get_unchecked(i)).m_gridPos, 1))
            return false;
    }

    return true;
}