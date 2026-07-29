#include "Entity/Camera.hpp"

void Camera::tick(float delta)
{
    (void)delta;
    update_frustum();
}

void Camera::update_frustum()
{
    m_frustum = Frustum(get_view_proj_matrix());
}
