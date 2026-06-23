#pragma once

#include "Entity/Entity.hpp"
#include "Mob.hpp"

class Zombie : public Mob
{
public:
    Zombie() : Mob(3)
    {
        m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
    }

    void start() override;
    void tick(float delta) override;
    void on_ready() override;
    void attack();

protected:
    float m_stopping_dst = 0.1f;

    Ref<Entity> m_threat_entity;

    float m_attack_range = 1.0f;
    float m_attack_cooldown = 1.5f;
    float m_attack_timer = 0.0f;
    float m_path_update_timer = 0.0f;
};
