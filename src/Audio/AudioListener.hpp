#pragma once

#include "Core/Math.hpp"

class AudioListener
{
public:
    void set_position(const glm::vec3& position) { m_position = position; }
    void set_forward(const glm::vec3& forward) { m_forward = forward; }
    void set_up(const glm::vec3& up) { m_up = up; }

    const glm::vec3& get_position() const { return m_position; }
    const glm::vec3& get_forward() const { return m_forward; }
    const glm::vec3& get_up() const { return m_up; }

private:
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
};
