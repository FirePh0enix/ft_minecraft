#include "Entity/Entity.hpp"

#include "Core/Filesystem.hpp"
#include "Core/Result.hpp"
#include "Engine.hpp"
#include "Network/Network.hpp"
#include "Type.hpp"
#include "Variant.hpp"
#include "World/World.hpp"

Result<void> EntitySerializer::save(const StringView& path) const
{
    File file = TRY(Filesystem::open_file(path, true));
    FileWriter writer = file.writer();

    for (auto& [_, name, value] : m_variants)
    {
        TRY(writer.write_variant(Variant(name)));
        TRY(writer.write_variant(value));
    }

    file.close();
    return Result<void>();
}

Result<void> EntitySerializer::load(const StringView& path)
{
    File file = TRY(Filesystem::open_file(path));
    FileReader reader = file.reader();

    while (!reader.eof())
    {
        Option<Variant> vname_opt = TRY(reader.read_variant());
        if (!vname_opt.has_value())
            break;

        Variant vname = vname_opt.value();
        ASSERT_V(vname.has(VariantType::String), "");
        String s = vname.get_unchecked<String>();

        Variant value = TRY(reader.read_variant()).value();
        m_variants.put(s, value);
    }

    file.close();
    return Result<void>();
}

void Entity::bind_methods()
{
    type.add_property("position", &Entity::get_position, &Entity::set_position);
    expose_rpc<Entity>("set/position");

    type.add_property("rotation", &Entity::get_rotation, &Entity::set_rotation);
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
    entity->m_id = World::next_id(); // FIXME: don't do that
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

Option<RpcTarget> Entity::get_rpc(StringView name)
{
    return Engine::get().registry().get_rpc(this, name);
}

void Entity::expose_rpc(ClassHashCode cls, String name, RpcTarget target)
{
    Engine::get().registry().register_rpc(cls, name, target);
}

void Entity::call_rpci(StringView name, View<Variant> args)
{
    auto rpc_target_maybe = get_rpc(name);
    if (!rpc_target_maybe.has_value()) {
        return;
    }

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
            // method.func((Object *)this, Arguments{.args = args});
        }
    }

    if (Engine::get().is_online() && (rpc_target == RpcTarget::Both || (Engine::singleton->is_server() && rpc_target == RpcTarget::Client) || (!Engine::singleton->is_server() && rpc_target == RpcTarget::Server)))
    {
        RpcCallPacket p;
        p.id = id();
        p.name = name;
	
	for (const Variant& v : args) {
	    p.args.append(v);
	}

        if (Engine::singleton->is_server())
            Engine::singleton->connection().broadcast(Engine::singleton->connection().create_packet(p));
        else
            Engine::singleton->connection().send(Engine::singleton->connection().create_packet(p));
    }
}

// Based on https://medium.com/@andrebluntindie/3d-aabb-collision-detection-and-resolution-for-voxel-games-5fcbfdb8cdb4
void Entity::move_and_collide()
{
    AABB mob_box = m_aabb.translate(m_transform.position());
    const Dimension& dimension = m_world->get_dimension(m_dimension);
    const Vector<AABB> colliders = dimension.get_boxes_that_may_collide(mob_box);

    glm::vec3 move_vector = m_velocity;
    glm::vec3 original_vector = move_vector;

    for (AABB collider : colliders)
        move_vector.y = mob_box.get_clip_y(collider, move_vector.y);
    mob_box = mob_box.translate(glm::vec3(0, move_vector.y, 0));

    for (AABB collider : colliders)
        move_vector.x = mob_box.get_clip_x(collider, move_vector.x);
    mob_box = mob_box.translate(glm::vec3(move_vector.x, 0, 0));

    for (AABB collider : colliders)
        move_vector.z = mob_box.get_clip_z(collider, move_vector.z);
    mob_box = mob_box.translate(glm::vec3(0, 0, move_vector.z));

    if (move_vector.x != original_vector.x)
        m_velocity.x = 0;
    if (move_vector.y != original_vector.y)
        m_velocity.y = 0;
    if (move_vector.z != original_vector.z)
        m_velocity.z = 0;

    m_blocked_x = move_vector.x != original_vector.x;
    m_blocked_z = move_vector.z != original_vector.z;

    m_on_ground = move_vector.y != original_vector.y && original_vector.y < 0;

    glm::vec3 center = (mob_box.max + mob_box.min) / 2.0f;
    set_position(center);
}

bool Entity::is_in_water() const
{
    return m_world->get_dimension(m_dimension).get_tag(get_position(), "water").has_value();
}

bool Entity::chunk_is_loaded() const
{
    return m_world->get_dimension(m_dimension).has_chunk(chunk_index(int64_t(std::round(get_position().x))), chunk_index(int64_t(std::round(get_position().z))));
}
