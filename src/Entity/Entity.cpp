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

    if (entity->get_name().size() == 0)
        entity->set_name(format("Entity #{}", (uint32_t)entity->id()));
}

void Entity::recurse_tick(float delta)
{
    for (Ref<Entity> entity : m_children)
        entity->recurse_tick(delta);
    tick(delta);
}

void Entity::register_rpc(const std::string& name, std::function<void(Entity&)> func, RpcTarget target)
{
    m_rpc[name] = {.func = func, .target = target};
}

void Entity::call_rpc(const std::string& name, Entity& caller)
{
    auto iter = m_rpc.find(name);
    if (iter != m_rpc.end())
    {
        RpcEntry& entry = iter->second;

        if (entry.target == RpcTarget::SERVER || entry.target == RpcTarget::BOTH)
        {
            entry.func(caller);
        }
    }
}