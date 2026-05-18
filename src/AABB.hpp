#pragma once

#include "Core/Math.hpp"

struct CollisionResult
{
    glm::vec3 normal = glm::vec3(0.0);
    float penetration = 0.0;
};

/**
 * Axis-Aligned Bounding Box.
 */
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    AABB()
        : min(0.0), max(0.0)
    {
    }

    AABB(glm::vec3 min, glm::vec3 max)
        : min(min), max(max)
    {
    }

    static AABB from_center_extent(glm::vec3 center, glm::vec3 half_extent)
    {
        return AABB(center - half_extent, center + half_extent);
    }

    glm::vec3 center() const
    {
        return (min + max) / 2.0f;
    }

    AABB expand(glm::vec3 v) const
    {
        AABB aabb = *this;

        if (v.x > 0)
            aabb.max.x += v.x;
        else
            aabb.min.x += v.x;

        if (v.y > 0)
            aabb.max.y += v.y;
        else
            aabb.min.y += v.y;

        if (v.z > 0)
            aabb.max.z += v.z;
        else
            aabb.min.z += v.z;

        return aabb;
    }

    AABB grow(glm::vec3 v) const
    {
        AABB aabb = *this;
        aabb.min -= v;
        aabb.max += v;
        return aabb;
    }

    AABB translate(glm::vec3 v) const
    {
        AABB aabb = *this;
        aabb.min += v;
        aabb.max += v;
        return aabb;
    }

    bool intersect_x(const AABB& o) const { return min.x < o.max.x && max.x > o.min.x; }
    bool intersect_y(const AABB& o) const { return min.y < o.max.y && max.y > o.min.y; }
    bool intersect_z(const AABB& o) const { return min.z < o.max.z && max.z > o.min.z; }
    bool intersect(const AABB& o) const { return intersect_x(o) && intersect_y(o) && intersect_z(o); }

    bool intersect(glm::vec3 position) const { return position.x >= min.x && position.x <= max.x && position.y >= min.y && position.y <= max.y && position.z >= min.z && position.z <= max.z; }

    float get_clip_x(const AABB& o, float d) const
    {
        if (intersect_y(o) && intersect_z(o))
        {
            if (d > 0 && max.x <= o.min.x)
            {
                float clip = o.min.x - max.x;
                if (d > clip)
                    d = clip;
            }
            if (d < 0 && min.x >= o.max.x)
            {
                float clip = o.max.x - min.x;
                if (d < clip)
                    d = clip;
            }
            return d;
        }
        return d;
    }

    float get_clip_y(const AABB& o, float d) const
    {
        if (intersect_x(o) && intersect_z(o))
        {
            if (d > 0 && max.y <= o.min.y)
            {
                float clip = o.min.y - max.y;
                if (d > clip)
                    d = clip;
            }
            if (d < 0 && min.y >= o.max.y)
            {
                float clip = o.max.y - min.y;
                if (d < clip)
                    d = clip;
            }
            return d;
        }
        return d;
    }

    float get_clip_z(const AABB& o, float d) const
    {
        if (intersect_x(o) && intersect_y(o))
        {
            if (d > 0 && max.z <= o.min.z)
            {
                float clip = o.min.z - max.z;
                if (d > clip)
                    d = clip;
            }
            if (d < 0 && min.z >= o.max.z)
            {
                float clip = o.max.z - min.z;
                if (d < clip)
                    d = clip;
            }
            return d;
        }
        return d;
    }
};
