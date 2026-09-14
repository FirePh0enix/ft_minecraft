#pragma once

#include "Core/Math.hpp"

#include <memory>
#include <vector>

const float SPAWN_INTERVAL = 10.0f;
constexpr int SPAWN_RADIUS = 16;
constexpr int VERTICAL_RADIUS = 16;
constexpr int SPAWN_ATTEMPTS = 16;
constexpr size_t MAX_MOBS_PER_TYPE = 10;
constexpr double MOB_DESPAWN_DISTANCE = 64.0;

class World;
class Player;

class MobSpawner
{
public:
    explicit MobSpawner(World& world, int dimension) : m_world(world), m_dimension(dimension) {};

    MobSpawner(const MobSpawner&) = delete;
    MobSpawner& operator=(const MobSpawner&) = delete;

    void tick(float delta);

private:
    void spawn_mobs();
    void despawn_mobs();
    bool try_spawn_zombie(Player& player);
    bool try_spawn_cow(Player& player);
    bool can_spawn_zombie(const glm::ivec3& pos) const;
    bool can_spawn_cow(const glm::ivec3& pos) const;

    World& m_world;
    int m_dimension;
    float m_spawn_timer = 1.0f;
    std::vector<std::shared_ptr<Player>> m_players;
};
