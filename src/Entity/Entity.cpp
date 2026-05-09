#include "Entity/Entity.hpp"
#include "Engine.hpp"
#include "Rpc.hpp"
#include "World/World.hpp"

Entity::Entity(String serialized_id)
    : m_serialized_id(serialized_id)
{
}

Transform3D Entity::get_global_transform() const
{
    return m_parent ? m_transform.with_parent(m_parent->get_global_transform()) : m_transform;
}

void Entity::add_child(Ref<Entity> entity)
{
    (void)m_children.append(entity);

    entity->set_parent(this);
    entity->m_world = m_world;
    entity->m_id = World::next_id();

    if (entity->get_name().size() == 0)
        entity->set_name(format("Entity #{}", (uint32_t)entity->id()));
}

void Entity::remove_child(size_t index)
{
    m_children.remove_at(index);
}

Ref<Entity> Entity::get_child(size_t index)
{
    return m_children.get_unchecked(index);
}

void Entity::recurse_tick(float delta)
{
    for (Ref<Entity> entity : m_children)
        entity->recurse_tick(delta);
    tick(delta);
}

void Entity::call_rpc(String name, Rpc rpc, Vector<Variant> args)
{
    if (rpc.target == RpcTarget::Both || (Engine::singleton->is_server() && rpc.target == RpcTarget::Server) || (!Engine::singleton->is_server() && rpc.target == RpcTarget::Client))
    {
        rpc.func(this, args);
    }

    if (rpc.target == RpcTarget::Both || (Engine::singleton->is_server() && rpc.target == RpcTarget::Client) || (!Engine::singleton->is_server() && rpc.target == RpcTarget::Server))
    {
        RpcCallPacket p;
        p.id = id();
        p.name = name;

        // FIXME: that sucks but using the STL is way easier with json...
        p.args.reserve(args.size());

        for (const Variant& v : args)
            p.args.push_back(v);

        if (Engine::singleton->is_server())
            Engine::singleton->get_connection().broadcast(Engine::singleton->get_connection().create_packet(p));
        else
            Engine::singleton->get_connection().send(Engine::singleton->get_connection().create_packet(p));
    }
}
