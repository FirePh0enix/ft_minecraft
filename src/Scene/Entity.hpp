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

public:
    Entity();
    virtual ~Entity() {}

    inline EntityId get_id() const
    {
        return m_id;
    }

    inline void set_id(EntityId id)
    {
        m_id = id;
    }

    const std::vector<Ref<Component>> get_components() const
    {
        return m_components;
    }

    template <typename T>
    Ref<T> get_component() const
    {
        for (const auto& comp : m_components)
        {
            if (comp->is<T>())
                return comp.cast_to<T>();
        }
        return nullptr;
    }

    Ref<Component> get_component(ClassHashCode class_hash) const
    {
        for (const auto& comp : m_components)
        {
            if (comp->is(class_hash))
                return comp;
        }
        return nullptr;
    }

    template <typename T>
    inline Ref<T> get_component(size_t index) const
    {
        return m_components[index].cast_to<T>();
    }

    template <typename T>
    bool has_component() const
    {
        for (const auto& comp : m_components)
        {
            if (comp->is<T>())
                return true;
        }
        return false;
    }

    bool has_component(ClassHashCode class_hash) const
    {
        for (const auto& comp : m_components)
        {
            if (comp->is(class_hash))
                return true;
        }
        return false;
    }

    void add_component(Ref<Component> comp)
    {
        ERR_COND_VR(comp->get_entity() != nullptr, "Component of type {} was already added to entity {}", comp->get_class_name(), (uint32_t)comp->get_entity()->get_id());

        comp->set_entity(this);
        m_components.push_back(comp);
    }

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

    inline const std::vector<Ref<Entity>>& get_children() const
    {
        return m_children;
    }

    inline std::vector<Ref<Entity>>& get_children()
    {
        return m_children;
    }

    inline Entity *get_parent() const
    {
        return m_parent;
    }

    inline void set_parent(Entity *parent)
    {
        m_parent = parent;
    }

    inline bool has_parent() const
    {
        return m_parent != nullptr;
    }

    Scene *get_scene() const
    {
        return m_scene;
    }

    void set_scene(Scene *scene)
    {
        m_scene = scene;
    }

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
    EntityId m_id = EntityId(0);
    std::string m_name;
    Entity *m_parent = nullptr;               // FIXME: This must be changed by either a Ref<Entity> or a EntityId.
    Scene *m_scene = nullptr;                 // FIXME: Must be replaced by a Ref<Scene>
    std::vector<Ref<Component>> m_components; // TODO: Could be replaced by a map to lower lookup time since components must be unique.
    std::vector<Ref<Entity>> m_children;
};
