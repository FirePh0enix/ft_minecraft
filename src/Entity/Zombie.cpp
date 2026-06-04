#include "Zombie.hpp"
#include "Core/Print.hpp"
#include "Core/Result.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "World/World.hpp"
#include <cstddef>
#include <limits>
#include <memory>

constexpr float PATH_UPDATE_INTERVAL = 1.0f;
constexpr float DETECTION_RADIUS = 20.0f;

void Zombie::start() {};

void Zombie::tick(float delta)
{

    m_attack_timer -= delta;
    m_path_update_timer -= delta;

    if (!m_on_ground)
        m_velocity.y -= 9.81f * delta;

    // Tracking.
    AABB search_box = AABB::from_center_extent(get_global_transform().position(), glm::vec3(DETECTION_RADIUS));
    const auto entities = m_world->get_dimension(m_dimension).cast_box(search_box);

    Ref<Player> target;
    float best_dist_sq = std::numeric_limits<float>::max();

    for (Ref<Entity>& entity : entities)
    {
        Ref<Player> player = entity.cast_to<Player>();
        if (!player)
            continue;

        float d2 = glm::distance2(glm::vec3(player->get_global_transform().position()), get_global_transform().position());
        if (d2 > DETECTION_RADIUS * DETECTION_RADIUS)
            continue;

        if (d2 < best_dist_sq)
        {
            best_dist_sq = d2;
            target = player;
        }
    }

    m_threat_entity = target;

    if (m_threat_entity)
    {
        const glm::ivec3 target_rounded_pos = glm::round(m_threat_entity->get_global_transform().position());

        bool is_target_reachable = m_pathfinding->is_walkable(target_rounded_pos, 0);

        if (is_target_reachable && m_path_update_timer <= 0.0f)
        {
            m_path_update_timer = PATH_UPDATE_INTERVAL;
            flee_to(target_rounded_pos);
            // println("Tracking...");
        }

        if (best_dist_sq < m_attack_range * m_attack_range)
        {
            m_following_path = false;
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
            attack();
            // println("Attacking...");
        }
    }
    else
    {
        // Patrol.
        if (m_on_ground && !m_following_path)
        {
            const glm::ivec3 to = find_random_walkable_position(DETECTION_RADIUS);
            flee_to(to);
            // println("Patrolling...");
        }
    }

    if (m_following_path)
    {
        if (!verify_if_path_still_valid())
        {
            const glm::ivec3& to = m_path->look_points.get_unchecked(m_path->finish_line_index);
            const int remaining_jump = m_on_ground ? 1 : 0;
            const bool is_final_pos_reachable = m_pathfinding->is_walkable(to, remaining_jump);

            if (is_final_pos_reachable)
                flee_to(m_path->look_points.get_unchecked(m_path->finish_line_index));
            else
                m_following_path = false;
        }
    }

    follow_path(delta);
    move_and_collide();

    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (m_on_ground && m_velocity.y < 0.0f)
        m_velocity.y = 0.0f;
}

void Zombie::draw(const RenderPassNode& node)
{
    m_model->encode(node, get_global_transform());
}

void Zombie::on_ready()
{

    m_model = EXPECT(Model::load("assets/models/player.json"));
    m_id = World::next_id();
    m_pathfinding = std::make_unique<Pathfinding>(m_world);
}

void Zombie::die()
{
    m_active = false;
    println("Zombie died !");
}

void Zombie::on_hit_by(Entity& entity)
{
    Mob *mob_caller = reinterpret_cast<Mob *>(&entity);
    int damage = mob_caller->get_attack_damage();
    take_damage(damage);

    if (m_health <= 0)
    {
        die();
        return;
    }
}

void Zombie::attack()
{
    if (!m_threat_entity || m_attack_timer > 0.0f)
        return;

    Ref<Mob> mob = m_threat_entity.cast_to<Mob>();
    if (!mob)
        return;

    mob->on_hit_by(*this);
    m_attack_timer = m_attack_cooldown;

    println("zombie attacked !");
}