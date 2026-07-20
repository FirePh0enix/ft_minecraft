#pragma once

#include "Entity/LivingEntity.hpp"
#include "Entity/Pathfinding/Path.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"

/**
 * @brief An entity controlled by AI.
 */
class Mob : public LivingEntity
{
    CLASS(Mob, LivingEntity);

public:
    Mob(int health)
        : LivingEntity(health)
    {
    }

    virtual void draw(const RenderPass& pass) override;
    virtual void die() override;

    void follow_path(float delta_time);
    void flee_to(const glm::ivec3& to);
    bool verify_if_path_still_valid();
    glm::ivec3 find_random_walkable_position(int radius, const glm::vec3& preferred_dir = glm::vec3(0.0f));
    glm::vec3 safe_normalize(const glm::vec3& v);

protected:
    Ref<Model> m_model;
    std::unique_ptr<Pathfinding> m_pathfinding;
    bool m_following_path = false;
    Option<Path> m_path;
    size_t m_path_index = 0;
    float m_turn_speed = 10;
    float m_stopping_dst = 2;

    float m_speed = 1.0f;
    float m_jump_force = 0.24f;
};
