#include "Camera.hpp"
#include "Input.hpp"
#include "Scene/Entity.hpp"

Frustum::Frustum()
    : Frustum(glm::mat4(1.0))
{
}

Frustum::Frustum(glm::mat4 mat)
{
    m_planes[0] = glm::vec4(mat[0][3] - mat[0][0],
                            mat[1][3] - mat[1][0],
                            mat[2][3] - mat[2][0],
                            mat[3][3] - mat[3][0]);
    m_planes[1] = glm::vec4(mat[0][3] + mat[0][0],
                            mat[1][3] + mat[1][0],
                            mat[2][3] + mat[2][0],
                            mat[3][3] + mat[3][0]);
    m_planes[2] = glm::vec4(mat[0][3] + mat[0][1],
                            mat[1][3] + mat[1][1],
                            mat[2][3] + mat[2][1],
                            mat[3][3] + mat[3][1]);
    m_planes[3] = glm::vec4(mat[0][3] - mat[0][1],
                            mat[1][3] - mat[1][1],
                            mat[2][3] - mat[2][1],
                            mat[3][3] - mat[3][1]);
    m_planes[4] = glm::vec4(mat[0][3] - mat[0][2],
                            mat[1][3] - mat[1][2],
                            mat[2][3] - mat[2][2],
                            mat[3][3] - mat[3][2]);
    m_planes[5] = glm::vec4(mat[0][3] + mat[0][2],
                            mat[1][3] + mat[1][2],
                            mat[2][3] + mat[2][2],
                            mat[3][3] + mat[3][2]);

    normalize_plane(0);
    normalize_plane(1);
    normalize_plane(2);
    normalize_plane(3);
    normalize_plane(4);
    normalize_plane(5);
}

void Frustum::normalize_plane(size_t side)
{
    const float magnitude = std::sqrt(m_planes[side][0] * m_planes[side][0] + m_planes[side][1] * m_planes[side][1] + m_planes[side][2] * m_planes[side][2] + m_planes[side][3] * m_planes[side][3]);
    m_planes[side][0] /= magnitude;
    m_planes[side][1] /= magnitude;
    m_planes[side][2] /= magnitude;
    m_planes[side][3] /= magnitude;
}

bool Frustum::contains(const AABB& aabb) const
{
    for (size_t i = 0; i < 6; i++)
    {
        if (m_planes[i][0] * (aabb.center()[0] - aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] - aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] - aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] + aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] - aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] - aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] - aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] + aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] - aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] + aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] + aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] - aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] - aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] - aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] + aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] + aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] - aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] + aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] - aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] + aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] + aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;
        if (m_planes[i][0] * (aabb.center()[0] + aabb.half_extent()[0]) + m_planes[i][1] * (aabb.center()[1] + aabb.half_extent()[1]) + m_planes[i][2] * (aabb.center()[2] + aabb.half_extent()[2]) + m_planes[i][3] > 0)
            continue;

        return false;
    }

    return true;
}

void Camera::start()
{
    m_transform = m_entity->get_component<TransformComponent3D>();
}

void Camera::tick(double delta)
{
    m_frustum = Frustum(get_view_proj_matrix());
}

void Camera::rotate(float x_rel, float y_rel)
{
    Transform3D transform = m_transform->get_transform();

    const glm::vec3 up(0.0, 1.0, 0.0);
    const glm::vec3 pitch_axis = glm::cross(get_forward(), up);

    glm::quat q_pitch = glm::angleAxis(y_rel * 0.01f, pitch_axis);
    glm::quat q_yaw = glm::angleAxis(x_rel * 0.01f, up);

    transform.rotation() *= glm::cross(q_pitch, q_yaw);

    m_transform->set_transform(transform);
}

glm::vec3 Camera::get_forward() const
{
    return glm::conjugate(m_transform->get_global_transform().rotation()) * glm::vec3(0.0, 0.0, -1.0);
}

glm::vec3 Camera::get_right() const
{
    return glm::conjugate(m_transform->get_global_transform().rotation()) * glm::vec3(1.0, 0.0, 0.0);
}
