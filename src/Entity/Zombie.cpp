#include "Zombie.hpp"

#include "Core/Print.hpp"
#include "Core/Result.hpp"
#include "Entity/Entity.hpp"
#include "Entity/LivingEntity.hpp"
#include "Entity/Player.hpp"
#include "World/World.hpp"

#include <limits>
#include <memory>

constexpr float PATH_UPDATE_INTERVAL = 1.0f;
constexpr float DETECTION_RADIUS = 20.0f;

void Zombie::start() {};

void Zombie::tick(float delta)
{
    m_attack_timer -= delta;
    m_path_update_timer -= delta;
    m_groan_timer -= delta;

    if (!m_on_ground)
    {
        float gravity = m_gravity_value;

        if (is_in_water())
            gravity = 0.0f;

        m_velocity.y -= gravity * delta;
    }

    // Tracking.
    AABBf search_box = AABBf::from_center_extent(get_global_transform().position(), glm::vec3(DETECTION_RADIUS));
    const auto entities = m_world->get_dimension(m_dimension).cast_box(search_box);

    std::shared_ptr<Player> target;
    float best_dist_sq = std::numeric_limits<float>::max();

    for (std::shared_ptr<Entity> entity : entities)
    {
        std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(entity);
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

        bool is_target_reachable = m_pathfinding->is_walkable(target_rounded_pos, 0, m_dimension);

        // Tracking.
        if (is_target_reachable && m_path_update_timer <= 0.0f)
        {
            m_path_update_timer = PATH_UPDATE_INTERVAL;
            flee_to(target_rounded_pos);
        }

        if (best_dist_sq < m_attack_range * m_attack_range)
        {
            m_following_path = false;
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
            attack();
        }
    }
    else
    {
        // Patrolling.
        if (m_on_ground && !m_following_path)
        {
            const glm::ivec3 to = find_random_walkable_position(DETECTION_RADIUS);
            flee_to(to);
        }
    }

    if (m_following_path)
    {
        if (!verify_if_path_still_valid())
        {
            const glm::ivec3& to = m_path.value().look_points[m_path.value().finish_line_index];
            const int remaining_jump = m_on_ground ? 1 : 0;
            const bool is_final_pos_reachable = m_pathfinding->is_walkable(to, remaining_jump, m_dimension);

            if (is_final_pos_reachable)
                flee_to(m_path.value().look_points[m_path.value().finish_line_index]);
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

    m_audio_source->set_position(get_global_transform().position());

    if (m_groan_timer <= 0.0f)
    {
        m_groan_timer = GROAN_INTERVAL;
        m_audio_source->play_one_shot(&m_groan_clip.value(), 0.5f);
    }

    const bool is_walking = m_on_ground && glm::length2(glm::vec2(m_velocity.x, m_velocity.z)) > 0.001f;
    if (is_walking)
        m_audio_source->play();
    else
        m_audio_source->stop();
}

void Zombie::on_ready()
{
    m_model = EXPECT(Model::load("assets/models/zombie.json"));
    m_id = World::next_id();
    m_pathfinding = std::make_unique<Pathfinding>(m_world);

    AudioMixer& audio = m_world->audio();
    auto path = std::filesystem::absolute("assets/audio/zombie/groan.wav");
    m_groan_clip.emplace(*audio.get_audio_mixer(), path);
    path = std::filesystem::absolute("assets/audio/zombie/walking.wav");
    m_walking_clip.emplace(*audio.get_audio_mixer(), path);

    m_audio_source.emplace(audio);
    m_audio_source->set_clip(&m_walking_clip.value());
}

void Zombie::attack()
{
    if (!m_threat_entity || m_attack_timer > 0.0f)
        return;

    std::shared_ptr<LivingEntity> mob = std::dynamic_pointer_cast<LivingEntity>(m_threat_entity);
    if (!mob)
        return;

    mob->damage(1, id());
    m_attack_timer = m_attack_cooldown;
}
