#pragma once

#include "Core/Class.hpp"
#include "Entity/Entity.hpp"
#include "Frustum.hpp"

class Camera : public Entity
{
    CLASS(Camera, Entity);

public:
    Camera()
        : m_projection_matrix(1.0)
    {
        // Default values for field members are set after the initializer list so this need to be here.
        m_projection_matrix = calculate_projection_matrix();
    }

    virtual ~Camera() {}

    virtual void tick(float delta) override;

    void update_projection(float aspect_ratio)
    {
        m_aspect_ratio = aspect_ratio;
        m_projection_matrix = calculate_projection_matrix();
    }

    glm::mat4 get_view_matrix() const
    {
        const Transform3D global_transform = get_global_transform();
        const glm::mat4 rotation = glm::toMat4(global_transform.rotation());
        const glm::mat4 translation = glm::translate(glm::mat4(1.0), -global_transform.position());
        return rotation * translation;
    }

    glm::mat4 get_rotation_matrix() const
    {
        return glm::toMat4(get_global_transform().rotation());
    }

    glm::mat4 get_inv_view_matrix() const
    {
        const Transform3D global_transform = get_global_transform();
        const glm::mat4 rotation = glm::toMat4(-global_transform.rotation());
        const glm::mat4 translation = glm::translate(glm::mat4(1.0), global_transform.position());
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

    inline const Frustum& frustum() const
    {
        return m_frustum;
    }

    inline Frustum& frustum()
    {
        return m_frustum;
    }

    void update_frustum();

    float near_plane() const { return m_near; }
    float far_plane() const { return m_far; }

private:
    glm::mat4 m_projection_matrix;
    float m_aspect_ratio = 1280.0 / 720.0;
    float m_fov = 70.0;
    Frustum m_frustum;

    float m_near = 0.01;
    float m_far = 800.0;

    glm::mat4 calculate_projection_matrix() const
    {
        glm::mat4 projection_matrix = glm::perspective((float)glm::radians(m_fov), m_aspect_ratio, m_near, m_far);

        // #ifndef __platform_web
        // FIXME: For some reason this is needed on desktop and not web.
        // projection_matrix[1][1] *= -1;
        // #endif

        return projection_matrix;
    }
};
