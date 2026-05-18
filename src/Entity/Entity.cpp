#include "Entity/Entity.hpp"
#include "Core/Result.hpp"
#include "Engine.hpp"
#include "Network/Network.hpp"
#include "Type.hpp"
#include "World/World.hpp"

void Entity::bind_methods()
{
    EXPECT(type.add_property("position", &Entity::get_position, &Entity::set_position));
    expose_rpc<Entity>("set/position");

    EXPECT(type.add_property("rotation", &Entity::get_rotation, &Entity::set_rotation));
    expose_rpc<Entity>("set/rotation");
}

Entity::Entity()
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

std::optional<RpcTarget> Entity::get_rpc(StringView name)
{
    for (ssize_t i = (ssize_t)get_classes().size(); i >= 0; i--)
    {
        ClassHashCode class_hash = get_classes()[i];
        if (s_exposed_rpc.contains(class_hash))
        {
            const auto& rpcs = s_exposed_rpc.at(class_hash);
            if (rpcs.contains(name))
                return rpcs.at(name);
        }
    }
    return std::nullopt;
}

void Entity::call_rpc(StringView name, View<Variant> args)
{
    auto rpc_target_maybe = get_rpc(name);
    if (!rpc_target_maybe.has_value())
        return;

    RpcTarget rpc_target = rpc_target_maybe.value();

    if (Engine::get().is_online() || rpc_target == RpcTarget::Both || (Engine::singleton->is_server() && rpc_target == RpcTarget::Server) || (!Engine::singleton->is_server() && rpc_target == RpcTarget::Client))
    {
        if (name.starts_with("set/"))
        {
            const Type::Property& property = get_type()->get_property(name.slice(4));
            property.setter((Object *)this, Arguments{.args = args});
        }
        else
        {
            const Type::Method& method = get_type()->get_method(name);
            method.func((Object *)this, Arguments{.args = args});
        }
    }

    if (Engine::get().is_online() && (rpc_target == RpcTarget::Both || (Engine::singleton->is_server() && rpc_target == RpcTarget::Client) || (!Engine::singleton->is_server() && rpc_target == RpcTarget::Server)))
    {
        RpcCallPacket p;
        p.id = id();
        p.name = name;

        // FIXME: that sucks but using the STL is way easier with json...
        p.args.reserve(args.size());

        for (const Variant& v : args)
            p.args.push_back(v);

        if (Engine::singleton->is_server())
            Engine::singleton->connection().broadcast(Engine::singleton->connection().create_packet(p));
        else
            Engine::singleton->connection().send(Engine::singleton->connection().create_packet(p));
    }
}
