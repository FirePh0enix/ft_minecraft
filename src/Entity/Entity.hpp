#pragma once

#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Core/Definitions.hpp"
#include "Core/Ref.hpp"
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
    Entity();
    virtual ~Entity() {}

    virtual void start()
    {
    }

    virtual void tick(float delta)
    {
        (void)delta;
    }

    virtual void draw()
    {
    }

    virtual void on_ready()
    {
    }

    ALWAYS_INLINE EntityId id() const { return m_id; }

    const Transform3D& get_transform() const { return m_transform; }
    Transform3D& get_transform() { return m_transform; }

    Transform3D get_global_transform() const
    {
        return m_parent ? m_transform.with_parent(m_parent->get_global_transform()) : m_transform;
    }

    void add_child(Ref<Entity> entity);

    void remove_child(size_t index)
    {
        m_children.erase(m_children.begin() + (ssize_t)index);
    }

    inline Ref<Entity> get_child(size_t index)
    {
        return m_children[index];
    }

    ALWAYS_INLINE const std::vector<Ref<Entity>>& get_children() const { return m_children; }
    ALWAYS_INLINE std::vector<Ref<Entity>>& get_children() { return m_children; }

    ALWAYS_INLINE Entity *get_parent() const { return m_parent; }
    ALWAYS_INLINE void set_parent(Entity *parent) { m_parent = parent; }
    ALWAYS_INLINE bool has_parent() const { return m_parent != nullptr; }

    const String& get_name() const { return m_name; }
    void set_name(const String& name) { m_name = name; }

    const AABB& get_aabb() const { return m_aabb; }

    void recurse_tick(float delta);

protected:
    EntityId m_id;
    String m_name;
    Entity *m_parent = nullptr; // FIXME: This must be changed by either a Ref<Entity> or a EntityId.
    std::vector<Ref<Entity>> m_children;
    Transform3D m_transform;
    AABB m_aabb;

    World *m_world = nullptr;
    size_t m_dimension = 0;
};
