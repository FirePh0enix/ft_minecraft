#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

Entity::Entity()
{
}

void Entity::add_child(Ref<Entity> entity)
{
    m_children.push_back(entity);

    entity->set_parent(this);
    entity->set_id(Scene::allocate_next_id());
}
