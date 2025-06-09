#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"

class Entity;

class Component : public Object
{
    CLASS(Component, Object);

public:
    virtual void start() {}
    virtual void tick(double delta) {}

    inline void set_entity(Entity *entity)
    {
        m_entity = entity;
    }

protected:
    Entity *m_entity;
};
