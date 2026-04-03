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
    Mob() : m_health(0), m_max_health(0), m_attack_damage(0), m_speed(0) {}
    Mob(int health, int attack_damage, float speed) : m_health(health), m_max_health(health), m_attack_damage(attack_damage), m_speed(speed) {}

    ALWAYS_INLINE bool is_on_ground() const { return m_on_ground; }

    ALWAYS_INLINE int get_attack_damage() const { return m_attack_damage; }

    void move_and_collide(bool enable_collision);

protected:
    bool m_on_ground = true;
    bool m_jumping = false;

    glm::vec3 m_velocity = glm::vec3();

    int m_health;
    int m_max_health;
    int m_attack_damage;

    float m_speed;

    virtual void die() {};
    void take_damage(int amount) { m_health -= amount; };

    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
    Ref<Shader> m_shader;
    Ref<Buffer> m_model_buffer;
    Model m_model;
};
