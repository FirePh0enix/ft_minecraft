#pragma once

#include "AABB.hpp"

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
    Grid,
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
    BoxCollider(glm::vec3 center, glm::vec3 half_extent)
        : Collider(ColliderType::Box), aabb(center, half_extent)
    {
    }

    AABB aabb;
};

struct GridCollider : public Collider
{
    GridCollider(glm::vec3 voxel_size, size_t width, size_t height, size_t depth)
        : Collider(ColliderType::Grid), voxel_size(voxel_size), width(width), height(height), depth(depth)
    {
        bits.resize(width * height * depth / 8);
    }

    void set(size_t x, size_t y, size_t z)
    {
        const size_t index = x * width * height + y * width + z;
        bits[index / 8] |= (1 << (index % 8));
    }

    void clr(size_t x, size_t y, size_t z)
    {
        const size_t index = x * width * height + y * width + z;
        bits[index / 8] &= ~(1 << (index % 8));
    }

    bool has(size_t x, size_t y, size_t z) const
    {
        const size_t index = x * width * height + y * width + z;
        return bits[index / 8] & (1 << (index % 8));
    }

    glm::vec3 voxel_size;
    size_t width;
    size_t height;
    size_t depth;
    std::vector<uint8_t> bits;
};
