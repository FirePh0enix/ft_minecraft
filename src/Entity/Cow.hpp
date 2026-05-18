#pragma once

#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/Path.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"
#include "Mob.hpp"
#include <cstddef>
#include <limits>

class Cow : public Mob
{
public:
    Cow() : Mob(3, 0, 1.0f, 1.0f)
    {
    }

    void start() override;
    void tick(float delta) override;
    void draw(const RenderPassNode& node) override;
    void on_ready() override;
    void on_hit_by(Entity& entity) override;

protected:
    void die() override;
    void flee_from(const Entity& threat, int radius);
    void flee_to(const glm::ivec3& to);
    void follow_path(float delta_time);
    bool verify_if_path_still_valid(const glm::vec3& waypoint);

    std::unique_ptr<Pathfinding> m_pathfinding;
    std::optional<Path> m_path;
    Entity *m_threat_entity = nullptr;

    float m_turn_speed = 10;
    float m_stopping_dst = 5;

    size_t m_path_index = 0;
    size_t m_last_jump_index = std::numeric_limits<size_t>::max();

    bool m_following_path = false;
};
