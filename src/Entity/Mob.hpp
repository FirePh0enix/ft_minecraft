#pragma once

#include "Entity/Entity.hpp"
#include "World/Chunk.hpp"

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

    void move_and_collide(bool enable_collision = true);
    virtual void on_hit_by(Entity *entity) override { (void)entity; };

protected:
    bool m_on_ground = true;

    glm::vec3 m_velocity = glm::vec3();

    int m_health;
    int m_max_health;
    int m_attack_damage;

    float m_speed;
    float m_jump_force;

    virtual void die() {};
    void take_damage(int amount) { m_health -= amount; };

    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
    Ref<Shader> m_shader;
    Ref<Buffer> m_model_buffer;
    Model m_model;
};
