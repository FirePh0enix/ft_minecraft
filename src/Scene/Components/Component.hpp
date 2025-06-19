#pragma once

#include "Core/Class.hpp"

class Entity;

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
    Entity *m_entity = nullptr;
};
