#include "Cow.hpp"
#include "Core/Print.hpp"
#include "Core/Result.hpp"
#include "Entity/Entity.hpp"
#include "World/World.hpp"

#include <memory>

void Cow::start() {};

void Cow::tick(float delta)
{

    if (!m_on_ground)
        m_velocity.y -= 9.81f * delta;

    if (m_following_path)
    {
        if (!verify_if_path_still_valid())
        {
            const glm::ivec3& to = m_path.value().look_points.get_unchecked(m_path.value().finish_line_index);
            const int remaining_jump = m_on_ground ? 1 : 0;
            bool is_final_pos_reachable = m_pathfinding->is_walkable(to, remaining_jump);

            if (is_final_pos_reachable)
                flee_to(m_path.value().look_points.get_unchecked(m_path.value().finish_line_index));
            else
                flee_from(20);
        }
    }
    // Patrol.
    else
    {
        if (m_on_ground)
        {
            const glm::ivec3 to = find_random_walkable_position(20);
            flee_to(to);
        }
    }

    follow_path(delta);
    move_and_collide();

    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (m_on_ground && m_velocity.y < 0.0f)
        m_velocity.y = 0.0f;
}

void Cow::on_ready()
{
    m_model = EXPECT(Model::load("assets/models/player.json"));
    m_id = World::next_id();
    m_pathfinding = std::make_unique<Pathfinding>(m_world);
}

void Cow::die()
{
    m_active = false;
    println("Cow died !");
}

void Cow::on_damage(int damage, EntityId damage_source)
{
    (void)damage;
    m_threat_entity = m_world->get_entity(damage_source);
    flee_from(20);
}

void Cow::flee_from(int radius)
{
    if (!m_threat_entity)
        return;

    glm::ivec3 cow_grid = glm::ivec3(glm::round(m_transform.position()));
    glm::ivec3 threat_grid = glm::ivec3(glm::round(m_threat_entity->get_global_transform().position()));

    glm::vec3 flee_dir = safe_normalize(glm::vec3(cow_grid - threat_grid));
    glm::ivec3 flee_position = find_random_walkable_position(radius, flee_dir);

    m_pathfinding->find_path(cow_grid, flee_position);

    if (m_pathfinding->m_path.empty())
    {
        m_following_path = false;
        return;
    }

    Vector<glm::vec3> waypoints = m_pathfinding->simplify_path(m_pathfinding->m_path);
    m_path = Path(waypoints, m_stopping_dst);
    m_following_path = true;
    // Cow is already at waypoint 0.
    m_path_index = 1;
}
