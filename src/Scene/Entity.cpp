#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

Entity::Entity()
{
}

const std::map<ClassHashCode, Ref<Component>>& Entity::get_components() const
{
    return m_components;
}

Ref<Component> Entity::get_component(ClassHashCode class_hash) const
{
    if (m_components.contains(class_hash))
        return m_components.at(class_hash);
    return nullptr;
}

void Entity::add_component(Ref<Component> comp)
{
    ERR_COND_VR(comp->get_entity() != nullptr, "Component of type {} was already added to entity {}", comp->get_class_name(), (uint32_t)comp->get_entity()->get_id());

    comp->set_entity(this);
    m_components[comp->get_class_hash_code()] = comp;
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
