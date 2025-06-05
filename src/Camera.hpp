#pragma once

#include "AABB.hpp"

#include <print>

class Frustum
{
public:
    Frustum();
    Frustum(glm::mat4 mat);

    bool contains(const AABB& aabb) const;

private:
    std::array<glm::vec4, 6> m_planes;

    void normalize_plane(size_t side);
};

class Camera
{
public:
    Camera(glm::vec3 position, glm::vec3 rotation, float speed)
        : m_position(position), m_rotation(rotation), m_speed(speed)
    {
        m_projection_matrix = calculate_projection_matix();
    }

    void tick();
    void rotate(float x_rel, float y_rel);

    glm::vec3 get_forward() const;
    glm::vec3 get_right() const;

    glm::mat4 get_view_matrix() const
    {
        const glm::mat4 rotation = glm::mat4_cast(get_rotation_quat());
        const glm::mat4 translation = glm::translate(glm::mat4(1.0), -m_position);

        return rotation * translation;
    }

    glm::mat4 get_view_proj_matrix() const
    {
        return get_projection_matrix() * get_view_matrix();
    }

    inline glm::mat4 get_projection_matrix() const
    {
        return m_projection_matrix;
    }

    glm::quat get_rotation_quat() const
    {
        glm::quat q_pitch = glm::rotate(glm::identity<glm::quat>(), m_rotation.x, glm::vec3(1.0, 0.0, 0.0));
        glm::quat q_yaw = glm::rotate(glm::identity<glm::quat>(), m_rotation.y, glm::vec3(0.0, 1.0, 0.0));
        glm::quat q_roll = glm::rotate(glm::identity<glm::quat>(), m_rotation.z, glm::vec3(0.0, 0.0, 1.0));

        return glm::normalize(q_pitch * q_yaw * q_roll);
    }

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    float m_speed;
    glm::mat4 m_projection_matrix;
    float m_aspect_ratio = 1280.0 / 720.0;
    float m_fov = 70.0;
    Frustum m_frustum;

    float m_near = 0.01;
    float m_far = 10'000.0;

    glm::mat4 calculate_projection_matix() const
    {
        glm::mat4 projection_matrix = glm::perspectiveRH((float)glm::radians(m_fov), m_aspect_ratio, m_near, m_far);

#ifndef __platform_web
        // FIXME: For some reason this is needed on desktop and not web.
        projection_matrix[1][1] *= -1;
#endif

        return projection_matrix;
    }
};
