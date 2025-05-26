#pragma once

class AABB
{
public:
    AABB()
        : m_center(0.0), m_half_extent(0.0)
    {
    }

    AABB(glm::vec3 center, glm::vec3 half_extent)
        : m_center(center), m_half_extent(half_extent)
    {
    }

    inline glm::vec3 center() const
    {
        return m_center;
    }

    inline glm::vec3 half_extent() const
    {
        return m_half_extent;
    }

private:
    glm::vec3 m_center;
    glm::vec3 m_half_extent;
};
