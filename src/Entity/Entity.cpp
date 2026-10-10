#include <cstddef>

#include "Entity/Entity.hpp"

#include "Core/Filesystem.hpp"
#include "Engine.hpp"
#include "Network/Network.hpp"
#include "Type.hpp"
#include "Variant.hpp"
#include "World/World.hpp"
#include "World/Registry.hpp"
#include "Entity/Player.hpp"

#include <string>

std::expected<void, Error> EntitySerializer::save(std::string_view path) const
{
    TRY_ASSIGN(File file, Filesystem::open_file(path, true));
    FileWriter writer = file.writer();

    for (auto& [name, value] : m_variants)
    {
        TRY(writer.write_variant(Variant(name)));
        TRY(writer.write_variant(value));
    }

    file.close();
    return std::expected<void, Error>();
}

std::expected<void, Error> EntitySerializer::load(std::string_view path)
{
    TRY_ASSIGN(File file, Filesystem::open_file(path));
    FileReader reader = file.reader();

    while (!reader.eof())
    {
        TRY_ASSIGN(std::optional<Variant> vname_opt, reader.read_variant());
        if (!vname_opt.has_value())
            break;

        Variant vname = vname_opt.value();
        if (!vname.has(VariantType::String))
            return std::unexpected(Error(ErrorKind::ReadFailure));
        std::string s = vname.get_unchecked<std::string>();

        TRY_ASSIGN(auto value_optional, reader.read_variant());

        Variant value = value_optional.value();
        m_variants[s] = value;
    }

    file.close();
    return std::expected<void, Error>();
}

