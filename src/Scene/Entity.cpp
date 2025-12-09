#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

Entity::Entity()
{
}

Ref<Component> Entity::get_component(ClassHashCode class_hash) const
{
    for (const Ref<Component>& comp : m_components)
        if (comp->is(class_hash))
            return comp;
    return nullptr;
}

void Entity::add_component(Ref<Component> comp)
{
    ERR_COND_VR(comp->get_entity() != nullptr, "Component of type {} was already added to entity {}", comp->get_class_name(), (uint32_t)comp->get_entity()->id());

    comp->set_entity(this);
    m_components.push_back(comp);
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
    entity->m_id = Scene::allocate_next_id();

    if (entity->get_name().empty())
        entity->set_name(format("Entity #{}", (uint32_t)entity->id()));
}
