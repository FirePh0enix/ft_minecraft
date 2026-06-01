#pragma once

#include "Entity/Entity.hpp"
#include "Mob.hpp"
#include <cstddef>

class Zombie : public Mob
{
public:
    Zombie() : Mob(3, 0, 3.0f, 1.0f)
    {
        m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
        m_attack_damage = 1;
    }

    void start() override;
    void tick(float delta) override;
    void draw(const RenderPassNode& node) override;
    void on_ready() override;
    void on_hit_by(Entity& entity) override;
    bool is_player_in_radius(float radius, const glm::vec3& player_pos) const;
    void attack();

protected:
    void die() override;

    float m_stopping_dst = 0.1f;

    Ref<Entity> m_threat_entity;
    glm::ivec3 m_last_threat_pos = glm::ivec3();

    float m_attack_range = 1.0f;
    bool m_can_attack = false;
    float m_attack_cooldown = 1.5f;
    float m_attack_timer = 0.0f;
    float m_path_update_timer = 0.0f;
};
