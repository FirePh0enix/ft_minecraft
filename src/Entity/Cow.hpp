#pragma once

#include "Entity/Entity.hpp"
#include "Mob.hpp"

class Cow : public Mob
{
public:
    Cow()
        : Mob(3)
    {
        m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
    }

    virtual void on_damage(int damage, EntityId damage_source) override;

    void start() override;
    void tick(float delta) override;
    void on_ready() override;

protected:
    void die() override;
    void flee_from(int radius);

    Ref<Entity> m_threat_entity;
};