void Entity::bind_methods()
{
    type.add_method("sync_dimension", &Entity::sync_dimension);
    expose_rpc<Entity>("sync_dimension", RpcTarget::Client);

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

void Entity::tick(float delta)
{
    (void)delta;

    // Only the server decides portal travel; clients receive the destination.
    const auto *player = dynamic_cast<const Player *>(this);
    if (!Engine::get().is_server() || !m_world || !player)
        return;
    if (player->is_dead())
    {
        m_world->cancel_portal_travel(id());
        return;
    }

    const AABBd box = m_aabb.translate(get_position());
    std::optional<BlockPos> touching_portal;
    for (int64_t x = int64_t(std::floor(box.min.x + 0.5)); x <= int64_t(std::floor(box.max.x + 0.5)); ++x)
        for (int64_t y = int64_t(std::floor(box.min.y + 0.5)); y <= int64_t(std::floor(box.max.y + 0.5)); ++y)
            for (int64_t z = int64_t(std::floor(box.min.z + 0.5)); z <= int64_t(std::floor(box.max.z + 0.5)); ++z)
                if (m_world->get_block_state(m_dimension, x, y, z).id == Blocks::portal.hash &&
                    m_world->is_valid_portal(m_dimension, {x, y, z}) &&
                    box.intersect(AABBd(glm::dvec3(-0.5), glm::dvec3(0.5)).translate(glm::dvec3(x, y, z))))
                    touching_portal = BlockPos(x, y, z);

    if (!touching_portal)
        m_inside_portal = false;
    // Crossing a portal commits travel; terrain loading can outlast the contact.
    if (m_world->has_pending_portal_travel(id()) || (touching_portal && !m_inside_portal))
    {
        if (m_world->travel_through_portal(id(), touching_portal))
            m_inside_portal = true;
    }
}


void Entity::sync_dimension(int64_t dimension, glm::dvec3 position)
{
    if (!m_world || dimension < 0 || dimension >= int64_t(World::max_dimensions))
        return;
    m_world->change_dimension(id(), static_cast<int>(dimension));
    set_position(position);
    m_velocity = glm::dvec3(0.0);
    m_on_ground = false;
    m_inside_portal = true;
}

void Entity::add_child(std::shared_ptr<Entity> entity)
{
    (void)m_children.push_back(entity);

    entity->set_parent(this);
    entity->m_world = m_world;
    entity->m_id = World::next_id(); // FIXME: don't do that
}

void Entity::remove_child(size_t index)
{
    m_children.erase(m_children.begin() + (std::ptrdiff_t)index);
}

std::shared_ptr<Entity> Entity::get_child(size_t index)
{
    return m_children[index];
}

void Entity::recurse_tick(float delta)
{
    for (std::shared_ptr<Entity> entity : m_children)
        entity->recurse_tick(delta);
    tick(delta);
}

std::optional<RpcTarget> Entity::get_rpc(std::string_view name)
{
    return Engine::get().registry().get_rpc(this, name);
}

void Entity::expose_rpc(ClassHashCode cls, std::string_view name, RpcTarget target)
{
    Engine::get().registry().register_rpc(cls, name, target);
}

void Entity::call_rpci(std::string_view name, std::span<const Variant> args)
{
    auto rpc_target_maybe = get_rpc(name);
    if (!rpc_target_maybe.has_value())
    {
        return;
    }

    RpcTarget rpc_target = rpc_target_maybe.value();

    if (Engine::get().server() != nullptr && (rpc_target == RpcTarget::Both || (Engine::get().is_server() && rpc_target == RpcTarget::Server) || (Engine::get().is_client() && rpc_target == RpcTarget::Client)))
    {
        if (name.starts_with("set/"))
        {
            const Type::Property& property = get_type()->get_property(name.substr(4));
            property.setter((Object *)this, Arguments{.args = args});
        }
        else
        {
            Type::Method method = get_type()->get_method(name);
            method.func((Object *)this, Arguments{.args = args});
        }
    }

    if (Engine::get().server() != nullptr && (rpc_target == RpcTarget::Both || (Engine::get().is_server() && rpc_target == RpcTarget::Client) || (Engine::get().is_client() && rpc_target == RpcTarget::Server)))
    {
        RpcCallPacket p;
        p.id = id();
        p.name = name;

        for (const Variant& v : args)
            p.args.push_back(v);

        if (Engine::get().is_server())
            Engine::get().server()->route_packet(NetworkConnection::create_packet(p));
        else
            Engine::get().server()->route_packet(NetworkConnection::create_packet(p));
    }
}

// Based on https://medium.com/@andrebluntindie/3d-aabb-collision-detection-and-resolution-for-voxel-games-5fcbfdb8cdb4
void Entity::move_and_collide()
{
    AABBd mob_box = m_aabb.translate(m_transform.position());
    const Dimension& dimension = m_world->get_dimension(m_dimension);
    const std::vector<AABBd> colliders = dimension.get_boxes_that_may_collide(mob_box);

    glm::dvec3 move_vector = m_velocity;
    glm::dvec3 original_vector = move_vector;

    for (AABBd collider : colliders)
        move_vector.y = mob_box.get_clip_y(collider, move_vector.y);
    mob_box = mob_box.translate(glm::dvec3(0, move_vector.y, 0));

    for (AABBd collider : colliders)
        move_vector.x = mob_box.get_clip_x(collider, move_vector.x);
    mob_box = mob_box.translate(glm::dvec3(move_vector.x, 0, 0));

    for (AABBd collider : colliders)
        move_vector.z = mob_box.get_clip_z(collider, move_vector.z);
    mob_box = mob_box.translate(glm::dvec3(0, 0, move_vector.z));

    if (move_vector.x != original_vector.x)
        m_velocity.x = 0;
    if (move_vector.y != original_vector.y)
        m_velocity.y = 0;
    if (move_vector.z != original_vector.z)
        m_velocity.z = 0;

    m_blocked_x = move_vector.x != original_vector.x;
    m_blocked_z = move_vector.z != original_vector.z;

    m_on_ground = move_vector.y != original_vector.y && original_vector.y < 0;

    glm::dvec3 center = (mob_box.max + mob_box.min) / 2.0;
    get_transform().position() = center;
}

bool Entity::is_in_water() const
{
    return m_world->get_dimension(m_dimension).get_tag(get_position(), "water").has_value();
}

bool Entity::chunk_is_loaded() const
{
    return m_world->get_dimension(m_dimension).has_chunk(chunk_index(int64_t(get_position().x)), chunk_index(int64_t(get_position().z)));
}
