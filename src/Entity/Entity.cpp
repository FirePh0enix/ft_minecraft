#include "Entity/Entity.hpp"
#include "World/World.hpp"

Entity::Entity()
{
}

void Entity::add_child(Ref<Entity> entity)
{
    m_children.push_back(entity);

    entity->set_parent(this);
    entity->m_world = m_world;
    entity->m_id = World::next_id();

    if (entity->get_name().empty())
        entity->set_name(format("Entity #{}", (uint32_t)entity->id()));
}

void Entity::recurse_tick(float delta)
{
    for (Ref<Entity> entity : m_children)
        entity->recurse_tick(delta);
    tick(delta);
}
