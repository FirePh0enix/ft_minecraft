#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

Entity::Entity()
{
}

Ref<Transformed3D> Entity::get_transform() const
{
    return get_component<Transformed3D>();
}

void Entity::add_child(Ref<Entity> entity)
{
    m_children.push_back(entity);

    entity->set_parent(this);
    entity->set_scene(m_scene);
    entity->set_id(Scene::allocate_next_id());

    if (entity->get_name().empty())
        entity->set_name(format("Entity #{}", (uint32_t)entity->get_id()));
}
