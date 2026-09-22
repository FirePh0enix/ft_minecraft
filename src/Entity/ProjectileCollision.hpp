#pragma once

#include "AABB.hpp"
#include "Ray.hpp"

#include <algorithm>
#include <optional>

// Distance to the first contact, including rays starting inside the box.
// Parallel axes are handled explicitly to avoid 0 * infinity at box edges.
inline std::optional<double> projectile_contact(const Ray& ray, const AABBd& box, double range)
{
    double near = 0.0;
    double far = range;
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
        near = std::max(near, first);
        far = std::min(far, last);
        if (near > far)
            return std::nullopt;
    }
    return near;
}
