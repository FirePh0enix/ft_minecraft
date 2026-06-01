#pragma once

#include "Entity/Entity.hpp"
#include "Model.hpp"

class LivingEntity : public Entity
{
public:
    LivingEntity() = delete;
    explicit LivingEntity(int health, int attack_damage, float speed, float jump_force) : m_health(health), m_max_health(health), m_attack_damage(attack_damage), m_speed(speed), m_jump_force(jump_force) {}
    void take_damage(int amount) { m_health -= amount; }
    ALWAYS_INLINE int get_attack_damage() const { return m_attack_damage; }

    virtual void on_hit_by(Entity& entity) { (void)entity; }

    virtual void die() {}

protected:
    int m_health = 1;
    int m_max_health = 1;
    int m_attack_damage = 1;
    float m_speed = 1;
    float m_jump_force = 1;

    Ref<Model> m_model;
};