#pragma once

#include "Entity/LivingEntity.hpp"
#include "Entity/Pathfinding/Path.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"
#include "Model.hpp"
#include <optional>

/**
 * @brief An entity controlled by AI.
 */
class Mob : public LivingEntity
{
    CLASS(Mob, LivingEntity);

public:
    Mob(int health) : LivingEntity(health)
    {
    }

    virtual void draw(const RenderPass& pass, bool shadowmap) override;
    virtual void die() override;
    bool is_dead() const override { return m_dead; }
    virtual void on_death();

    void follow_path(float delta_time);
    void flee_to(const glm::ivec3& to);
    bool verify_if_path_still_valid();
    glm::ivec3 find_random_walkable_position(int radius, const glm::vec3& preferred_dir = glm::vec3(0.0f));

    void animate_movement(float delta_time, MovementSound movement_state);
    bool tick_death(float delta_time);

protected:
    std::shared_ptr<ModelLegacy> m_model;
    Animator m_animator;
    std::unique_ptr<Pathfinding> m_pathfinding;
    std::optional<Path> m_path;

    bool m_following_path = false;

    size_t m_path_index = 0;

    float m_turn_speed = 10;
    float m_stopping_dst = 2;
    float m_speed = 1.0f;
    float m_jump_force = 0.24f;
    bool m_dead = false;
    float m_death_animation_time = 0.0f;
    static constexpr float death_duration = 1.0f;
};
