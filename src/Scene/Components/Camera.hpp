#pragma once

#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/Components/Transform3D.hpp"

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

class Camera : public Component
{
    CLASS(Camera, Component);

public:
    Camera()
    {
        m_projection_matrix = calculate_projection_matrix();
    }

    virtual void start() override;
    virtual void tick(double delta) override;

    void rotate(float x_rel, float y_rel);

    glm::mat4 get_view_matrix() const
    {
        const glm::mat4 rotation = glm::toMat4(m_transform->get_global_transform().rotation());
        const glm::mat4 translation = glm::translate(glm::mat4(1.0), -m_transform->get_global_transform().position());
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

private:
    Ref<TransformComponent3D> m_transform;

    glm::mat4 m_projection_matrix;
    float m_aspect_ratio = 1280.0 / 720.0;
    float m_fov = 70.0;
    Frustum m_frustum;

    float m_near = 0.01;
    float m_far = 10'000.0;

    glm::mat4 calculate_projection_matrix() const
    {
        glm::mat4 projection_matrix = glm::perspectiveRH((float)glm::radians(m_fov), m_aspect_ratio, m_near, m_far);

#ifndef __platform_web
        // FIXME: For some reason this is needed on desktop and not web.
        projection_matrix[1][1] *= -1;
#endif

        return projection_matrix;
    }
};
