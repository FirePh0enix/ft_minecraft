#pragma once

#include "Core/Math.hpp"

class Ray
{
public:
    Ray(glm::vec3 origin, glm::vec3 dir)
        : m_origin(origin), m_dir(dir)
    {
    }

    inline glm::vec3 origin() const
    {
        return m_origin;
    }

    inline glm::vec3 dir() const
    {
        return m_dir;
    }

    glm::vec3 at(float t) const
    {
        return m_origin + m_dir * t;
    }

private:
    glm::vec3 m_origin;
    glm::vec3 m_dir;
};
