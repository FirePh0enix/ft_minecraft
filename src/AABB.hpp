#pragma once

#include "Core/Math.hpp"

struct AABB
{
public:
    AABB()
        : center(0.0), half_extent(0.0)
    {
    }

    AABB(glm::vec3 center, glm::vec3 half_extent)
        : center(center), half_extent(half_extent)
    {
    }

    bool intersect(const AABB& other) const
    {
        return center.x - half_extent.x <= other.center.x + other.half_extent.x &&
               center.x + half_extent.x >= other.center.x - other.half_extent.x &&
               center.y - half_extent.y <= other.center.y + other.half_extent.y &&
               center.y + half_extent.y >= other.center.y - other.half_extent.y &&
               center.z - half_extent.z <= other.center.z + other.half_extent.z &&
               center.z + half_extent.z >= other.center.z - other.half_extent.z;
    }

    glm::vec3 center;
    glm::vec3 half_extent;
};
