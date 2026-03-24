#pragma once

#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Core/Ref.hpp"

class PhysicsBody : public Object
{
    CLASS(PhysicsBody, Object);

public:
    PhysicsBody(const AABB& aabb)
        : m_aabb(aabb)
    {
    }

private:
    AABB m_aabb;
};

class PhysicsSystem
{
public:
    PhysicsSystem();

    void step();

    void add_body(Ref<PhysicsBody> body) { m_bodies.push_back(body); }

private:
    std::vector<Ref<PhysicsBody>> m_bodies;
};
