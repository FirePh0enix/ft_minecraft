#include "Entity/LivingEntity.hpp"

void LivingEntity::damage(int value, EntityId damage_source)
{
    if (is_dead())
        return;

    m_health -= value;
    m_last_damaged_source = damage_source;
    on_damage(value, damage_source);

    if (m_health <= 0)
        die();
}
