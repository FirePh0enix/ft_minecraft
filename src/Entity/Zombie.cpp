#include "Zombie.hpp"
#include "Core/Print.hpp"
#include "Core/Result.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "World/World.hpp"
#include <cstddef>
#include <memory>

constexpr float PATH_UPDATE_INTERVAL = 1.0f;

void Zombie::start() {};

void Zombie::tick(float delta)
{

    m_attack_timer -= delta;
    m_path_update_timer -= delta;
    if (m_attack_timer < 0.0f)
        m_attack_timer = 0.0f;

    if (!m_on_ground)
        m_velocity.y -= 9.81f * delta;

    const glm::ivec3 player_pos = glm::round(Engine::get().get_player()->get_global_transform().position());

    // Tracking.
    // Maybe do the math once and adjust action by comparing result and threshold.
    if (is_player_in_radius(20, player_pos))
    {
        bool is_target_reachable = m_pathfinding->is_walkable(player_pos, 0);

        if (m_last_threat_pos != player_pos && is_target_reachable && m_path_update_timer <= 0.0f)
        {
            m_last_threat_pos = player_pos;
            m_path_update_timer = PATH_UPDATE_INTERVAL;
            flee_to(player_pos);
        }

        const float dist_sq = glm::distance2(glm::vec3(player_pos), get_global_transform().position());
        if (dist_sq <= m_attack_range * m_attack_range)
        {
            m_following_path = false;
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
            m_can_attack = true;
            m_threat_entity = Engine::get().get_player();
            attack();
        }
        else
            m_can_attack = false;
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
    // Patrol.
    if (m_on_ground && !m_following_path && !m_can_attack)
    {
        const glm::ivec3 to = find_random_walkable_position(20);
        flee_to(to);
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

bool Zombie::is_player_in_radius(float radius, const glm::vec3& player_pos) const
{
    glm::vec3 zombie_pos = get_global_transform().position();
    float distance_sq = glm::distance2(player_pos, zombie_pos);
    return distance_sq <= radius * radius;
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