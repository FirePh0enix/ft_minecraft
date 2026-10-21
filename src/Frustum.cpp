#include "Frustum.hpp"

Frustum::Frustum()
    : Frustum(glm::mat4(1.0))
{
}

Frustum::Frustum(glm::mat4 mat)
    : m_planes{glm::vec4(mat[0][3] - mat[0][0],
                         mat[1][3] - mat[1][0],
                         mat[2][3] - mat[2][0],
                         mat[3][3] - mat[3][0]),
               glm::vec4(mat[0][3] + mat[0][0],
                         mat[1][3] + mat[1][0],
                         mat[2][3] + mat[2][0],
                         mat[3][3] + mat[3][0]),
               glm::vec4(mat[0][3] + mat[0][1],
                         mat[1][3] + mat[1][1],
                         mat[2][3] + mat[2][1],
                         mat[3][3] + mat[3][1]),
               glm::vec4(mat[0][3] - mat[0][1],
                         mat[1][3] - mat[1][1],
                         mat[2][3] - mat[2][1],
                         mat[3][3] - mat[3][1]),
               glm::vec4(mat[0][3] - mat[0][2],
                         mat[1][3] - mat[1][2],
                         mat[2][3] - mat[2][2],
                         mat[3][3] - mat[3][2]),
               glm::vec4(mat[0][3] + mat[0][2],
                         mat[1][3] + mat[1][2],
                         mat[2][3] + mat[2][2],
                         mat[3][3] + mat[3][2])}
{
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

bool Frustum::contains(const AABBf& aabb) const
{
    for (const glm::vec4& plane : m_planes)
    {
        // A plane's maximum dot product over an axis-aligned box is at the
        // corner farthest along its normal. If that corner is outside, every
        // corner is outside; this replaces up to eight tests with one.
        const float x = plane.x >= 0.0f ? aabb.max.x : aabb.min.x;
        const float y = plane.y >= 0.0f ? aabb.max.y : aabb.min.y;
        const float z = plane.z >= 0.0f ? aabb.max.z : aabb.min.z;
        if (!(plane.x * x + plane.y * y + plane.z * z + plane.w > 0.0f))
            return false;
    }

    return true;
}
