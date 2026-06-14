#pragma once

#include "Entity/Entity.hpp"
#include "Model.hpp"

class LivingEntity : public Entity
{
    CLASS(LivingEntity, Entity);

public:
    LivingEntity()
    {
    }

    explicit LivingEntity(int health)
        : m_max_health(health), m_health(health)
    {
    }

    void damage(int value, EntityId damage_source);

    virtual void on_damage(int value, EntityId damage_source)
    {
        (void)value;
        (void)damage_source;
    }
    virtual void die() {}

protected:
    int m_max_health = 20;
    int m_health = 20;

    EntityId m_last_damaged_source;
};
