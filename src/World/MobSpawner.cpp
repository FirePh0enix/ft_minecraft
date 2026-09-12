#include "MobSpawner.hpp"
#include "Engine.hpp"
#include "Entity/Cow.hpp"
#include "Entity/Player.hpp"
#include "Entity/Zombie.hpp"

#include <print>

void MobSpawner::tick(float delta)
{
    if (Engine::get().is_client())
        return;

    m_spawn_timer -= delta;
    if (m_spawn_timer > 0.0f)
        return;

    m_spawn_timer = SPAWN_INTERVAL;

    m_players.clear();

    const auto& entities = m_world.get_dimension(m_dimension).get_entities();

    for (const auto& entity : entities)
    {
        if (auto player = std::dynamic_pointer_cast<Player>(entity))
            m_players.push_back(std::move(player));
    }

    despawn_mobs();
    spawn_mobs();
    m_players.clear();
}

void MobSpawner::despawn_mobs()
{
    const auto& entities = m_world.get_dimension(m_dimension).get_entities();
    const double despawn_distance_squared = MOB_DESPAWN_DISTANCE * MOB_DESPAWN_DISTANCE;

    for (const std::shared_ptr<Entity>& entity : entities)
    {
        const bool is_zombie = dynamic_cast<Zombie *>(entity.get()) != nullptr;
        const bool is_cow = dynamic_cast<Cow *>(entity.get()) != nullptr;
        if (!is_zombie && !is_cow)
            continue;

        bool player_is_nearby = false;
        for (const auto& player : m_players)
        {
            if (glm::distance2(entity->get_position(), player->get_position()) <= despawn_distance_squared)
            {
                player_is_nearby = true;
                break;
            }
        }

        if (!player_is_nearby)
            m_world.remove_entity(m_dimension, entity);
    }
}

void MobSpawner::spawn_mobs()
{
    for (const std::shared_ptr<Player>& player : m_players)
    {
        try_spawn_zombie(*player);
        try_spawn_cow(*player);
    }
}

void MobSpawner::try_spawn_zombie(Player& player)
{
    const glm::dvec3 player_pos = player.get_position();
    const int player_x = static_cast<int>(glm::floor(player_pos.x));
    const int player_y = static_cast<int>(glm::floor(player_pos.y));
    const int player_z = static_cast<int>(glm::floor(player_pos.z));

    for (int attempt = 0; attempt < SPAWN_ATTEMPTS; ++attempt)
    {
        const int x = player_x + rand_int(-SPAWN_RADIUS, SPAWN_RADIUS);
        const int z = player_z + rand_int(-SPAWN_RADIUS, SPAWN_RADIUS);
        bool has_ceiling = false;

        // Scan from top to bottom. Once a solid block has been crossed, any
        // valid space below is considered covered, so it might be a cave.
        for (int y = player_y + VERTICAL_RADIUS; y >= player_y - VERTICAL_RADIUS; --y)
        {
            if (m_world.get_dimension(m_dimension).has_solid_block(x, y, z))
            {
                has_ceiling = true;
                continue;
            }

            const glm::ivec3 spawn_pos(x, y, z);
            if (!has_ceiling || !can_spawn_zombie(spawn_pos))
                continue;

            auto zombie = std::make_shared<Zombie>();

            // Place the zombie on top of the ground block.
            zombie->set_position(glm::dvec3(x, static_cast<double>(y) + 0.4, z));

            m_world.add_entity(m_dimension, zombie);

            const glm::dvec3 pos = zombie->get_position();
            std::println("Spawned zombie at {} {} {}", pos.x, pos.y, pos.z);
            return;
        }
    }
}

bool MobSpawner::can_spawn_zombie(const glm::ivec3& pos) const
{
    const BlockState feet = m_world.get_block_state(m_dimension, pos.x, pos.y, pos.z);
    const BlockState head = m_world.get_block_state(m_dimension, pos.x, pos.y + 1, pos.z);
    const bool has_ground = m_world.get_dimension(m_dimension).has_solid_block(pos.x, pos.y - 1, pos.z);

    return feet.is_air() && head.is_air() && has_ground;
}

void MobSpawner::try_spawn_cow(Player& player)
{
    const glm::dvec3 player_pos = player.get_position();
    const int player_x = static_cast<int>(glm::floor(player_pos.x));
    const int player_y = static_cast<int>(glm::floor(player_pos.y));
    const int player_z = static_cast<int>(glm::floor(player_pos.z));

    for (int attempt = 0; attempt < SPAWN_ATTEMPTS; ++attempt)
    {
        const int x = player_x + rand_int(-SPAWN_RADIUS, SPAWN_RADIUS);
        const int z = player_z + rand_int(-SPAWN_RADIUS, SPAWN_RADIUS);

        // The first valid position found from the top is exposed surface,
        // rather than an enclosed cave below it.
        for (int y = player_y + VERTICAL_RADIUS; y >= player_y - VERTICAL_RADIUS; --y)
        {
            const glm::ivec3 spawn_pos(x, y, z);
            if (!can_spawn_cow(spawn_pos))
                continue;

            auto cow = std::make_shared<Cow>();
            cow->set_position(glm::dvec3(x, static_cast<double>(y) + 0.4, z));
            m_world.add_entity(m_dimension, cow);

            const glm::dvec3 pos = cow->get_position();
            std::println("Spawned cow at {} {} {}", pos.x, pos.y, pos.z);
            return;
        }
    }
}

bool MobSpawner::can_spawn_cow(const glm::ivec3& pos) const
{
    const BlockState feet = m_world.get_block_state(m_dimension, pos.x, pos.y, pos.z);
    const BlockState head = m_world.get_block_state(m_dimension, pos.x, pos.y + 1, pos.z);
    const bool has_ground = m_world.get_dimension(m_dimension).has_solid_block(pos.x, pos.y - 1, pos.z);

    return feet.is_air() && head.is_air() && has_ground;
}
