#pragma once

#include "AABB.hpp"
#include "Core/Math.hpp"

class Frustum
{
public:
    Frustum();
    Frustum(glm::mat4 mat);

    bool contains(const AABBf& aabb) const;

private:
    glm::vec4 m_planes[6];

    void normalize_plane(size_t side);
};
