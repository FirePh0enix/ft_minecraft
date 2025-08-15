#pragma once

#include "Core/Class.hpp"

class Entity;

/**
 * Components are piece of code that can be attached to an entity that contains the game logic.
 * Each component type can be only added once per entity.
 */
class Component : public Object
{
    CLASS(Component, Object);

public:
    virtual void start()
    {
    }

    virtual void tick(double delta)
    {
        (void)delta;
    }

    inline Entity *get_entity() const
    {
        return m_entity;
    }

    inline void set_entity(Entity *entity)
    {
        m_entity = entity;
    }

protected:
    Entity *m_entity = nullptr; // FIXME: Should be an EntityId or Ref<Entity>
};
