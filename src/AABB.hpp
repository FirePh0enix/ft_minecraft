#pragma once

#include "Core/Definitions.hpp"
#include "Core/Math.hpp"
#include "Core/Print.hpp"

struct CollisionResult
{
    glm::vec3 normal = glm::vec3(0.0);
    float penetration = 0.0;
};

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

    ALWAYS_INLINE float min_x() const { return center.x - half_extent.x; }
    ALWAYS_INLINE float min_y() const { return center.y - half_extent.y; }
    ALWAYS_INLINE float min_z() const { return center.z - half_extent.z; }
    ALWAYS_INLINE float max_x() const { return center.x + half_extent.x; }
    ALWAYS_INLINE float max_y() const { return center.y + half_extent.y; }
    ALWAYS_INLINE float max_z() const { return center.z + half_extent.z; }

    ALWAYS_INLINE glm::vec3 min() const { return glm::vec3(min_x(), min_y(), min_z()); }
    ALWAYS_INLINE glm::vec3 max() const { return glm::vec3(max_x(), max_y(), max_z()); }

    bool intersect_x(const AABB& o) const
    {
        return min_x() < o.max_x() && max_x() > o.min_x();
    }

    bool intersect_y(const AABB& o) const
    {
        return min_y() < o.max_y() && max_y() > o.min_y();
    }

    bool intersect_z(const AABB& o) const
    {
        return min_z() < o.max_z() && max_z() > o.min_z();
    }

    bool intersect(const AABB& other) const
    {
        return intersect_x(other) && intersect_y(other) && intersect_z(other);
    }

    bool intersect(const AABB& other, CollisionResult& result) const
    {
        constexpr glm::vec3 normals[6] = {
            {-1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, 1.0f},
        };

        const float distances[6] = {
            (other.center.x + other.half_extent.x) - (center.x - half_extent.x),
            (center.x + half_extent.x) - (other.center.x - other.half_extent.x),
            (other.center.y + other.half_extent.y) - (center.y - half_extent.y),
            (center.y + half_extent.y) - (other.center.y - other.half_extent.y),
            (other.center.z + other.half_extent.z) - (center.z - half_extent.z),
            (center.z + half_extent.z) - (other.center.z - other.half_extent.z),
        };

        int collided_face = 0;

        for (int i = 0; i < 6; i++)
        {
            if (distances[i] <= 0.0f)
                return false;

            if (distances[i] < distances[collided_face])
                collided_face = i;
        }

        result.normal = normals[collided_face];
        result.penetration = distances[collided_face];

        return true;
    }

    AABB translate(const glm::vec3& pos) const
    {
        return AABB(center + pos, half_extent);
    }

    glm::vec3 center;
    glm::vec3 half_extent;
};
