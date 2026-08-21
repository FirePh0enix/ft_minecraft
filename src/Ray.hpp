#pragma once

#include "Core/Math.hpp"

class Ray
{
public:
    Ray(glm::dvec3 origin, glm::dvec3 dir)
        : m_origin(origin), m_dir(dir)
    {
    }

    inline glm::dvec3 origin() const
    {
        return m_origin;
    }

    inline glm::dvec3 dir() const
    {
        return m_dir;
    }

    glm::dvec3 at(double t) const
    {
        return m_origin + m_dir * t;
    }

private:
    glm::dvec3 m_origin;
    glm::dvec3 m_dir;
};
