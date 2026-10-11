#pragma once

#include "Core/Math.hpp"

struct Path
{
    std::vector<glm::vec3> look_points;
    size_t finish_line_index;

    explicit Path(const std::vector<glm::vec3>& waypoints)
        : look_points(waypoints),
          finish_line_index(waypoints.empty() ? 0 : waypoints.size() - 1)
    {
    }
};
