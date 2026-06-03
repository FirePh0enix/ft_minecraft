
#pragma once

#include "Core/Containers/Vector.hpp"
#include "Core/Math.hpp"

struct Path
{
    Vector<glm::vec3> look_points;
    size_t finish_line_index;
    size_t slow_down_index;

    Path(const Vector<glm::vec3>& waypoints, float stopping_dst)
    {
        look_points = waypoints;
        finish_line_index = look_points.size() > 0 ? look_points.size() - 1 : 0;
        slow_down_index = 0;

        float dst_from_end_point = 0;
        if (look_points.size() > 0)
        {
            for (size_t i = look_points.size() - 1; i > 0; i--)
            {
                dst_from_end_point += glm::distance(look_points.get_unchecked(i), look_points.get_unchecked(i - 1));
                if (dst_from_end_point > stopping_dst)
                {
                    slow_down_index = i;
                    break;
                }
            }
        }
    }
};