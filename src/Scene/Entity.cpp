#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

Entity::Entity()
{
}

void Entity::add_child(Ref<Entity> entity)
{
    m_children.push_back(entity);

    // TODO: Move this (and the same code in Scene.hpp) in a common function.
    entity->set_parent(this);
    entity->set_id(Scene::allocate_next_id());
    entity->start(); // this must be after calling setters
}
