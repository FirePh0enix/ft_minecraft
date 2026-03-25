#pragma once

#include "Entity/Entity.hpp"

/**
 * @brief An entity controlled by inputs or AI.
 */
class Mob : public Entity
{
    CLASS(Mob, Entity);

public:
    Mob();

    ALWAYS_INLINE bool is_on_ground() const { return m_on_ground; }

    void move_and_collide();

protected:
    bool m_on_ground = false;
    glm::vec3 m_velocity = glm::vec3();
};
