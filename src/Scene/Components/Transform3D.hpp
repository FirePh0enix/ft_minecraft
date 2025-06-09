#pragma once

#include "Core/Class.hpp"
#include "Scene/Components/Component.hpp"

class Transform3D : public Component
{
    CLASS(Transform3D, Component);

public:
    Transform3D()
    {
    }

    Transform3D(glm::vec3 position, glm::vec3 rotation)
        : m_position(position), m_rotation(rotation)
    {
    }

    inline glm::vec3& position()
    {
        return m_position;
    }

    inline const glm::vec3& position() const
    {
        return m_position;
    }

    inline glm::vec3& rotation()
    {
        return m_rotation;
    }

    inline const glm::vec3& rotation() const
    {
        return m_rotation;
    }

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
};
