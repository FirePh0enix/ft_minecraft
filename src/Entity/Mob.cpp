#include "Entity/Mob.hpp"
#include "World/World.hpp"

// Based on https://medium.com/@andrebluntindie/3d-aabb-collision-detection-and-resolution-for-voxel-games-5fcbfdb8cdb4
void Mob::move_and_collide()
{
    AABB mob_box = m_aabb.translate(m_transform.position());
    const Dimension& dimension = m_world->get_dimension(m_dimension);
    const Vector<AABB> colliders = dimension.get_boxes_that_may_collide(mob_box);

    glm::vec3 move_vector = m_velocity;
    glm::vec3 original_vector = move_vector;

    for (AABB collider : colliders)
        move_vector.y = mob_box.get_clip_y(collider, move_vector.y);
    mob_box = mob_box.translate(glm::vec3(0, move_vector.y, 0));

    for (AABB collider : colliders)
        move_vector.x = mob_box.get_clip_x(collider, move_vector.x);
    mob_box = mob_box.translate(glm::vec3(move_vector.x, 0, 0));

    for (AABB collider : colliders)
        move_vector.z = mob_box.get_clip_z(collider, move_vector.z);
    mob_box = mob_box.translate(glm::vec3(0, 0, move_vector.z));

    if (move_vector.x != original_vector.x)
        m_velocity.x = 0;
    if (move_vector.y != original_vector.y)
        m_velocity.y = 0;
    if (move_vector.z != original_vector.z)
        m_velocity.z = 0;

    m_on_ground = move_vector.y != original_vector.y && original_vector.y < 0;

    glm::vec3 center = (mob_box.max + mob_box.min) / 2.0f;
    set_position(center);
}
