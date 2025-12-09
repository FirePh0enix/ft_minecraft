#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Scene/Components/Component.hpp"

class Scene;
class Transformed3D;

struct EntityId
{
public:
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

struct EntityPath
{
    EntityPath()
    {
    }

    EntityPath(const std::string& path)
    {
        if (!path.starts_with('/'))
            m_path = "/" + path;
        else
            m_path = path;
    }

    EntityPath(EntityPath parent, const std::string& path)
    {
        m_path = parent.m_path + "/" + path;
    }

    const std::string& path() const
    {
        return m_path;
    }

private:
    std::string m_path;
};

class Entity : public Object
{
    CLASS(Entity, Object);

    friend class Scene;

public:
    Entity();
    virtual ~Entity() {}

    ALWAYS_INLINE EntityId id() const { return m_id; }

    const std::vector<Ref<Component>>& get_components() const { return m_components; }

    template <typename T>
        requires IsObject<T>
    Ref<T> get_component() const
    {
        return get_component(T::get_static_hash_code()).template cast_to<T>();
    }

    Ref<Component> get_component(ClassHashCode class_hash) const;

    template <typename T>
        requires IsObject<T>
    bool has_component() const
    {
        return has_component(T::get_static_hash_code());
    }

    bool has_component(ClassHashCode class_hash) const
    {
        for (const Ref<Component>& comp : m_components)
            if (comp->is(class_hash))
                return true;
        return false;
    }

    void add_component(Ref<Component> comp);

    /**
     *  @brief Shorthand for `get_component<Transformed3D>()`.
     */
    Ref<Transformed3D> get_transform() const;

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

    Scene *get_scene() const { return m_scene; }
    void set_scene(Scene *scene) { m_scene = scene; }

    void start()
    {
        for (auto& comp : m_components)
            comp->start();
    }

    void do_start()
    {
        start();

        for (auto& child : m_children)
        {
            child->do_start();
        }
    }

    const std::string& get_name() const { return m_name; }
    void set_name(const std::string& name) { m_name = name; }

    EntityPath get_path()
    {
        if (m_parent)
        {
            EntityPath parent_path = m_parent->get_path();
            return EntityPath(parent_path, m_name);
        }
        else
        {
            return EntityPath(m_name);
        }
    }

private:
    EntityId m_id;
    std::string m_name;
    Entity *m_parent = nullptr; // FIXME: This must be changed by either a Ref<Entity> or a EntityId.
    Scene *m_scene = nullptr;   // FIXME: Must be replaced by a Ref<Scene>
    std::vector<Ref<Component>> m_components;
    std::vector<Ref<Entity>> m_children;
};

/**
 *  @brief Helper function to create an entity and attach components to it.
 */
template <typename... Ts>
Ref<Entity> make_entity(const std::string& name, Ref<Ts>&&...components)
{
    Ref<Entity> entity = newobj(Entity);
    entity->set_name(name);

    (entity->add_component(components), ...);

    return entity;
}
