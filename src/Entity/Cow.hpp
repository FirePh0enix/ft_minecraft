#pragma once

#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/Path.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"
#include "Mob.hpp"
#include <cstddef>

class Cow : public Mob
{
public:
    Cow() : Mob(3, 0, 3.0f, 1.0f)
    {
        m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
    }

    void start() override;
    void tick(float delta) override;
    void draw(const RenderPassNode& node) override;
    void on_ready() override;
    void on_hit_by(Entity& entity) override;

protected:
    void die() override;
    void flee_from(int radius);
    void flee_to(const glm::ivec3& to);
    void follow_path(float delta_time);
    bool verify_if_path_still_valid();

    glm::ivec3 find_random_walkable_position(int radius, const glm::vec3& preferred_dir);

    std::unique_ptr<Pathfinding> m_pathfinding;
    std::optional<Path> m_path;
    Entity *m_threat_entity = nullptr;

    float m_turn_speed = 10;
    float m_stopping_dst = 2;

    size_t m_path_index = 0;

    bool m_following_path = false;

    size_t new_path_count = 0;

};
