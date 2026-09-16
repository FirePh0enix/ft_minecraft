#include "Cow.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "World/World.hpp"

#include <memory>

void Cow::bind_methods()
{
    type.add_method("set_movement_state", &Cow::set_movement_state);
    expose_rpc<Cow>("set_movement_state", RpcTarget::Both);
}

void Cow::start() {};

void Cow::tick(float delta)
{
    if (Engine::get().is_client())
    {
        m_audio_source->set_position(get_global_transform().position());
        animate_movement(delta, m_movement_sound);
        return;
    }

    m_velocity.y -= m_gravity_value * delta;

    if (m_following_path)
    {
        if (!verify_if_path_still_valid())
        {
            const glm::ivec3& to = m_path.value().look_points[m_path.value().finish_line_index];
            const int remaining_jump = m_on_ground ? 1 : 0;
            bool is_final_pos_reachable = m_pathfinding->is_walkable(to, remaining_jump, m_dimension);

            if (is_final_pos_reachable)
                flee_to(m_path.value().look_points[m_path.value().finish_line_index]);
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

    const bool is_moving = glm::length2(glm::vec2(m_velocity.x, m_velocity.z)) > 1e-6f;

    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (m_on_ground && m_velocity.y < 0.0f)
        m_velocity.y = 0.0f;

    m_audio_source->set_position(get_global_transform().position());

    MovementSound movement_sound = MovementSound::None;
    if (is_moving)
    {
        if (is_in_water())
            movement_sound = MovementSound::Swimming;
        else if (m_on_ground)
            movement_sound = MovementSound::Walking;
    }
    if (Engine::get().is_server() && movement_sound != m_movement_sound)
    {
        set_movement_state(static_cast<int64_t>(movement_sound));
        call_rpc("set_movement_state", static_cast<int64_t>(movement_sound));
    }
    animate_movement(delta, movement_sound);
}

void Cow::set_movement_state(int64_t state)
{
    m_movement_sound = static_cast<MovementSound>(state);
    if (m_movement_sound == MovementSound::Swimming)
        m_audio_source->set_clip(&m_swimming_clip.value());
    else if (m_movement_sound == MovementSound::Walking)
        m_audio_source->set_clip(&m_walking_clip.value());
    else
    {
        m_audio_source->stop();
        return;
    }
    m_audio_source->play();
}

void Cow::on_ready()
{
    m_model = EXPECT(ModelLegacy::load("data/models/cow.json"));
    m_animator.set_model(m_model);
    m_pathfinding = std::make_unique<Pathfinding>(m_world);

    AudioMixer& audio = m_world->audio();
    auto path = std::filesystem::absolute("assets/audio/cow/walking.wav");
    m_walking_clip.emplace(*audio.get_audio_mixer(), path);
    path = std::filesystem::absolute("assets/audio/cow/swimming.wav");
    m_swimming_clip.emplace(*audio.get_audio_mixer(), path);

    m_audio_source.emplace(audio);
    m_audio_source->set_clip(&m_walking_clip.value());
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

    m_pathfinding->find_path(cow_grid, flee_position, m_dimension);

    if (m_pathfinding->m_path.empty())
    {
        m_following_path = false;
        return;
    }

    std::vector<glm::vec3> waypoints = m_pathfinding->simplify_path(m_pathfinding->m_path);
    m_path = Path(waypoints, m_stopping_dst);
    m_following_path = true;
    // Cow is already at waypoint 0.
    m_path_index = 1;
}
