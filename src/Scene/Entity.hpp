#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Scene/Components/Component.hpp"

class Scene;

enum class EntityId : uint32_t
{
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

    template <typename T>
    Ref<T> get_component(size_t index) const
    {
        return m_components[index].cast_to<T>();
    }

    template <typename T>
    void add_component(Ref<T> comp)
    {
        ERR_COND_VR(comp->get_entity() != nullptr, "Component of type {} was already added to entity {}", T::get_static_class_name(), (uint32_t)comp->get_entity()->get_id());

        comp->set_entity(this);

        Ref<Component> c = comp.template cast_to<Component>();
        ERR_COND_VR(c.is_null(), "{} is not derived from Component", T::get_static_class_name());

        m_components.push_back(comp.template cast_to<Component>());
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

    void tick()
    {
        for (auto& comp : m_components)
            comp->tick(0.166666666666666);

        for (auto& child : m_children)
            child->tick();
    }

    void do_start()
    {
        start();

        for (auto& child : m_children)
        {
            child->do_start();
        }
    }

private:
    EntityId m_id = EntityId(0);
    Entity *m_parent = nullptr; // FIXME: This must be changed by either a Ref<Entity> or a EntityId.
    Scene *m_scene = nullptr;   // FIXME: Same here.
    std::vector<Ref<Component>> m_components;
    std::vector<Ref<Entity>> m_children;
};
