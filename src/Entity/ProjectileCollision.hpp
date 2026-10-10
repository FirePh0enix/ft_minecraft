#pragma once

#include "AABB.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <optional>

// Distance to the first contact, including rays starting inside the box.
// Parallel axes are handled explicitly to avoid 0 * infinity at box edges.
inline std::optional<double> projectile_contact(const Ray& ray, const AABBd& box, double range)
{
    double near_distance = 0.0;
    double far_distance = range;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (ray.dir()[axis] == 0.0)
        {
            if (ray.origin()[axis] < box.min[axis] || ray.origin()[axis] > box.max[axis])
                return std::nullopt;
            continue;
        }
        double first = (box.min[axis] - ray.origin()[axis]) / ray.dir()[axis];
        double last = (box.max[axis] - ray.origin()[axis]) / ray.dir()[axis];
        if (first > last)
            std::swap(first, last);
        near_distance = std::max(near_distance, first);
        far_distance = std::min(far_distance, last);
        if (near_distance > far_distance)
            return std::nullopt;
    }
    return near_distance;
}
