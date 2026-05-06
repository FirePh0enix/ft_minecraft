#pragma once

#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Core/Definitions.hpp"
#include "Core/Ref.hpp"
#include "Render/Renderer.hpp"
#include "Rpc.hpp"
#include "Transform3D.hpp"

class World;
class Dimension;

struct EntityId
{
    EntityId()
        : m_inner(0)
    {
    }

    explicit EntityId(uint32_t id)
        : m_inner(id)
    {
    }

    ALWAYS_INLINE operator uint32_t() const
    {
        return m_inner;
    }

    ALWAYS_INLINE bool operator==(const EntityId& b) const
    {
        return m_inner == b.m_inner;
    }

    ALWAYS_INLINE std::strong_ordering operator<=>(const EntityId& b) const
    {
        return m_inner <=> b.m_inner;
    }

    ALWAYS_INLINE bool is_valid() const { return m_inner != 0; }

private:
    uint32_t m_inner;
};

class Entity : public Object
{
    CLASS(Entity, Object);

    friend class World;

public:
    Entity(String serialized_id);
    virtual ~Entity() {}

    virtual void start()
    {
    }

    virtual void tick(float delta)
    {
        (void)delta;
    }

    virtual void draw(const RenderPassNode& node)
    {
        (void)node;
    }

    virtual void on_ready()
    {
    }

    const Transform3D& get_transform() const { return m_transform; }
    Transform3D& get_transform() { return m_transform; }

    Transform3D get_global_transform() const
    {
        return m_parent ? m_transform.with_parent(m_parent->get_global_transform()) : m_transform;
    }

    void add_child(Ref<Entity> entity);

    void remove_child(size_t index)
    {
        m_children.remove_at(index);
    }

    inline Ref<Entity> get_child(size_t index)
    {
        return m_children.get_unchecked(index);
    }

    ALWAYS_INLINE const Vector<Ref<Entity>>& get_children() const { return m_children; } // TODO: use View<Ref<Entity>> here
    ALWAYS_INLINE Vector<Ref<Entity>>& get_children() { return m_children; }

    ALWAYS_INLINE Entity *get_parent() const { return m_parent; }
    ALWAYS_INLINE void set_parent(Entity *parent) { m_parent = parent; }
    ALWAYS_INLINE bool has_parent() const { return m_parent != nullptr; }

    ALWAYS_INLINE void set_id(EntityId id) { m_id = id; }
    ALWAYS_INLINE EntityId id() const { return m_id; }

    ALWAYS_INLINE String get_serialized_id() const { return m_serialized_id; }

    template <typename... Args>
    void call_rpc(const StringView& name, Args&&...args)
    {
        Vector<Variant> variants;
        (variants.append(args), ...);

        std::optional<Rpc> f = m_rpc_dispatch.get(name);
        if (!f.has_value())
            return;

        call_rpc(name, *f, variants);
    }

    void call_rpc(const StringView& name)
    {
        std::optional<Rpc> f = m_rpc_dispatch.get(name);
        if (!f.has_value())
            return;

        call_rpc(name, *f, {});
    }

    std::optional<Rpc> get_rpc(String name) { return m_rpc_dispatch.get(name); }

    const String& get_name() const { return m_name; }
    void set_name(const String& name) { m_name = name; }

    const AABB& get_aabb() const { return m_aabb; }

    void recurse_tick(float delta);

    ALWAYS_INLINE bool is_active() const { return m_active; }

protected:
    EntityId m_id;
    String m_serialized_id;
    String m_name;
    Entity *m_parent = nullptr; // FIXME: This must be changed by either a Ref<Entity> or a EntityId.
    Vector<Ref<Entity>> m_children;
    Transform3D m_transform;
    AABB m_aabb;

    World *m_world = nullptr;

    bool m_active = true;
    size_t m_dimension = 0;

    RpcDispatch m_rpc_dispatch;

    template <typename E, typename... Args>
    void register_rpc(String name, RpcFunc f, RpcTarget target = RpcTarget::Both)
    {
        m_rpc_dispatch.add<E, Args...>(name, f, target);
    }

    void call_rpc(String name, Rpc rpc, Vector<Variant> args);
};
