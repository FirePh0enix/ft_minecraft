#pragma once

struct Collider;
class PhysicsBody;

struct CollisionResult
{
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 normal;
    float depth;
    PhysicsBody *body;
    bool has_collision;

    operator bool() const { return has_collision; }
};

typedef CollisionResult (*TestCollisionFunc)(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b);

enum class ColliderType
{
    Box,
};

struct Collider
{
    Collider(ColliderType type)
        : type(type)
    {
    }

    ColliderType type;
};

struct BoxCollider : public Collider
{
    BoxCollider(glm::vec3 min, glm::vec3 max)
        : Collider(ColliderType::Box), min(min), max(max)
    {
    }

    glm::vec3 min;
    glm::vec3 max;
};
