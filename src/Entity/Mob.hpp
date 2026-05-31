#pragma once

#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/Path.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"
#include "Model.hpp"

/**
 * @brief An entity controlled by inputs or AI.
 */
class Mob : public Entity
{
    CLASS(Mob, Entity);

public:
    Mob() : m_health(1), m_max_health(1), m_attack_damage(1), m_speed(1), m_jump_force(1) {}
    Mob(int health, int attack_damage, float speed, float jump_force) : m_health(health), m_max_health(health), m_attack_damage(attack_damage), m_speed(speed), m_jump_force(jump_force) {}

    ALWAYS_INLINE int get_attack_damage() const { return m_attack_damage; }

    void move_and_collide();

    void follow_path(float delta_time);
    void flee_to(const glm::ivec3& to);
    bool verify_if_path_still_valid();
    glm::ivec3 find_random_walkable_position(int radius, const glm::vec3& preferred_dir = glm::vec3(0.0f));

    virtual void on_hit_by(Entity& entity) { (void)entity; }

    static glm::vec3 safe_normalize(const glm::vec3& v);

    void take_damage(int amount) { m_health -= amount; }

protected:
    int m_health;
    int m_max_health;
    int m_attack_damage;

    float m_speed;
    float m_jump_force;

    virtual void die() {}

    Ref<Model> m_model;

    bool m_blocked_x = false;
    bool m_blocked_z = false;

    std::unique_ptr<Pathfinding> m_pathfinding;
    bool m_following_path = false;
    std::optional<Path> m_path;
    size_t m_path_index = 0;
    float m_turn_speed = 10;
    float m_stopping_dst = 2;
};
