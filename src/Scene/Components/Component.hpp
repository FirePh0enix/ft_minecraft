#pragma once

#include "Core/Class.hpp"

class Component : public Object
{
    CLASS(Component, Object);

public:
    virtual void start() {}

    virtual void tick() {}
};
